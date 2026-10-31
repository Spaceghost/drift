# One storage and network interface

An OS initializes the controller once, then binds it to a common interface.
Its filesystem and network stack use the same calls for every supported device.
The facade is native Nelua and allocates no memory. It borrows an initialized
driver object; it neither copies that object nor takes responsibility for closing
it. Link `generated/io.c` with the native drivers it dispatches to.

## Block devices

```c
#include "drift_io.h"
#include "drift_nvme.h"

static drift_nvme controller;
static drift_block disk;

/* After drift_nvme_init(&controller, ...) succeeds: */
int bind_disk(void)
{
    return drift_block_bind(&disk, DRIFT_BLOCK_NVME, &controller);
}

int read_sectors(uint64_t lba, uint32_t sectors, uint8_t *data, uint32_t bytes)
{
    return drift_block_read(&disk, lba, sectors, data, bytes);
}
```

The same `read_sectors` function works when `disk` is bound to Virtio, AHCI or
USB mass storage. There is no per-driver filesystem adapter.

| Binding | Native object | Maximum hardware command |
| --- | --- | --- |
| `DRIFT_BLOCK_VIRTIO` | `drift_virtio`, block device | 128 sectors of 512 bytes |
| `DRIFT_BLOCK_NVME` | `drift_nvme` | 4096 bytes |
| `DRIFT_BLOCK_AHCI` | `drift_ahci` | 65536 bytes |
| `DRIFT_BLOCK_USB` | `drift_usb`, mass-storage device | 16384 bytes |

A bind validates the identified sector size and capacity. USB bind issues READ
CAPACITY through the class driver; the other controllers already cache geometry
from initialization. `disk.blocks`, `disk.sector_size` and `disk.max_sectors`
describe the bound device. Sector sizes are powers of two from 512 through 4096.

`drift_block_read` and `drift_block_write` accept a 64-bit starting LBA, a sector
count, a buffer and the buffer's byte capacity. Before issuing any command, the
facade checks the complete LBA range and verifies that the buffer covers the
request without overflowing the sector-to-byte calculation. It automatically
splits a large request into consecutive commands of the size supported by the
controller. The underlying driver's own address and feature restrictions still
apply.

On success `disk.completed` equals the requested sector count. On failure it
counts the prefix of fully successful commands in sectors. The failed command
may itself have partially transferred or written data: `completed` is progress,
not a claim that a failed write had no effects. Read/write reset this count at
the beginning of every call, including calls rejected before I/O.

Call `drift_block_flush(&disk)` when persistence matters. It delegates to the
native device and returns any unsupported-flush or hardware failure. Flush does
not silently pretend success and does not add implicit flushes to writes.
It resets `completed` to zero.

## Ethernet devices

```c
#include "drift_io.h"
#include "drift_ethernet.h"

static drift_ethernet controller;
static drift_net interface;
static uint8_t packet[16384];

/* After drift_eth_init(&controller, ...) succeeds: */
int bind_network(void)
{
    return drift_net_bind(&interface, DRIFT_NET_ETHERNET, &controller);
}

int poll_network(void)
{
    return drift_net_receive(&interface, packet, sizeof(packet));
}
```

`DRIFT_NET_VIRTIO` binds a Virtio network device. `DRIFT_NET_ETHERNET` binds
either native e1000 or RTL8139. `DRIFT_NET_USB_ECM` binds an initialized
`drift_usb_cdc` ECM object. All bindings require an identified, nonzero unicast
MAC address. The facade exposes it as `interface.mac`, with the supported
maximum frame size in `interface.max_frame`.

`drift_net_send` returns zero on success or a negative error. It normalizes
CDC's byte-count result and rejects short writes. `drift_net_receive` returns
the received frame length, zero when idle, or a negative error. It normalizes
the native Ethernet driver's separate status/length outputs. A received frame
must contain at least a 14-byte Ethernet header and fit the bound interface.
Frames exclude the FCS. IP, ARP and other network protocols remain in the OS's
network stack.

The receive buffer must cover `interface.max_frame` so all three native APIs
can deliver a complete frame. Larger caller buffers are accepted; the facade
limits the native receive capacity to the interface's maximum. This also keeps
USB transfer-size limits out of OS call sites. Current maxima are 2036 bytes for
Virtio, 1518 for e1000/RTL8139, and the descriptor's maximum segment for ECM.

## Lifetime and errors

Serialize facade operations with all direct calls to the underlying driver.
Keep every borrowed object and hardware resource alive. After reset, close,
reattach or reconfiguration, initialize the native driver as necessary and bind
again. The facade checks that native state and identified geometry still match;
it returns `ESTATE` for a stale binding. Facades have no close operation because
the initialized controller remains owned by the caller.

A failed bind clears the facade. Native negative errors pass through unchanged;
facade validation uses a separate range:

| Error | Meaning |
| --- | --- |
| `DRIFT_IO_EARG` (`-1000`) | Null object/buffer or zero-sized block request |
| `DRIFT_IO_ESTATE` (`-1001`) | Uninitialized, closed, changed or stale native binding |
| `DRIFT_IO_ERANGE` (`-1002`) | LBA, byte-capacity or frame-size violation |
| `DRIFT_IO_ENOTSUP` (`-1003`) | Unknown binding kind |
| `DRIFT_IO_EPROTO` (`-1004`) | Invalid geometry/MAC or contradictory native result |

`tests/test_io.c` stubs the native entry points to isolate facade behavior:
chunk boundaries, dispatch, 64-bit LBAs, capacity arithmetic, successful-prefix
progress, stale bindings and network result normalization. The bootable reference
kernel separately exercises the same facade against QEMU Virtio, NVMe, AHCI,
USB storage, e1000 and RTL8139 controllers.
