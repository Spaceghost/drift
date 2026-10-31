#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""MINIX3.3 i386 modern PCI/coherent DMA qualification in private QEMU."""
import argparse,hashlib,importlib.util,json,os,select,shlex,shutil,struct,subprocess,sys,time
from pathlib import Path
HERE=Path(__file__).resolve().parent
REPO=HERE.parents[2]
sys.path.insert(0,str(HERE.parent))
from mfs import MFS
spec=importlib.util.spec_from_file_location('drift_minix_common',HERE.parent/'run.py')
common=importlib.util.module_from_spec(spec);spec.loader.exec_module(common)

def build(work,sdk,cc,kind):
    flags=['-m32','-std=c11','-Os','-ffreestanding','-fno-builtin','-fno-strict-aliasing','-fno-stack-protector','-fno-pie','-D__minix','-D_MINIX_SYSTEM','--sysroot='+str(sdk),'-isystem',str(sdk/'usr/include'),'-I'+str(REPO/'include'),'-I'+str(HERE)]
    extra=[];metadata=[];directory=None
    if kind=='raw':
        units=[HERE/'resources.c',HERE/'probe_service.c',REPO/'generated/virtio.c',REPO/'generated/pci.c']
    else:
        directory=REPO/'imports'/('linux/virtio-rng' if kind=='linux' else 'windows/viorng')
        flags+=['-DDRIFT_PROFILE_'+kind.upper(),'-I'+str(directory)]
        target=work/'profile-objects';shutil.rmtree(target,ignore_errors=True)
        metadata=[p for p in directory.rglob('*') if p.is_file() and (p.relative_to(directory).parts[0] in ('upstream','compat') or p.suffix in ('.c','.h','.nelua') and p.name not in ('test.c','tests.c'))]
        units=[HERE/'resources.c',HERE/'profile_service.c',REPO/'generated/pci.c']
    inputs=[*units,*metadata,REPO/'generated/virtio.c',REPO/'native/virtio.nelua',REPO/'native/pci.nelua',REPO/'lib/drift/hw.nelua',HERE/'resources.h',HERE/'fixture.h',HERE/'rc-test.sh',HERE/'run.py',REPO/'include/drift_hw.h',REPO/'include/drift_pci.h',REPO/'include/drift_virtio.h']
    before={str(p.relative_to(REPO)):common.sha(p) for p in inputs}
    if directory:
        common.run(['make','-C',str(directory),'library','BUILD='+str(target),'CC='+shlex.join([cc,*flags])],work/'profile-build.log')
        extra=list(target.glob('*.a'))
        if len(extra)!=1:raise RuntimeError('expected one source-profile archive')
    objects=[]
    for i,unit in enumerate(units):
        obj=work/(str(i)+'.o');common.run([cc,*flags,'-c',str(unit),'-o',str(obj)],work/('build-'+str(i)+'.log'));objects.append(obj)
    libs=sdk/'usr/lib';out=work/'drift-rng-service'
    command=['ld','-m','elf_i386','-static','-e','_start','-z','noseparate-code','-z','norelro','-z','noexecstack','-o',str(out)]
    command+=list(map(str,[libs/'crt0.o',libs/'crti.o',libs/'crtbeginT.o',*objects]))+list(map(str,extra))+['-L'+str(libs),'-lsys','-lc','-lsys']+list(map(str,[libs/'crtend.o',libs/'crtn.o']))
    common.run(command,work/'link.log')
    if subprocess.check_output(['nm','-u',str(out)]).strip():raise RuntimeError('unresolved payload symbols')
    after={str(p.relative_to(REPO)):common.sha(p) for p in inputs}
    if before!=after:raise RuntimeError('sources changed during build; rerun with stable inputs')
    common.save(work/'build-metadata.json',{'compiler':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'flags':flags,'profile':kind,'sources':after,'base_iso_sha256':common.ISO_SHA,'qemu_packages_manifest_sha256':common.sha(HERE.parent/'qemu-ubuntu-noble.json'),'payload_sha256':common.sha(out),'payload_bytes':out.stat().st_size})
    return out

def patch(iso,work,payload):
    target=work/'drift-rng-test.iso';shutil.copyfile(iso,target);records=[]
    for sector,name,data,keep in [(82088,'/bin/fetch',payload.read_bytes(),True),(12288,'/etc/rc.cd',(HERE/'rc-test.sh').read_bytes(),False)]:
        fs=MFS(target,sector);ino=fs.lookup(name);mode,size,z=fs.inode(ino);blocks=list(z[:7]);count=(size+fs.zs-1)//fs.zs
        if count>7:blocks+=list(fs.blocks(z[7]))
        if count>len(blocks) or len(data)>(size if keep else count*fs.zs):raise RuntimeError('donor capacity exceeded')
        old=fs.content(ino);padded=data.ljust(size if keep else (len(data)+fs.zs-1)//fs.zs*fs.zs,b'\0')
        with target.open('r+b') as f:
            for i in range((len(padded)+fs.zs-1)//fs.zs):f.seek(fs.base+blocks[i]*fs.zs);f.write(padded[i*fs.zs:(i+1)*fs.zs])
            if not keep:f.seek(fs.base+fs.ib+(ino-1)*64+8);f.write(struct.pack('<I',len(data)))
        records.append({'path':name,'partition_sector':sector,'old_sha256':hashlib.sha256(old).hexdigest(),'payload_sha256':hashlib.sha256(data).hexdigest(),'kept_size':keep})
    common.save(work/'image-changes.json',{'base_sha256':common.ISO_SHA,'test_sha256':common.sha(target),'changes':records});return target

def guest(work,q,env,boot,iso,kind):
    fixture=work/'rng-input.bin';state=0x6d2b79f5;data=bytearray()
    for _ in range(1024*1024):
        state^=(state<<13)&0xffffffff;state^=state>>17;state^=(state<<5)&0xffffffff
        data.append(state>>24)
    fixture.write_bytes(data)
    if common.sha(fixture)!='9be4f35563ede119bb4a85dd735a75da95b2d815d83a28e257efa93296d384a7':raise RuntimeError('fixture generator mismatch')
    command=[str(q/'usr/bin/qemu-system-i386'),'-machine','pc,accel=tcg','-m','256','-bios',str(q/'usr/share/seabios/bios-256k.bin'),'-L',str(q/'usr/share/qemu'),'-kernel',str(boot/'kernel'),'-initrd',','.join(str(p) for p in sorted(boot.glob('mod*'))),'-append','bootcd=1 cdproberoot=1 rootdevname=ram disable=inet cttyline=1 cttybaud=115200 nobeep=1 hz=60','-cdrom',str(iso),'-display','none','-serial','stdio','-serial','file:'+str(work/'kernel-serial.log'),'-monitor','none','-no-reboot','-nic','none','-object','rng-random,id=drift-rng,filename='+str(fixture),'-device','virtio-rng-pci,rng=drift-rng,disable-legacy=on']
    common.save(work/'command.json',command)
    (work/'qemu-version.txt').write_text(subprocess.check_output([str(q/'usr/bin/qemu-system-i386'),'--version'],env=env,text=True))
    proc=subprocess.Popen(command,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env);output=bytearray();start=time.monotonic()
    try:
        while time.monotonic()-start<150:
            if proc.poll() is not None:output.extend(proc.stdout.read());break
            if select.select([proc.stdout],[],[],1)[0]:
                b=os.read(proc.stdout.fileno(),65536)
                if b:output.extend(b);print(b.decode(errors='replace'),end='',flush=True)
            if b'DRIFT_MINIX_RNG_ALL_DONE' in output:break
    finally:
        if proc.poll() is None:proc.terminate()
        try:proc.wait(timeout=5)
        except subprocess.TimeoutExpired:proc.kill();proc.wait()
    (work/'serial.log').write_bytes(output);text=output.decode(errors='replace')
    checks={k:v in text for k,v in {'up':'DRIFT_MINIX_RNG_UP_EXIT=0','down':'DRIFT_MINIX_RNG_DOWN_EXIT=0','status':'DRIFT_MINIX_RNG_STATUS=PASS'}.items()}
    if kind=='raw':
        checks.update({k:v in text for k,v in {'pci':'vendor_device=10441af4','probe':'DRIFT_MINIX_RNG_PROBE state=4','data':'DRIFT_MINIX_RNG_DATA bytes=128 exact_fixture=1 used_ring_callback=1 hardware_irq=0','pass':'DRIFT_MINIX_RNG_PASS:','resource_denial':'DRIFT_MINIX_RNG_DENIAL physical=fee00000 map_failed=1 errno_unavailable=1','reset_quarantine':'DRIFT_MINIX_RNG_RESET_FAULT injected=write-drop code=-2 allocations=2 releases=0 retained=1 cleanup_refused=1','balanced_release':'DRIFT_MINIX_RNG_CLEANUP allocations=2 releases=2'}.items()})
    else:
        checks.update({k:v in text for k,v in {'pci':'vendor_device=10441af4','data':'DRIFT_MINIX_RNG_PROFILE_DATA kind='+kind+' bytes=512 exact_fixture=1 hardware_irq=0 canaries=1','pass':'DRIFT_MINIX_RNG_PROFILE_PASS kind='+kind+' unchanged_source=1 actual_device_dma=1 polled_completion=1'}.items()})
    if kind=='linux':
        checks['prefetch']='DRIFT_MINIX_RNG_PROFILE_PREFETCH kind=linux delivered=1 discarded_bytes=64 restored_absolute_offset=576' in text
        checks['hash']='DRIFT_MINIX_RNG_PROFILE_HASH kind=linux fnv64=acb83090e9c1a505' in text
        checks['lifecycle']='DRIFT_MINIX_RNG_PROFILE_LIFECYCLE kind=linux freeze_restore=1 restored_bytes=64 restored_absolute_offset=576 exact_fixture=1 cancel_next=-4' in text
        checks['balanced_release']='DRIFT_MINIX_RNG_PROFILE_CLEANUP kind=linux allocations=4 releases=4' in text
    if kind=='windows':
        checks['hash']='DRIFT_MINIX_RNG_PROFILE_HASH kind=windows fnv64=0c6cd5630a81302c' in text
        checks['wdf_completion']='DRIFT_MINIX_RNG_PROFILE_WDF completions=1 notifications=1 information=512' in text
        checks['lifecycle']='DRIFT_MINIX_RNG_PROFILE_LIFECYCLE kind=windows cancel_before_dispatch=1 cancelled_once=1 output_unchanged=1 dma_retained=1' in text
        checks['balanced_release']='DRIFT_MINIX_RNG_PROFILE_CLEANUP kind=windows allocations=2 releases=2' in text
    checks['no_failure']=not any(v in text for v in ['DRIFT_MINIX_RNG_FAIL','DRIFT_MINIX_RNG_QUARANTINE','DRIFT_MINIX_RNG_PROFILE_FAIL','DRIFT_MINIX_RNG_PROFILE_QUARANTINE','DRIFT_MINIX_RNG_PROFILE_DATA_FAIL','DRIFT_MINIX_RNG_PROFILE_READ_FAIL'])
    result={'status':'passed' if all(checks.values()) else 'failed','checks':checks,'elapsed_seconds':time.monotonic()-start,'serial_sha256':hashlib.sha256(output).hexdigest(),'fixture_sha256':common.sha(fixture),'fixture':'xorshift32 seed0x6d2b79f5 highbyte; 1MiB deterministic I/O oracle, not cryptographic entropy','profile':kind,'scope':'actual MINIX RS PCI reservation/BAR MMIO/coherent page DMA/raw modern Virtio split queue in QEMU TCG; polling used-ring completion; no physical IRQ or hardware qualification; '+('raw transport only' if kind=='raw' else 'unchanged '+kind+' source personality, separately linked executable')}
    common.save(work/'result.json',result)
    if result['status']!='passed':raise RuntimeError('MINIX RNG qualification failed; preserve serial/result')
    return result

def main():
    p=argparse.ArgumentParser();p.add_argument('--work',type=Path,required=True);p.add_argument('--iso',type=Path);p.add_argument('--qemu-root',type=Path);p.add_argument('--sdk',type=Path);p.add_argument('--cc',default='cc');p.add_argument('--profile',choices=['raw','linux','windows'],default='raw');a=p.parse_args();work=a.work.resolve();work.mkdir(parents=True,exist_ok=True)
    iso=common.image(work,a.iso);sdk=a.sdk.resolve() if a.sdk else common.sdk_from_image(iso,work);q,env=common.tool_qemu(work,a.qemu_root)
    payload=build(work,sdk,a.cc,a.profile);test=patch(iso,work,payload);boot=common.boot_files(iso,work);print(json.dumps(guest(work,q,env,boot,test,a.profile),indent=2))
if __name__=='__main__':main()
