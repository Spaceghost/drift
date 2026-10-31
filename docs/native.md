# Portable controller drivers

Drift includes native Nelua drivers for controller hardware. Their generated C
links into a kernel or userspace driver server through `libdrift-native.a`.
The bytecode interpreter remains useful for small device protocols; native
controllers share its preference for explicit ownership and bounded operations.

## Nelua without its runtime

Every native driver uses `nogc`, `noentrypoint` and explicit validation with
`nochecks`, matching the C-host pattern used by the multicore-server challenge.
No executable module initialization is required before calling these exports.
The host supplies storage, DMA allocation and platform callbacks; those explicit
resources do not introduce a Nelua GC or allocator. Drift's own protocol-bytecode
VM is separate from a Nelua runtime.

`make native-check` combines the native objects and requires `nm -u` to be empty;
`make kernel-check` links and boots them without libc. `nocstaticassert` in these
fixed-width modules is a separate cross-target generation choice, not a runtime
switch. Static C ABI assertions normally have no runtime cost.

See the compiler fork's [runtime-free guide](https://github.com/Spaceghost/nelua-lang/blob/master/docs/pages/runtime-free.md)
for exact pragma meanings, initialization rules, audited C-host and no-libc
examples, and checks to repeat for each C compiler/target.

## One block and network interface

After controller initialization, bind it through `drift_io.h`. The same
`drift_block_read`, `drift_block_write` and `drift_block_flush` calls work across
Virtio, NVMe, AHCI and USB storage. The facade checks buffer sizes and ranges,
splits requests at each backend's limit, and reports the known completed prefix.
The network facade similarly normalizes Virtio, Intel/Realtek and USB ECM frame
I/O. See [the class API](io.md) and the booted reference kernel for complete use.

## One platform interface

Include `drift_hw.h`. Bind a driver to host-selected register regions and supply
these resource operations:

| Operation | OS responsibility |
| --- | --- |
| `read` / `write` | Access a bound register region at the requested width; return numeric little-endian device register values |
| `dma` / `release` | Allocate coherent or uncached CPU/device mappings with the requested size and alignment; release only after quiescence |
| `sync` | Publish CPU writes or acquire device writes, including the required memory ordering |
| `time_us` / `wait_us` | Monotonic time, with waits that advance time or yield |

The OS discovers devices, assigns/maps register resources, enables PCI memory or
I/O decoding and bus mastering as required, and controls access. Driver code
owns feature negotiation, command encoding, descriptor rings, resets and
completion checking. No host callback reads sectors or constructs packets on
behalf of the driver.

`drift_pci.h` supplies portable enumeration, capability-chain lookup, BAR decoding
and command-register enablement. Its host hooks are only config-space read and
write. It handles multifunction devices, rejects capability loops, supports
64-bit BARs, and avoids echoing write-one-to-clear status bits. Firmware or your
OS still configures PCI bridge routing and assigns BAR addresses.

## DMA ownership

CPU addresses and device addresses are independent. Do not substitute a pointer
for a bus address. Device-visible allocations must be coherent or uncached:
control rings can contain CPU-owned and device-owned fields in one cache line.
Cache maintenance rounded across unrelated owned fields is not sufficient.

`TO_DEVICE` publishes CPU writes. `FROM_DEVICE` acquires device writes.
`BIDIRECTIONAL` is a publish operation with clean/invalidate semantics before
handoff to a device that can both read and write. Reclaim such buffers with
`FROM_DEVICE`. Every synchronization also performs the appropriate release or
acquire ordering on coherent machines.

A simulator may implement the same boundary with separate CPU/device shadows.
That tests ownership and synchronization calls, not a particular CPU's cache
hardware. The provided QEMU host maps those shadows onto emulated guest RAM.

## Scheduling and failure

The initial APIs are synchronous and use bounded polling. Serialize each device
instance, retain its context and resources during calls, and supply a real clock.
A polling wait may yield to your scheduler; these drivers create no threads.
They do not establish an interrupt-driven or high-throughput performance claim.

A timeout does not make a DMA buffer safe to free. Drivers attempt to stop/reset
hardware before releasing allocations. If the device cannot be quiesced, the
state remains failed and retains allocations. Keep the host resources alive,
retry close/reset as documented by that driver, or isolate the device through
platform mechanisms before reclaiming memory. Never reuse a live instance by
zeroing its structure.

## Build and verify

```sh
make
make native-check
make native-sanitize
make qemu-check
make kernel-check
```

The first two checks need only the ordinary compiler toolchain. QEMU checks
require `qemu-system-x86_64`; see the driver-specific guides for the tested
models and supported subsets. Generated C is included. To regenerate from
Nelua:

```sh
make native-regen NELUA="$PWD/.toolchain/nelua/nelua"
make verify-generated NELUA="$PWD/.toolchain/nelua/nelua"
```

Each driver has a C header, a complete Nelua implementation and independent
register/DMA model tests. Device-specific guides describe actual support:
[Virtio](virtio.md), [USB/xHCI](usb.md), [NVMe](nvme.md),
[Ethernet](ethernet.md), [AHCI](ahci.md), [USB classes](usb-classes.md) and
[USB hubs](usb_hub.md).
