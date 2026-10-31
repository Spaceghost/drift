#!/usr/bin/env python3
"""Boot the Genode Virtio resource probe and preserve actual guest evidence."""
from pathlib import Path
import argparse, fcntl, re, hashlib, json, os, select, shutil, statistics, subprocess, sys, time
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from source_tree import digest

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--genode-dir', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--scenario', choices=['rng_probe', 'rng_resources', 'rng_resources_denied', 'rng_linux', 'rng_windows', 'rng_linux_shootout', 'rng_windows_shootout'], default='rng_probe')
p.add_argument('--qemu', required=True)
p.add_argument('--qemu-data', type=Path, required=True)
p.add_argument('--bios', type=Path, required=True)
p.add_argument('--module-dir', type=Path, required=True)
p.add_argument('--timeout', type=int, default=60)
a = p.parse_args()
if not 1 <= a.timeout <= 300:
    p.error('timeout must be 1..300 seconds')
root = Path(__file__).resolve().parents[3]
src, build = a.genode_dir.resolve(), a.build_dir.resolve()
build_lock = (build / 'build.lock').open('a')
try:
    fcntl.flock(build_lock, fcntl.LOCK_SH | fcntl.LOCK_NB)
except BlockingIOError:
    p.error('This build directory has an active builder')
lock = (build / (a.scenario + '.lock')).open('a')
try:
    fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
except BlockingIOError:
    p.error('This scenario already has an active runner')
meta = json.loads((build / (a.scenario + '-build.json')).read_text())
if digest(src) != meta['genode_source_tree_sha256']:
    p.error('SDK differs from successful build')
for relative, sha in meta['build_configuration'].items():
    if hashlib.sha256((build / relative).read_bytes()).hexdigest() != sha:
        p.error('Build configuration differs from successful build: ' + relative)
for relative, sha in meta['source_sha256'].items():
    if hashlib.sha256((root / relative).read_bytes()).hexdigest() != sha:
        p.error('Source differs from successful build: ' + relative)
image = build / 'var/run' / a.scenario / 'boot/image-hw.elf'
if hashlib.sha256(image.read_bytes()).hexdigest() != meta['image_sha256']:
    p.error('Image differs from successful build')
if hashlib.sha256((build / 'bin' / meta['component']).read_bytes()).hexdigest() != meta['component_sha256']:
    p.error('Component differs from successful build')
log = build / (a.scenario + '-guest.log')
qemu_log = build / (a.scenario + '-qemu.log')
fixture = build / (a.scenario + '-rng-input.bin')
fixture_data = bytearray()
state = 0x6d2b79f5
for _ in range(1048576):
    state ^= (state << 13) & 0xffffffff
    state ^= state >> 17
    state ^= (state << 5) & 0xffffffff
    state &= 0xffffffff
    fixture_data.append(state >> 24)
fixture.write_bytes(fixture_data)
fixture_sha = hashlib.sha256(fixture.read_bytes()).hexdigest()
if fixture_sha != '9be4f35563ede119bb4a85dd735a75da95b2d815d83a28e257efa93296d384a7':
    p.error('Entropy fixture digest mismatch')
log.write_bytes(b'')
cmd = [shutil.which(a.qemu) or a.qemu, '-L', str(a.qemu_data.resolve()),
       '-bios', str(a.bios.resolve()), '-accel', 'tcg', '-display', 'none',
       '-serial', 'file:' + str(log), '-monitor', 'none', '-m', '512', '-machine', 'q35',
       '-cpu', 'Skylake-Client', '-no-reboot', '-no-shutdown', '-net', 'none',
       '-qmp', 'stdio',
       '-object', 'rng-random,id=rng0,filename=' + str(fixture),
       '-device', 'virtio-rng-pci,rng=rng0,disable-legacy=on,addr=3.0',
       '-kernel', str(src / 'tool/boot/bender'), '-append', 'serial intel_hwp_off',
       '-initrd', str(image)]
env = os.environ.copy()
env['QEMU_MODULE_DIR'] = str(a.module_dir.resolve())
marker = {'rng_probe': b'DRIFT_GENODE_RNG_PROBE PASS',
          'rng_resources': b'DRIFT_GENODE_RNG_RESOURCES PASS allocations=3 releases=3 mappings=0',
          'rng_resources_denied': b'DRIFT_GENODE_RNG_RESOURCES DENIED PASS allocations=0 mappings=0',
          'rng_linux': b'DRIFT_GENODE_RNG_LINUX PASS bytes=576 exact=576 freeze_restore=1 cancellation=1 mappings=0',
          'rng_windows': b'DRIFT_GENODE_RNG_WINDOWS PASS bytes=576 exact_prefix=576 queued_cancel=1 active_cancel=1 notifications=5 mappings=0',
          'rng_linux_shootout': b'DRIFT_GENODE_RNG_SHOOTOUT PASS profile=linux rows=8 delivered_bytes=262144 allocations=16 releases=16 mappings=0',
          'rng_windows_shootout': b'DRIFT_GENODE_RNG_SHOOTOUT PASS profile=windows rows=8 delivered_bytes=262144 allocations=16 releases=16 mappings=0'}[a.scenario]
started = time.monotonic()
seen = False
pci = None
with qemu_log.open('wb') as f:
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=f, env=env, bufsize=0)
    def response():
        if not select.select([proc.stdout], [], [], 5)[0]:
            raise RuntimeError('QMP reply timeout')
        value = json.loads(proc.stdout.readline())
        if 'error' in value:
            raise RuntimeError('QMP command failed: ' + str(value['error']))
        return value
    try:
        while proc.poll() is None and time.monotonic() - started < a.timeout:
            if b'Error:' in log.read_bytes():
                break
            if marker in log.read_bytes():
                seen = True
                response()
                proc.stdin.write(b'{"execute":"qmp_capabilities"}\n')
                proc.stdin.flush()
                while 'return' not in response():
                    pass
                proc.stdin.write(b'{"execute":"query-pci"}\n')
                proc.stdin.flush()
                while True:
                    reply = response()
                    if 'return' in reply:
                        pci = reply['return']
                        break
                break
            time.sleep(.1)
    finally:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
raw = log.read_bytes()
text = raw.decode(errors='replace')
targets = [dev for bus in (pci or []) for dev in bus['devices']
           if dev.get('id', {}).get('vendor') == 0x1af4 and dev['id']['device'] == 0x1044]
released = a.scenario in ['rng_resources', 'rng_linux', 'rng_windows', 'rng_linux_shootout', 'rng_windows_shootout']
pci_valid = (len(targets) == 1 and targets[0]['bus'] == 0 and targets[0]['slot'] == 3
             and targets[0]['function'] == 0 and
             {(r['bar'], r['size'], r['address']) for r in targets[0]['regions']}
             == {(1, 4096, -1 if released else 0xfebf1000),
                 (4, 16384, -1 if released else 0xfe000000)})
handshakes = [tuple(map(int, row)) for row in re.findall(
    r'RNG_HANDSHAKE round=(\d+) open=(-?\d+) state=(\d+) live=(\d+)', text)]
quiescence = [tuple(map(int, row)) for row in re.findall(
    r'RNG_QUIESCENCE round=(\d+) close=(-?\d+) state=(\d+) live=(\d+) allocated=(\d+) released=(\d+)', text)]
allocations = [{'token': int(t), 'size': int(n), 'cpu': int(c, 16), 'bus': int(b, 16)}
               for t, n, c, b in re.findall(
                   r'RNG_RESOURCE DMA token=(\d+) bytes=(\d+) cpu=(0x[0-9a-f]+) bus=(0x[0-9a-f]+)', text)]
expected_sizes = {'rng_probe': [], 'rng_resources_denied': [], 'rng_resources': [4096]*3,
                  'rng_linux': [4096, 64, 4096, 64], 'rng_windows': [4096, 4096],
                  'rng_linux_shootout': [size for r in range(4) for p in range(2)
                                         for size in [4096, 64 if (r+p)%2 else 4096]],
                  'rng_windows_shootout': [4096]*16}[a.scenario]
dma_valid = ([v['size'] for v in allocations] == expected_sizes and
             [v['token'] for v in allocations] == list(range(1, len(expected_sizes)+1)) and
             all(v['cpu'] and v['bus'] and v['cpu'] != v['bus'] for v in allocations))
if a.scenario == 'rng_probe':
    detail_valid = (b'host=0x29c08086 pciexbar_lo=0xb0000001 pciexbar_hi=0x0' in raw and
                    b'bdf=0x18 identity=0x10441af4 class=0xff0001 subsystem=0x11001af4' in raw and
                    b'bar=1 value=0xfebf1000' in raw and b'bar=4 value=0xfe00000c' in raw)
elif a.scenario == 'rng_resources':
    detail_valid = (handshakes == [(i, 0, 4, 1) for i in range(3)] and
                    quiescence == [(i, 0, 0, 0, i+1, i+1) for i in range(3)])
elif a.scenario == 'rng_resources_denied':
    detail_valid = (not handshakes and not quiescence and b'RNG_RESOURCE ' not in raw)
elif a.scenario == 'rng_linux':
    detail_valid = (b'RNG_LINUX OPEN status=0 transport=5 phase=2 live=2' in raw and
                    b'RNG_LINUX READ first=256 second=256 hash_first=0xaa59f13cde2a276 hash_second=0xc807aab7083387d3' in raw and
                    b'RNG_LINUX PREFETCH completion=1 bytes=64 next_offset=576' in raw and
                    b'RNG_LINUX CHECKSUM bytes=576 fnv64=0xacb83090e9c1a505' in raw and
                    b'RNG_LINUX FREEZE status=0 transport=0 phase=4 live=0' in raw and
                    b'RNG_LINUX RESTORE status=0 bytes=64 first=219 offset=576 transport=5 phase=2 live=2' in raw and
                    b'RNG_LINUX CLOSE status=0 transport=0 phase=0 live=0 allocated=4 released=4' in raw)
elif a.scenario == 'rng_windows':
    detail_valid = (b'RNG_WINDOWS OPEN status=0 transport=5 phase=2 live=2' in raw and
                    b'RNG_WINDOWS READ first=256 second=256 exact=1 notifications=2' in raw and
                    b'RNG_WINDOWS CHECKSUM bytes=576 fnv64=0xa7001fd839da826f' in raw and
                    b'RNG_WINDOWS CLOSE status=0 transport=0 phase=0 live=0 allocated=2 released=2 notifications=5' in raw)
else:
    detail_valid = True
rows = []
clock = {}
if a.scenario.endswith('_shootout'):
    measurements = re.findall(r'RNG_CLOCK unit_us=1 deltas=128 min_positive_us=(\d+) zero_deltas=(\d+)', text)
    if len(measurements) == 1:
        clock = {'unit_us': 1, 'deltas': 128, 'min_positive_consecutive_read_us': int(measurements[0][0]),
                 'zero_deltas': int(measurements[0][1]),
                 'scope': 'Timer RPC returns integer microseconds; observed minimum includes RPC/read overhead, not an intrinsic clock-resolution measurement'}
    names = ['round', 'position', 'variant', 'calls', 'bytes', 'offset', 'next_offset', 'prefetch',
             'features', 'allocations', 'releases', 'mappings', 'notifications', 'guest_us', 'fnv64']
    pattern = (r'RNG_SHOOTOUT round=(\d+) position=(\d+) variant=(\w+) calls=(\d+) bytes=(\d+) '
               r'offset=(\d+) next_offset=(\d+) prefetch=(\d+) features=(\d+) allocations=(\d+) '
               r'releases=(\d+) mappings=(\d+) notifications=(\d+) guest_us=(\d+) fnv64=(0x[0-9a-f]+)')
    for match in re.finditer(pattern, text):
        rows.append({name: value if name == 'variant' else int(value, 16 if name == 'fnv64' else 10)
                     for name, value in zip(names, match.groups())})
    profile = 'linux' if 'linux' in a.scenario else 'windows'
    cursor = 0
    detail_valid = (len(rows) == 8 and bool(clock) and
                    clock['min_positive_consecutive_read_us'] > 0 and 0 <= clock['zero_deltas'] < 128)
    for index, row in enumerate(rows):
        r, p = divmod(index, 2)
        variant = profile if (r+p)%2 else 'native'
        prefetch = 64 if variant == 'linux' else 0
        expected_hash = 14695981039346656037
        for value in fixture_data[cursor:cursor+32768]:
            expected_hash = ((expected_hash ^ value) * 1099511628211) & 0xffffffffffffffff
        detail_valid &= (row == {'round': r, 'position': p, 'variant': variant, 'calls': 64,
                                 'bytes': 32768, 'offset': cursor, 'next_offset': cursor+32768+prefetch,
                                 'prefetch': prefetch, 'features': 1, 'allocations': 2, 'releases': 2,
                                 'mappings': 0, 'notifications': 64 if variant == 'windows' else 0,
                                 'guest_us': row['guest_us'], 'fnv64': expected_hash} and row['guest_us'] > 0)
        cursor += 32768 + prefetch
    if rows:
        (build / (a.scenario + '-samples.tsv')).write_text(
            '\t'.join(names) + '\n' + ''.join('\t'.join(str(row[n]) for n in names) + '\n' for row in rows))
passed = seen and b'Error:' not in raw and pci_valid and detail_valid and dma_valid
scope = {'rng_probe': 'Actual guest PCI config reads matched against independent QMP resources; no DMA or entropy request',
         'rng_resources': 'Actual Platform/MMIO/coherent DMA feature/queue handshake and reset; no DRIVER_OK or entropy request',
         'rng_resources_denied': 'Platform policy denies device acquisition; no BAR mapping or DMA allocation',
         'rng_linux': 'Unchanged Linux source RNG over actual QEMU Virtio DMA, polling completions, cancellation and reset; no OS entropy integration',
         'rng_windows': 'Unchanged Windows RNG source over actual QEMU Virtio DMA, polling virtual ISR/DPC, deferred notification, cancellation and reset; no WDF IRQ/IRQL/PnP/CNG integration',
         'rng_linux_shootout': 'Paired native Nelua versus unchanged Linux source, same useful byte workload/features/O2, lifecycle and validation included; source prefetch separately counted; no OS ranking',
         'rng_windows_shootout': 'Paired native Nelua versus unchanged Windows source, same useful byte workload/features/O2, lifecycle and validation included; no OS ranking'}[a.scenario]
result = {'pass': passed, 'scope': scope, 'pci_valid': pci_valid, 'detail_valid': detail_valid,
          'dma_valid': dma_valid, 'dma_allocations': allocations,
          'qmp_resource_state': 'BAR decoding disabled after Platform release' if released else 'BIOS assigned resources retained',
          'command': cmd, 'marker': marker.decode(), 'qemu_pci': pci,
          'fixture': {'size': fixture.stat().st_size, 'sha256': fixture_sha,
                      'formula': 'xorshift32 seed0x6d2b79f5 shifts13,17,5, high byte; QEMU device reads file; provider does not'},
          'handshakes': handshakes, 'quiescence': quiescence, 'samples': rows,
          'clock': clock, 'not_measured': ['CPU utilization', 'peak memory', 'latency percentiles', 'physical interrupt latency'],
          'median_guest_us_per_32768_delivered_bytes': {variant: statistics.median(row['guest_us'] for row in rows if row['variant'] == variant) for variant in sorted({row['variant'] for row in rows})},
          'image_sha256': meta['image_sha256'], 'log_sha256': hashlib.sha256(raw).hexdigest(),
          'qemu_log_sha256': hashlib.sha256(qemu_log.read_bytes()).hexdigest(),
          'qemu_version': subprocess.check_output([cmd[0], '--version'], env=env, text=True).splitlines()[0],
          'qemu_exit_after_controlled_stop': proc.returncode}
(build / (a.scenario + '-result.json')).write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
raise SystemExit(0 if passed else 2)
