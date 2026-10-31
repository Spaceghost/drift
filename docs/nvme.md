# Native NVMe

`native/nvme.nelua` owns controller initialization, command encoding, queues,
completion checking, namespace geometry and read/write/flush operations.
It compiles to `generated/nvme.c` and exposes `include/drift_nvme.h`.
The OS supplies only the generic resources in `drift_hw.h`.

## Integrate

Enumerate a PCI NVMe controller, map its register BAR into an exclusively owned
resource region, enable PCI memory space and bus mastering, and provide DMA
memory accessible to that controller. Keep interrupts disabled for this polling
implementation. The driver does not enumerate PCI or alter PCI configuration.

```c
#include "drift_nvme.h"

static drift_nvme disk;                  /* zero-initialized, caller-owned */
uint8_t sector[4096];

int result = drift_nvme_init(&disk, &resources, region, 1, 1000000);
if (!result)
    result = drift_nvme_read(&disk, 0, 1, sector);
/* disk.blocks and disk.block_size describe the selected namespace. */
int closed = drift_nvme_close(&disk);
```

The example is a call sequence inside a host function, not a standalone
program. The selected namespace ID is explicit; inactive or unsupported
namespaces are rejected by Identify. Serialize calls to each instance and keep
its host resource object alive until close succeeds.

The register resource must cover the controller registers and queue 0/1
submission/completion doorbells. Their stride comes from CAP.DSTRD. Resource
reads/writes support numeric widths 4 and 8. Every DMA allocation requests a
4096-byte, 4096-aligned buffer; its CPU pointer and device address may differ.

The implementation allocates five pages through the host: Admin SQ/CQ, I/O
SQ/CQ, and a transfer buffer. Allocations must be coherent or uncached, as required by `drift_hw.h`.
Explicit synchronization still establishes CPU/device ownership and ordering;
streaming-only noncoherent mappings are not supported for shared control rings. There is no heap, libc dependency, hidden worker or native
pointer in a device command. Native pointers stay on the host side.

## Supported operations

| Operation | Behavior |
| --- | --- |
| Initialize | Reset, negotiate 4 KiB controller pages, identify controller/namespace, create I/O queue 1 |
| Read/write | 64-bit LBA, positive block count, at most 4096 data bytes per call |
| Flush | NVMe NVM Flush on the selected namespace |
| Close | Disable the controller, wait for RDY clear, then release DMA |

Queue depth is the smaller of eight entries and CAP.MQES+1, with a minimum of
two. One command is outstanding at a time. This is a polling implementation;
interrupt routing, multiqueue scheduling, SGLs and multi-page PRP lists are
future work. A 4 KiB aligned transfer buffer needs only PRP1.

The conventional NVM command set, 4 KiB controller pages, and 512-, 1024-,
2048- or 4096-byte namespace blocks are supported. Separate metadata, extended
metadata blocks and protection information are rejected. The transfer limit
also bounds work and avoids assuming arbitrary controller MDTS values.
Callers split larger requests into supported blocks.

A successful Write means the command completed, not that volatile controller
write cache is persistent. Call Flush when persistent completion is required.
A timed-out write may already have taken effect; the driver does not retry it.

## Failure and DMA lifetime

The driver checks completion phase, command ID, submission queue ID and
submission queue head. Device completion errors return `DRIFT_NVME_EIO`, and
`last_status` preserves the NVMe completion status without the phase bit.
A normal device error consumes its completion and leaves the queue usable.

Timeouts, controller fatal status and malformed completions trigger a reset.
DMA is released only after CSTS.RDY becomes zero. If the controller will not
quiesce, state 3 quarantines the allocations, and further I/O returns
`DRIFT_NVME_ESTATE`. A later `drift_nvme_close` retries the reset. The host must
not release quarantined pages or recycle the driver object while device DMA
can still address them. A platform may revoke the device's DMA authority
through its own teardown policy before reclaiming those resources.

A failed initialization may leave state 2 (stopped) or 3 (quarantined). Close
before attempting initialization again. No teardown failure silently frees
pages still accessible to the controller.

## Validation

`tests/test_nvme.c` models registers, queue processing and device memory
independently. CPU and device DMA views differ, so missed clean/invalidate
operations produce incorrect commands or data. Checks cover addresses above
4 GiB, 64-bit LBAs, phase/tail wrapping, all allocation failure points, capacity
and transfer limits, unsupported formats, erroneous completions, timeout reset,
failed-reset quarantine and recovery, doorbell stride and 4 KiB blocks.

The model tests and ASan/UBSan pass. The independent QEMU8.2.2 qtest host also
completed Identify, read, write, Flush, read-back comparison and data restoration
through the same generated driver. These establish model and emulator behavior;
physical NVMe hardware remains unqualified.

Reference: [NVM Express Base Specification 1.4c](https://nvmexpress.org/wp-content/uploads/NVM-Express-1_4c-2021.06.28-Ratified.pdf),
controller registers, command/completion queues, Identify and NVM commands.
The reference is published by NVM Express, Inc. ©2007–2021 NVM Express, Inc.
