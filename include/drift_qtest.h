/* QEMU qtest resource backend. SPDX-License-Identifier: MIT */
#ifndef DRIFT_QTEST_H
#define DRIFT_QTEST_H
#include "drift_hw.h"
#ifdef __cplusplus
extern "C" {
#endif
#define DRIFT_QTEST_REGIONS 64
#define DRIFT_QTEST_ALLOCS 128

typedef struct drift_qtest_region { uint64_t base; uint32_t size; int io; } drift_qtest_region;
typedef struct drift_qtest {
    drift_hw hardware;
    int fd, failed, clock;
    uint64_t now, next_dma;
    uint32_t region_count, allocation_count;
    drift_qtest_region regions[DRIFT_QTEST_REGIONS];
    drift_dma allocations[DRIFT_QTEST_ALLOCS];
} drift_qtest;
int drift_qtest_connect(drift_qtest *, const char *port);
void drift_qtest_disconnect(drift_qtest *);
uint32_t drift_qtest_pci_read(drift_qtest *, uint32_t bdf, uint32_t offset);
void drift_qtest_pci_write(drift_qtest *, uint32_t bdf, uint32_t offset, uint32_t value);
int drift_qtest_pci_find(drift_qtest *, uint16_t vendor, uint16_t device, unsigned instance, uint32_t *bdf);
int drift_qtest_pci_bar(drift_qtest *, uint32_t bdf, unsigned bar, uint32_t *region);
/* Map ordinary BAR resources, skipping the upper halves of 64-bit BARs. */
int drift_qtest_pci_regions(drift_qtest *, uint32_t bdf, uint32_t regions[6], uint32_t sizes[6]);
int drift_qtest_memory(drift_qtest *, uint64_t address, uint8_t *bytes, uint32_t length, int write);
#ifdef __cplusplus
}
#endif
#endif
