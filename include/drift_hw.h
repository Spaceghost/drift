/* Portable controller resources. SPDX-License-Identifier: MIT */
#ifndef DRIFT_HW_H
#define DRIFT_HW_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct drift_dma {
    uint8_t *data;
    uint64_t address;
    uint32_t size;
    uint64_t token;
} drift_dma;

enum { DRIFT_DMA_TO_DEVICE=1, DRIFT_DMA_FROM_DEVICE=2, DRIFT_DMA_BIDIRECTIONAL=3 };
/* Regions are host-bound register windows, never guest-supplied addresses.
 * read/write use width 1/2/4/8 and return/accept native numeric register values.
 * dma returns zero on success; CPU and device addresses may differ. Allocations
 * must be coherent or uncached: control rings can contain CPU-owned and
 * device-owned fields in the same cache line. A streaming-only noncoherent
 * mapping cannot satisfy this contract by rounding sync ranges to cache lines.
 * sync performs cache maintenance AND ordering, even on coherent machines.
 * TO_DEVICE publishes CPU writes (clean + release); FROM_DEVICE acquires device
 * writes (invalidate + acquire). BIDIRECTIONAL is a CPU-to-device handoff for
 * buffers the device may both read and write: clean + invalidate + release.
 * Reclaim all such buffers with FROM_DEVICE; BIDIRECTIONAL never copies device
 * data back to the CPU. Do not publish bytes the device still owns.
 * time_us is monotonic; wait_us must advance time or yield until it advances.
 * All resources remain valid until a driver has stopped device DMA. */
typedef struct drift_hw {
    void *context;
    uint64_t (*read)(void *, uint32_t region, uint32_t offset, uint32_t width);
    void (*write)(void *, uint32_t region, uint32_t offset, uint32_t width, uint64_t value);
    int32_t (*dma)(void *, uint32_t size, uint32_t alignment, drift_dma *out);
    void (*release)(void *, drift_dma *buffer);
    void (*sync)(void *, drift_dma *buffer, uint32_t offset, uint32_t length, uint32_t direction);
    uint64_t (*time_us)(void *);
    void (*wait_us)(void *, uint32_t duration);
} drift_hw;

static inline uint64_t drift_hw_read(drift_hw *h,uint32_t r,uint32_t o,uint32_t w) { return h->read(h->context,r,o,w); }
static inline void drift_hw_write(drift_hw *h,uint32_t r,uint32_t o,uint32_t w,uint64_t v) { h->write(h->context,r,o,w,v); }
static inline int32_t drift_hw_dma(drift_hw *h,uint32_t n,uint32_t a,drift_dma *b) { return h->dma(h->context,n,a,b); }
static inline void drift_hw_release(drift_hw *h,drift_dma *b) { h->release(h->context,b); }
static inline void drift_hw_sync(drift_hw *h,drift_dma *b,uint32_t o,uint32_t n,uint32_t d) { h->sync(h->context,b,o,n,d); }
static inline uint64_t drift_hw_time(drift_hw *h) { return h->time_us(h->context); }
static inline void drift_hw_wait(drift_hw *h,uint32_t us) { h->wait_us(h->context,us); }
static inline int32_t drift_hw_valid(drift_hw *h) {
    return h && h->read && h->write && h->dma && h->release && h->sync && h->time_us && h->wait_us;
}
#ifdef __cplusplus
}
#endif
#endif
