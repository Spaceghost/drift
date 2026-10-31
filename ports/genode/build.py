#!/usr/bin/env python3
"""Build the pinned Genode PC-speaker example. No installation or hardware access."""
from pathlib import Path
import argparse, hashlib, json, os, shutil, subprocess
import sys
sys.dont_write_bytecode = True
from source_tree import digest as source_tree_digest

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--genode-dir', type=Path, required=True)
p.add_argument('--build-dir', type=Path, default=Path('build/genode'))
p.add_argument('--toolchain-prefix', required=True)
p.add_argument('--expect', default='expect')
p.add_argument('--jobs', type=int, default=4)
p.add_argument('--scenario', choices=['drift_pcspkr', 'drift_pcspkr_denied'], default='drift_pcspkr')
a = p.parse_args()
root = Path(__file__).resolve().parents[2]
port = Path(__file__).resolve().parent
src, build = a.genode_dir.resolve(), a.build_dir.resolve()
pins = dict(line.split('=', 1) for line in (port / 'SDK').read_text().splitlines() if line)
source_digest = source_tree_digest(src)
if source_digest != pins['source_tree_sha256']:
    p.error('Genode SDK source tree differs from the verified archive: ' + source_digest)
if (src / 'VERSION').read_text().strip() != '26.08':
    p.error('This tested port requires Genode 26.08')
if not 1 <= a.jobs <= 16:
    p.error('jobs must be 1..16')
if any(' ' in str(x) for x in [root, src, build]):
    p.error('Genode make paths must not contain spaces')
expect = shutil.which(a.expect)
if not expect:
    p.error('Expect is required; pass --expect /path/to/expect')
for line in (root / 'imports/linux/pcspkr/SHA256SUMS').read_text().splitlines():
    sha, relative = line.split(maxsplit=1)
    file = root / 'imports/linux/pcspkr' / relative
    if hashlib.sha256(file.read_bytes()).hexdigest() != sha:
        p.error('Upstream source checksum mismatch: ' + relative)
if not build.exists():
    subprocess.run([str(src / 'tool/create_builddir'), 'x86_64', 'BUILD_DIR=' + str(build)], check=True)
expected = 'CROSS_DEV_PREFIX = ' + a.toolchain_prefix + '\n'
tools = build / 'etc/tools.conf'
if tools.exists() and tools.read_text() != expected:
    p.error('Existing tools.conf differs; use a fresh build directory')
tools.write_text(expected)
config = build / 'etc/build.conf'
text, line = config.read_text(), 'REPOSITORIES += ' + str(port)
if line not in text:
    config.write_text(text + '\n' + line + '\n')
repos = [src / 'repos' / n for n in ['base-hw', 'base', 'os', 'demo']] + [port]
cmd = [expect, str(src / 'tool/run/run'), '--genode-dir', str(src), '--name', a.scenario,
       '--specs', 'hw x86_64 x86 64bit', '--board', 'pc', '--repositories', ' '.join(map(str, repos)),
       '--cross-dev-prefix', a.toolchain_prefix,
       '--include', 'boot_dir/hw', '--include', str(port / 'run' / (a.scenario + '.run'))]
log = build / (a.scenario + '-build.log')
env = os.environ.copy()
env.update(KERNEL='hw', BOARD='pc', MAKEFLAGS='-j' + str(a.jobs))
with log.open('wb') as f:
    result = subprocess.run(cmd, cwd=build, stdout=f, stderr=subprocess.STDOUT, env=env)
print(log)
if result.returncode:
    raise SystemExit(result.returncode)
image = build / 'var/run' / a.scenario / 'boot/image-hw.elf'
if not image.is_file():
    raise SystemExit('Build returned without a boot image')
files = [x for base in [port, root / 'imports/linux/pcspkr'] for x in base.rglob('*')
         if x.is_file() and '__pycache__' not in x.parts and 'records' not in x.parts
         and (x.suffix in ['.c', '.cc', '.h', '.mk', '.run', '.py', '.nelua']
              or x.name in ['SDK', 'SOURCE', 'SHA256SUMS', 'COPYING.GPL-2.0'])]
metadata = {'scenario': a.scenario, 'genode': '26.08',
            'genode_source_commit': pins['source_commit'], 'genode_source_tree_sha256': source_digest, 'command': cmd,
            'make_environment': {'KERNEL': 'hw', 'BOARD': 'pc', 'MAKEFLAGS': '-j' + str(a.jobs)},
            'image_sha256': hashlib.sha256(image.read_bytes()).hexdigest(),
            'source_sha256': {str(x.relative_to(root)): hashlib.sha256(x.read_bytes()).hexdigest() for x in files},
            'compiler': subprocess.check_output([a.toolchain_prefix + 'g++', '--version'], text=True).splitlines()[0]}
(build / (a.scenario + '-build.json')).write_text(json.dumps(metadata, indent=2) + '\n')
print(image)
