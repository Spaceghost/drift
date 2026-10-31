# Genode: unchanged Linux PC-speaker source

A real Genode service runs the pinned Linux `pcspkr.c` through Drift's Nelua
compatibility layer. A separate client issues tone/off requests over RPC and
checks the actual emulated speaker-control port. This is an opt-in native
component, not Drift VM bytecode or a Linux module loader.

Tested: **Genode 26.08, x86_64 base-hw, QEMU 8.2.2 TCG, q35,
Skylake-Client, one CPU and 512 MiB**. Source and toolchain pins are in [SDK](SDK).
No physical or audible-speaker qualification is claimed.

## Build and use

Requires Python 3, GNU make, Expect, QEMU x86_64 and the Genode 25.05 toolchain.
Use the pinned Genode source/toolchain archives; verify their SHA-256 values
before extracting. Extract the toolchain into a private directory with
`tar --no-same-owner` and **without** `-P`; no system installation is needed.
Its compiler prefix is then `.../usr/local/genode/tool/25.05/bin/genode-x86-`.
The toolchain's host binaries require `libfl2` on Ubuntu 24.04.

The build also enforces the extracted SDK source-tree digest in `SDK`, derived
independently from the checksum-verified archive. The canonical format hashes
sorted paths, file contents and symlink targets using length-prefixed fields;
Git metadata is excluded. Added, removed or changed source entries fail before
the build. Ownership, timestamps and permission bits are outside this digest.

From the Drift repository:

```sh
python3 ports/genode/build.py --genode-dir /path/to/genode-26.08 \
  --toolchain-prefix /path/to/toolchain/bin/genode-x86-
python3 ports/genode/boot.py --genode-dir /path/to/genode-26.08
```

`build.py` creates `build/genode/` and uses this repository's actual import
sources. It invokes Expect explicitly; `--expect` accepts a privately unpacked
interpreter. No Genode source patch is needed. `boot.py` checks the built source
and image hashes, boots the shipped Bender loader with the compiled Genode
image, and stops QEMU only after success or timeout. Privately unpacked QEMU
supports `--qemu`, `--qemu-data`, `--bios` and `--module-dir`; its libraries may
need an explicit `LD_LIBRARY_PATH`.

The client verifies tone, off, suspend, close, rejection after close and a new
session after teardown. It uses the `Drift_speaker` session defined in
[include/speaker.h](include/speaker.h). Both build and boot retain logs and
metadata under the build directory. Guest images and SDKs are never committed.

To verify real resource denial, repeat both commands with
`--scenario drift_pcspkr_denied`. That configuration omits `IO_PORT` authority.
The expected result is Genode's explicit **startup fail-stop**, before the
service is announced. It is not a successful driver operation or graceful
application exit.

## Resource and interrupt boundary

The backend requests only ports `0x42–0x43` and `0x61`, uses physical port
numbers, a real Genode mutex and a dedicated blocking Timer connection for
Linux's post-I/O delays. No IRQ handler is registered. The mutex does not
pretend to disable global hardware interrupts.

In this pinned base-hw PC configuration, bootstrap disables PIT channel 0
before starting components; the running kernel uses the local APIC timer.
Core allocates non-overlapping I/O-port sessions. The test image starts no other
PIT/speaker consumer. These facts justify this configuration's exclusive
channel-2 use. They do not justify sharing PIT resources with an arbitrary
Genode or other OS configuration. A shared configuration needs a coordinated
owner/broker, not competing port writes.

Genode's byte/void I/O API does not return an errno for every core allocation
failure. The normal runner rejects Genode error logs as well as bad readback;
the separate denial case tests missing service authority. This example does
not establish recovery from overlapping allocations in arbitrary images.

## Diagnostic benchmark

Four rounds reverse import/reference order. Every trial performs 100 tone/off
cycles with readback after each operation: **400 RPC calls, 500 port writes,
400 port reads and 500 actual delay calls**. The guest asserts equal counts.
Every sample and median is retained; this is service-level elapsed time under
TCG, including RPC, locks and delays.

The reference is a small implementation of the same PIT sequence using the
**same resource callbacks and RPC path**. It is not a complete native Genode
controller. This comparison does not establish an OS capacity winner or a
performance ranking against other driver ecosystems. Timing differences at
this scale are dominated by timer and emulator variation.

The [verified public-path run](records/drift_pcspkr-result.json) retains all
eight samples (four per variant). Median time per 400-RPC trial was
**381,544 us for the import** and **364,005 us for the reference**: the import
took about 4.8% longer in this short diagnostic. This includes virtual timers
and emulation rather than isolating compatibility-layer CPU cost. The [guest log](records/drift_pcspkr-guest.log)
records readback/lifecycle success; [denial evidence](records/drift_pcspkr_denied-result.json)
records the separate startup failure. Earlier public samples are retained in
`records/before-sdk-enforcement/`; they predate the enforced SDK-tree guard and
are not pooled with this strict-pin rerun. No private prototype trials are
pooled into these results.

`python3 ports/genode/test_source_tree.py` verifies the pin's file/path/link
tamper detection, Git-metadata exclusion, and rejection of a fake SDK that
only matches the version string. The independently computed archive/tree
identity is retained in `records/source-tree-verification.json`.

## Licensing

New adapter and test code is MIT. The unchanged Linux driver is GPL-2.0-only;
the linked imported-driver executable is **not MIT-only**. Retain its license
and notices. Genode's own [LICENSE](https://codeberg.org/genodelabs/genode/src/tag/26.08/LICENSE)
includes its linking exception for independent modules under approved licenses.
