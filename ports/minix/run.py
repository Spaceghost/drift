#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Run separate imported-driver models and a resource-backed PC speaker in MINIX."""
import argparse, bz2, hashlib, importlib.util, json, os, re, select, shlex
import shutil, struct, subprocess, time, urllib.request
from pathlib import Path
from mfs import MFS
HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
COMPRESSED_SHA = 'cbd389e6c97fcc80104d1426259e759d7a67970cf2b36f97ef7d827f813419f8'
ISO_SHA = '389be8cbc2b74022671c082d0b88511b17e63b18003a14647b03331e3467b4c4'
IMAGE_URL = 'https://www.minix3.org/iso/minix_R3.3.0-588a35b.iso.bz2?drift-imports=1'
NAME = 'minix_R3.3.0-588a35b.iso'

def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        while b:=f.read(1024*1024): h.update(b)
    return h.hexdigest()

def save(path,obj): path.write_text(json.dumps(obj,indent=2)+'\n')

def fetch(url,path,expected):
    if path.exists() and sha(path)==expected: return
    temporary=path.with_name(path.name+'.partial')
    with urllib.request.urlopen(url,timeout=60) as r,temporary.open('wb') as f:
        if r.status!=200: raise RuntimeError(f'full download expected HTTP200: {url}')
        shutil.copyfileobj(r,f)
    if sha(temporary)!=expected: raise RuntimeError(f'checksum mismatch: {url}')
    temporary.replace(path)

def tool_qemu(work,requested):
    q=requested.resolve() if requested else work/'qemu-root'
    if not requested:
        for item in json.loads((HERE/'qemu-ubuntu-noble.json').read_text()):
            deb=work/item['url'].rsplit('/',1)[-1]
            fetch(item['url'],deb,item['sha256'])
            subprocess.run(['dpkg-deb','-x',str(deb),str(q)],check=True)
    if not (q/'usr/bin/qemu-system-i386').is_file(): raise RuntimeError('missing qemu-system-i386')
    # Ubuntu's compiled firmware search paths need relative links after relocation.
    for source in (q/'usr/share/seabios').glob('*.bin'):
        target=q/'usr/share/qemu'/source.name
        if not target.exists(): target.symlink_to('../seabios/'+source.name)
    env=dict(os.environ,LD_LIBRARY_PATH=str(q/'usr/lib/x86_64-linux-gnu'),
             QEMU_MODULE_DIR=str(q/'usr/lib/x86_64-linux-gnu/qemu'))
    return q,env

def image(work,provided):
    iso=provided.resolve() if provided else work/NAME
    if not iso.exists():
        archive=work/(NAME+'.bz2');fetch(IMAGE_URL,archive,COMPRESSED_SHA)
        with bz2.open(archive,'rb') as src,iso.open('wb') as dst: shutil.copyfileobj(src,dst)
    if sha(iso)!=ISO_SHA: raise RuntimeError('unrecognized MINIX base image')
    return iso

def sdk_from_image(iso,work):
    sdk=work/'sdk';fs=MFS(iso,82088);records=[]
    for directory in ['/include','/lib']:
        (sdk/'usr'/directory.lstrip('/')).mkdir(parents=True,exist_ok=True)
        for row in fs.list(directory,16):
            target=sdk/'usr'/row['path'].lstrip('/');kind=int(row['mode'],8)&0xf000
            if kind==0x4000: target.mkdir(parents=True,exist_ok=True);continue
            data=fs.content(row['inode']);target.parent.mkdir(parents=True,exist_ok=True)
            if kind==0xa000:
                link=data.decode();resolved=sdk/link.lstrip('/') if link.startswith('/') else target.parent/link
                if not resolved.resolve().is_relative_to(sdk.resolve()): raise RuntimeError('SDK symlink escapes work directory')
                link=os.path.relpath(resolved,target.parent)
                if not target.is_symlink(): target.symlink_to(link)
                records.append({'path':row['path'],'link':link})
            elif kind==0x8000:
                target.write_bytes(data);records.append({'path':row['path'],'sha256':hashlib.sha256(data).hexdigest()})
    save(work/'sdk-files.json',records)
    return sdk

def run(command,log,cwd=REPO):
    with log.open('w') as f: subprocess.run(command,cwd=cwd,stdout=f,stderr=subprocess.STDOUT,check=True)

def linux_model(work):
    text=(REPO/'imports/linux/pcspkr/test.c').read_text()
    text=text.replace('#include <pthread.h>','/* MINIX: pthread contention is omitted, not emulated. */')
    text,n=re.subn(r'static void \*contender\(void \*context\)\n\{.*?\n\}', '',text,count=1,flags=re.S)
    if n!=1: raise RuntimeError('Linux model source changed: inspect pthread omission')
    text,n=re.subn(r'    if \(m->concurrent\) \{\n        pthread_t thread;.*?\n    \}',
                  '    assert(!m->concurrent);',text,count=1,flags=re.S)
    if n!=1: raise RuntimeError('Linux model source changed: inspect thread case')
    text=text.replace('m.reenter=1; m.concurrent=1;','m.reenter=1; m.concurrent=0;')
    text=text.replace('m.nested_result==-16 && m.concurrent_result==-16','m.nested_result==-16')
    text=re.sub(r'puts\("[^"\n]*"\);','puts("DRIFT_MINIX_MODEL_PASS: upstream driver; modeled ports; pthread case omitted");',text)
    if 'pthread_create' in text or 'pthread_join' in text: raise RuntimeError('unhandled pthread test')
    path=work/'linux_model.c';path.write_text(text)
    return path

def compile_payloads(work,sdk,cc):
    flags=['-m32','-std=c11','-Os','-fno-strict-aliasing','-fno-stack-protector','-fno-pie',
           '-D__minix','--sysroot='+str(sdk),'-isystem',str(sdk/'usr/include')]
    libs=sdk/'usr/lib';result={}
    startup=[libs/'crt0.o',libs/'crti.o',libs/'crtbeginT.o'];finish=[libs/'crtend.o',libs/'crtn.o']
    for kind,profile,source in [('linux','linux/pcspkr',linux_model(work)),
                              ('windows','windows/moufiltr',REPO/'imports/windows/moufiltr/tests.c')]:
        directory=REPO/'imports'/profile;target=work/(kind+'-objects');shutil.rmtree(target,ignore_errors=True);target.mkdir()
        run(['make','-C',str(directory),'library','BUILD='+str(target),
             'CC='+shlex.join([cc,*flags])],work/(kind+'-build.log'))
        obj=target/'test.o'
        run([cc,*flags,'-I'+str(directory),'-I'+str(directory/'compat'),'-c',str(source),'-o',str(obj)],work/(kind+'-test-build.log'))
        wraps=['drift_lx_input_allocate_device','drift_lx_input_register_device'] if kind=='linux' else ['WdfDriverCreate','WdfDeviceCreate','WdfIoQueueCreate']
        output=work/('drift-'+kind+'-model')
        command=['ld','-m','elf_i386','-static','-e','_start','-z','noseparate-code','-z','norelro','-z','noexecstack','-o',str(output)]
        command+=list(map(str,startup))+[str(obj),str(target/'compat.o'),str(target/'driver.o'),'-L'+str(libs)]
        command+=['--wrap='+name for name in wraps]+['-lc']+list(map(str,finish))
        run(command,work/(kind+'-link.log'));result[kind]=output
    source=HERE/'pio_service.c';obj=work/'pio.o';directory=REPO/'imports/linux/pcspkr'
    run([cc,*flags,'-D_MINIX_SYSTEM','-I'+str(directory),'-c',str(source),'-o',str(obj)],work/'pio-build.log')
    target=work/'linux-objects';output=work/'drift-pio-service'
    command=['ld','-m','elf_i386','-static','-e','_start','-z','noseparate-code','-z','norelro','-z','noexecstack','-o',str(output)]
    command+=list(map(str,startup))+[str(obj),str(target/'compat.o'),str(target/'driver.o'),'-L'+str(libs),'-lmthread','-lsys','-lc','-lmthread','-lsys']+list(map(str,finish))
    run(command,work/'pio-link.log');result['pio']=output
    # Shared model fixture: all three translation units use identical flags.
    bench=work/'shared-model-objects';shutil.rmtree(bench,ignore_errors=True);bench.mkdir()
    objects=[]
    for index,unit in enumerate([REPO/'tests/bench_linux_import.c',directory/'compat.generated.c',directory/'driver.c']):
        obj=bench/(str(index)+'.o')
        run([cc,*flags,'-ffreestanding','-fno-builtin','-I'+str(directory),'-I'+str(directory/'compat'),'-I'+str(REPO/'tests'),'-c',str(unit),'-o',str(obj)],work/('shared-model-build-'+str(index)+'.log'));objects.append(obj)
    output=work/'drift-shared-model-bench'
    command=['ld','-m','elf_i386','-static','-e','_start','-z','noseparate-code','-z','norelro','-z','noexecstack','-o',str(output)]
    command+=list(map(str,startup))+list(map(str,objects))+['-L'+str(libs),'-lc']+list(map(str,finish))
    run(command,work/'shared-model-link.log');result['shared_model']=output
    for kind,binary in result.items():
        if subprocess.check_output(['nm','-u',str(binary)]).strip(): raise RuntimeError('unresolved symbols in '+kind)
    save(work/'build-metadata.json',{'cc_version':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'flags':flags,'pio_source_sha256':sha(HERE/'pio_service.c'),'linux_model_source_sha256':sha(work/'linux_model.c'),'windows_model_source_sha256':sha(REPO/'imports/windows/moufiltr/tests.c'),'linux_upstream_sha256':sha(REPO/'imports/linux/pcspkr/upstream/pcspkr.c'),'windows_upstream_sha256':sha(REPO/'imports/windows/moufiltr/upstream/moufiltr.c')})
    save(work/'payloads.json',{k:{'sha256':sha(v),'bytes':v.stat().st_size} for k,v in result.items()})
    return result

def patch_image(iso,work,payloads):
    output=work/'drift-test.iso';shutil.copyfile(iso,output);changes=[]
    def replace(sector,name,data,keep_size=False):
        fs=MFS(output,sector);ino=fs.lookup(name);mode,size,z=fs.inode(ino)
        if mode&0xf000!=0x8000: raise RuntimeError('donor must be regular file')
        blocks=list(z[:7]);needed=(size+fs.zs-1)//fs.zs
        if needed>7: blocks+=list(fs.blocks(z[7]))
        if len(data)>(size if keep_size else needed*fs.zs): raise RuntimeError('payload exceeds existing donor capacity')
        old=fs.content(ino);padded=data.ljust(size if keep_size else ((len(data)+fs.zs-1)//fs.zs)*fs.zs,b'\0')
        with output.open('r+b') as f:
            for i in range((len(padded)+fs.zs-1)//fs.zs):
                f.seek(fs.base+blocks[i]*fs.zs);f.write(padded[i*fs.zs:(i+1)*fs.zs])
            if not keep_size:
                f.seek(fs.base+fs.ib+(ino-1)*64+8);f.write(struct.pack('<I',len(data)))
        changes.append({'path':name,'partition_sector':sector,'old_sha256':hashlib.sha256(old).hexdigest(),
                        'payload_sha256':hashlib.sha256(data).hexdigest(),'kept_file_size':keep_size})
    # No filesystem allocator or host mount: reuse known, non-boot executable extents.
    replace(82088,'/bin/calendar',payloads['linux'].read_bytes(),True)
    replace(82088,'/bin/cal',payloads['windows'].read_bytes(),True)
    replace(82088,'/bin/fetch',payloads['pio'].read_bytes(),True)
    replace(82088,'/bin/csplit',payloads['shared_model'].read_bytes(),True)
    script=(HERE/'rc-test.sh').read_bytes();replace(12288,'/etc/rc.cd',script)
    save(work/'image-changes.json',{'base_sha256':ISO_SHA,'test_sha256':sha(output),'changes':changes})
    return output

def boot_files(iso,work):
    boot=work/'boot';boot.mkdir(exist_ok=True)
    with iso.open('rb') as f:
        f.seek(16*2048+156);root=f.read(34);offset=int.from_bytes(root[2:6],'little')*2048;size=int.from_bytes(root[10:14],'little')
        f.seek(offset);directory=f.read(size);i=0;rows=[]
        while i<len(directory):
            n=directory[i]
            if not n: i=(i//2048+1)*2048;continue
            r=directory[i:i+n];i+=n;name=r[33:33+r[32]].decode('ascii','replace').split(';')[0].lower()
            if name=='kernel' or name.startswith('mod'):
                rows.append((name,int.from_bytes(r[2:6],'little')*2048,int.from_bytes(r[10:14],'little')))
        for name,offset,size in rows:
            f.seek(offset);(boot/name).write_bytes(f.read(size))
    if len(rows)!=12: raise RuntimeError('unexpected MINIX boot module set')
    return boot

def guest(work,qemu,env,boot,iso):
    command=[str(qemu/'usr/bin/qemu-system-i386'),'-machine','pc,accel=tcg','-m','256',
             '-bios',str(qemu/'usr/share/seabios/bios-256k.bin'),'-L',str(qemu/'usr/share/qemu'),
             '-kernel',str(boot/'kernel'),'-initrd',','.join(str(p) for p in sorted(boot.glob('mod*'))),
             '-append','bootcd=1 cdproberoot=1 rootdevname=ram disable=inet cttyline=1 cttybaud=115200 nobeep=1 hz=60',
             '-cdrom',str(iso),'-display','none','-serial','stdio','-serial','file:'+str(work/'kernel-serial.log'),
             '-monitor','none','-no-reboot','-nic','none']
    save(work/'command.json',command)
    version=subprocess.check_output([str(qemu/'usr/bin/qemu-system-i386'),'--version'],env=env,text=True)
    (work/'qemu-version.txt').write_text(version)
    proc=subprocess.Popen(command,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env)
    output=bytearray();start=time.monotonic()
    try:
        while time.monotonic()-start<150:
            if proc.poll() is not None: output.extend(proc.stdout.read());break
            if select.select([proc.stdout],[],[],1)[0]:
                chunk=os.read(proc.stdout.fileno(),65536)
                if chunk: output.extend(chunk);print(chunk.decode(errors='replace'),end='',flush=True)
            if b'DRIFT_MINIX_ALL_DONE' in output: break
    finally:
        if proc.poll() is None: proc.terminate()
        try: proc.wait(timeout=5)
        except subprocess.TimeoutExpired: proc.kill();proc.wait()
    (work/'serial.log').write_bytes(output)
    text=output.decode(errors='replace')
    rows=[{'kind':k,'round':int(r),'events':int(n),'elapsed_us':int(t),'reads':int(a),'writes':int(b),'waits':int(c),'trace':int(h)}
          for k,r,n,t,a,b,c,h in re.findall(r'DRIFT_MINIX_PIO_BENCH kind=(\w+) round=(\d+) events=(\d+) elapsed_us=(\d+) reads=(\d+) writes=(\d+) waits=(\d+) trace=(\d+)',text)]
    model_rows=[line.split() for line in text.replace('\r','').splitlines() if re.match(r'^[1-8]\s+[1-4]\s+[12]\s+(?:imported-linux|direct-reference)\s+',line)]
    golden=expected_trace()
    checks={'shared_model': 'DRIFT_MINIX_SHARED_MODEL_EXIT=0' in text and len(model_rows)==8 and all(len(r)==10 and int(r[4])>0 and int(r[5])==500000 and int(r[6])==3500000 and r[7]=='64f59f6df89549d1' and r[8]=='a8' and int(r[9])==1093750 for r in model_rows),
            'linux_model': 'DRIFT_MINIX_MODEL_PASS:' in text and 'DRIFT_MINIX_LINUX_MODEL_EXIT=0' in text,
            'windows_model': 'checks passed (actual source,' in text and 'DRIFT_MINIX_WINDOWS_MODEL_EXIT=0' in text,
            'pio': 'DRIFT_MINIX_PIO_PASS:' in text and 'DRIFT_MINIX_PIO_UP_EXIT=0' in text and 'DRIFT_MINIX_PIO_DOWN_EXIT=0' in text and 'DRIFT_MINIX_PIO_STATUS=PASS' in text,
            'tty_ownership': 'DRIFT_MINIX_NOBEEP=1' in text,
            'denial': bool(re.search(r'DRIFT_MINIX_PIO_DENIAL port=0x80 code=-\d+',text)),
            'benchmarks':len(rows)==8 and all(r['events']==10000 and r['reads']==10000 and r['writes']==40000 and r['waits']==40000 and r['elapsed_us']>0 for r in rows) and all(r['trace']==golden for r in rows) and len({(r['kind'],r['round']) for r in rows})==8}
    result={'status':'passed' if all(checks.values()) and 'DRIFT_MINIX_TRACE_BAD' not in text else 'failed','checks':checks,'benchmarks':rows,'shared_model_rows':model_rows,
            'elapsed_seconds':time.monotonic()-start,'serial_sha256':hashlib.sha256(output).hexdigest(),
            'scopes':{'shared_model':'same 500k-event monotonic-wall component fixture as other OSes; modeled ports and waits, not hardware',
                      'linux_model':'actual MINIX userspace; modeled ports; pthread contention omitted',
                      'windows_model':'actual MINIX userspace; modeled WDF; separate MS-PL executable, no Windows kernel ABI',
                      'pio':'actual MINIX RS service, checked port I/O, native mutex and delay, emulated PIT2 readback; no physical speaker qualification'},
            'benchmark_scope':'TCG component I/O loops in one service; not OS request latency, physical hardware speed or a cross-host ranking'}
    save(work/'result.json',result)
    if result['status']!='passed': raise RuntimeError('guest verification failed; preserve serial.log/result.json')
    return result

def expected_trace():
    value=2166136261
    sequence=[('L',0,0),('W',0x43,0xb6),('D',0,1),('W',0x42,(1193182//880)&255),('D',0,1),('W',0x42,(1193182//880)>>8),('R',0x61,3),('D',0,1),('W',0x61,3),('D',0,1),('U',0,0)]
    for iteration in range(10000):
        for kind,port,data in sequence:
            if iteration==0 and kind=='R' and port==0x61: data=0
            for byte in bytes([ord(kind)])+struct.pack('<HI',port,data): value=((value^byte)*16777619)&0xffffffff
    return value

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--work',type=Path,default=REPO/'build/minix');p.add_argument('--iso',type=Path);p.add_argument('--qemu-root',type=Path);p.add_argument('--cc',default='cc');a=p.parse_args()
    work=a.work.resolve();work.mkdir(parents=True,exist_ok=True)
    iso=image(work,a.iso);sdk=sdk_from_image(iso,work);qemu,env=tool_qemu(work,a.qemu_root)
    payloads=compile_payloads(work,sdk,a.cc);test=patch_image(iso,work,payloads);boot=boot_files(iso,work)
    print(json.dumps(guest(work,qemu,env,boot,test),indent=2))
if __name__=='__main__': main()
