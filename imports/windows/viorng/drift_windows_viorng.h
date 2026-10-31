/* Bounded unchanged-source Virtio-Windows RNG personality.
 * SPDX-License-Identifier: MIT */
#ifndef DRIFT_WINDOWS_VIORNG_H
#define DRIFT_WINDOWS_VIORNG_H
#include <stddef.h>
#include <stdint.h>
#include "drift_virtio.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct drift_win_rng drift_win_rng;
typedef struct drift_win_rng_request drift_win_rng_request;
typedef struct drift_win_rng_object {
    drift_win_rng *owner;
    uint32_t kind;
} drift_win_rng_object;
typedef struct drift_win_rng_host {
    void *context;
    void (*completed)(void *,drift_win_rng_request *);
} drift_win_rng_host;
struct drift_win_rng_request {
    drift_win_rng *owner;
    uint8_t *output;
    size_t capacity, length;
    uint32_t phase, cancelable, cancel_owned, notifying, completions;
    int32_t status;
    uintptr_t information;
    void (*cancel)(drift_win_rng_request *);
};
struct drift_win_rng {
    drift_win_rng_host *host;
    drift_hw *hw;
    drift_pci *pci;
    drift_virtio_bar *bars;
    uint32_t mode,region,bdf,phase,depth,locked,configured,prepared,powered;
    uint32_t callbacks,dpc,irq_latch,latch_ready,latch_length;
    uint32_t pool_used,pool_quarantined,pending_count,notify_count,dispatching;
    int32_t error,last_error;
    void *latch_cookie;
    drift_win_rng_request *active,*pending[16],*notifications[16];
    drift_virtio transport;
    drift_virtio_raw_transfer transfer;
    drift_dma common;
    drift_win_rng_object driver,device,queue,interrupt,spinlock,init,driver_object;
    uint64_t context_storage[64],config_storage[128],entry_storage[16][4];
};
enum { DRIFT_WIN_RNG_IDLE=0, DRIFT_WIN_RNG_QUEUED=1,
       DRIFT_WIN_RNG_ACTIVE=2, DRIFT_WIN_RNG_COMPLETE=3 };
#define DRIFT_WIN_RNG_SUCCESS ((int32_t)0)
#define DRIFT_WIN_RNG_PENDING ((int32_t)0x103)
#define DRIFT_WIN_RNG_CANCELLED ((int32_t)0xc0000120)
#define DRIFT_WIN_RNG_INVALID_PARAMETER ((int32_t)0xc000000d)
#define DRIFT_WIN_RNG_INVALID_STATE ((int32_t)0xc0000184)
#define DRIFT_WIN_RNG_BUSY ((int32_t)0x80000011)
/* Zero-initialize state and requests before their first use. Serialize all
 * calls across ALL instances in one owner domain: the scoped source allocator
 * binding is global and non-atomic. Notifications are deferred until
 * source callbacks and locks have returned; they may enqueue other requests.
 * Keep requests/buffers alive through completion AND the outer API return.
 * This profile supports a polling-backed virtual WDF interrupt, not real
 * Windows IRQ/IRQL, PnP or CNG registration. All entropy comes through the
 * actual externally owned DMA buffer and native split queue.
 * Failed reset retains/quarantines DMA and metadata. Retry close with all
 * device resources still alive; never zero or reuse a live/failed instance. */
int32_t drift_windows_viorng_open_mmio(drift_win_rng *,drift_hw *,uint32_t region,drift_win_rng_host *);
int32_t drift_windows_viorng_open_function(drift_win_rng *,drift_hw *,drift_pci *,uint32_t bdf,
    drift_virtio_bar *,drift_win_rng_host *);
int32_t drift_windows_viorng_read(drift_win_rng *,drift_win_rng_request *,uint8_t *,size_t capacity,size_t length);
/* Pump validates a real used-ring completion before invoking original ISR/DPC.
 * Returns delivered notifications, zero if pending, or a negative NTSTATUS. */
int32_t drift_windows_viorng_pump(drift_win_rng *);
int32_t drift_windows_viorng_cancel(drift_win_rng *,drift_win_rng_request *);
int32_t drift_windows_viorng_request_reset(drift_win_rng_request *);
int32_t drift_windows_viorng_close(drift_win_rng *);
#ifdef __cplusplus
}
#endif
#endif
