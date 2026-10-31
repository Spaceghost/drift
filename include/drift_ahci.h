/* A synchronous, one-slot AHCI/SATA disk. SPDX-License-Identifier: MIT */
#ifndef DRIFT_AHCI_H
#define DRIFT_AHCI_H
#include "drift_hw.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Zero initialize before first attach. Do not copy or concurrently use a disk.
 * The host supplies an exclusive AHCI controller BAR, enabled PCI bus mastering,
 * a powered/link-ready SATA disk, and DMA addresses valid for that controller.
 * A failed operation poisons the disk. close stops DMA before releasing memory;
 * if close times out, keep the object, bus and DMA alive and retry after recovery. */
typedef struct drift_ahci {
    drift_hw *hw;
    drift_dma memory;
    uint64_t sectors;
    uint32_t sector_size, region, port, timeout_us, cap, flush_command;
    int32_t state;
} drift_ahci;
enum { DRIFT_AHCI_EARG=-1, DRIFT_AHCI_ENOTSUP=-2, DRIFT_AHCI_ENODEV=-3,
       DRIFT_AHCI_ESTATE=-4, DRIFT_AHCI_ETIMEOUT=-5, DRIFT_AHCI_EIO=-6,
       DRIFT_AHCI_EDMA=-7 };
int32_t drift_ahci_init(drift_ahci *, drift_hw *, uint32_t region,
                        uint32_t port, uint32_t timeout_us);
/* One request is at most 65536 bytes. buffer must hold sectors*sector_size bytes.
 * write borrows a read-only input buffer despite the unqualified C pointer. */
int32_t drift_ahci_read(drift_ahci *, uint64_t lba, uint32_t sectors, uint8_t *buffer);
int32_t drift_ahci_write(drift_ahci *, uint64_t lba, uint32_t sectors, uint8_t *buffer);
int32_t drift_ahci_flush(drift_ahci *);
int32_t drift_ahci_close(drift_ahci *);
#ifdef __cplusplus
}
#endif
#endif
