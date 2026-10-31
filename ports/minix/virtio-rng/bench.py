#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Paired actual MINIX DMA read loops; independent fresh guest per licensed ELF."""
import argparse,hashlib,importlib.util,json,os,re,select,shlex,shutil,struct,subprocess,sys,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;REPO=HERE.parents[2]
sys.path.insert(0,str(HERE.parent))
from mfs import MFS
spec=importlib.util.spec_from_file_location('drift_minix_common',HERE.parent/'run.py')
common=importlib.util.module_from_spec(spec);spec.loader.exec_module(common)
KINDS={'native':'native-nelua','linux':'imported-linux','windows':'imported-windows'}
FIXTURE_SHA='9be4f35563ede119bb4a85dd735a75da95b2d815d83a28e257efa93296d384a7'

def build(work,sdk,cc,kind,events):
    work.mkdir(parents=True,exist_ok=True)
    flags=['-m32','-std=c11','-Os','-ffreestanding','-fno-builtin','-fno-strict-aliasing','-fno-stack-protector','-fno-pie','-D__minix','-D_MINIX_SYSTEM','--sysroot='+str(sdk),'-isystem',str(sdk/'usr/include'),'-I'+str(REPO/'include'),'-I'+str(HERE),'-DDRIFT_BENCH_EVENTS='+str(events)]
    common_flags=flags.copy();flags+=['-DDRIFT_BENCH_'+kind.upper()]
    units=[HERE/'resources.c',HERE/'bench_service.c',REPO/'generated/pci.c'];metadata=[];directory=None
    if kind=='native':units+=[REPO/'generated/virtio.c']
    else:
        directory=REPO/'imports'/('linux/virtio-rng' if kind=='linux' else 'windows/viorng');flags+=['-I'+str(directory)]
        metadata=[p for p in directory.rglob('*') if p.is_file() and (p.relative_to(directory).parts[0] in ('upstream','compat') or p.suffix in ('.c','.h','.nelua') and p.name not in ('test.c','tests.c'))]
    inputs=[*units,*metadata,REPO/'generated/virtio.c',REPO/'native/virtio.nelua',REPO/'native/pci.nelua',REPO/'lib/drift/hw.nelua',HERE/'resources.h',HERE/'fixture.h',HERE/'rc-bench.sh',HERE/'bench.py',REPO/'include/drift_hw.h',REPO/'include/drift_pci.h',REPO/'include/drift_virtio.h']
    before={str(p.relative_to(REPO)):common.sha(p) for p in inputs};extra=[]
    if directory:
        target=work/'profile-objects';shutil.rmtree(target,ignore_errors=True)
        common.run(['make','-C',str(directory),'library','BUILD='+str(target),'CC='+shlex.join([cc,*flags])],work/'profile-build.log')
        extra=list(target.glob('*.a'))
        if len(extra)!=1:raise RuntimeError('expected one profile archive')
    objects=[]
    for i,unit in enumerate(units):
        obj=work/(str(i)+'.o');common.run([cc,*flags,'-c',str(unit),'-o',str(obj)],work/('build-'+str(i)+'.log'));objects.append(obj)
    libs=sdk/'usr/lib';out=work/'drift-rng-service'
    command=['ld','-m','elf_i386','-static','-e','_start','-z','noseparate-code','-z','norelro','-z','noexecstack','-o',str(out)]
    command+=list(map(str,[libs/'crt0.o',libs/'crti.o',libs/'crtbeginT.o',*objects,*extra]))+['-L'+str(libs),'-lsys','-lc','-lsys']+list(map(str,[libs/'crtend.o',libs/'crtn.o']))
    common.run(command,work/'link.log')
    if subprocess.check_output(['nm','-u',str(out)]).strip():raise RuntimeError('unresolved payload symbols')
    after={str(p.relative_to(REPO)):common.sha(p) for p in inputs}
    if before!=after:raise RuntimeError('sources changed during build')
    common.save(work/'build-metadata.json',{'kind':kind,'compiler':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'flags':flags,'common_flags':common_flags,'sources':after,'payload_sha256':common.sha(out),'payload_bytes':out.stat().st_size,'base_iso_sha256':common.ISO_SHA,'qemu_packages_manifest_sha256':common.sha(HERE.parent/'qemu-ubuntu-noble.json')})
    return out

def patch(iso,work,payload):
    out=work/'drift-bench.iso';shutil.copyfile(iso,out);records=[]
    for sector,name,data,keep in [(82088,'/bin/fetch',payload.read_bytes(),True),(12288,'/etc/rc.cd',(HERE/'rc-bench.sh').read_bytes(),False)]:
        fs=MFS(out,sector);ino=fs.lookup(name);mode,size,z=fs.inode(ino);blocks=list(z[:7]);count=(size+fs.zs-1)//fs.zs
        if count>7:blocks+=list(fs.blocks(z[7]))
        if count>len(blocks) or len(data)>(size if keep else count*fs.zs):raise RuntimeError('donor capacity exceeded')
        old=fs.content(ino);padded=data.ljust(size if keep else (len(data)+fs.zs-1)//fs.zs*fs.zs,b'\0')
        with out.open('r+b') as f:
            for i in range((len(padded)+fs.zs-1)//fs.zs):f.seek(fs.base+blocks[i]*fs.zs);f.write(padded[i*fs.zs:(i+1)*fs.zs])
            if not keep:f.seek(fs.base+fs.ib+(ino-1)*64+8);f.write(struct.pack('<I',len(data)))
        records.append({'path':name,'sector':sector,'old_sha256':hashlib.sha256(old).hexdigest(),'payload_sha256':hashlib.sha256(data).hexdigest(),'kept_size':keep})
    common.save(work/'image-changes.json',{'base_sha256':common.ISO_SHA,'test_sha256':common.sha(out),'changes':records});return out

def fixture(work,events):
    x=0x6d2b79f5;data=bytearray()
    for _ in range(1024*1024):
        x^=(x<<13)&0xffffffff;x^=x>>17;x^=(x<<5)&0xffffffff;data.append(x>>24)
    out=work/'rng-input.bin';out.write_bytes(data)
    if common.sha(out)!=FIXTURE_SHA:raise RuntimeError('fixture generator mismatch')
    value=14695981039346656037
    for b in data[64:64+events*64]:value=((value^b)*1099511628211)&0xffffffffffffffff
    return out,format(value,'016x')

def trial(work,q,env,boot,iso,source,kind,events,golden,round_,slot):
    work.mkdir(parents=True,exist_ok=True)
    command=[str(q/'usr/bin/qemu-system-i386'),'-machine','pc,accel=tcg','-m','256','-bios',str(q/'usr/share/seabios/bios-256k.bin'),'-L',str(q/'usr/share/qemu'),'-kernel',str(boot/'kernel'),'-initrd',','.join(str(p) for p in sorted(boot.glob('mod*'))),'-append','bootcd=1 cdproberoot=1 rootdevname=ram disable=inet cttyline=1 cttybaud=115200 nobeep=1 hz=60','-cdrom',str(iso),'-display','none','-serial','stdio','-serial','file:'+str(work/'kernel-serial.log'),'-monitor','none','-no-reboot','-nic','none','-object','rng-random,id=drift-rng,filename='+str(source),'-device','virtio-rng-pci,rng=drift-rng,disable-legacy=on']
    common.save(work/'command.json',command)
    proc=subprocess.Popen(command,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env);output=bytearray();start=time.monotonic()
    try:
        while time.monotonic()-start<150:
            if proc.poll() is not None:output.extend(proc.stdout.read());break
            if select.select([proc.stdout],[],[],1)[0]:
                b=os.read(proc.stdout.fileno(),65536)
                if b:output.extend(b)
            if b'DRIFT_MINIX_DMA_BENCH_ALL_DONE' in output:break
    finally:
        if proc.poll() is None:proc.terminate()
        try:proc.wait(timeout=5)
        except subprocess.TimeoutExpired:proc.kill();proc.wait()
    (work/'serial.log').write_bytes(output);text=output.decode(errors='strict')
    pattern=r'DRIFT_MINIX_DMA_BENCH kind=(\S+) events=(\d+) bytes=(\d+) elapsed_us=(\d+) resolution_ns=(\d+) fnv64=([0-9a-f]{16}) posted=(\d+) features_low=(\d+) status=(\d+) poll_wait_us=(\d+)'
    found=re.findall(pattern,text);row=None
    if len(found)==1:
        name,n,b,t,res,h,posted,features,status,poll=found[0]
        row={'kind':name,'events':int(n),'bytes':int(b),'elapsed_us':int(t),'resolution_ns':int(res),'fnv64':h,'posted':int(posted),'features_low':int(features),'status':int(status),'poll_wait_us':int(poll),'round':round_,'slot':slot}
    checks={'row':bool(row),'up':'DRIFT_MINIX_DMA_BENCH_UP_EXIT=0' in text,'down':'DRIFT_MINIX_DMA_BENCH_DOWN_EXIT=0' in text,'status':'DRIFT_MINIX_DMA_BENCH_STATUS=PASS' in text,'pass':'DRIFT_MINIX_DMA_BENCH_PASS kind='+KINDS[kind] in text,'cleanup':'DRIFT_MINIX_DMA_BENCH_CLEANUP kind='+KINDS[kind]+' allocations=2 releases=2 fault=0' in text,'no_failure':not any(v in text for v in ('DRIFT_MINIX_DMA_BENCH_FAIL','DRIFT_MINIX_DMA_BENCH_REJECT','DRIFT_MINIX_DMA_BENCH_QUARANTINE'))}
    checks['parity']=bool(row and row['kind']==KINDS[kind] and row['events']==events and row['bytes']==events*64 and row['fnv64']==golden and row['features_low']==0 and row['status']==15 and row['poll_wait_us']==10 and row['posted']==events+(2 if kind=='linux' else 1))
    checks['resolution']=bool(row and row['resolution_ns']>0 and row['elapsed_us']>(row['resolution_ns']+999)//1000)
    result={'status':'passed' if all(checks.values()) else 'failed','checks':checks,'row':row,'serial_sha256':hashlib.sha256(output).hexdigest(),'guest_wall_seconds':time.monotonic()-start}
    common.save(work/'result.json',result)
    if result['status']!='passed':raise RuntimeError('rejected actual MINIX trial '+str(work))
    return row

def main():
    p=argparse.ArgumentParser();p.add_argument('--work',type=Path,required=True);p.add_argument('--iso',type=Path);p.add_argument('--sdk',type=Path);p.add_argument('--qemu-root',type=Path);p.add_argument('--cc',default='cc');p.add_argument('--events',type=int,default=4096);p.add_argument('--rounds',type=int,default=4);a=p.parse_args()
    if not 16<=a.events<=16000 or not 1<=a.rounds<=4:p.error('events16..16000; rounds1..4')
    work=a.work.resolve();work.mkdir(parents=True,exist_ok=True);iso=common.image(work,a.iso);sdk=a.sdk.resolve() if a.sdk else common.sdk_from_image(iso,work);q,env=common.tool_qemu(work,a.qemu_root);boot=common.boot_files(iso,work);source,golden=fixture(work,a.events)
    (work/'qemu-version.txt').write_text(subprocess.check_output([str(q/'usr/bin/qemu-system-i386'),'--version'],env=env,text=True))
    images={kind:patch(iso,work/('build-'+kind),build(work/('build-'+kind),sdk,a.cc,kind,a.events)) for kind in KINDS}
    rows=[];start=time.monotonic();result={'status':'running','events':a.events,'rounds':a.rounds,'request_bytes':64,'warmup_bytes':64,'fixture_sha256':FIXTURE_SHA,'expected_fnv64':golden,'rows':rows,'scope':'actual MINIX single RS device/read loops in QEMU TCG, including identical byte validation/FNV work; excludes boot/open/close, not IPC service latency, physical hardware throughput or a cross-host ranking','policy':'same delivered bytes, VERSION_1-only,queue8,-Os,10us polling; Linux source naturally posts one extra prefetch; fresh guest/backend file descriptor per separate licensed ELF; alternate native/Linux/Windows then reverse; zero or <=one-resolution-tick trials rejected'}
    common.save(work/'result.json',result)
    try:
        for round_ in range(1,a.rounds+1):
            order=list(KINDS) if round_%2 else list(reversed(KINDS))
            for slot,kind in enumerate(order,1):
                directory=work/('round-%d-slot-%d-%s'%(round_,slot,kind))
                row=trial(directory,q,env,boot,images[kind],source,kind,a.events,golden,round_,slot);rows.append(row);common.save(work/'result.json',result);print(json.dumps(row),flush=True)
        flags=[json.loads((work/('build-'+kind)/'build-metadata.json').read_text())['common_flags'] for kind in KINDS]
        if any(f!=flags[0] for f in flags):raise RuntimeError('common compiler flags differ')
        result['status']='passed';result['elapsed_seconds']=time.monotonic()-start;common.save(work/'result.json',result)
    except BaseException:
        result['status']='failed';result['elapsed_seconds']=time.monotonic()-start;common.save(work/'result.json',result);raise
if __name__=='__main__':main()
