# Source imports: verified scope and measurements

Two original drivers compile unchanged against bounded Nelua compatibility
profiles: Linux 6.18 `pcspkr.c` and Microsoft's KMDF `moufiltr.c`/`.h`.
Upstream files and licenses are SHA-256 pinned. The first profile is not general
Linux compatibility; the second is not complete WDF/NDIS/StorPort or a `.sys`
loader. [Target selection and qualification](../../docs/compatibility.md)
separates implemented profiles from the network/storage work still pending.

Latest complete qualification is source commit `d9cbb4411c3378b96acc33fbf27cabb625419149`:
both [native host jobs](https://github.com/Spaceghost/drift/actions/runs/41228402217)
and all three [real guest jobs](https://github.com/Spaceghost/drift/actions/runs/41228402323)
are green. Fresh CI archives are independently checked and retained for
[hosts](hosts/ci-41228402217), [Linux](../../ports/linux/evidence/ci-41228402323),
[MINIX](../../ports/minix/evidence/ci-41228402323) and
[Genode](../../ports/genode/records/ci-41228402323). They remain separate from
the earlier canonical measurement tables below; runner variation is not pooled.

## What actually executed

| OS | Verified execution | Limit |
| --- | --- | --- |
| Linux | Native CI models and benchmark; booted Linux 6.12.107 guest runs both source models and actual restricted PIT/speaker I/O | Emulated hardware; not an installed Linux kernel module or physical device |
| Windows | Native x86-64 PE executables run both source models, concurrency/fault checks, strict object audits and benchmark | Userspace model; no Windows kernel loading, real mouse stack or device qualification |
| MINIX | Booted 3.3.0 i386 guest runs both models, shared benchmark and RS-managed actual checked port-I/O service | Linux model omits pthread contention; actual service uses native MINIX locking; emulated device |
| Genode | Booted 26.08 x86-64 base-hw guest runs Linux import in a real RPC service plus separate shared model component | No Windows-profile guest run; emulated device, not physical/audio qualification |

[CI run 41227386461](https://github.com/Spaceghost/drift/actions/runs/41227386461)
passes both hosts: original tests, source identity, **584** Windows-profile
checks, **572** optimized/LTO checks, **794** benchmark API checks, strict
undefined-symbol audits and Linux sanitizers. Existing controller QEMU/kernel
checks and pinned Nelua regeneration also pass. Windows partial-object audits
disable PE auto-import/runtime pseudo-relocation synthesis only for that audit;
normal executable/test flags remain unchanged and no symbol is allowlisted.

## Same shared component model on four OSes

Every trial executes **500,000 events**, **3,500,000 modeled operations**, final
control `a8`, **1,093,750 modeled delay microseconds** and transcript checksum
`64f59f6df89549d1`. Delays are counters, not sleeps. Eight individual samples
reverse reference/import order across four rounds. Exact preflight transcripts,
statuses and pair digests must agree before the fixture emits results. The
794-check API test rejects failed/frozen/backward clocks and output failures.

| Execution environment | Reference median, µs | Import median, µs | Import/reference |
| --- | ---: | ---: | ---: |
| Native Linux CI | 12,281 | 26,298 | 2.14× |
| Native Windows CI | 16,745 | 44,607.5 | 2.66× |
| MINIX i386 guest, TCG | 350,000 | 550,000 | 1.57× |
| Genode x86-64 guest, TCG | 198,226.5 | 441,941.5 | 2.23× |

These quantify **software-model overhead**, including validation, dispatch,
locking and transcript instrumentation. They are neither hardware I/O rates
nor a comparison with a full native-kernel driver. Optimization is `-Os` for
both paths within each run; compilers, ABI/code-model flags and environments
still differ across OSes. MINIX has coarse 60-Hz clock resolution. Do not rank
OSes using this table. Windows imported repeats range from 36.493 to 54.691 ms;
all original repetitions are retained.

Raw evidence: [Linux/Windows](hosts/ci-41227386461),
[MINIX](../../ports/minix/evidence/with-shared-model),
[Genode](../../ports/genode/component-model/records).

## Separate real-I/O comparisons

These invoke actual QEMU PIT/speaker resources through the OS backend. Within
each OS the import and small independent reference use the same callbacks,
locks, delays and register sequence. Counts and normalized transcripts must
match. The references are test fixtures, not complete native driver stacks.

| OS | Trial unit | Reference median, µs | Import median, µs |
| --- | --- | ---: | ---: |
| Linux | 200 tone/off events with readbacks | 51,016 | 50,562.5 |
| MINIX | 10,000 events inside one RS service | 858,333.5 | 850,000 |
| Genode | 400 RPC calls including readbacks | 364,005 | 381,544 |

Each row retains four reversed rounds/eight samples. The workloads, guest
configurations and units differ, so this is **not a cross-OS shootout**.
Scheduler sleeps, timers and TCG variation dominate these short runs; no speed
advantage is established. Windows has no real-I/O result and is not silently
substituted with its model.

Reproduce and inspect raw commands, image/source hashes and permission-denial
tests: [Linux](../../ports/linux), [MINIX](../../ports/minix),
[Genode](../../ports/genode). MINIX retains the rejected unequal-start-state
comparison; Genode retains earlier pre-SDK-enforcement trials separately. No
historical or failed trial is pooled into the canonical medians above.

## What this does not finish

Straight **source** import works for the two documented profiles. General NT
or Linux kernel emulation, arbitrary `.ko`/`.sys` binaries, current Wi-Fi/GPU
stacks, Windows kernel/device execution and physical NIC/storage qualification
are not implemented by these proofs. The selected useful follow-ons are paired
Virtio RNG, network and block drivers, then AHCI and named physical 2.5GbE
devices. Those require genuine queue/DMA/IRQ/framework support and their own
data-integrity tests before performance claims.
