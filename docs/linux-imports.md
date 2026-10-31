# Import Linux source; port the environment

The intended boundary is an unchanged, pinned Linux driver source file compiled
against a small Drift compatibility profile. Drift's host backend supplies the
operating-system resources. The driver is rebuilt for each target.

This does not load Linux `.ko` files. Linux's internal source and binary
interfaces are deliberately unstable, so compatibility belongs to a particular
driver revision and its dependency set, not to "Linux drivers" in general.

| Layer | Responsibility |
| --- | --- |
| Upstream driver | Original Linux C, copyright and SPDX identifier; no local patches |
| Compatibility profile | Required Linux types, registration, lifecycle and calls; implemented outside the upstream file |
| Drift platform backend | Authorized register access, allocation, synchronization, time and any required interrupts |
| OS frontend | Publish a Genode session or MINIX service and translate its requests |

Unsupported APIs must fail compilation or return an explicit error. A dummy
function returning success is not an implementation of locking, DMA, IRQs or
device registration. Native imports are trusted native code, not modules for
Drift's bounded protocol VM.

## First profile: PC speaker

The first profile targets Linux 6.18's `drivers/input/misc/pcspkr.c`. It is small
enough to audit: input/platform registration, lifecycle, an actual lock and byte
I/O for the PIT/speaker. There is no DMA, worker thread, IRQ handler or Linux
scheduler to emulate. The original driver performs the tone/divisor/register
operations; Drift must not replace those with a host "beep" implementation.

```sh
make linux-check
make linux-sanitize
make linux-verify-generated NELUA="$PWD/.toolchain/nelua/nelua"
```

`sh tools/import-linux.sh pcspkr` re-fetches the pinned source and license,
verifies their SHA-256 hashes and refuses to overwrite local edits. Ordinary
checks use the included files and need no network access.

The import is opt-in, separate from `libdrift-native.a` and the default install.
The upstream file is GPL-2.0-only; its notices and license are retained. The
imported-driver artifact is not MIT-only. See the profile's provenance and
license files under [imports/linux/pcspkr](../imports/linux/pcspkr).

These checks exercise the actual imported source against an independent port
model. They establish the source/compatibility boundary, not a booted OS port,
audible sound, or physical hardware qualification.

## Genode first

Genode already runs unmodified Linux source through DDE Linux. Its 21.08
[architecture description](https://genode.org/documentation/release-notes/21.08#Linux-device-driver_environment_re-imagined)
and the [26.08 update](https://genode.org/documentation/release-notes/26.08)
are the main references. Existing `lx_emul`/`lx_kit` components are valuable
working precedents, but their scheduler and service integration depend on
Genode; they are not an OS-independent library ready to install on MINIX.

For this profile, the next implementation is a Genode component that grants
only the required I/O-port resources, supplies real synchronization, invokes
the same compiled source and publishes an explicit tone interface. PIT access
must be coordinated with other timer/speaker users; permission to a port does
not imply exclusive ownership of the whole controller. Do not steal the OS's
clock device to make a demonstration work.

The [verified Genode example](../ports/genode) boots the pinned base-hw guest,
executes this unchanged driver through a real RPC service, checks actual QEMU
port readback and lifecycle, and tests missing resource authority. Its matched
reference measurements are recorded as service diagnostics, not a physical
hardware qualification or an arbitrary Linux-driver environment.

## MINIX next

MINIX has DDEKit and historical Linux reuse, but its documented `dde-linux26`
targets Linux **2.6.29**, not a current 6.x driver. See the official
[DDEKit USB documentation](https://wiki.minix3.org/doku.php?id=ddekitusb).
Some in-tree DDEKit operations are explicitly unimplemented; their presence
must not be counted as working resource support.

The backend needs a MINIX service with explicit I/O permissions, checked
`sys_inb`/`sys_outb` operations, synchronization and lifecycle integration.
Publish requests through MINIX IPC and its appropriate driver library. The
same PIT ownership constraint applies. Preserve kernel-call failures rather
than inventing successful reads or writes.

The [verified MINIX example](../ports/minix) boots a pinned image, runs a real
RS-managed port-I/O service and verifies readback, resource denial and cleanup.
It also runs the source models as separate native programs. See MINIX's
[driver overview](https://wiki.minix3.org/doku.php?id=developersguide:overviewofminixdrivers)
and [cross-build guide](https://wiki.minix3.org/doku.php?id=developersguide:crosscompiling).
The recorded model and real-I/O results have separate scopes and timing limits.

## Then a useful DMA driver

Linux's `virtio-rng.c` is a good next import: one queue and no network or block
stack. It still needs genuine virtqueue submission, callbacks/completions,
memory ordering, DMA ownership and reset/teardown.

Drift currently exposes synchronous Virtio class operations, not the generic
virtqueue API that Linux expects. Calling `drift_virtio_entropy` underneath a
compatibility wrapper would let Drift do the device work instead of running
the imported Linux driver's queue logic. Expose and verify the missing queue
boundary first, then test the unchanged Linux source against QEMU on both OSes.

The Linux [driver-interface explanation](https://docs.kernel.org/process/stable-api-nonsense.html)
describes why source/configuration pins are necessary. Importing a second
driver is the test of whether a profile contains reusable mechanisms rather
than assumptions hidden for the first one.
