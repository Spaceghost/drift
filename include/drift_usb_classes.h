/* USB CDC ACM serial and ECM Ethernet. SPDX-License-Identifier: MIT */
#ifndef DRIFT_USB_CLASSES_H
#define DRIFT_USB_CLASSES_H
#include "drift_usb.h"
#ifdef __cplusplus
extern "C" {
#endif
enum { DRIFT_USB_CDC_ACM=1, DRIFT_USB_CDC_ECM=2 };
typedef struct drift_usb_cdc {
    drift_usb *device;
    uint32_t kind, control_interface, data_interface, alternate;
    uint32_t endpoint_in, endpoint_out, out_packet_size, maximum_segment, state;
    uint8_t mac[6];
} drift_usb_cdc;
/* Zero-initialize class state; core must have attached/configured a device.
 * UINT32_MAX selects the first matching control interface. Only USB-standard
 * CDC ACM and ECM are accepted, not vendor UARTs, RNDIS or NCM. */
int32_t drift_usb_cdc_open(drift_usb_cdc *,drift_usb *,uint32_t kind,uint32_t control_interface);
/* stop: 0=one,1=one-and-half,2=two; parity0..4; bits5..8 or16;
 * lines: bit0 DTR, bit1 RTS. A failed second request can leave changed coding. */
int32_t drift_usb_acm_configure(drift_usb_cdc *,uint32_t baud,uint32_t stop,uint32_t parity,uint32_t bits,uint32_t lines);
/* Read returns byte count, 0 when idle, or negative error. ECM capacity must
 * cover maximum_segment so a whole frame is received. Write returns bytes.
 * Calls sharing the core/controller must be serialized. */
int32_t drift_usb_cdc_read(drift_usb_cdc *,uint8_t *,uint32_t capacity);
int32_t drift_usb_cdc_write(drift_usb_cdc *,uint8_t *,uint32_t size);
int32_t drift_usb_cdc_close(drift_usb_cdc *);
#ifdef __cplusplus
}
#endif
#endif
