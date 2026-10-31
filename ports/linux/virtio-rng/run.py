#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Exercise real Virtio RNG PCI/DMA through a generic loopback qtest broker."""
import argparse, hashlib, importlib.util, json, os, re, shutil, socket, subprocess
import threading, time
from pathlib import Path

HERE=Path(__file__).resolve().parent
REPO=HERE.parents[2]
NAMES={'native':'native-nelua','linux':'linux-source','windows':'windows-source'}

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def save(path,obj): path.write_text(json.dumps(obj,indent=2)+'\n')
def advance(value):
    value^=(value<<13)&0xffffffff;value^=value>>17;value^=(value<<5)&0xffffffff
    return value&0xffffffff
def expected_digest(size,offset=0):
    value=14695981039346656037;state=0x6d2b79f5
    for _ in range(offset): state=advance(state)
    for _ in range(size):
        state=advance(state);value=((value^(state>>24))*1099511628211)&((1<<64)-1)
    return f'{value:016x}'

class EntropyFixture:
    """Independent deterministic input to QEMU's real rng-egd backend.

    No controller or resource callback can access these bytes directly.
    This is a data-integrity fixture, not a cryptographic entropy claim.
    """
    def __init__(self):
        self.server=socket.socket();self.server.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
        self.server.bind(('127.0.0.1',0));self.server.listen(1);self.server.settimeout(.2)
        self.port=self.server.getsockname()[1];self.stop=threading.Event()
        self.sent=0;self.state=0x6d2b79f5;self.requests=[];self.error=None;self.client=None
        self.thread=threading.Thread(target=self.run,daemon=True);self.thread.start()
    def run(self):
        try:
            while not self.stop.is_set():
                try: self.client,_=self.server.accept();break
                except socket.timeout: continue
            if self.client is None: return
            self.client.settimeout(.2);pending=bytearray()
            while not self.stop.is_set():
                try: data=self.client.recv(2-len(pending))
                except socket.timeout: continue
                if not data: return
                pending.extend(data)
                if len(pending)!=2: continue
                command,size=pending;pending.clear()
                if command!=2 or not size: raise RuntimeError(f'unsupported EGD request {command}/{size}')
                start=self.sent;values=[]
                for _ in range(size): self.state=advance(self.state);values.append(self.state>>24)
                payload=bytes(values)
                self.client.sendall(payload);self.requests.append({'offset':start,'bytes':size});self.sent+=size
        except (OSError,RuntimeError) as error:
            if not self.stop.is_set(): self.error=str(error)
    def close(self):
        self.stop.set()
        if self.client:
            try: self.client.shutdown(socket.SHUT_RDWR)
            except OSError: pass
            self.client.close()
        self.server.close();self.thread.join(timeout=2)

def free_port():
    with socket.socket() as s: s.bind(('127.0.0.1',0));return s.getsockname()[1]

def tools(args,work):
    if args.qemu:
        executable=Path(shutil.which(str(args.qemu)) or args.qemu).resolve()
        if not executable.is_file(): raise RuntimeError('QEMU executable not found')
        return executable,dict(os.environ),[]
    # Reuse the pinned, private package bootstrap without editing the earlier
    # Linux guest harness or installing host packages.
    spec=importlib.util.spec_from_file_location('drift_linux_guest_tools',HERE.parent/'run.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    root,binary,env=module.qemu(work,args.qemu_root)
    return binary,env,['-L',str(root/'usr/share/qemu')]

def call(argv,path):
    with path.open('w') as log:
        log.write('$ '+repr(list(map(str,argv)))+'\n');log.flush()
        subprocess.run(list(map(str,argv)),cwd=REPO,stdout=log,stderr=subprocess.STDOUT,check=True)

def build(args,work,variant):
    target=subprocess.check_output([args.cc,'-dumpmachine'],text=True).strip()
    windows_target=os.name=='nt' or 'mingw' in target or 'windows' in target
    suffix='.exe' if windows_target else ''
    executable=work/(variant+suffix)
    common=[args.cc,'-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-Wno-unknown-pragmas','-Wno-multichar',
            '-Os','-ffreestanding','-fno-builtin','-fno-stack-protector','-fno-strict-aliasing',
            '-I'+str(REPO/'include'),'-I'+str(REPO/'hosts'),
            HERE/'controller.c',REPO/'hosts/virtio_qtest.c',REPO/'generated/pci.c',REPO/'generated/virtio.c']
    if variant=='native':
        command=common+['-DDRIFT_RNG_NATIVE']
    else:
        profile=REPO/'imports'/('linux/virtio-rng' if variant=='linux' else 'windows/viorng')
        for line in (profile/'SHA256SUMS').read_text().splitlines():
            digest,path=line.split(None,1);path=path.lstrip('* ')
            if sha(profile/path)!=digest: raise RuntimeError('changed upstream file: '+path)
        if variant=='linux': sources=[profile/'compat.generated.c',profile/'driver.c']
        else: sources=[profile/'compat.generated.c',profile/'bridge.c',*[profile/'upstream'/(s+'.c') for s in ['viorng','read','isrdpc','power','virtio']]]
        command=common+['-DDRIFT_RNG_'+variant.upper(),'-I'+str(profile),'-I'+str(profile/'compat'),'-I'+str(profile/'upstream'),*sources]
    command+=['-o',executable]
    if windows_target: command+=['-lws2_32']
    call(command,work/(variant+'-build.log'))
    return executable,{'compiler':subprocess.check_output([args.cc,'--version'],text=True).splitlines()[0],
          'target':target,'executable_sha256':sha(executable),'command':list(map(str,command))}

def snapshot():
    paths=[REPO/'hosts/virtio_qtest.c',REPO/'hosts/virtio_qtest.h',HERE/'controller.c',HERE/'run.py',
           REPO/'generated/virtio.c',REPO/'generated/pci.c',REPO/'native/virtio.nelua',
           REPO/'native/pci.nelua',REPO/'lib/drift/hw.nelua']
    paths+=sorted((REPO/'include').glob('drift_*.h'))
    for folder in ['imports/linux/virtio-rng','imports/windows/viorng']:
        paths+=sorted(p for p in (REPO/folder).rglob('*') if p.is_file())
    return {str(p.relative_to(REPO)):sha(p) for p in paths}

def run_case(args,work,qemu,env,qemu_options,executable,variant,round_number,stop_cpu):
    directory=work/f'round-{round_number}'/variant;directory.mkdir(parents=True,exist_ok=True)
    fixture=EntropyFixture();port=free_port()
    # Running CLI/HLT firmware keeps the VM event loop/timers active without
    # any guest configuring PCI. --stopped tests QEMU's stopped run-state too.
    firmware=work/'halt.bin'
    if not firmware.exists():
        data=bytearray(65536);data[65520:65524]=b'\xfa\xf4\xeb\xfd';firmware.write_bytes(data)
    command=[str(qemu),*qemu_options,'-machine','q35,accel=tcg','-m','128M','-nodefaults','-display','none',
             '-monitor','none','-serial','none','-nic','none','-no-reboot','-bios',str(firmware),
             '-qtest',f'tcp:127.0.0.1:{port},server=on,wait=off','-qtest-log',os.devnull,
             '-chardev',f'socket,id=egd,host=127.0.0.1,port={fixture.port}',
             '-object','rng-egd,id=rng,chardev=egd',
             '-device','virtio-rng-pci,disable-legacy=on,rng=rng,addr=5,max-bytes=1048576,period=1000']
    if stop_cpu: command+=['-S']
    result={'variant':variant,'round':round_number,'status':'preparing','qemu_command':command,'stopped_cpu':stop_cpu}
    process=None
    try:
        with (directory/'qemu.log').open('w') as log:
            process=subprocess.Popen(command,cwd=work,env=env,stdout=log,stderr=subprocess.STDOUT)
            # The provider retries refused loopback connections, without
            # consuming the single qtest connection in a readiness probe.
            time.sleep(.05)
            if process.poll() is not None: raise RuntimeError('QEMU exited before controller; inspect qemu.log')
            completed=subprocess.run([str(executable),str(port),str(args.requests)],cwd=REPO,
                       stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=args.timeout)
            (directory/'controller.log').write_text(completed.stdout)
            if completed.returncode or 'DRIFT_ACTUAL_VIRTIO_RNG_PASS' not in completed.stdout:
                raise RuntimeError(f'controller exit {completed.returncode}: '+completed.stdout[-1500:])
            rows=re.findall(r'^RNG_RESULT\t([^\r\n]+)',completed.stdout,re.M)
            if len(rows)!=1: raise RuntimeError('missing/duplicate controller result')
            fields=rows[0].split('\t')
            if len(fields)!=18: raise RuntimeError('unexpected controller result schema')
            if fields[0]!=NAMES[variant] or int(fields[1])!=args.requests or int(fields[2])!=args.requests*32:
                raise RuntimeError('controller workload mismatch')
            if fields[4]!=expected_digest(args.requests*32,256): raise RuntimeError('controller measured data digest mismatch')
            if (int(fields[11]),int(fields[12]),fields[14],int(fields[15]),int(fields[16]))!=(8,256,expected_digest(256),(args.requests+8)*32,1):
                raise RuntimeError('controller warmup/data/canary qualification mismatch')
            if any(int(fields[i])<=0 for i in [3,9,10,13]): raise RuntimeError('non-positive measurement clock window')
            if int(fields[17])<=0: raise RuntimeError('invalid monotonic clock resolution')
            if fixture.error: raise RuntimeError('EGD fixture: '+fixture.error)
            result.update(status='passed',requests=int(fields[1]),bytes=int(fields[2]),elapsed_us=int(fields[3]),
                          data_digest=fields[4],qtest_commands=int(fields[5]),allocations=int(fields[6]),releases=int(fields[7]),sync_bytes=int(fields[8]),
                          open_us=int(fields[9]),close_us=int(fields[10]),warmup_requests=int(fields[11]),warmup_bytes=int(fields[12]),
                          warmup_us=int(fields[13]),warmup_digest=fields[14],all_delivered_bytes=int(fields[15]),canaries_passed=True,
                          controller_exit=completed.returncode,clock_resolution_ns=int(fields[17]),clock_output_unit_ns=1000)
    except (RuntimeError,subprocess.SubprocessError,OSError) as error:
        result.update(status='failed',error=str(error))
    finally:
        fixture.close()
        if process and process.poll() is None:
            process.terminate()
            try: process.wait(timeout=5)
            except subprocess.TimeoutExpired: process.kill();process.wait()
        result['egd_bytes_supplied']=fixture.sent;result['egd_requests']=fixture.requests
        if fixture.error:
            result.update(status='failed',egd_error=fixture.error,error='EGD fixture error: '+fixture.error)
        save(directory/'result.json',result)
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work',type=Path,default=REPO/'build/virtio-rng-proof')
    parser.add_argument('--qemu-root',type=Path)
    parser.add_argument('--qemu',type=Path)
    parser.add_argument('--cc',default='cc')
    parser.add_argument('--variants',default='native,linux,windows')
    parser.add_argument('--rounds',type=int,default=2)
    parser.add_argument('--requests',type=int,default=64)
    parser.add_argument('--timeout',type=int,default=30)
    parser.add_argument('--stopped',action='store_true')
    args=parser.parse_args();variants=args.variants.split(',')
    if any(v not in NAMES for v in variants) or len(set(variants))!=len(variants): parser.error('unknown/duplicate variant')
    if not 1<=args.requests<=4096 or not 1<=args.rounds<=8 or not 1<=args.timeout<=120: parser.error('invalid bounded workload')
    work=args.work.resolve();work.mkdir(parents=True,exist_ok=True)
    before=snapshot();qemu,env,options=tools(args,work)
    binaries={};builds={}
    for variant in variants: binaries[variant],builds[variant]=build(args,work,variant)
    if before!=snapshot(): raise RuntimeError('source changed during controller build; rerun')
    rows=[]
    for round_number in range(1,args.rounds+1):
        order=variants if round_number%2 else list(reversed(variants))
        for variant in order:
            row=run_case(args,work,qemu,env,options,binaries[variant],variant,round_number,args.stopped)
            rows.append(row);print(json.dumps({k:row.get(k) for k in ['variant','round','status','bytes','error']}),flush=True)
    passed=all(r['status']=='passed' for r in rows)
    source_stable=before==snapshot()
    if not source_stable: passed=False
    save(work/'result.json',{'status':'passed' if passed else 'failed','scope':'real QEMU PCI/rings/DMA via native host TCP broker; not an OS kernel driver load',
         'qemu':subprocess.check_output([str(qemu),'--version'],env=env,text=True).splitlines()[0],
         'qemu_sha256':sha(qemu),'sources':before,'sources_unchanged_after_trials':source_stable,'builds':builds,'rows':rows,
         'data_fixture':'independent rng-egd xorshift32 stream, seed0x6d2b79f5, steps13/17/5, high byte; not cryptographic entropy','requests_per_trial':args.requests,
         'bytes_per_request':32,'warmup_requests_per_trial':8,'timing_window':'read loop after 8 warmup requests; open, warmup, close timed separately',
         'metrics_not_collected':['CPU utilization','RSS','per-request latency percentiles','cryptographic entropy quality'],
         'comparison_note':'same application work, differing source fetch/prefetch/sync sizes; TCP broker/QEMU overhead included; no physical/cross-ISA ranking'})
    return 0 if passed else 2

if __name__=='__main__': raise SystemExit(main())
