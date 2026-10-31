/* One USB 1.x/2.0 hub and one selected child. SPDX-License-Identifier: MIT */
#ifndef DRIFT_USB_HUB_H
#define DRIFT_USB_HUB_H
#include "drift_usb.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Call after attaching a root hub device. port0 selects its first connected
 * downstream port; explicit ports are1..15. Powers, debounces and resets the
 * port, then asks the controller engine to enumerate that child. Selection is
 * one-way until controller close; no background hub interrupt polling. */
int32_t drift_usb_attach_hub(drift_usb *, uint32_t port);
#ifdef __cplusplus
}
#endif
#endif
