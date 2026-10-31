#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Boot a private Linux guest, run both source models and real PC speaker I/O."""
import argparse, gzip, hashlib, json, os, re, shlex, shutil, stat, subprocess, time
import statistics
import urllib.request
from pathlib import Path

HERE=Path(__file__).resolve().parent
REPO=HERE.parent.parent
RELEASE='20250803+deb13u7'
BASE=f'https://deb.debian.org/debian/dists/trixie/main/installer-amd64/{RELEASE}/images/'
KERNEL_SHA='2b2358b37674d2505350528875bb17afae2a36522a9e8a9417eaca65a7da0e08'

def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        while data:=f.read(1024*1024): h.update(data)
    return h.hexdigest()

def save(path,value): path.write_text(json.dumps(value,indent=2)+'\n')

def fetch(url,path,expected):
    if path.is_file() and sha(path)==expected: return
    partial=path.with_suffix(path.suffix+'.partial')
    with urllib.request.urlopen(url,timeout=60) as response,partial.open('wb') as out:
        if response.status!=200: raise RuntimeError('expected complete HTTP200 download: '+url)
        shutil.copyfileobj(response,out)
    if sha(partial)!=expected: raise RuntimeError('download checksum mismatch: '+url)
    partial.replace(path)

def kernel(work):
    # Retain and check the official release checksum manifest as provenance.
    url=BASE+'SHA256SUMS'
    with urllib.request.urlopen(url,timeout=30) as response: manifest=response.read()
    (work/'debian-SHA256SUMS.txt').write_bytes(manifest)
    rows=[line for line in manifest.decode().splitlines() if line.endswith('netboot/debian-installer/amd64/linux')]
    if len(rows)!=1 or rows[0].split()[0]!=KERNEL_SHA: raise RuntimeError('unexpected Debian kernel checksum manifest')
    path=work/'vmlinuz';fetch(BASE+'netboot/debian-installer/amd64/linux',path,KERNEL_SHA)
    return path

def qemu(work,requested):
    root=requested.resolve() if requested else work/'qemu-root'
    if not requested:
        inventory=REPO/'ports/minix/qemu-ubuntu-noble.json'
        for package in json.loads(inventory.read_text()):
            deb=work/package['url'].rsplit('/',1)[-1]
            fetch(package['url'],deb,package['sha256'])
            subprocess.run(['dpkg-deb','-x',str(deb),str(root)],check=True)
        for firmware in (root/'usr/share/seabios').glob('*.bin'):
            destination=root/'usr/share/qemu'/firmware.name
            if not destination.exists(): destination.symlink_to('../seabios/'+firmware.name)
    binary=root/'usr/bin/qemu-system-x86_64'
    if not binary.is_file(): raise RuntimeError('missing private qemu-system-x86_64')
    env=dict(os.environ,LD_LIBRARY_PATH=str(root/'usr/lib/x86_64-linux-gnu'),
             QEMU_MODULE_DIR=str(root/'usr/lib/x86_64-linux-gnu/qemu'))
    return root,binary,env

def command(argv,log):
    argv=list(map(str,argv))
    with log.open('w') as output:
        output.write('$ '+shlex.join(argv)+'\n');output.flush()
        subprocess.run(argv,cwd=REPO,stdout=output,stderr=subprocess.STDOUT,check=True)

def inputs():
    files=[HERE/'guest.c',HERE/'run.py']
    for folder in ['imports/linux/pcspkr','imports/windows/moufiltr']:
        files+=sorted(p for p in (REPO/folder).rglob('*') if p.is_file())
    return {str(p.relative_to(REPO)):sha(p) for p in files}

def build(work,cc):
    snapshot=inputs()
    target=subprocess.check_output([cc,'-dumpmachine'],text=True).strip()
    if not target.startswith('x86_64') or 'linux' not in target:
        raise RuntimeError('this recipe requires a native x86_64 Linux C compiler')
    for profile in ['linux/pcspkr','windows/moufiltr']:
        subprocess.run(['sha256sum','-c','SHA256SUMS'],cwd=REPO/'imports'/profile,check=True)
    flags=['-std=c11','-Os','-fno-strict-aliasing','-fno-stack-protector','-fno-pie','-no-pie','-static']
    linux=REPO/'imports/linux/pcspkr';windows=REPO/'imports/windows/moufiltr'
    linux_objects=work/'linux-objects'
    command(['make','-B','-C',linux,'library','BUILD='+str(linux_objects),'CC='+cc,'CFLAGS=-Os'],work/'linux-library-build.log')
    archive=linux_objects/'libdrift-linux-pcspkr.a'
    command([cc,*flags,'-I'+str(linux),HERE/'guest.c',archive,'-pthread','-o',work/'init'],work/'init-build.log')
    command([cc,*flags,'-I'+str(linux),'-I'+str(linux/'compat'),linux/'test.c',archive,'-pthread',
             '-Wl,--wrap=drift_lx_input_allocate_device,--wrap=drift_lx_input_register_device','-o',work/'linux-model'],work/'linux-model-build.log')
    windows_objects=work/'windows-objects'
    command(['make','-B','-C',windows,'library','BUILD='+str(windows_objects),'CC='+cc,'CFLAGS=-Os'],work/'windows-library-build.log')
    command([cc,*flags,'-I'+str(windows),'-I'+str(windows/'compat'),windows/'tests.c',windows_objects/'libdrift-windows-moufiltr.a',
             '-Wl,--wrap=WdfDriverCreate,--wrap=WdfDeviceCreate,--wrap=WdfIoQueueCreate','-o',work/'windows-model'],work/'windows-model-build.log')
    for filename in ['init','linux-model','windows-model']:
        if subprocess.check_output(['nm','-u',str(work/filename)]).strip(): raise RuntimeError('undefined symbols in '+filename)
    if snapshot!=inputs(): raise RuntimeError('source changed during payload compilation')
    save(work/'build.json',{'sources':snapshot,'cc':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],
         'target':target,'flags':flags,'profile_cflags':'-Os',
         'payloads':{n:sha(work/n) for n in ['init','linux-model','windows-model']}})

def initramfs(work):
    archive=bytearray();inode=1
    def entry(name,mode,data=b'',rdevmajor=0,rdevminor=0):
        nonlocal inode
        encoded=name.encode()+b'\0'
        values=[inode,mode,0,0,2 if stat.S_ISDIR(mode) else 1,0,len(data),0,0,rdevmajor,rdevminor,len(encoded),0]
        archive.extend(b'070701'+''.join(f'{value:08x}' for value in values).encode()+encoded)
        archive.extend(b'\0'*(-len(archive)%4));archive.extend(data);archive.extend(b'\0'*(-len(archive)%4));inode+=1
    for directory in ['dev','proc','sys']: entry(directory,stat.S_IFDIR|0o755)
    entry('dev/console',stat.S_IFCHR|0o600,rdevmajor=5,rdevminor=1)
    for name in ['init','linux-model','windows-model']: entry(name,stat.S_IFREG|0o755,(work/name).read_bytes())
    entry('TRAILER!!!',0)
    output=work/'initramfs.cpio.gz'
    with output.open('wb') as f:
        with gzip.GzipFile(fileobj=f,mode='wb',mtime=0) as stream: stream.write(archive)
    return output

def qualify(args,work):
    vmlinuz=kernel(work);qroot,binary,env=qemu(work,args.qemu_root)
    build(work,args.cc);ramfs=initramfs(work)
    meta=json.loads((work/'build.json').read_text())
    if meta['sources']!=inputs(): raise RuntimeError('source changed while preparing image')
    if any(sha(work/name)!=digest for name,digest in meta['payloads'].items()):
        raise RuntimeError('compiled payload changed before boot')
    argv=[str(binary),'-L',str(qroot/'usr/share/qemu'),'-bios',str(qroot/'usr/share/seabios/bios-256k.bin'),
          '-machine','q35,accel=tcg','-cpu','max','-m','256M','-smp','1','-display','none','-serial','stdio',
          '-monitor','none','-nic','none','-no-reboot','-kernel',str(vmlinuz),'-initrd',str(ramfs),
          '-append','console=ttyS0 quiet panic=-1 rdinit=/init modprobe.blacklist=pcspkr']
    version=subprocess.check_output([str(binary),'--version'],env=env,text=True).splitlines()[0]
    started=time.monotonic()
    with (work/'guest-serial.log').open('w') as log:
        process=subprocess.Popen(argv,cwd=work,env=env,stdout=log,stderr=subprocess.STDOUT)
        try: returncode=process.wait(timeout=args.timeout)
        except subprocess.TimeoutExpired:
            process.kill();process.wait();raise RuntimeError('Linux guest timed out; inspect guest-serial.log')
    text=(work/'guest-serial.log').read_text()
    samples=[]
    for row in re.findall(r'^PIO_SAMPLE\t([^\r\n]+)',text,re.M):
        fields=row.split('\t');samples.append({'round':int(fields[0]),'variant':fields[1],'elapsed_us':int(fields[2]),
                'requests':int(fields[3]),'writes':int(fields[4]),'reads':int(fields[5]),'delays':int(fields[6]),'digest':fields[7]})
    required=['DRIFT_LINUX_KERNEL_IOPERM_DENIAL_PASS','DRIFT_LINUX_GUEST_BOTH_SOURCE_MODELS_PASS',
              'DRIFT_LINUX_REAL_PIO_PASS','DRIFT_LINUX_GUEST_EXIT=0','Windows Moufiltr: 584 checks passed']
    if returncode or any(marker not in text for marker in required) or len(samples)!=8:
        raise RuntimeError('Linux guest did not pass every gate; inspect guest-serial.log')
    if len({x['digest'] for x in samples})!=1: raise RuntimeError('trial transcript mismatch')
    if [(x['round'],x['variant']) for x in samples]!=[(r,v) for r in range(1,5) for v in (['reference','import'] if r%2 else ['import','reference'])]:
        raise RuntimeError('unexpected paired trial ordering')
    if any((x['requests'],x['writes'],x['reads'],x['delays'])!=(200,500,200,500) for x in samples):
        raise RuntimeError('real I/O or delay count mismatch')
    result={'status':'passed','scope':'Linux x86_64 guest; two source-profile models and actual authorized PIT2 I/O',
            'not_claimed':['physical hardware','Windows kernel loading','native-kernel-driver performance'],
            'guest':re.search(r'^DRIFT_LINUX_GUEST (.+)',text,re.M).group(1),
            'kernel_url':BASE+'netboot/debian-installer/amd64/linux','kernel_sha256':sha(vmlinuz),
            'debian_manifest_url':BASE+'SHA256SUMS','qemu':version,'qemu_sha256':sha(binary),
            'initramfs_sha256':sha(ramfs),'command':argv,'qemu_exit':returncode,'host_elapsed_seconds':time.monotonic()-started,
            'source_inputs':meta['sources'],'build':meta,'samples':samples,
            'median_elapsed_us':{v:statistics.median(x['elapsed_us'] for x in samples if x['variant']==v) for v in ['import','reference']}}
    save(work/'result.json',result);print(json.dumps({'status':'passed','samples':len(samples),'log':str(work/'guest-serial.log')}))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work',type=Path,default=REPO/'build/linux-preflight')
    parser.add_argument('--qemu-root',type=Path)
    parser.add_argument('--cc',default='cc')
    parser.add_argument('--timeout',type=int,default=90)
    args=parser.parse_args()
    if not 5<=args.timeout<=300: parser.error('timeout must be 5..300 seconds')
    work=args.work.resolve();work.mkdir(parents=True,exist_ok=True)
    save(work/'result.json',{'status':'preparing','scope':'Linux guest source-models and real authorized PIT2 PIO; not physical hardware'})
    try: qualify(args,work)
    except Exception as error:
        save(work/'result.json',{'status':'failed','error':type(error).__name__+': '+str(error),
             'scope':'incomplete Linux guest qualification; no passing measurements claimed',
             'log':str(work/'guest-serial.log')})
        raise

if __name__=='__main__': main()
