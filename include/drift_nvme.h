/* Small polling NVMe controller. SPDX-License-Identifier: MIT */
#ifndef DRIFT_NVME_H
#define DRIFT_NVME_H
#include "drift_hw.h"
#ifdef __cplusplus
extern "C" {
#endif

enum { DRIFT_NVME_OK=0, DRIFT_NVME_EARG=-1, DRIFT_NVME_ENOTSUP=-2,
       DRIFT_NVME_ENOMEM=-3, DRIFT_NVME_ETIMEOUT=-4, DRIFT_NVME_EIO=-5,
       DRIFT_NVME_EPROTO=-6, DRIFT_NVME_ESTATE=-7 };
/* Zero initialize before init. Serialize all calls. State 3 quarantines DMA:
 * close can retry the reset; never release its resources behind the driver. */
typedef struct drift_nvme {
    drift_hw *hw;
    drift_dma asq, acq, isq, icq, data;
    uint64_t blocks;
    uint32_t region, nsid, block_size, depth, stride, timeout_us, reset_us;
    uint32_t sq_tail[2], cq_head[2], phase[2];
    uint32_t cid, state, last_status, last_result;
} drift_nvme;
int32_t drift_nvme_init(drift_nvme *, drift_hw *, uint32_t region,
                        uint32_t namespace_id, uint32_t timeout_us);
/* Caller buffer holds count*block_size bytes, maximum4096 bytes per call.
 * Writes complete at the controller; flush establishes nonvolatile completion. */
int32_t drift_nvme_read(drift_nvme *, uint64_t lba, uint32_t count, uint8_t *);
int32_t drift_nvme_write(drift_nvme *, uint64_t lba, uint32_t count, uint8_t *);
int32_t drift_nvme_flush(drift_nvme *);
int32_t drift_nvme_close(drift_nvme *);
#ifdef __cplusplus
}
#endif
#endif
