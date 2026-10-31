#!/usr/bin/env python3
"""Build/boot the shared component MODEL fixture; never accesses device ports."""
from pathlib import Path
import argparse,hashlib,importlib.util,json,os,re,shutil,statistics,subprocess,sys,time
sys.dont_write_bytecode=True
port=Path(__file__).resolve().parent;parent=port.parent;root=port.parents[2]
spec=importlib.util.spec_from_file_location('source_tree',parent/'source_tree.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--genode-dir',type=Path,required=True);p.add_argument('--build-dir',type=Path,default=Path('build/genode-component-model'))
p.add_argument('--toolchain-prefix',required=True);p.add_argument('--expect',default='expect');p.add_argument('--qemu',default='qemu-system-x86_64')
p.add_argument('--qemu-data',type=Path);p.add_argument('--bios',type=Path);p.add_argument('--module-dir',type=Path);p.add_argument('--timeout',type=int,default=180)
a=p.parse_args();src=a.genode_dir.resolve();build=a.build_dir.resolve()
if not 1<=a.timeout<=300:p.error('timeout must be 1..300')
pins=dict(line.split('=',1) for line in (parent/'SDK').read_text().splitlines() if line)
sdk_digest=module.digest(src)
if sdk_digest!=pins['source_tree_sha256']:p.error('SDK tree differs from pinned archive')
expect,qemu=shutil.which(a.expect),shutil.which(a.qemu)
if not expect or not qemu:p.error('Expect and QEMU x86_64 are required')
if any(' ' in str(x) for x in [src,build,root]):p.error('Genode make paths must not contain spaces')
if not build.exists():subprocess.run([str(src/'tool/create_builddir'),'x86_64','BUILD_DIR='+str(build)],check=True)
line='REPOSITORIES += '+str(port);cfg=build/'etc/build.conf'
if line not in cfg.read_text():cfg.write_text(cfg.read_text()+'\n'+line+'\n')
tools=build/'etc/tools.conf';desired='CROSS_DEV_PREFIX = '+a.toolchain_prefix+'\n'
if tools.exists() and tools.read_text()!=desired:p.error('Existing tools.conf differs')
tools.write_text(desired)
env=os.environ.copy();env.update(KERNEL='hw',BOARD='pc',MAKEFLAGS='-j4',VERBOSE='')
repos=[src/'repos'/name for name in ['base-hw','base','os','demo']]+[port]
command=[expect,str(src/'tool/run/run'),'--genode-dir',str(src),'--name','linux_component','--specs','hw x86_64 x86 64bit','--board','pc','--repositories',' '.join(map(str,repos)),'--cross-dev-prefix',a.toolchain_prefix,'--include','boot_dir/hw','--include',str(port/'run/linux_component.run')]
with (build/'build.log').open('wb') as f:r=subprocess.run(command,cwd=build,env=env,stdout=f,stderr=subprocess.STDOUT)
if r.returncode:raise SystemExit(r.returncode)
image=build/'var/run/linux_component/boot/image-hw.elf'
logtext=(build/'build.log').read_text(errors='replace')
commands=[line for line in logtext.splitlines() if ' -c ' in line and any(name in line for name in ['bench_linux_import.c','/driver.c','compat.generated.c'])]
if len(commands)!=3 or any('-Os' not in line for line in commands):raise SystemExit('Did not verify all three fixture C translation units use -Os')
(build/'fixture-compiler-commands.log').write_text('\n'.join(commands)+'\n')
cmd=[qemu]
if a.qemu_data:cmd+=['-L',str(a.qemu_data.resolve())]
if a.bios:cmd+=['-bios',str(a.bios.resolve())]
cmd+=['-accel','tcg','-display','none','-serial','stdio','-monitor','none','-m','512','-machine','q35','-cpu','Skylake-Client','-no-reboot','-no-shutdown','-net','none','-kernel',str(src/'tool/boot/bender'),'-append','serial intel_hwp_off','-initrd',str(image)]
if a.module_dir:env['QEMU_MODULE_DIR']=str(a.module_dir.resolve())
log=build/'guest.log';marker=b'Drift Genode shared Linux component MODEL PASS';start=time.monotonic()
with log.open('wb') as f:
 process=subprocess.Popen(cmd,stdout=f,stderr=subprocess.STDOUT,env=env)
 try:
  while process.poll() is None and time.monotonic()-start<a.timeout:
   if marker in log.read_bytes():break
   time.sleep(.2)
 finally:
  if process.poll() is None:
   process.terminate()
   try:process.wait(timeout=3)
   except subprocess.TimeoutExpired:process.kill();process.wait()
raw=log.read_bytes();text=raw.decode(errors='replace');rows=[]
pattern=r'DRIFT_GENODE_COMPONENT sample=(\d+) round=(\d+) position=(\d+) variant=([\w-]+) elapsed_us=(\d+) events=(\d+) operations=(\d+) checksum=(0x[0-9a-f]+) control=(0x[0-9a-f]+) modeled_delay_us=(\d+)'
for match in re.finditer(pattern,text):
 values=match.groups();names=['sample','round','position','variant','elapsed_us','events','operations','checksum','control','modeled_delay_us']
 row={k:(v if k=='variant' else int(v,16) if k in ['checksum','control'] else int(v)) for k,v in zip(names,values)};rows.append(row)
passed=(marker in raw and b'Error:' not in raw and len(rows)==8 and {r['sample'] for r in rows}==set(range(1,9)) and all((r['events'],r['operations'],r['modeled_delay_us'],r['checksum'],r['control'])==(500000,3500000,1093750,0x64f59f6df89549d1,0xa8) for r in rows))
files=[x for base in [port,root/'imports/linux/pcspkr'] for x in base.rglob('*') if x.is_file() and 'records' not in x.parts and '__pycache__' not in x.parts]+[root/'tests/bench_linux_import.c',root/'tests/bench_linux_import.h']
result={'pass':passed,'scope':'component-model-no-hardware','modeled_waits_sleep':False,'io_port_authority':False,'sdk_tree_sha256':sdk_digest,'source_sha256':{str(x.relative_to(root)):hashlib.sha256(x.read_bytes()).hexdigest() for x in files},'compiler_commands':commands,'compiler':subprocess.check_output([a.toolchain_prefix+'g++','--version'],text=True).splitlines()[0],'build_command':command,'qemu_command':cmd,'image_sha256':hashlib.sha256(image.read_bytes()).hexdigest(),'samples':rows,'median_elapsed_us':{v:statistics.median(r['elapsed_us'] for r in rows if r['variant']==v) for v in ['imported-linux','direct-reference']} if rows else {}}
(build/'result.json').write_text(json.dumps(result,indent=2)+'\n')
if rows:
 keys=['sample','round','position','variant','elapsed_us','events','operations','checksum','control','modeled_delay_us']
 lines=['\t'.join(keys)]+['\t'.join(format(r[k],'016x') if k=='checksum' else format(r[k],'02x') if k=='control' else str(r[k]) for k in keys) for r in rows]
 (build/'samples.tsv').write_text('\n'.join(lines)+'\n')
print(json.dumps(result,indent=2));print(log);raise SystemExit(0 if passed else 2)
