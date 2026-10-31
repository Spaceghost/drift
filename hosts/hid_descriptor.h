/* Strict, allocation-free report-layout proof. SPDX-License-Identifier: MIT */
#ifndef DRIFT_HID_DESCRIPTOR_H
#define DRIFT_HID_DESCRIPTOR_H
#include <stddef.h>
#include <stdint.h>

/* Accept a conservative subset of HID descriptors with exactly the unnumbered
 * eight-byte boot keyboard input layout. No mode change or device I/O. */
int drift_boot_keyboard_descriptor(const uint8_t *data, size_t size);
#endif
