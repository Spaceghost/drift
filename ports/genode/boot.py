#!/usr/bin/env python3
"""Boot a built Genode example under QEMU TCG and retain real guest evidence."""
from pathlib import Path
import argparse, hashlib, json, os, re, shutil, statistics, subprocess, time
import sys
sys.dont_write_bytecode = True
from source_tree import digest as source_tree_digest

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--genode-dir', type=Path, required=True)
p.add_argument('--build-dir', type=Path, default=Path('build/genode'))
p.add_argument('--scenario', choices=['drift_pcspkr', 'drift_pcspkr_denied'], default='drift_pcspkr')
p.add_argument('--qemu', default='qemu-system-x86_64')
p.add_argument('--qemu-data', type=Path)
p.add_argument('--bios', type=Path)
p.add_argument('--module-dir', type=Path)
p.add_argument('--timeout', type=int, default=45)
a = p.parse_args()
src, build = a.genode_dir.resolve(), a.build_dir.resolve()
denied = a.scenario.endswith('_denied')
if not 1 <= a.timeout <= 300:
    p.error('timeout must be 1..300 seconds')
qemu = shutil.which(a.qemu)
if not qemu:
    p.error('QEMU x86_64 is required')
image = build / 'var/run' / a.scenario / 'boot/image-hw.elf'
expected = json.loads((build / (a.scenario + '-build.json')).read_text())
if source_tree_digest(src) != expected['genode_source_tree_sha256']:
    p.error('Genode SDK source differs from the successful build')
root = Path(__file__).resolve().parents[2]
for relative, digest in expected['source_sha256'].items():
    source = root / relative
    if not source.is_file() or hashlib.sha256(source.read_bytes()).hexdigest() != digest:
        p.error('Source differs from the successful build: ' + relative)
sha = hashlib.sha256(image.read_bytes()).hexdigest()
if sha != expected['image_sha256']:
    p.error('Image differs from the successful build metadata')
cmd = [qemu]
if a.qemu_data:
    cmd += ['-L', str(a.qemu_data.resolve())]
if a.bios:
    cmd += ['-bios', str(a.bios.resolve())]
cmd += ['-accel', 'tcg', '-display', 'none', '-serial', 'stdio', '-monitor', 'none', '-m', '512',
        '-machine', 'q35', '-cpu', 'Skylake-Client', '-no-reboot', '-no-shutdown', '-net', 'none',
        '-kernel', str(src / 'tool/boot/bender'), '-append', 'serial intel_hwp_off', '-initrd', str(image)]
env = os.environ.copy()
if a.module_dir:
    env['QEMU_MODULE_DIR'] = str(a.module_dir.resolve())
log = build / (a.scenario + '-guest.log')
marker = b'stop because parent denied IO_PORT-session' if denied else b'Drift Linux pcspkr Genode REAL-IO PASS'
start, seen = time.monotonic(), False
with log.open('wb') as f:
    process = subprocess.Popen(cmd, stdout=f, stderr=subprocess.STDOUT, env=env)
    try:
        while process.poll() is None and time.monotonic() - start < a.timeout:
            if marker in log.read_bytes():
                seen = True
                break
            time.sleep(.1)
    finally:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
raw = log.read_bytes()
seen = seen or marker in raw
text, rows = raw.decode(errors='replace'), []
for m in re.finditer(r'DRIFT_GENODE_SAMPLE round=(\d+) position=(\d+) variant=(\w+) cycles=(\d+) rpc=(\d+) writes=(\d+) reads=(\d+) delays=(\d+) guest_us=(\d+)', text):
    rows.append({k: (v if k == 'variant' else int(v)) for k, v in
                 zip(['round', 'position', 'variant', 'cycles', 'rpc', 'writes', 'reads', 'delays', 'guest_us'], m.groups())})
if denied:
    passed = seen and b'resource-backed service ready' not in raw and not rows
else:
    passed = (seen and b'Error:' not in raw and len(rows) == 8 and len({(r['round'], r['position']) for r in rows}) == 8
              and all((r['cycles'], r['rpc'], r['writes'], r['reads'], r['delays']) == (100, 400, 500, 400, 500) for r in rows))
result = {'test': 'expected framework resource-denial fail-stop' if denied else 'real I/O, RPC, readback, lifecycle and paired service diagnostic',
          'pass': passed, 'expected_marker': marker.decode(), 'marker_seen': seen,
          'qemu_exit_after_controlled_stop': process.returncode, 'command': cmd, 'image_sha256': sha,
          'bender_sha256': hashlib.sha256((src / 'tool/boot/bender').read_bytes()).hexdigest(),
          'log_sha256': hashlib.sha256(raw).hexdigest(), 'samples': rows,
          'median_guest_us_per_400_rpc': {v: statistics.median(r['guest_us'] for r in rows if r['variant'] == v)
                                        for v in ['import', 'reference']} if rows else {},
          'scope': 'Emulated PC speaker; timing includes RPC, real port I/O, locks, readbacks and minimum delays. Not an OS capacity or native-driver ranking.'}
(build / (a.scenario + '-result.json')).write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
print(log)
raise SystemExit(0 if passed else 2)
