# Drift in MINIX 3

Boot a pinned MINIX 3.3.0 guest and execute four **separate** programs:

| Program | Evidence |
| --- | --- |
| Linux `pcspkr` model | The actual imported driver runs with modeled ports; probe, lifecycle, reentry and failure checks. The pthread contention case is explicitly omitted. |
| Windows `moufiltr` model | The actual imported source runs its 584-check WDF model in MINIX userspace. This is not a Windows kernel or WDF binary ABI. |
| Shared Linux component benchmark | The same 500,000-event monotonic-wall model fixture as the other OSes, with eight reverse-order samples; no hardware. |
| Linux `pcspkr` RS service | Actual MINIX checked port I/O, native mutex/delay, emulated PIT channel 2 mode/gate readback, denied unrelated I/O, and service cleanup. |

On an **Ubuntu 24.04 x86-64 build host**, with Python 3, GCC, Binutils, Make and
`dpkg-deb` available:

```sh
python3 ports/minix/run.py
```

The first run downloads the SHA-256-pinned 302 MB official compressed image and
roughly 15 MB of pinned Ubuntu QEMU packages into `build/minix`. No global
installation, privileged mount, KVM access or host port-I/O permission is used.
Existing verified files can be supplied:

```sh
python3 ports/minix/run.py --work build/minix \
  --iso /absolute/path/minix_R3.3.0-588a35b.iso \
  --qemu-root /absolute/path/extracted-qemu-root
```

The shared fixture has the same event count and transcript/checksum requirements
as the other OSes; MINIX TTY output expands tabs, so the verifier accepts its
whitespace-delimited records while retaining the exact raw serial bytes.

The SDK headers, startup objects and static libraries come from that exact
image. GCC emits the bounded i386 C payloads; matching MINIX startup/library
objects link them. This is a verified route for these payloads, not a general
replacement MINIX toolchain. Normal userspace models link libc; the RS service
also links MINIX libsys and libmthread. The unchanged upstream Linux and Windows
files remain in their separately licensed import directories.

The image builder copies the original ISO and reuses existing, non-boot file
extents: `/usr/bin/calendar` holds the Linux model, `/usr/bin/cal` the Windows
model, `/usr/bin/fetch` the RS service, and `/usr/bin/csplit` the shared model
benchmark. Only the test copy is changed. A
small `/etc/rc.cd` script starts the tests. `image-changes.json` records donor
hashes and payload hashes; the base image is retained untouched. The reader
handles this exact pinned filesystem, not arbitrary untrusted disks.

The service owns PIT channel 2 and speaker gates in this isolated test image.
`nobeep=1` disables MINIX TTY beeps; PIT channel 0 remains the system clock.
RS grants only I/O ranges `42:2` and `61`, plus the device-I/O kernel call.
The service registers no interrupt handler. A real MINIX mutex serializes its
calls; Drift's opaque saved flags carry a lock cookie, not invented CPU IRQ
state. `micro_delay(1)` supplies the delayed-I/O operation. The test verifies
that reading unrelated port `0x80` is denied and that suspend/shutdown/close
leave both speaker gate bits clear before the service is removed.

Eight reversed-order component measurements compare 10,000 imported events
with a direct independent C reference using the **same** mutex, checked I/O and
delay callbacks. Every trial first suspends/checks gate bits 0, then resets its
digest and timer. They check operation counts, a normalized transcript hash and
actual mode/gate readback. Only port `0x61` values are normalized to gate bits
0–1 because its other hardware status bits vary over time; the port sequences,
PIT command/divisor bytes, lock/unlock and delay operations remain in the hash.
The guest clock uses `hz=60`, so timings have coarse tick resolution. These are
TCG component I/O loops inside one service: **not** request latency, physical
hardware speed, a four-OS performance ranking, or physical-speaker qualification.

Raw serial output, exact commands, source/artifact hashes, build logs and parsed
results are written to the selected work directory. A missing marker, a failed
model, missing benchmark/denial case, transcript mismatch or failed service
cleanup makes the harness fail. [Recorded evidence](evidence/) is retained
separately from generated guest images and binaries.
