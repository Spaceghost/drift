/* One OS-facing block/frame interface for every shipped controller.
 * SPDX-License-Identifier: MIT */
#ifndef DRIFT_IO_H
#define DRIFT_IO_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

enum { DRIFT_BLOCK_VIRTIO=1, DRIFT_BLOCK_NVME=2, DRIFT_BLOCK_AHCI=3, DRIFT_BLOCK_USB=4 };
enum { DRIFT_NET_VIRTIO=1, DRIFT_NET_ETHERNET=2, DRIFT_NET_USB_ECM=3 };
enum { DRIFT_IO_EARG=-1000, DRIFT_IO_ESTATE=-1001, DRIFT_IO_ERANGE=-1002,
       DRIFT_IO_ENOTSUP=-1003, DRIFT_IO_EPROTO=-1004 };
typedef struct drift_block {
    uint32_t kind;
    void *driver;
    uint64_t blocks;
    uint32_t sector_size, max_sectors, completed;
} drift_block;
typedef struct drift_net {
    uint32_t kind;
    void *driver;
    uint8_t mac[6];
    uint32_t max_frame;
} drift_net;
/* Bind an already initialized native driver. Objects are borrowed, not copied
 * or closed. USB block bind queries READ CAPACITY; all other binds read cached
 * identified geometry. Any failed bind clears the facade. Rebind after reset,
 * reattach or reconfiguration. Serialize facade and native driver calls. */
int32_t drift_block_bind(drift_block *,uint32_t kind,void *driver);
/* bytes must cover count*sector_size. Requests are automatically split to the
 * controller's limit. completed counts fully successful sectors in this call;
 * on failure the failing hardware command itself may have partially written.
 * Flush is explicit and required when persistence matters. */
int32_t drift_block_read(drift_block *,uint64_t lba,uint32_t count,uint8_t *buffer,uint32_t bytes);
int32_t drift_block_write(drift_block *,uint64_t lba,uint32_t count,uint8_t *buffer,uint32_t bytes);
int32_t drift_block_flush(drift_block *);
int32_t drift_net_bind(drift_net *,uint32_t kind,void *driver);
/* Ethernet frames exclude FCS. send returns zero; receive returns frame bytes,
 * zero when idle, or a negative error. RX capacity must be at least max_frame
 * for consistent whole-frame semantics across the different native drivers. */
int32_t drift_net_send(drift_net *,uint8_t *frame,uint32_t bytes);
int32_t drift_net_receive(drift_net *,uint8_t *frame,uint32_t capacity);
#ifdef __cplusplus
}
#endif
#endif
