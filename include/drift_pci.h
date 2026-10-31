/* PCI discovery without an OS-specific enumeration layer. SPDX-License-Identifier: MIT */
#ifndef DRIFT_PCI_H
#define DRIFT_PCI_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* bdf = bus<<8 | device<<3 | function; offset is naturally aligned.
 * Host implements the platform's config mechanism (ECAM or legacy ports).
 * Calls never assign BAR addresses or change routing. */
typedef struct drift_pci {
    void *context;
    uint32_t (*read)(void *, uint32_t bdf, uint32_t offset);
    void (*write)(void *, uint32_t bdf, uint32_t offset, uint32_t value);
} drift_pci;
typedef struct drift_pci_device {
    uint32_t bdf, vendor, device, class_code;
} drift_pci_device;
typedef struct drift_pci_bar {
    uint64_t address;
    uint32_t io, prefetch, width;
} drift_pci_bar;
static inline uint32_t drift_pci_read(drift_pci *p,uint32_t b,uint32_t o) {return p->read(p->context,b,o);}
static inline void drift_pci_write(drift_pci *p,uint32_t b,uint32_t o,uint32_t v) {p->write(p->context,b,o,v);}
static inline int32_t drift_pci_valid(drift_pci *p) {return p && p->read && p->write;}
/* Return 0 found, 1 not found, -1 invalid arguments, -2 malformed config.
 * 0xffff vendor/device filters are wildcards. Buses are inclusive.
 * index selects the Nth match. The host enables PCI bridge routing first. */
int32_t drift_pci_find(drift_pci *, uint32_t first_bus, uint32_t last_bus,
                       uint32_t vendor, uint32_t device, uint32_t index, drift_pci_device *);
int32_t drift_pci_cap(drift_pci *, uint32_t bdf, uint32_t cap_id, uint32_t index, uint32_t *offset);
int32_t drift_pci_get_bar(drift_pci *, uint32_t bdf, uint32_t bar, drift_pci_bar *);
/* Add IO/memory decoding and/or bus mastering: mask may contain only bits0..2. */
int32_t drift_pci_enable(drift_pci *, uint32_t bdf, uint32_t mask);
#ifdef __cplusplus
}
#endif
#endif
