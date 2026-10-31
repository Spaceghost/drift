/* Owned virtual-PCI resources over QEMU qtest TCP. SPDX-License-Identifier: MIT */
#ifndef DRIFT_VIRTIO_QTEST_H
#define DRIFT_VIRTIO_QTEST_H
#include "drift_hw.h"
#include "drift_pci.h"
#include "drift_virtio.h"
#ifdef __cplusplus
extern "C" {
#endif
#define DRIFT_VQT_REGIONS 16
#define DRIFT_VQT_ALLOCS 128
typedef struct drift_vqt_region { uint64_t base, size; uint32_t io; } drift_vqt_region;
typedef struct drift_vqt {
    drift_hw hardware;
    drift_pci pci;
    uintptr_t socket;
    uint64_t next_dma, next_mmio, timer_frequency, timer_resolution_ns;
    uint32_t failed, winsock, region_count, allocation_count, bound_bdf;
    drift_vqt_region regions[DRIFT_VQT_REGIONS];
    drift_dma allocations[DRIFT_VQT_ALLOCS];
    uint64_t commands, pci_reads, pci_writes, register_reads, register_writes;
    uint64_t allocated, released, live, published_bytes, acquired_bytes;
    char error[192];
} drift_vqt;

/* Zero-initialize once; connect only to an owned loopback qtest server.
 * An initialized state cannot be reconnected. One serialized caller.
 * This host implements resources only: PCI config, mapped BAR access, guest
 * RAM DMA with independent CPU shadows, synchronization and monotonic time.
 * It does not construct Virtio descriptors or supply entropy. */
int drift_vqt_connect(drift_vqt *,uint16_t port);
/* Find/assign a bus-0 function's ordinary BARs while no firmware/CPU configures
 * it. Uses Drift's portable PCI enumerator; bdf is its normal bus<<8|dev<<3|fn.
 * Memory/IO decoding and bus mastering are enabled only after assignment. */
int drift_vqt_bind(drift_vqt *,uint16_t vendor,uint16_t device,unsigned instance,
                   uint32_t *bdf,drift_virtio_bar bars[6]);
/* Refuses to release live DMA unless the caller confirms the owned VM has
 * stopped. Successful device reset/close should release all mappings first.
 * Never pass vm_stopped merely because a driver operation timed out. */
int drift_vqt_dispose(drift_vqt *,int vm_stopped);
int drift_vqt_failed(const drift_vqt *);
#ifdef __cplusplus
}
#endif
#endif
