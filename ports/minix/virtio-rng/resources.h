/* MINIX i386/QEMU resource provider. SPDX-License-Identifier: MIT */
#ifndef DRIFT_MINIX_RESOURCES_H
#define DRIFT_MINIX_RESOURCES_H
#include "drift_virtio.h"
#define DRIFT_MINIX_DMA_SLOTS 8
struct minix_dma_slot { drift_dma allocation; unsigned actual_size; };
typedef struct minix_resources {
    drift_hw hw;
    drift_pci pci;
    drift_virtio_bar bars[6];
    volatile unsigned char *window[6];
    unsigned window_size[6];
    struct minix_dma_slot dma_slots[DRIFT_MINIX_DMA_SLOTS];
    unsigned bdf, devind, fault, allocations, releases, syncs, maps;
    unsigned long long next_token;
} minix_resources;
/* Zero-initialize once and retain the provider through driver quiescence.
 * PCI ACL is installed by RS before entry. Reserves exactly 1af4:1044.
 * Do not call open again while mappings or DMA allocations remain live. */
int minix_resources_open(minix_resources *);
/* Caller must prove device reset/quiescence first. Refuses live DMA slots. */
int minix_resources_close(minix_resources *);
#endif
