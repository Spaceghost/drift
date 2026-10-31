#!/usr/bin/env python3
"""Build the pinned Genode Virtio resource qualification. No host hardware access."""
from pathlib import Path
import argparse, fcntl, hashlib, json, os, shutil, subprocess
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from source_tree import digest as source_tree_digest

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--genode-dir', type=Path, required=True)
p.add_argument('--build-dir', type=Path, default=Path('build/genode-virtio-rng'))
p.add_argument('--toolchain-prefix', required=True)
p.add_argument('--expect', default='expect')
p.add_argument('--jobs', type=int, default=4)
p.add_argument('--scenario', choices=['rng_probe', 'rng_resources', 'rng_resources_denied', 'rng_linux', 'rng_windows', 'rng_linux_shootout', 'rng_windows_shootout'], default='rng_probe')
a = p.parse_args()
root = Path(__file__).resolve().parents[3]
port = Path(__file__).resolve().parent
src, build = a.genode_dir.resolve(), a.build_dir.resolve()
pins = dict(line.split('=', 1) for line in (port.parent / 'SDK').read_text().splitlines() if line)
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
profile = (root / 'imports/linux/virtio-rng' if a.scenario.startswith('rng_linux') else
           root / 'imports/windows/viorng' if a.scenario.startswith('rng_windows') else None)
if profile:
    for row in (profile / 'SHA256SUMS').read_text().splitlines():
        sha, relative = row.split(maxsplit=1)
        if hashlib.sha256((profile / relative).read_bytes()).hexdigest() != sha:
            p.error('Upstream source checksum mismatch: ' + relative)
created = not build.exists()
if created:
    subprocess.run([str(src / 'tool/create_builddir'), 'x86_64', 'BUILD_DIR=' + str(build)], check=True)
lock = (build / 'build.lock').open('a')
try:
    fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
except BlockingIOError:
    p.error('This build directory already has an active builder')
expected = 'CROSS_DEV_PREFIX = ' + a.toolchain_prefix + '\n'
tools = build / 'etc/tools.conf'
if tools.exists() and tools.read_text() != expected:
    p.error('Existing tools.conf differs; use a fresh build directory')
if not tools.exists():
    tools.write_text(expected)
repos = [src / 'repos' / n for n in ['base-hw', 'base', 'os', 'demo']] + [port]
config = build / 'etc/build.conf'
configuration = ('# Drift Genode Virtio RNG build configuration v1\n'
                 'GENODE_DIR := ' + str(src) + '\nBASE_DIR := $(GENODE_DIR)/repos/base\n'
                 'CONTRIB_DIR := $(GENODE_DIR)/contrib\nBOARD := pc\nKERNEL := hw\n'
                 'REPOSITORIES := ' + ' '.join(map(str, repos)) + '\nCC_OLEVEL := -O2\n')
if config.read_text() != configuration:
    if not created:
        p.error('Existing build.conf differs from the canonical configuration; use a fresh directory')
    config.write_text(configuration)
if (build / 'etc/specs.conf').read_text() != 'SPECS += x86_64\n':
    p.error('Existing specs.conf differs from the canonical architecture')
cmd = [expect, str(src / 'tool/run/run'), '--genode-dir', str(src), '--name', a.scenario,
       '--specs', 'hw x86_64 x86 64bit', '--board', 'pc', '--repositories', ' '.join(map(str, repos)),
       '--cross-dev-prefix', a.toolchain_prefix,
       '--include', 'boot_dir/hw', '--include', str(port / 'run' / (a.scenario + '.run'))]
log = build / (a.scenario + '-build.log')
env = {k: os.environ[k] for k in ['PATH', 'HOME', 'TMPDIR', 'TMP', 'TEMP', 'LANG', 'LC_ALL', 'LD_LIBRARY_PATH', 'TCL_LIBRARY'] if k in os.environ}
env.update(KERNEL='hw', BOARD='pc', MAKEFLAGS='-j' + str(a.jobs), VERBOSE='', CC_OLEVEL='-O2')
def source_hashes():
    component = a.scenario.removeprefix('rng_').removesuffix('_denied')
    paths = [port / 'build.py', port / 'boot.py', port.parent / 'SDK', port.parent / 'source_tree.py',
             port / 'run' / (a.scenario + '.run'),
             port / 'src/test' / ('rng_' + component) / 'main.cc',
             port / 'src/test' / ('rng_' + component) / 'target.mk']
    if component != 'probe':
        paths += [port / 'include/resources.h', port / 'run/rng_resources.run']
        paths += [root / n for n in ['generated/virtio.c', 'generated/pci.c', 'native/virtio.nelua',
                  'native/pci.nelua', 'lib/drift/hw.nelua', 'include/drift_hw.h', 'include/drift_pci.h',
                  'include/drift_virtio.h']]
    if component.startswith(('linux', 'windows')):
        paths += [port / 'include/fixture.h']
    if component.endswith('_shootout'):
        kind = component.removesuffix('_shootout')
        paths += [port / 'include/shootout.h', port / 'include' / (kind + '_case.h'),
                  port / 'src/test' / ('rng_' + kind) / 'target.mk']
    if profile:
        paths += [x for x in profile.rglob('*') if x.is_file() and
                  x.name not in ['test.c', 'tests.c', 'test_models.c'] and
                  (x.suffix in ['.c', '.h', '.nelua', '.tmh'] or
                   x.name in ['SOURCE', 'SHA256SUMS', 'COPYING.GPL-2.0', 'LICENSE'])]
    return {str(x.relative_to(root)): hashlib.sha256(x.read_bytes()).hexdigest() for x in sorted(set(paths))}

initial_sources = source_hashes()
with log.open('wb') as f:
    result = subprocess.run(cmd, cwd=build, stdout=f, stderr=subprocess.STDOUT, env=env)
print(log)
if result.returncode:
    raise SystemExit(result.returncode)
image = build / 'var/run' / a.scenario / 'boot/image-hw.elf'
if not image.is_file():
    raise SystemExit('Build returned without a boot image')
if source_hashes() != initial_sources:
    raise SystemExit('Source changed during build; rebuild before recording evidence')
component_name = a.scenario.removesuffix('_denied').replace('_', '-')
component = build / 'bin' / component_name
metadata = {'scenario': a.scenario, 'genode': '26.08',
            'genode_source_commit': pins['source_commit'], 'genode_source_tree_sha256': source_digest, 'command': cmd,
            'make_environment': {'KERNEL': 'hw', 'BOARD': 'pc', 'MAKEFLAGS': '-j' + str(a.jobs), 'VERBOSE': '', 'CC_OLEVEL': '-O2'},
            'image_sha256': hashlib.sha256(image.read_bytes()).hexdigest(),
            'component': component_name, 'component_sha256': hashlib.sha256(component.read_bytes()).hexdigest(),
            'component_size': subprocess.check_output([a.toolchain_prefix + 'size', str(component)], text=True),
            'source_sha256': initial_sources,
            'build_configuration': {str(x.relative_to(build)): hashlib.sha256(x.read_bytes()).hexdigest() for x in [config, tools, build / 'etc/specs.conf']},
            'compiler': subprocess.check_output([a.toolchain_prefix + 'g++', '--version'], text=True).splitlines()[0]}
(build / (a.scenario + '-build.json')).write_text(json.dumps(metadata, indent=2) + '\n')
print(image)
