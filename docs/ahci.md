# AHCI/SATA disks

`native/ahci.nelua` is a native Nelua driver for one SATA disk on one AHCI port.
Its C interface is [drift_ahci.h](../include/drift_ahci.h); the generated C is a
build artifact. The driver uses the generic [hardware resource interface](../include/drift_hw.h).
No ATA command formatting, queue construction or disk-specific logic belongs in
the OS adapter.

## Supported path

- AHCI register BAR and a powered SATA port with an active link.
- ATA disks advertising LBA48; 512-byte or 4096-byte logical sectors.
- READ DMA EXT (`25`), WRITE DMA EXT (`35`), IDENTIFY (`ec`).
- FLUSH CACHE EXT (`ea`) or FLUSH CACHE (`e7`), as advertised by IDENTIFY.
- One synchronous request at a time; up to 64 KiB per call; polling deadlines.

ATAPI, NCQ, port multipliers, hotplug, link power management and drive spin-up
policy are outside this initial path. The caller supplies exclusive controller
ownership, PCI memory decoding/bus mastering, an enabled BAR mapping and a
ready link. The engine respects BIOS/OS ownership handoff where advertised and
stops command/FIS engines before installing its DMA addresses. It does not reset
other ports.

## Integrate

```c
#include "drift_ahci.h"

static drift_ahci disk;             /* zero initialized, stable address */
uint8_t sector[4096];

/* resources binds an AHCI BAR to region 0; port 0 has a SATA disk. */
int status = drift_ahci_init(&disk, &resources, 0, 0, 1000000);
if (!status) status = drift_ahci_read(&disk, 0, 1, sector);
/* disk.sectors and disk.sector_size describe the admitted disk. */
```

`read` and `write` borrow a buffer of `count * disk.sector_size` bytes. LBA and
count are checked against the disk's identified capacity and the 64 KiB request
limit. Write copies the caller's input into a DMA lease without modifying that
input. Read only copies data back after a complete successful command. Flush
returns `ENOTSUP` without issuing a command when neither flush instruction is
advertised. A successful write alone does not promise nonvolatile persistence.

One host DMA lease contains the aligned command list, received-FIS area, command
table and a 64 KiB bounce buffer. Its CPU pointer and controller address can
differ; a 32-bit-only HBA rejects leases crossing 4 GiB. Cache synchronization
explicitly transfers ownership before and after DMA. Structures on the device
side are built as little-endian bytes, independent of the CPU's structure layout.

## Failures and resource lifetime

Any uncertain command completion poisons the disk. Further I/O returns
`ESTATE`; it cannot quietly reuse a still-active queue. Call `drift_ahci_close`
to stop the command engine and FIS receiver. Only after hardware clears both
running bits will the driver release its DMA lease. If stopping times out, the
lease remains quarantined: keep the object and host alive, recover the hardware,
then retry close. Never free the lease from outside the driver.

After a successful close, the zero-state object can be initialized again. A
failed attach can also require close; inspect `state` and follow the same rule.
The host's monotonic clock and wait callback must progress. A deadline cannot
bound an OS adapter that itself blocks indefinitely inside a register callback.

## Evidence

The independent register/DMA model in `tests/test_ahci.c` checks 22 scenarios,
including noncoherent copies, >32-bit DMA addresses and LBAs, 512/4096 sectors,
64 KiB transfers, IDENTIFY rejection, short DMA, errors, flush selection,
timeouts and retained resources when stopping fails. The same suite passes
AddressSanitizer and UndefinedBehaviorSanitizer. Model results do not qualify a
physical controller.

The controller behavior follows Intel's
[AHCI 1.3.1 specification](https://www.intel.com/content/www/us/en/io/serial-ata/serial-ata-ahci-spec-rev1-3-1.html).
