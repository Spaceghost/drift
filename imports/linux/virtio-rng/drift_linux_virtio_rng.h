/* Bounded source-compatible Linux Virtio RNG personality. SPDX-License-Identifier: MIT */
#ifndef DRIFT_LINUX_VIRTIO_RNG_H
#define DRIFT_LINUX_VIRTIO_RNG_H
#include "drift_virtio.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct drift_linux_virtio_rng {
    drift_virtio *transport;
    drift_dma payload;
    drift_virtio_raw_transfer transfer;
    void *hwrng, *ida, *source_buffer, *completed_cookie;
    uint8_t *read_output;
    uint64_t read_started;
    uint32_t phase, allocated, queue_created, registered, id_owned, index;
    uint32_t free_requested, id_free_requested, completion_latched, completed_length;
    uint32_t read_length, read_active, cancel_requested;
    int32_t error;
    uint64_t source_slot[256], device_slot[16], queue_slot[16];
} drift_linux_virtio_rng;

/* Zero-initialize state once; retain it and the transport/resources until
 * close succeeds. Open requires raw deferred-ready device 4 with one queue.
 * The unchanged Linux source calls device_ready and owns all RNG read logic.
 * This source uses Linux's unsigned-long/pointer data model: LP64 or ILP32.
 * The profile deliberately rejects LLP64 (native Windows x64) at compile time.
 * Calls are serialized with a non-spinning gate; overlap/reentry is -16.
 * Completion callbacks are pumped from actual used-ring entries; no IRQ is
 * installed and no host entropy/read operation is accepted. */
int32_t drift_linux_virtio_rng_open(drift_linux_virtio_rng *,drift_virtio *);
/* Return copied bytes, zero for a pending nonblocking request, or negative
 * errno. On a negative result ignore the output: the upstream read may already
 * have consumed/copied a prefix before interruption or a transport failure. */
int32_t drift_linux_virtio_rng_read(drift_linux_virtio_rng *,uint8_t *,uint32_t,int32_t wait);
int32_t drift_linux_virtio_rng_service(drift_linux_virtio_rng *);
/* Cancellation is an atomic request; caller must retain an open instance.
 * It interrupts an active/next blocking read without reclaiming pending DMA.
 * Nonblocking reads do not consume the cancellation request. */
int32_t drift_linux_virtio_rng_cancel(drift_linux_virtio_rng *);
int32_t drift_linux_virtio_rng_close(drift_linux_virtio_rng *);
/* Freeze performs the actual upstream freeze/reset lifecycle. Restore requires
 * a newly opened raw deferred-ready transport after freeze has quiesced. */
int32_t drift_linux_virtio_rng_freeze(drift_linux_virtio_rng *);
int32_t drift_linux_virtio_rng_restore(drift_linux_virtio_rng *,drift_virtio *);

#ifdef __cplusplus
}
#endif
#endif
