/* Modern Virtio, polling split queues. SPDX-License-Identifier: MIT */
#ifndef DRIFT_VIRTIO_H
#define DRIFT_VIRTIO_H
#include "drift_hw.h"
#include "drift_pci.h"
#ifdef __cplusplus
extern "C" {
#endif

/* One externally owned coherent DMA range. Keep this context and its mapping
 * alive and immutable until get or a successful device reset. Native close
 * never releases this external allocation. */
typedef struct drift_virtio_raw_transfer {
    drift_dma mapping;
    void *cookie, *owner;
    uint64_t started_us;
    uint32_t queue, offset, length, direction, active;
} drift_virtio_raw_transfer;

typedef struct drift_virtqueue {
    drift_dma ring, buffer;
    uint16_t available, used;
    uint32_t notify, pending, capacity;
    drift_virtio_raw_transfer *raw;
} drift_virtqueue;

typedef struct drift_virtio {
    drift_hw *hw;
    drift_virtqueue queue[2];
    uint64_t capacity;
    uint32_t region, common_offset, notify_region, notify_offset;
    uint32_t notify_multiplier, notify_length, device_region, device_offset;
    uint32_t transport, device_id, features, timeout_us, state;
    uint8_t mac[6];
} drift_virtio;

typedef struct drift_virtio_input_event {
    uint16_t type, code;
    uint32_t value;
} drift_virtio_input_event;

/* Zero-initialize caller-owned state. Regions must stay mapped and bound to
 * this device, with bus mastering enabled by the host. No concurrent calls.
 * Return 0 success; -1 arguments/device, -2 timeout, -3 range, -4 unsupported,
 * -5 invalid completion/device failure, -6 allocation. After errors that
 * poison state, call close and retain all storage until close succeeds. */
int32_t drift_virtio_open_mmio(drift_virtio *, drift_hw *, uint32_t region, uint32_t expected_id);
int32_t drift_virtio_open_pci(drift_virtio *, drift_hw *, uint32_t common_region,
    uint32_t common_offset, uint32_t notify_region, uint32_t notify_offset,
    uint32_t notify_multiplier, uint32_t device_region, uint32_t device_offset,
    uint32_t device_id);
typedef struct drift_virtio_bar {
    uint32_t region;
    uint64_t size;
} drift_virtio_bar;
/* Preferred PCI attach: host binds six BAR regions (UINT32_MAX = unmapped).
 * Native code walks vendor capabilities, checks each range and negotiates.
 * Host must already enable memory decoding and bus mastering. */
int32_t drift_virtio_open_function(drift_virtio *,drift_hw *,drift_pci *,
                                  uint32_t bdf,drift_virtio_bar *bindings);
int32_t drift_virtio_close(drift_virtio *);
/* Bounded generic split-queue transport for source compatibility frameworks.
 * Negotiate VERSION_1 with no device-specific features; one or two queues,
 * one descriptor/in-flight transfer per queue. Deferred open leaves DRIVER_OK
 * unset (state 4). device_ready enters raw-active state 5. Existing class
 * operations require their separate state 2 and cannot consume raw queues.
 * Host maps/registers/DMAs/time are the only platform services involved. */
int32_t drift_virtio_raw_open_mmio(drift_virtio *, drift_hw *, uint32_t region,
    uint32_t expected_id, uint32_t queues);
int32_t drift_virtio_raw_open_pci(drift_virtio *, drift_hw *, uint32_t common_region,
    uint32_t common_offset, uint32_t notify_region, uint32_t notify_offset,
    uint32_t notify_multiplier, uint32_t device_region, uint32_t device_offset,
    uint32_t expected_id, uint32_t queues);
int32_t drift_virtio_raw_open_function(drift_virtio *,drift_hw *,drift_pci *,
    uint32_t bdf,drift_virtio_bar *,uint32_t expected_id,uint32_t queues);
int32_t drift_virtio_raw_device_ready(drift_virtio *);
/* add publishes one buffer but does not notify. direction is TO_DEVICE or
 * FROM_DEVICE. DMA must meet drift_hw's coherent/uncached contract and belong
 * to this device's host domain. The context snapshots its mapping metadata.
 * get returns 1 once with cookie/used length after DMA acquisition, 0 pending
 * or no transfer, or a negative error. A timeout/malformed completion resets
 * before reclamation; on failed reset retain all external resources. */
int32_t drift_virtio_raw_add(drift_virtio *,uint32_t,drift_virtio_raw_transfer *,
    drift_dma *,uint32_t offset,uint32_t length,uint32_t direction,void *cookie);
int32_t drift_virtio_raw_kick(drift_virtio *,uint32_t);
int32_t drift_virtio_raw_get(drift_virtio *,uint32_t,drift_virtio_raw_transfer *,
    void **cookie,uint32_t *used_length);
/* Sectors are always 512 bytes; count 1..128; copied synchronously. */
int32_t drift_virtio_block(drift_virtio *, uint64_t sector, uint32_t count, uint8_t *, int32_t write);
int32_t drift_virtio_flush(drift_virtio *);
/* send returns 0; receive returns frame bytes, 0 if no frame, or error. */
int32_t drift_virtio_send(drift_virtio *, uint8_t *, uint32_t size);
int32_t drift_virtio_receive(drift_virtio *, uint8_t *, uint32_t capacity);
/* Console and entropy return byte counts. Console read and input poll return
 * zero if no input; input poll returns 1 for one complete event. */
int32_t drift_virtio_console_write(drift_virtio *, uint8_t *, uint32_t size);
int32_t drift_virtio_console_read(drift_virtio *, uint8_t *, uint32_t capacity);
int32_t drift_virtio_entropy(drift_virtio *, uint8_t *, uint32_t capacity);
int32_t drift_virtio_input(drift_virtio *, drift_virtio_input_event *);
#ifdef __cplusplus
}
#endif
#endif
