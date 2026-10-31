/* Polling Ethernet controller drivers. SPDX-License-Identifier: MIT */
#ifndef DRIFT_ETHERNET_H
#define DRIFT_ETHERNET_H
#include "drift_hw.h"
#ifdef __cplusplus
extern "C" {
#endif

enum { DRIFT_ETH_E1000=1, DRIFT_ETH_RTL8139=2 };
enum { DRIFT_ETH_AGAIN=1, DRIFT_ETH_EARG=-1, DRIFT_ETH_ESTATE=-2,
       DRIFT_ETH_ENOMEM=-3, DRIFT_ETH_ETIMEOUT=-4, DRIFT_ETH_EDEVICE=-5,
       DRIFT_ETH_EBOUNDS=-6, DRIFT_ETH_EUNSUPPORTED=-7 };
/* Zero-initialize before first init. Serialized calls; no concurrent access.
 * region binds e1000 BAR0 MMIO, or RTL8139 BAR0 I/O / BAR1 MMIO, selected by host.
 * Host enables PCI bus mastering and maps resources before init. */
typedef struct drift_ethernet {
    drift_hw *hw;
    drift_dma rx_ring, tx_ring, rx_data, tx_data;
    uint32_t region, kind, rx_next, tx_next, timeout_us;
    int32_t state;
    uint8_t mac[6];
} drift_ethernet;
int32_t drift_eth_init(drift_ethernet *, drift_hw *, uint32_t region,
                       uint32_t kind, uint32_t timeout_us);
/* Ethernet frames exclude FCS. TX pads frames below 60 bytes. Max frame1518.
 * TX is synchronous; RX returns AGAIN if empty, EBOUNDS without consuming a
 * frame if the destination is too small, or zero with *length bytes copied. */
int32_t drift_eth_tx(drift_ethernet *, uint8_t *frame, uint32_t length);
int32_t drift_eth_rx(drift_ethernet *, uint8_t *frame, uint32_t capacity, uint32_t *length);
/* Reset stops DMA before releasing allocations. If reset times out, resources
 * stay quarantined in the object and close may be retried; do not free them. */
int32_t drift_eth_close(drift_ethernet *);
#ifdef __cplusplus
}
#endif
#endif
