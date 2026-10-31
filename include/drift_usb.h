/* Polled xHCI and portable USB classes. SPDX-License-Identifier: MIT */
#ifndef DRIFT_USB_H
#define DRIFT_USB_H
#include "drift_hw.h"
#ifdef __cplusplus
extern "C" {
#endif
#define DRIFT_USB_TRANSFER_MAX 16384u
/* A controller instance selects one USB 1.x/2.0 device, direct or via one hub.
 * USB addresses, contexts, TRBs and class protocols live in this driver.
 * The caller enables PCI memory/bus mastering and maps its xHCI BAR. */
typedef struct drift_usb {
    drift_hw *hw;
    drift_dma arena;
    uint32_t region, op, runtime, doorbells, context_size, ports;
    uint32_t command_index, command_cycle, event_index, event_cycle;
    uint32_t slot, port, speed, state, error, configuration, interface_number;
    uint32_t kind, endpoint_in, endpoint_out, max_packet, total_config;
    uint32_t ring_offset[32], ring_index[32], ring_cycle[32];
    uint32_t tag, sector_size;
    uint64_t blocks;
    uint32_t pending_count[32];
    uint64_t pending_pointer[32];
    int32_t completion[32];
    uint32_t completed_length[32], halted[32];
    uint32_t device_context, route, parent_slot, parent_port, hub_speed, config_index;
    uint8_t config[1024];
} drift_usb;
enum { DRIFT_USB_HID_KEYBOARD=1, DRIFT_USB_HID_MOUSE=2, DRIFT_USB_STORAGE=3, DRIFT_USB_GENERIC=4, DRIFT_USB_HUB=5 };
/* Zero success; errors use negative errno-style values. Timeouts stop the
 * controller. If it cannot halt, DMA is quarantined and close returns failure;
 * the host must keep that memory/device mapping alive until a bus reset. */
int32_t drift_usb_open(drift_usb *,drift_hw *,uint32_t region);
int32_t drift_usb_attach_config(drift_usb *,uint32_t root_port,uint32_t descriptor_index);
int32_t drift_usb_attach(drift_usb *,uint32_t root_port); /* 0: first connected */
int32_t drift_usb_control(drift_usb *,uint8_t request_type,uint8_t request,
                         uint16_t value,uint16_t index,uint8_t *,uint32_t length,uint32_t *received);
int32_t drift_usb_transfer(drift_usb *,uint8_t endpoint,uint8_t *,uint32_t length,uint32_t *received);
int32_t drift_usb_hub_child(drift_usb *,uint8_t hub_port,uint8_t speed,uint8_t hub_ports,uint8_t tt_think);
int32_t drift_usb_set_interface(drift_usb *,uint8_t interface_number,uint8_t alternate);
int32_t drift_usb_hid(drift_usb *,uint8_t *,uint32_t capacity,uint32_t *received);
int32_t drift_usb_storage_info(drift_usb *,uint64_t *blocks,uint32_t *sector_size);
int32_t drift_usb_storage_read(drift_usb *,uint64_t lba,uint32_t count,uint8_t *);
int32_t drift_usb_storage_write(drift_usb *,uint64_t lba,uint32_t count,uint8_t *);
int32_t drift_usb_storage_flush(drift_usb *);
int32_t drift_usb_recover(drift_usb *,uint8_t endpoint);
int32_t drift_usb_close(drift_usb *);
#ifdef __cplusplus
}
#endif
#endif
