# Linux guest qualification

This recipe boots a private Linux guest and runs Drift's imported source
profiles. The PC speaker additionally uses **real QEMU PIT/speaker I/O** through
Linux `ioperm`, `inb` and `outb`. No physical hardware is qualified.

```sh
python3 ports/linux/run.py --work "$PWD/build/linux-guest"
```

Requirements: x86-64 Linux, a C compiler with static glibc/pthread development
files, Make, binutils, Python 3.10+ and `dpkg-deb`. On Ubuntu 24.04 the script
can fetch checksum-pinned QEMU packages into its work directory, using the
shared inventory in `ports/minix/qemu-ubuntu-noble.json`. It installs nothing
globally. Reuse an already extracted private installation with
`--qemu-root /absolute/path/to/qemu-root`.

The kernel comes from Debian's pinned `20250803+deb13u7` installer release.
Both the exact kernel hash and its official SHA256SUMS entry are checked.
The observed guest is **Linux 6.12.107+deb13-amd64, x86_64**. The initramfs is
created directly in `newc` format; no mount, root host privilege, BusyBox,
guest network, disk image or external service is needed. The init payload
refuses execution unless it is PID 1; do not execute it as a host program.

## Resource policy

The guest has no module loader or other userspace service. The PC speaker
module is absent, the observed clocksource is TSC, and only one CPU is online.
The imported source owns PIT channel 2 and the speaker gate in this isolated
test. This is not a claim that arbitrary Linux hosts have an unowned speaker.

The init thread obtains Linux permission for exactly ports `0x42–0x43` and
`0x61`. Its adapter admits only the imported profile's allowed byte accesses.
A real pthread mutex serializes them; the opaque saved value is checked for
owner-thread unlock. No IRQ handler runs driver code in this userspace domain,
so it does not pretend to disable CPU interrupts. The `_p` delays call real
`nanosleep` for at least one microsecond, not a counter increment. Linux port
permissions are thread-scoped; this test's real I/O stays on the granted thread.

A child clears its effective/permitted capabilities and verifies that the
Linux kernel rejects `ioperm` with `EPERM`. Separately, a denied resource
adapter propagates `EACCES`, performs no I/O and retains failed-close state;
after authority is granted, close/reopen succeeds. The actual device checks
verify speaker gate on/off and PIT mode readback, suspend, shutdown, event
rejection after shutdown and teardown.

## Separate models, separate licenses

The same boot also executes two separate native ELF programs:

- the complete Linux import model, including real pthread contention and
  linker-wrapped allocation/registration failures;
- all **584** Microsoft mouse-filter profile assertions.

These exercise actual unchanged upstream source with modeled resources. The
mouse filter is not loaded as a Windows kernel driver. The two profiles are
separate executables; the GPL Linux source and MS-PL Microsoft source retain
their licenses and are not combined into one test binary.

## Paired real-I/O measurement

Four rounds reverse import/reference order, yielding eight individual trials.
Each trial performs 100 tone/off cycles, 200 gate readbacks, 500 writes,
200 profile reads, 500 real delays and 200 lock/unlock pairs. Every gate
readback must match; every trial must have identical counts and transcript
digest. Changing PIT output/refresh input bits are normalized in the digest;
their preservation across each actual read/write is checked separately.

The reference is a small independent test fixture performing the same register
sequence through the **same** callbacks, mutex and delays. It is not Linux's
in-kernel driver, another native driver stack, or a physical throughput ceiling.
Timing is guest monotonic elapsed time, including device access and sleeps.
Record individual samples; do not rank these numbers against Genode or MINIX
on different guest/ISA configurations, or against host-model timings.

The canonical [serial log](evidence/guest-serial.log) and
[result record](evidence/result.json) retain all samples and medians. The
shared host was not isolated, and these short trials establish no speed win.

The runner checks source snapshots before and after compilation, payload
hashes before boot, exact paired ordering/counts, all pass markers and QEMU
exit status. A partial, timed-out or failed run is not a passing result.
Sources, compiler/flags, kernel, QEMU and initramfs hashes are retained;
SDKs, kernels and executable images remain local build artifacts.
