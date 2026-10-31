/* Explicit resources for a bounded, source-only KMDF mouse-filter profile.
 * SPDX-License-Identifier: MIT */
#ifndef DRIFT_WINDOWS_MOUFILTR_H
#define DRIFT_WINDOWS_MOUFILTR_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct drift_win_moufiltr drift_win_moufiltr;
typedef struct drift_win_mou_request drift_win_mou_request;
typedef struct drift_win_mou_object {
    drift_win_moufiltr *owner;
    uint32_t kind;
    void *context;
} drift_win_mou_object;

typedef struct drift_win_mouse_packet {
    uint16_t UnitId, Flags;
    union { uint32_t Buttons; struct { uint16_t ButtonFlags, ButtonData; }; };
    uint32_t RawButtons;
    int32_t LastX, LastY;
    uint32_t ExtraInformation;
} drift_win_mouse_packet;
typedef void (*drift_win_mouse_service)(drift_win_mou_object *,
    drift_win_mouse_packet *, drift_win_mouse_packet *, uint32_t *);
typedef struct drift_win_mouse_connect {
    drift_win_mou_object *ClassDeviceObject;
    drift_win_mouse_service ClassService;
} drift_win_mouse_connect;

typedef struct drift_win_mou_host {
    void *context;
    /* NT_SUCCESS return transfers ownership, including inline completion.
     * Failure leaves the request with the imported driver. Never both fail
     * the send and complete it. This callback may reenter with another request. */
    int32_t (*send)(void *, drift_win_mou_request *, uint32_t control);
    void (*completed)(void *, drift_win_mou_request *);
} drift_win_mou_host;

struct drift_win_mou_request {
    drift_win_moufiltr *owner;
    uint32_t phase, dispatching, completions, control;
    int32_t status, error;
    void *input;
    size_t input_length;
    uintptr_t information;
};

typedef int32_t (*drift_win_mou_add_fn)(drift_win_mou_object *, drift_win_mou_object *);
typedef void (*drift_win_mou_control_fn)(drift_win_mou_object *, drift_win_mou_request *,
    size_t, size_t, uint32_t);
struct drift_win_moufiltr {
    drift_win_mou_host *host;
    uint32_t phase, outstanding, depth, filter, device_type;
    int32_t error;
    drift_win_mou_object driver, device, queue, target, init, driver_object, device_object;
    uint64_t extension[16];
    drift_win_mou_add_fn add;
    drift_win_mou_control_fn control;
};

#define DRIFT_WIN_SUCCESS ((int32_t)0)
#define DRIFT_WIN_PENDING ((int32_t)0x103)
#define DRIFT_WIN_NOT_IMPLEMENTED ((int32_t)0xc0000002)
#define DRIFT_WIN_INVALID_HANDLE ((int32_t)0xc0000008)
#define DRIFT_WIN_INVALID_PARAMETER ((int32_t)0xc000000d)
#define DRIFT_WIN_BUFFER_TOO_SMALL ((int32_t)0xc0000023)
#define DRIFT_WIN_SHARING_VIOLATION ((int32_t)0xc0000043)
#define DRIFT_WIN_INSUFFICIENT_RESOURCES ((int32_t)0xc000009a)
#define DRIFT_WIN_INVALID_DEVICE_STATE ((int32_t)0xc0000184)
#define DRIFT_WIN_DEVICE_BUSY ((int32_t)0x80000011)
#define DRIFT_WIN_MOUSE_CONNECT 0x000f0203U
#define DRIFT_WIN_MOUSE_DISCONNECT 0x000f0403U
#define DRIFT_WIN_MOUSE_HOOK 0x000f3fc3U
#define DRIFT_WIN_MOUSE_QUERY_ATTRIBUTES 0x000f0000U

/* Zero-initialize state and each request. One serialized owner domain may
 * reenter on another request, and multiple forwarded requests may remain
 * pending. Cross-thread callers must marshal to that domain. This is not a
 * general KMDF scheduler or kernel IRQL implementation.
 * At most 64 requests can be outstanding and control nesting is bounded to 64.
 * Keep request storage alive through the outer control/complete API return,
 * including after a completed callback; never free or reset it in that callback.
 * Keep buffers alive until completion. CONNECT retains the upper class object,
 * callback/context and function code until hooks are quiesced and state closed.
 * Marshal every raw installed mouse/ISR hook to the same owner domain and
 * drain active hook calls before close; close cannot detect raw hook activity. */
int32_t drift_windows_moufiltr_open(drift_win_moufiltr *, drift_win_mou_host *);
int32_t drift_windows_moufiltr_control(drift_win_moufiltr *, drift_win_mou_request *,
    uint32_t control, void *input, size_t length);
int32_t drift_windows_moufiltr_complete(drift_win_mou_request *, int32_t status, uintptr_t information);
int32_t drift_windows_moufiltr_request_reset(drift_win_mou_request *);
int32_t drift_windows_moufiltr_packets(drift_win_moufiltr *, drift_win_mouse_packet *,
    uint32_t count, uint32_t *consumed);
int32_t drift_windows_moufiltr_close(drift_win_moufiltr *);
#ifdef __cplusplus
}
#endif
#endif
