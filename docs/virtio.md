# Virtio: a disk and Ethernet interface for a new kernel

`native/virtio.nelua` implements modern Virtio PCI and MMIO version 2. The
portable code owns feature negotiation, device reset, split virtqueues,
descriptors, completion validation and five device classes. Generated C is
included in `generated/virtio.c`; its interface is `include/drift_virtio.h`.
The bytecode VM is unchanged. This native tier has no allocator or C-library
dependency of its own; all register and DMA resources come from `drift_hw`.

## Connect an OS

Implement the generic callbacks in [drift_hw.h](../include/drift_hw.h). Allocate
coherent or uncached DMA memory according to that interface, translate bus
addresses there, and implement `sync` ordering. Time must be monotonic and
`wait_us` must allow time to advance. Each region is a host-bound register
window; device code cannot choose another device's physical address.

For PCI, enumerate a modern Virtio function, bind its six BAR windows, and enable
memory decoding and bus mastering. Pass a six-element `drift_virtio_bar` array
(region identifier and mapped size, with `UINT32_MAX` for an unmapped region)
to `drift_virtio_open_function`. The Nelua library walks and validates vendor
capabilities, associates common/notification/device windows, derives the device
type and checks notification offsets against the advertised window. The OS
needs only generic PCI configuration and BAR mapping. `drift_virtio_open_pci`
is also available for callers that already have validated capability windows.
For MMIO, pass
the bound Virtio register window to `drift_virtio_open_mmio`.

```c
drift_virtio disk = {0};
int rc = drift_virtio_open_mmio(&disk, &hardware, disk_region, 2);
if (rc == 0) {
    uint8_t sector[512];
    rc = drift_virtio_block(&disk, 0, 1, sector, 0);
    /* Consume sector, then close before releasing device resources. */
    int stopped = drift_virtio_close(&disk);
    /* Both rc and stopped must be checked by the caller. */
}
```

Use a fresh zero-initialized instance. Calls are serialized; each queue has one
outstanding descriptor chain. The implementation polls rather than installing
interrupt handlers. The host can integrate the nonblocking receive/input polls
with its own scheduler. Synchronous operations use a one-second deadline by
default; `timeout_us` can be adjusted after opening.

## Device interfaces

| Device ID | Interface | Behavior |
| --- | --- | --- |
| 2 | `drift_virtio_block`, `drift_virtio_flush` | Read/write 1–128 512-byte sectors per request; bounds and read-only checks; optional negotiated flush |
| 1 | `drift_virtio_send`, `drift_virtio_receive` | Raw Ethernet frames; MAC configuration when offered; no checksum/GSO offload |
| 3 | `drift_virtio_console_write`, `drift_virtio_console_read` | A single console port; no multiport control protocol |
| 4 | `drift_virtio_entropy` | Up to 4096 returned entropy bytes per request |
| 18 | `drift_virtio_input` | One type/code/value input event per poll; no LED/status output yet |

Receive and input calls return zero when nothing is available. Ethernet and
console reads return a byte count; input returns one for one event. A receive
buffer that is too small drops that packet and returns `-3`. Block and send
return zero on success. The header lists argument, deadline, bounds,
unsupported-feature, device and resource errors.

Each split queue has eight entries. Most queues have one outstanding chain;
input supplies eight separate event buffers so an EV_KEY/EV_SYN batch has enough
credits. The complete published input batch is validated for unique buffer IDs
before any buffer is recycled. Queue memory and
payload buffers are separate, with explicit little-endian encoding. The driver
negotiates `VIRTIO_F_VERSION_1`, optional block RO/FLUSH and optional network MAC.
It deliberately declines packed rings, indirect descriptors, multiqueue,
notification data, IOMMU-platform semantics and offloads. The host must provide
bus addresses valid under the resulting DMA model. Disk capacity and MAC are
sampled consistently at attachment; dynamic device reconfiguration needs a
close/reopen. Ethernet frames are limited to 2036 bytes, excluding the Virtio
header. This is a small synchronous baseline rather than a throughput claim.

## Stop before freeing

Timeouts, malformed completions and reset-required status poison the device and
attempt a full reset. DMA is released only after status reads back as zero. If
reset times out, the instance retains its allocations in quarantined state 3.
Keep the instance, register windows and buffers alive; retry
`drift_virtio_close` after the host has made reset possible. Never free storage
while the device may still DMA into it.

## Evidence

`tests/test_virtio.c` is an independent MMIO/PCI device model. Its CPU and device
memory copies are separate, so descriptor and payload visibility require the
actual synchronization calls. It exercises all five device classes, DMA above
4 GiB, sector limits, ring-index wrap, feature rejection, inconsistent config,
failed allocations, oversized/invalid completions, timeout and reset quarantine.
PCI attachment fixtures reject cyclic, truncated, overlapping, duplicate,
unmapped, unaligned and overflowing capabilities, upper halves of 64-bit BARs,
incorrect device IDs and incomplete register windows.
These model tests are not physical-hardware qualification. The qtest target
also runs the actual modern PCI driver against QEMU: block read/write/flush,
network TX/RX through a socket peer, console TX/RX, entropy and QMP-injected
keyboard input have passed on QEMU 8.2.2.

Protocol reference: [OASIS Virtio 1.3](https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html),
sections 2–5 (modern transports, split queues and device types).
