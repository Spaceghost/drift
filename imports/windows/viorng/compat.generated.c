/* ------------------------------ DIRECTIVES -------------------------------- */
/* Disable some warnings that the generated code can trigger. */
#if defined(__clang__) && __clang_major__ >= 3
  #pragma clang diagnostic ignored "-Wtype-limits"
  #pragma clang diagnostic ignored "-Wwrite-strings"
  #pragma clang diagnostic ignored "-Wunused"
  #pragma clang diagnostic ignored "-Wunused-parameter"
  #pragma clang diagnostic ignored "-Wmissing-field-initializers"
  #pragma clang diagnostic ignored "-Wparentheses-equality"
  #pragma clang diagnostic ignored "-Wtautological-compare"
  #pragma clang diagnostic ignored "-Wmissing-braces"
  #ifndef __cplusplus
    #pragma clang diagnostic ignored "-Wincompatible-pointer-types"
    #pragma clang diagnostic error   "-Wimplicit-function-declaration"
    #pragma clang diagnostic error   "-Wimplicit-int"
  #else
    #pragma clang diagnostic ignored "-Wnarrowing"
    #pragma clang diagnostic ignored "-Wc99-designator"
  #endif
#elif defined(__GNUC__) && __GNUC__ >= 5
  #pragma GCC diagnostic ignored "-Wtype-limits"
  #pragma GCC diagnostic ignored "-Wwrite-strings"
  #pragma GCC diagnostic ignored "-Wunused-parameter"
  #pragma GCC diagnostic ignored "-Wunused-value"
  #pragma GCC diagnostic ignored "-Wunused-variable"
  #pragma GCC diagnostic ignored "-Wunused-function"
  #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  #ifndef __cplusplus
    #pragma GCC diagnostic ignored "-Wmissing-braces"
    #pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
    #pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
    #pragma GCC diagnostic error   "-Wimplicit-function-declaration"
    #pragma GCC diagnostic error   "-Wimplicit-int"
  #else
    #pragma GCC diagnostic ignored "-Wnarrowing"
  #endif
#endif
#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
  #define _CRT_SECURE_NO_WARNINGS
#endif
/* Enable 64 bit offsets for stdio APIs. */
#if !defined(_FILE_OFFSET_BITS) && __SIZEOF_LONG__ >= 8
  #define _FILE_OFFSET_BITS 64
#endif
/* Enable POSIX APIs in included headers. */
#if !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE) && !defined(_GNU_SOURCE) && !defined(_DEFAULT_SOURCE)
  #if defined(__gnu_linux__)
    #define _GNU_SOURCE
  #else
    #define _XOPEN_SOURCE 600
  #endif
#endif
#include <stdint.h>
#include "profile.h"
#include <stddef.h>
#define NELUA_NIL (nlniltype){}
/* Macro used to export C functions. */
#ifdef _WIN32
  #define NELUA_CEXPORT __declspec(dllexport)
#elif defined(__GNUC__)
  #define NELUA_CEXPORT __attribute__((visibility("default")))
#else
  #define NELUA_CEXPORT
#endif
#include <stdbool.h>
/* Macro used for branch prediction. */
#if defined(__GNUC__) || defined(__clang__)
  #define NELUA_LIKELY(x) __builtin_expect(x, 1)
#else
  #define NELUA_LIKELY(x) (x)
#endif
/* Macro used for branch prediction. */
#if defined(__GNUC__) || defined(__clang__)
  #define NELUA_UNLIKELY(x) __builtin_expect(x, 0)
#else
  #define NELUA_UNLIKELY(x) (x)
#endif
/* Macro used to force inlining a function. */
#ifdef __GNUC__
  #define NELUA_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
  #define NELUA_INLINE __forceinline
#elif __STDC_VERSION__ >= 199901L
  #define NELUA_INLINE inline
#else
  #define NELUA_INLINE
#endif
/* ------------------------------ DECLARATIONS ------------------------------ */
static int32_t runtime_types_record_47Aim8M22hNeuoKr2_INVALID = -1073741811;
static int32_t runtime_types_record_47Aim8M22hNeuoKr2_STATE = -1073741436;
static int32_t runtime_types_record_47Aim8M22hNeuoKr2_UNSUCCESSFUL = -1073741823;
static int32_t runtime_types_record_47Aim8M22hNeuoKr2_OOM = -1073741670;
static int32_t runtime_types_record_47Aim8M22hNeuoKr2_CANCELLED = -1073741536;
static int32_t runtime_types_record_47Aim8M22hNeuoKr2_SMALL = -1073741789;
static int32_t runtime_types_record_47Aim8M22hNeuoKr2_UNSUPPORTED = -1073741637;
static int32_t runtime_types_record_47Aim8M22hNeuoKr2_IOERROR = -1073741435;
static int32_t runtime_types_record_47Aim8M22hNeuoKr2_TIMEOUT = -1073741643;
static int32_t runtime_types_record_47Aim8M22hNeuoKr2_BUSY = -2147483631;
static int32_t runtime_types_record_47Aim8M22hNeuoKr2_PENDING = 0x103;
typedef drift_win_rng* drift_win_rng_ptr;
typedef drift_win_rng_object* drift_win_rng_object_ptr;
static drift_win_rng_ptr runtime_types_record_47Aim8M22hNeuoKr2_owner(drift_win_rng_object_ptr o, uint32_t k);
static void runtime_types_record_47Aim8M22hNeuoKr2_fail(drift_win_rng_ptr s, int32_t status);
static int32_t runtime_types_record_47Aim8M22hNeuoKr2_native_error(int32_t n);
typedef struct nlniltype {} nlniltype;
typedef struct nlniltype nltype;
typedef drift_win_rng_object_ptr* drift_win_rng_object_ptr_arr0_ptr;
NELUA_CEXPORT int32_t WdfDriverCreate(drift_win_rng_object_ptr o, void* registry, void* attrs, void* c, drift_win_rng_object_ptr_arr0_ptr result);
NELUA_CEXPORT void WdfDeviceInitSetPnpPowerEventCallbacks(drift_win_rng_object_ptr o, void* c);
NELUA_CEXPORT void WdfDeviceInitSetIoType(drift_win_rng_object_ptr o, uint32_t t);
NELUA_CEXPORT int32_t WdfDeviceCreate(drift_win_rng_object_ptr_arr0_ptr i, void* a, drift_win_rng_object_ptr_arr0_ptr out);
NELUA_CEXPORT int32_t WdfInterruptCreate(drift_win_rng_object_ptr d, void* c, void* a, drift_win_rng_object_ptr_arr0_ptr out);
NELUA_CEXPORT int32_t WdfSpinLockCreate(void* a, drift_win_rng_object_ptr_arr0_ptr out);
NELUA_CEXPORT int32_t WdfDeviceCreateDeviceInterface(drift_win_rng_object_ptr d, void* g, void* r);
NELUA_CEXPORT int32_t WdfIoQueueCreate(drift_win_rng_object_ptr d, void* c, void* a, drift_win_rng_object_ptr_arr0_ptr out);
NELUA_CEXPORT int32_t WdfDeviceConfigureRequestDispatching(drift_win_rng_object_ptr d, drift_win_rng_object_ptr q, uint32_t t);
NELUA_CEXPORT drift_win_rng_object_ptr WdfDriverWdmGetDriverObject(drift_win_rng_object_ptr d);
NELUA_CEXPORT drift_win_rng_object_ptr WdfIoQueueGetDevice(drift_win_rng_object_ptr q);
NELUA_CEXPORT drift_win_rng_object_ptr WdfInterruptGetDevice(drift_win_rng_object_ptr q);
typedef drift_win_rng_request* drift_win_rng_request_ptr;
NELUA_CEXPORT drift_win_rng_object_ptr WdfRequestGetIoQueue(drift_win_rng_request_ptr r);
NELUA_CEXPORT void* drift_win_rng_get_context(drift_win_rng_object_ptr o);
static bool drift_windows_viorng_request_valid(drift_win_rng_request_ptr r);
static int32_t drift_windows_viorng_finish_request(drift_win_rng_ptr s, drift_win_rng_request_ptr r, int32_t status, uintptr_t info);
typedef drift_win_rng_request_ptr* drift_win_rng_request_ptr_arr0_ptr;
NELUA_CEXPORT void drift_win_rng_assert_fail(void);
NELUA_CEXPORT void WdfSpinLockAcquire(drift_win_rng_object_ptr o);
NELUA_CEXPORT void WdfSpinLockRelease(drift_win_rng_object_ptr o);
NELUA_CEXPORT void WdfInterruptGetInfo(drift_win_rng_object_ptr o, void* p);
NELUA_CEXPORT uint8_t WdfInterruptQueueDpcForIsr(drift_win_rng_object_ptr o);
typedef size_t* nlcsize_ptr;
NELUA_CEXPORT int32_t WdfRequestRetrieveOutputBuffer(drift_win_rng_request_ptr r, size_t minimum, void* result, nlcsize_ptr length);
typedef uint8_t* nluint8_arr0_ptr;
typedef void (*runtime_types_record_47Aim8M22hNeuoKr2_Cancel)(drift_win_rng_request_ptr);
NELUA_CEXPORT int32_t WdfRequestMarkCancelableEx(drift_win_rng_request_ptr r, runtime_types_record_47Aim8M22hNeuoKr2_Cancel f);
NELUA_CEXPORT int32_t WdfRequestUnmarkCancelable(drift_win_rng_request_ptr r);
NELUA_CEXPORT void WdfRequestCompleteWithInformation(drift_win_rng_request_ptr r, int32_t status, uintptr_t n);
NELUA_CEXPORT void WdfRequestComplete(drift_win_rng_request_ptr r, int32_t status);
NELUA_CEXPORT void WdfRequestStopAcknowledge(drift_win_rng_request_ptr r, uint8_t requeue);
NELUA_CEXPORT void* ExAllocatePoolUninitialized(int32_t pool, size_t size, uint32_t tag);
static NELUA_INLINE uint32_t nelua_shl_nluint32(uint32_t a, int32_t b);
NELUA_CEXPORT void ExFreePoolWithTag(void* p, uint32_t tag);
NELUA_CEXPORT void RtlCopyMemory(void* dst, void* src, size_t length);
typedef drift_dma* drift_dma_ptr;
typedef VIRTIO_WDF_DRIVER* VIRTIO_WDF_DRIVER_ptr;
static drift_win_rng_ptr drift_windows_viorng_vdriver(VIRTIO_WDF_DRIVER_ptr v);
NELUA_CEXPORT int32_t VirtIOWdfInitialize(VIRTIO_WDF_DRIVER_ptr v, drift_win_rng_object_ptr d, drift_win_rng_object_ptr resources, drift_win_rng_object_ptr interrupt, uint32_t tag);
typedef drift_rng_virtqueue* drift_rng_virtqueue_ptr;
typedef drift_rng_virtqueue_ptr* drift_rng_virtqueue_ptr_arr0_ptr;
NELUA_CEXPORT int32_t VirtIOWdfInitQueues(VIRTIO_WDF_DRIVER_ptr v, uint32_t n, drift_rng_virtqueue_ptr_arr0_ptr out, void* param);
NELUA_CEXPORT void VirtIOWdfSetDriverOK(VIRTIO_WDF_DRIVER_ptr v);
NELUA_CEXPORT void VirtIOWdfSetDriverFailed(VIRTIO_WDF_DRIVER_ptr v);
NELUA_CEXPORT int32_t VirtIOWdfDestroyQueues(VIRTIO_WDF_DRIVER_ptr v);
NELUA_CEXPORT int32_t VirtIOWdfShutdown(VIRTIO_WDF_DRIVER_ptr v);
typedef VirtIODevice* VirtIODevice_ptr;
NELUA_CEXPORT void* VirtIOWdfDeviceAllocDmaMemory(VirtIODevice_ptr v, size_t n, uint32_t tag);
NELUA_CEXPORT PHYSICAL_ADDRESS VirtIOWdfDeviceGetPhysicalAddress(VirtIODevice_ptr v, void* p);
NELUA_CEXPORT void VirtIOWdfDeviceFreeDmaMemory(VirtIODevice_ptr v, void* p);
static drift_win_rng_ptr drift_windows_viorng_queue_owner(drift_rng_virtqueue_ptr q);
typedef drift_rng_sg* drift_rng_sg_ptr;
NELUA_CEXPORT int virtqueue_add_buf(drift_rng_virtqueue_ptr q, drift_rng_sg_ptr sg, uint32_t out, uint32_t inside, void* cookie, void* indirect, uint64_t address);
NELUA_CEXPORT void virtqueue_kick(drift_rng_virtqueue_ptr q);
typedef uint32_t* nluint32_arr0_ptr;
NELUA_CEXPORT void* virtqueue_get_buf(drift_rng_virtqueue_ptr q, nluint32_arr0_ptr length);
NELUA_CEXPORT bool virtqueue_enable_cb(drift_rng_virtqueue_ptr q);
NELUA_CEXPORT void virtqueue_disable_cb(drift_rng_virtqueue_ptr q);
NELUA_CEXPORT uint8_t VirtIOWdfGetISRStatus(VIRTIO_WDF_DRIVER_ptr v);
static void drift_windows_viorng_dispatch(drift_win_rng_ptr s);
NELUA_CEXPORT int32_t drift_rng_open(drift_win_rng_ptr s);
NELUA_CEXPORT int32_t drift_rng_submit(drift_win_rng_ptr s, drift_win_rng_request_ptr r, nluint8_arr0_ptr p, size_t capacity, size_t length);
NELUA_CEXPORT int32_t drift_rng_pump(drift_win_rng_ptr s);
NELUA_CEXPORT int32_t drift_rng_cancel(drift_win_rng_ptr s, drift_win_rng_request_ptr r);
NELUA_CEXPORT int32_t drift_rng_drain(drift_win_rng_ptr s);
NELUA_CEXPORT int32_t drift_windows_viorng_request_reset(drift_win_rng_request_ptr r);
NELUA_CEXPORT int32_t drift_rng_close(drift_win_rng_ptr s);
/* ------------------------------ DEFINITIONS ------------------------------- */
drift_win_rng_ptr runtime_types_record_47Aim8M22hNeuoKr2_owner(drift_win_rng_object_ptr o, uint32_t k) {
  if((((o == ((drift_win_rng_object_ptr)NULL)) || (o->owner == ((drift_win_rng_ptr)NULL))) || (o->kind != k))) {
    return ((drift_win_rng_ptr)NULL);
  }
  if(((o != drift_rng_object(o->owner, k)) || (o->owner->phase == 0))) {
    return ((drift_win_rng_ptr)NULL);
  }
  return o->owner;
}
void runtime_types_record_47Aim8M22hNeuoKr2_fail(drift_win_rng_ptr s, int32_t status) {
  if((s != ((drift_win_rng_ptr)NULL))) {
    if((s->error == 0)) {
      s->error = status;
    }
    s->last_error = status;
  }
}
int32_t runtime_types_record_47Aim8M22hNeuoKr2_native_error(int32_t n) {
  if((n == -2)) {
    return runtime_types_record_47Aim8M22hNeuoKr2_TIMEOUT;
  }
  if((n == -6)) {
    return runtime_types_record_47Aim8M22hNeuoKr2_OOM;
  }
  if((n == -4)) {
    return runtime_types_record_47Aim8M22hNeuoKr2_UNSUPPORTED;
  }
  if(((n == -1) || (n == -3))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  return runtime_types_record_47Aim8M22hNeuoKr2_IOERROR;
}
int32_t WdfDriverCreate(drift_win_rng_object_ptr o, void* registry, void* attrs, void* c, drift_win_rng_object_ptr_arr0_ptr result) {
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(o, 7U);
  if(((((s == ((drift_win_rng_ptr)NULL)) || (s->phase != 1)) || (s->configured != 0)) || (drift_rng_driver_meta(s, c, attrs) == 0))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  s->configured = 1U;
  if((result != ((drift_win_rng_object_ptr_arr0_ptr)NULL))) {
    result[0] = drift_rng_object(s, 1U);
  }
  return 0;
}
void WdfDeviceInitSetPnpPowerEventCallbacks(drift_win_rng_object_ptr o, void* c) {
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(o, 6U);
  if((s == ((drift_win_rng_ptr)NULL))) {
    return;
  }
  if(((s->configured != 1) || (drift_rng_power_meta(s, c) == 0))) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_INVALID);
    return;
  }
  s->configured = 5U;
}
void WdfDeviceInitSetIoType(drift_win_rng_object_ptr o, uint32_t t) {
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(o, 6U);
  if((s == ((drift_win_rng_ptr)NULL))) {
    return;
  }
  if(((s->configured != 5) || (t != 3))) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_INVALID);
    return;
  }
  drift_rng_set_iotype(s, t);
}
int32_t WdfDeviceCreate(drift_win_rng_object_ptr_arr0_ptr i, void* a, drift_win_rng_object_ptr_arr0_ptr out) {
  if(((i == ((drift_win_rng_object_ptr_arr0_ptr)NULL)) || (out == ((drift_win_rng_object_ptr_arr0_ptr)NULL)))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(i[0], 6U);
  if((((s == ((drift_win_rng_ptr)NULL)) || (s->configured != 5)) || (drift_rng_device_meta(s, a) == 0))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  drift_rng_device_result(s, i, out);
  s->configured = 7U;
  return 0;
}
int32_t WdfInterruptCreate(drift_win_rng_object_ptr d, void* c, void* a, drift_win_rng_object_ptr_arr0_ptr out) {
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(d, 2U);
  if((((((s == ((drift_win_rng_ptr)NULL)) || (s->configured != 7)) || (a != (void*)NULL)) || (out == ((drift_win_rng_object_ptr_arr0_ptr)NULL))) || (drift_rng_interrupt_meta(s, c) == 0))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  s->configured = 15U;
  out[0] = drift_rng_object(s, 4U);
  return 0;
}
int32_t WdfSpinLockCreate(void* a, drift_win_rng_object_ptr_arr0_ptr out) {
  drift_win_rng_ptr s = drift_win_rng_current();
  if(((((s == ((drift_win_rng_ptr)NULL)) || (s->configured != 15)) || (out == ((drift_win_rng_object_ptr_arr0_ptr)NULL))) || (drift_rng_lock_meta(s, a) == 0))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  s->configured = 31U;
  out[0] = drift_rng_object(s, 5U);
  return 0;
}
int32_t WdfDeviceCreateDeviceInterface(drift_win_rng_object_ptr d, void* g, void* r) {
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(d, 2U);
  if(((((s == ((drift_win_rng_ptr)NULL)) || (s->configured != 31)) || (r != (void*)NULL)) || (drift_rng_interface_meta(s, g) == 0))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  s->configured = 63U;
  return 0;
}
int32_t WdfIoQueueCreate(drift_win_rng_object_ptr d, void* c, void* a, drift_win_rng_object_ptr_arr0_ptr out) {
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(d, 2U);
  if((((((s == ((drift_win_rng_ptr)NULL)) || (s->configured != 63)) || (a != (void*)NULL)) || (out == ((drift_win_rng_object_ptr_arr0_ptr)NULL))) || (drift_rng_queue_meta(s, c) == 0))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  s->configured = 127U;
  out[0] = drift_rng_object(s, 3U);
  return 0;
}
int32_t WdfDeviceConfigureRequestDispatching(drift_win_rng_object_ptr d, drift_win_rng_object_ptr q, uint32_t t) {
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(d, 2U);
  if(((((s == ((drift_win_rng_ptr)NULL)) || (s->configured != 127)) || (q != drift_rng_object(s, 3U))) || (t != 3))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  s->configured = 255U;
  return 0;
}
drift_win_rng_object_ptr WdfDriverWdmGetDriverObject(drift_win_rng_object_ptr d) {
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(d, 1U);
  if((s == ((drift_win_rng_ptr)NULL))) {
    return ((drift_win_rng_object_ptr)NULL);
  }
  return drift_rng_object(s, 7U);
}
drift_win_rng_object_ptr WdfIoQueueGetDevice(drift_win_rng_object_ptr q) {
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(q, 3U);
  if((s == ((drift_win_rng_ptr)NULL))) {
    return ((drift_win_rng_object_ptr)NULL);
  }
  return drift_rng_object(s, 2U);
}
drift_win_rng_object_ptr WdfInterruptGetDevice(drift_win_rng_object_ptr q) {
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(q, 4U);
  if((s == ((drift_win_rng_ptr)NULL))) {
    return ((drift_win_rng_object_ptr)NULL);
  }
  return drift_rng_object(s, 2U);
}
drift_win_rng_object_ptr WdfRequestGetIoQueue(drift_win_rng_request_ptr r) {
  if(((((r == ((drift_win_rng_request_ptr)NULL)) || (r->owner == ((drift_win_rng_ptr)NULL))) || (r->phase != 2)) || (r->owner->active != (void*)r))) {
    return ((drift_win_rng_object_ptr)NULL);
  }
  return drift_rng_object(r->owner, 3U);
}
void* drift_win_rng_get_context(drift_win_rng_object_ptr o) {
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(o, 2U);
  if(((s == ((drift_win_rng_ptr)NULL)) || ((s->configured & 2) == 0))) {
    return (void*)NULL;
  }
  return drift_rng_context(s);
}
bool drift_windows_viorng_request_valid(drift_win_rng_request_ptr r) {
  return (((((r != ((drift_win_rng_request_ptr)NULL)) && (r->owner != ((drift_win_rng_ptr)NULL))) && (r->phase == 2)) && (r->owner->active == (void*)r)) && (r->owner->phase != 0));
}
int32_t drift_windows_viorng_finish_request(drift_win_rng_ptr s, drift_win_rng_request_ptr r, int32_t status, uintptr_t info) {
  if(((((r == ((drift_win_rng_request_ptr)NULL)) || (r->owner != s)) || ((r->phase != 1) && (r->phase != 2))) || (r->completions != 0))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_STATE;
  }
  if(((status == runtime_types_record_47Aim8M22hNeuoKr2_PENDING) || (info > r->capacity))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  if((s->notify_count >= 16)) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_STATE);
    return runtime_types_record_47Aim8M22hNeuoKr2_STATE;
  }
  if(((s->error != 0) && (status >= 0))) {
    status = s->error;
    info = 0U;
  }
  r->phase = 3U;
  r->status = status;
  r->information = info;
  r->completions = 1U;
  r->cancelable = 0U;
  if((s->active == (void*)r)) {
    s->active = (void*)NULL;
  }
  drift_win_rng_request_ptr_arr0_ptr list = drift_rng_notifications(s);
  list[s->notify_count] = r;
  s->notify_count = (s->notify_count + 1);
  return 0;
}
void drift_win_rng_assert_fail(void) {
  runtime_types_record_47Aim8M22hNeuoKr2_fail(drift_win_rng_current(), runtime_types_record_47Aim8M22hNeuoKr2_STATE);
}
void WdfSpinLockAcquire(drift_win_rng_object_ptr o) {
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(o, 5U);
  if((s == ((drift_win_rng_ptr)NULL))) {
    return;
  }
  if((s->locked != 0)) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_STATE);
    return;
  }
  s->locked = 1U;
}
void WdfSpinLockRelease(drift_win_rng_object_ptr o) {
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(o, 5U);
  if((s == ((drift_win_rng_ptr)NULL))) {
    return;
  }
  if((s->locked != 1)) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_STATE);
    return;
  }
  s->locked = 0U;
}
void WdfInterruptGetInfo(drift_win_rng_object_ptr o, void* p) {
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(o, 4U);
  if((s == ((drift_win_rng_ptr)NULL))) {
    return;
  }
  if((drift_rng_irq_info_valid(p) == 0)) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_INVALID);
    return;
  }
  drift_rng_irq_info(p);
}
uint8_t WdfInterruptQueueDpcForIsr(drift_win_rng_object_ptr o) {
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(o, 4U);
  if(((s == ((drift_win_rng_ptr)NULL)) || (s->dpc != 0))) {
    return 0U;
  }
  if((s->latch_ready == 0)) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_STATE);
    return 0U;
  }
  s->dpc = 1U;
  return 1U;
}
int32_t WdfRequestRetrieveOutputBuffer(drift_win_rng_request_ptr r, size_t minimum, void* result, nlcsize_ptr length) {
  if(((!drift_windows_viorng_request_valid(r)) || (result == (void*)NULL))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_STATE;
  }
  if((r->capacity < minimum)) {
    return runtime_types_record_47Aim8M22hNeuoKr2_SMALL;
  }
  if((r->output == ((nluint8_arr0_ptr)NULL))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  drift_rng_output(r, result, length);
  return 0;
}
int32_t WdfRequestMarkCancelableEx(drift_win_rng_request_ptr r, runtime_types_record_47Aim8M22hNeuoKr2_Cancel f) {
  if((((!drift_windows_viorng_request_valid(r)) || (r->cancelable != 0)) || (f == ((runtime_types_record_47Aim8M22hNeuoKr2_Cancel)NULL)))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_STATE;
  }
  if((r->cancel_owned != 0)) {
    return runtime_types_record_47Aim8M22hNeuoKr2_CANCELLED;
  }
  drift_rng_set_cancel(r, f);
  r->cancelable = 1U;
  return 0;
}
int32_t WdfRequestUnmarkCancelable(drift_win_rng_request_ptr r) {
  if((!drift_windows_viorng_request_valid(r))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_STATE;
  }
  if((r->cancel_owned != 0)) {
    return runtime_types_record_47Aim8M22hNeuoKr2_CANCELLED;
  }
  if((r->cancelable == 0)) {
    return runtime_types_record_47Aim8M22hNeuoKr2_STATE;
  }
  r->cancelable = 0U;
  return 0;
}
void WdfRequestCompleteWithInformation(drift_win_rng_request_ptr r, int32_t status, uintptr_t n) {
  if((!drift_windows_viorng_request_valid(r))) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(drift_win_rng_current(), runtime_types_record_47Aim8M22hNeuoKr2_STATE);
    return;
  }
  int32_t result = drift_windows_viorng_finish_request(r->owner, r, status, n);
  if((result != 0)) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(r->owner, result);
  }
}
void WdfRequestComplete(drift_win_rng_request_ptr r, int32_t status) {
  WdfRequestCompleteWithInformation(r, status, 0U);
}
void WdfRequestStopAcknowledge(drift_win_rng_request_ptr r, uint8_t requeue) {
  if((!drift_windows_viorng_request_valid(r))) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(drift_win_rng_current(), runtime_types_record_47Aim8M22hNeuoKr2_STATE);
    return;
  }
  if((requeue != 0)) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(r->owner, runtime_types_record_47Aim8M22hNeuoKr2_UNSUPPORTED);
    return;
  }
  if((r->cancel_owned == 0)) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(r->owner, runtime_types_record_47Aim8M22hNeuoKr2_STATE);
  }
}
uint32_t nelua_shl_nluint32(uint32_t a, int32_t b) {
  if(NELUA_LIKELY(b >= 0 && b < 32)) {
    return ((uint32_t)a) << b;
  } else if(NELUA_UNLIKELY(b < 0 && b > -32)) {
    return (uint32_t)a >> -b;
  } else {
    return 0;
  }
}
void* ExAllocatePoolUninitialized(int32_t pool, size_t size, uint32_t tag) {
  drift_win_rng_ptr s = drift_win_rng_current();
  if(((((s == ((drift_win_rng_ptr)NULL)) || (pool != 0)) || (size != drift_rng_entry_size())) || (drift_rng_tag_valid(tag) == 0))) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_INVALID);
    return (void*)NULL;
  }
  for(uint32_t i = 0U; i < 16U; i += 1) {
    uint32_t bit = nelua_shl_nluint32(1U, i);
    if(((s->pool_used & bit) == 0)) {
      s->pool_used = (s->pool_used | bit);
      drift_rng_entry_clear(s, i);
      return drift_rng_entry(s, i);
    }
  }
  return (void*)NULL;
}
void ExFreePoolWithTag(void* p, uint32_t tag) {
  drift_win_rng_ptr s = drift_win_rng_current();
  if((s == ((drift_win_rng_ptr)NULL))) {
    return;
  }
  int32_t index = drift_rng_entry_index(s, p);
  if(((index < 0) || (drift_rng_tag_valid(tag) == 0))) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_INVALID);
    return;
  }
  uint32_t bit = nelua_shl_nluint32(1U, (uint32_t)index);
  if(((s->pool_used & bit) == 0)) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_STATE);
    return;
  }
  if(((drift_rng_dma_active(s) != 0) && (drift_rng_dma_cookie(s) == p))) {
    s->pool_quarantined = (s->pool_quarantined | bit);
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_STATE);
    return;
  }
  s->pool_used = (s->pool_used & (~bit));
}
void RtlCopyMemory(void* dst, void* src, size_t length) {
  drift_win_rng_ptr s = drift_win_rng_current();
  if((s == ((drift_win_rng_ptr)NULL))) {
    return;
  }
  drift_win_rng_request_ptr r = ((drift_win_rng_request_ptr)s->active);
  drift_dma_ptr m = drift_rng_dma(s);
  if((((((((!drift_windows_viorng_request_valid(r)) || (dst != (void*)r->output)) || (src != (void*)m->data)) || (length > r->capacity)) || (length > 4096)) || (m->data == ((nluint8_arr0_ptr)NULL))) || (drift_rng_dma_active(s) != 0))) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_STATE);
    return;
  }
  nluint8_arr0_ptr to = ((nluint8_arr0_ptr)dst);
  nluint8_arr0_ptr from = ((nluint8_arr0_ptr)src);
  for(size_t i = 0U, _end = length; i < _end; i += 1) {
    to[i] = from[i];
  }
}
drift_win_rng_ptr drift_windows_viorng_vdriver(VIRTIO_WDF_DRIVER_ptr v) {
  if((((v == ((VIRTIO_WDF_DRIVER_ptr)NULL)) || (v->owner == ((drift_win_rng_ptr)NULL))) || (v->owner->phase == 0))) {
    return ((drift_win_rng_ptr)NULL);
  }
  return v->owner;
}
int32_t VirtIOWdfInitialize(VIRTIO_WDF_DRIVER_ptr v, drift_win_rng_object_ptr d, drift_win_rng_object_ptr resources, drift_win_rng_object_ptr interrupt, uint32_t tag) {
  drift_win_rng_ptr s = runtime_types_record_47Aim8M22hNeuoKr2_owner(d, 2U);
  if(((((s == ((drift_win_rng_ptr)NULL)) || (resources != drift_rng_object(s, 6U))) || (interrupt != ((drift_win_rng_object_ptr)NULL))) || (drift_rng_tag_valid(tag) == 0))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  drift_rng_bind_vdevice(v, s);
  int32_t n = drift_rng_native_open(s);
  if((n < 0)) {
    int32_t e = runtime_types_record_47Aim8M22hNeuoKr2_native_error(n);
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, e);
    return e;
  }
  return 0;
}
int32_t VirtIOWdfInitQueues(VIRTIO_WDF_DRIVER_ptr v, uint32_t n, drift_rng_virtqueue_ptr_arr0_ptr out, void* param) {
  drift_win_rng_ptr s = drift_windows_viorng_vdriver(v);
  if((((((s == ((drift_win_rng_ptr)NULL)) || (n != 1)) || (out == ((drift_rng_virtqueue_ptr_arr0_ptr)NULL))) || (drift_rng_queue_param(s, param) == 0)) || (drift_rng_native_state(s) != 4))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  drift_rng_init_vq(s, out);
  return 0;
}
void VirtIOWdfSetDriverOK(VIRTIO_WDF_DRIVER_ptr v) {
  drift_win_rng_ptr s = drift_windows_viorng_vdriver(v);
  if((s == ((drift_win_rng_ptr)NULL))) {
    return;
  }
  int32_t n = drift_rng_native_ready(s);
  if((n < 0)) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_native_error(n));
  }
}
void VirtIOWdfSetDriverFailed(VIRTIO_WDF_DRIVER_ptr v) {
  runtime_types_record_47Aim8M22hNeuoKr2_fail(drift_windows_viorng_vdriver(v), runtime_types_record_47Aim8M22hNeuoKr2_IOERROR);
}
int32_t VirtIOWdfDestroyQueues(VIRTIO_WDF_DRIVER_ptr v) {
  drift_win_rng_ptr s = drift_windows_viorng_vdriver(v);
  if((s == ((drift_win_rng_ptr)NULL))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  int32_t n = drift_rng_native_close(s);
  if((n < 0)) {
    int32_t e = runtime_types_record_47Aim8M22hNeuoKr2_native_error(n);
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, e);
    return e;
  }
  s->latch_ready = 0U;
  s->latch_cookie = (void*)NULL;
  s->latch_length = 0U;
  s->irq_latch = 0U;
  s->dpc = 0U;
  return 0;
}
int32_t VirtIOWdfShutdown(VIRTIO_WDF_DRIVER_ptr v) {
  return VirtIOWdfDestroyQueues(v);
}
void* VirtIOWdfDeviceAllocDmaMemory(VirtIODevice_ptr v, size_t n, uint32_t tag) {
  if(((v == ((VirtIODevice_ptr)NULL)) || (v->owner == ((drift_win_rng_ptr)NULL)))) {
    return (void*)NULL;
  }
  drift_win_rng_ptr s = v->owner;
  drift_dma_ptr m = drift_rng_dma(s);
  if((((n != 4096) || (tag != 0)) || (m->data != ((nluint8_arr0_ptr)NULL)))) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_INVALID);
    return (void*)NULL;
  }
  if((drift_rng_alloc_dma(s) != 0)) {
    s->last_error = runtime_types_record_47Aim8M22hNeuoKr2_OOM;
    return (void*)NULL;
  }
  if(((((m->data == ((nluint8_arr0_ptr)NULL)) || (m->size < 4096)) || (m->address == 0)) || ((m->address & 4095) != 0))) {
    if((m->data != ((nluint8_arr0_ptr)NULL))) {
      drift_rng_free_dma(s);
    }
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_IOERROR);
    return (void*)NULL;
  }
  return (void*)m->data;
}
PHYSICAL_ADDRESS VirtIOWdfDeviceGetPhysicalAddress(VirtIODevice_ptr v, void* p) {
  PHYSICAL_ADDRESS result = (PHYSICAL_ADDRESS){0};
  if(((v == ((VirtIODevice_ptr)NULL)) || (v->owner == ((drift_win_rng_ptr)NULL)))) {
    return result;
  }
  drift_win_rng_ptr s = v->owner;
  drift_dma_ptr m = drift_rng_dma(s);
  if(((p != (void*)m->data) || (p == (void*)NULL))) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_INVALID);
    return result;
  }
  result.QuadPart = (int64_t)m->address;
  return result;
}
void VirtIOWdfDeviceFreeDmaMemory(VirtIODevice_ptr v, void* p) {
  if(((v == ((VirtIODevice_ptr)NULL)) || (v->owner == ((drift_win_rng_ptr)NULL)))) {
    return;
  }
  drift_win_rng_ptr s = v->owner;
  drift_dma_ptr m = drift_rng_dma(s);
  if(((p != (void*)m->data) || (p == (void*)NULL))) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_INVALID);
    return;
  }
  if(((drift_rng_native_state(s) != 0) || (drift_rng_dma_active(s) != 0))) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_STATE);
    return;
  }
  drift_rng_free_dma(s);
}
drift_win_rng_ptr drift_windows_viorng_queue_owner(drift_rng_virtqueue_ptr q) {
  if(((((q == ((drift_rng_virtqueue_ptr)NULL)) || (q->owner == ((drift_win_rng_ptr)NULL))) || (q->index != 0)) || (q != drift_rng_vq(q->owner)))) {
    return ((drift_win_rng_ptr)NULL);
  }
  return q->owner;
}
int virtqueue_add_buf(drift_rng_virtqueue_ptr q, drift_rng_sg_ptr sg, uint32_t out, uint32_t inside, void* cookie, void* indirect, uint64_t address) {
  drift_win_rng_ptr s = drift_windows_viorng_queue_owner(q);
  if((s == ((drift_win_rng_ptr)NULL))) {
    return -1;
  }
  int32_t index = drift_rng_entry_index(s, cookie);
  if((((((((sg == ((drift_rng_sg_ptr)NULL)) || (out != 0)) || (inside != 1)) || (indirect != (void*)NULL)) || (address != 0)) || (index < 0)) || ((s->pool_used & nelua_shl_nluint32(1U, (uint32_t)index)) == 0))) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_INVALID);
    return -1;
  }
  drift_dma_ptr m = drift_rng_dma(s);
  if((((((uint64_t)sg->physAddr.QuadPart != m->address) || (sg->length == 0)) || (sg->length > 4096)) || (s->locked != 1))) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_INVALID);
    return -1;
  }
  int32_t n = drift_rng_native_add(s, sg->length, cookie);
  if((n < 0)) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_native_error(n));
    return -1;
  }
  return 0;
}
void virtqueue_kick(drift_rng_virtqueue_ptr q) {
  drift_win_rng_ptr s = drift_windows_viorng_queue_owner(q);
  if((s == ((drift_win_rng_ptr)NULL))) {
    return;
  }
  if((drift_rng_dma_active(s) == 0)) {
    return;
  }
  int32_t n = drift_rng_native_kick(s);
  if((n < 0)) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_native_error(n));
  }
}
void* virtqueue_get_buf(drift_rng_virtqueue_ptr q, nluint32_arr0_ptr length) {
  drift_win_rng_ptr s = drift_windows_viorng_queue_owner(q);
  if(((s == ((drift_win_rng_ptr)NULL)) || (length == ((nluint32_arr0_ptr)NULL)))) {
    return (void*)NULL;
  }
  if((s->locked != 1)) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_STATE);
    return (void*)NULL;
  }
  if((s->latch_ready == 0)) {
    return (void*)NULL;
  }
  void* p = s->latch_cookie;
  length[0] = s->latch_length;
  s->latch_ready = 0U;
  s->latch_cookie = (void*)NULL;
  s->latch_length = 0U;
  return p;
}
bool virtqueue_enable_cb(drift_rng_virtqueue_ptr q) {
  drift_win_rng_ptr s = drift_windows_viorng_queue_owner(q);
  if((s == ((drift_win_rng_ptr)NULL))) {
    return false;
  }
  s->callbacks = 1U;
  return (s->latch_ready == 0);
}
void virtqueue_disable_cb(drift_rng_virtqueue_ptr q) {
  drift_win_rng_ptr s = drift_windows_viorng_queue_owner(q);
  if((s != ((drift_win_rng_ptr)NULL))) {
    s->callbacks = 0U;
  }
}
uint8_t VirtIOWdfGetISRStatus(VIRTIO_WDF_DRIVER_ptr v) {
  drift_win_rng_ptr s = drift_windows_viorng_vdriver(v);
  if((s == ((drift_win_rng_ptr)NULL))) {
    return 0U;
  }
  uint32_t n = s->irq_latch;
  s->irq_latch = 0U;
  return (uint8_t)n;
}
void drift_windows_viorng_dispatch(drift_win_rng_ptr s) {
  for(uint32_t iteration = 0U; iteration < 16U; iteration += 1) {
    if(((((((((s->phase != 2) || (s->error != 0)) || (s->locked != 0)) || (s->dispatching != 0)) || (s->active != (void*)NULL)) || (drift_rng_dma_active(s) != 0)) || (s->latch_ready != 0)) || (s->pending_count == 0))) {
      return;
    }
    drift_win_rng_request_ptr_arr0_ptr list = drift_rng_pending(s);
    drift_win_rng_request_ptr r = list[0];
    for(uint32_t i = 1U, _end = s->pending_count; i < _end; i += 1) {
      list[(i - 1)] = list[i];
    }
    s->pending_count = (s->pending_count - 1);
    list[s->pending_count] = ((drift_win_rng_request_ptr)NULL);
    r->phase = 2U;
    s->active = (void*)r;
    s->dispatching = 1U;
    drift_rng_call_read(s, r);
    s->dispatching = 0U;
  }
}
int32_t drift_rng_open(drift_win_rng_ptr s) {
  if((((((s == ((drift_win_rng_ptr)NULL)) || (s->phase != 0)) || (s->depth != 1)) || (drift_rng_host_valid(s->host) == 0)) || (drift_rng_hw_valid(s) == 0))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  if(((drift_rng_native_state(s) != 0) || (drift_rng_dma(s)->data != ((nluint8_arr0_ptr)NULL)))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_BUSY;
  }
  drift_rng_storage_init(s);
  s->phase = 1U;
  s->error = 0;
  s->last_error = 0;
  s->configured = 0U;
  s->prepared = 0U;
  s->powered = 0U;
  s->locked = 0U;
  s->callbacks = 0U;
  s->dpc = 0U;
  s->irq_latch = 0U;
  s->latch_ready = 0U;
  s->latch_cookie = (void*)NULL;
  s->latch_length = 0U;
  s->pool_used = 0U;
  s->pool_quarantined = 0U;
  s->pending_count = 0U;
  s->notify_count = 0U;
  s->dispatching = 0U;
  s->active = (void*)NULL;
  int32_t status = drift_rng_call_entry(s);
  if((status >= 0)) {
    status = drift_rng_call_add(s);
  }
  if(((status >= 0) && (s->configured != 255))) {
    status = runtime_types_record_47Aim8M22hNeuoKr2_STATE;
  }
  if((status >= 0)) {
    s->prepared = 1U;
    status = drift_rng_call_prepare(s);
  }
  if((status >= 0)) {
    s->powered = 1U;
    status = drift_rng_call_power(s, 1);
  }
  if(((status >= 0) && (s->error == 0))) {
    status = drift_rng_call_irq(s, 1);
  }
  if((s->error != 0)) {
    status = s->error;
  }
  if((status < 0)) {
    s->last_error = status;
    return status;
  }
  s->phase = 2U;
  return 0;
}
int32_t drift_rng_submit(drift_win_rng_ptr s, drift_win_rng_request_ptr r, nluint8_arr0_ptr p, size_t capacity, size_t length) {
  if((((s == ((drift_win_rng_ptr)NULL)) || (r == ((drift_win_rng_request_ptr)NULL))) || (s->phase != 2))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_STATE;
  }
  if((s->error != 0)) {
    return s->error;
  }
  if((((r->phase != 0) || (r->owner != ((drift_win_rng_ptr)NULL))) || (r->notifying != 0))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_BUSY;
  }
  if(((length > 1048576) || (capacity > 1048576))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  uint32_t active = 0U;
  if((s->active != (void*)NULL)) {
    active = 1U;
  }
  if((((s->pending_count + s->notify_count) + active) >= 16)) {
    return runtime_types_record_47Aim8M22hNeuoKr2_BUSY;
  }
  r->owner = s;
  r->output = p;
  r->capacity = capacity;
  r->length = length;
  r->phase = 1U;
  r->cancelable = 0U;
  r->cancel_owned = 0U;
  r->notifying = 0U;
  r->completions = 0U;
  r->status = runtime_types_record_47Aim8M22hNeuoKr2_PENDING;
  r->information = 0U;
  if((length == 0)) {
    drift_windows_viorng_finish_request(s, r, 0, 0U);
  } else {
    drift_win_rng_request_ptr_arr0_ptr list = drift_rng_pending(s);
    list[s->pending_count] = r;
    s->pending_count = (s->pending_count + 1);
    drift_windows_viorng_dispatch(s);
  }
  if((r->phase == 3)) {
    return r->status;
  }
  return runtime_types_record_47Aim8M22hNeuoKr2_PENDING;
}
int32_t drift_rng_pump(drift_win_rng_ptr s) {
  if((((s == ((drift_win_rng_ptr)NULL)) || (s->phase != 2)) || (s->locked != 0))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_STATE;
  }
  if((s->error != 0)) {
    return s->error;
  }
  drift_windows_viorng_dispatch(s);
  if((s->error != 0)) {
    return s->error;
  }
  if(((drift_rng_dma_active(s) != 0) && (s->latch_ready == 0))) {
    void* cookie = (void*)NULL;
    uint32_t n = 0U;
    int32_t result = drift_rng_native_get(s, (&cookie), (&n));
    if((result < 0)) {
      int32_t e = runtime_types_record_47Aim8M22hNeuoKr2_native_error(result);
      runtime_types_record_47Aim8M22hNeuoKr2_fail(s, e);
      return e;
    }
    if((result == 1)) {
      if(((drift_rng_entry_index(s, cookie) < 0) || (n > 4096))) {
        runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_IOERROR);
        return s->error;
      }
      s->latch_cookie = cookie;
      s->latch_length = n;
      s->latch_ready = 1U;
    }
  }
  if(((s->latch_ready != 0) && (s->callbacks != 0))) {
    s->irq_latch = 1U;
    drift_rng_call_isr(s);
  }
  if((s->dpc != 0)) {
    s->dpc = 0U;
    drift_rng_call_dpc(s);
  }
  if((s->locked != 0)) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_STATE);
  }
  drift_windows_viorng_dispatch(s);
  return s->error;
}
int32_t drift_rng_cancel(drift_win_rng_ptr s, drift_win_rng_request_ptr r) {
  if(((((s == ((drift_win_rng_ptr)NULL)) || (r == ((drift_win_rng_request_ptr)NULL))) || (s->phase != 2)) || (r->owner != s))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_STATE;
  }
  if((r->phase == 1)) {
    drift_win_rng_request_ptr_arr0_ptr list = drift_rng_pending(s);
    bool found = false;
    for(uint32_t i = 0U, _end = s->pending_count; i < _end; i += 1) {
      if((list[i] == r)) {
        for(uint32_t j = (i + 1), _end = s->pending_count; j < _end; j += 1) {
          list[(j - 1)] = list[j];
        }
        s->pending_count = (s->pending_count - 1);
        list[s->pending_count] = ((drift_win_rng_request_ptr)NULL);
        found = true;
        break;
      }
    }
    if((!found)) {
      return runtime_types_record_47Aim8M22hNeuoKr2_STATE;
    }
    return drift_windows_viorng_finish_request(s, r, runtime_types_record_47Aim8M22hNeuoKr2_CANCELLED, 0U);
  }
  if(((!drift_windows_viorng_request_valid(r)) || (r->cancelable == 0))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_STATE;
  }
  r->cancel_owned = 1U;
  drift_rng_call_cancel(r);
  if((r->phase != 3)) {
    runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_STATE);
    return runtime_types_record_47Aim8M22hNeuoKr2_STATE;
  }
  return s->error;
}
int32_t drift_rng_drain(drift_win_rng_ptr s) {
  if(((s == ((drift_win_rng_ptr)NULL)) || (s->locked != 0))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_STATE;
  }
  int32_t count = 0;
  drift_win_rng_request_ptr_arr0_ptr list = drift_rng_notifications(s);
  for(uint32_t iteration = 0U; iteration < 16U; iteration += 1) {
    if((s->notify_count == 0)) {
      break;
    }
    drift_win_rng_request_ptr r = list[0];
    for(uint32_t i = 1U, _end = s->notify_count; i < _end; i += 1) {
      list[(i - 1)] = list[i];
    }
    s->notify_count = (s->notify_count - 1);
    list[s->notify_count] = ((drift_win_rng_request_ptr)NULL);
    r->notifying = 1U;
    drift_rng_notify(s, r);
    r->notifying = 0U;
    count = (count + 1);
  }
  return count;
}
int32_t drift_windows_viorng_request_reset(drift_win_rng_request_ptr r) {
  if((r == ((drift_win_rng_request_ptr)NULL))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  if(((r->phase != 3) || (r->notifying != 0))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_BUSY;
  }
  r->owner = ((drift_win_rng_ptr)NULL);
  r->output = ((nluint8_arr0_ptr)NULL);
  r->capacity = 0U;
  r->length = 0U;
  r->phase = 0U;
  r->cancelable = 0U;
  r->cancel_owned = 0U;
  r->completions = 0U;
  r->status = 0;
  r->information = 0U;
  return 0;
}
int32_t drift_rng_close(drift_win_rng_ptr s) {
  if((s == ((drift_win_rng_ptr)NULL))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_INVALID;
  }
  if(((s->depth != 1) || (s->locked != 0))) {
    return runtime_types_record_47Aim8M22hNeuoKr2_BUSY;
  }
  if((s->phase == 0)) {
    return 0;
  }
  s->phase = 3U;
  s->error = 0;
  drift_win_rng_request_ptr_arr0_ptr list = drift_rng_pending(s);
  while((s->pending_count > 0)) {
    drift_win_rng_request_ptr r = list[(s->pending_count - 1)];
    s->pending_count = (s->pending_count - 1);
    list[s->pending_count] = ((drift_win_rng_request_ptr)NULL);
    drift_windows_viorng_finish_request(s, r, runtime_types_record_47Aim8M22hNeuoKr2_CANCELLED, 0U);
  }
  drift_win_rng_request_ptr active = ((drift_win_rng_request_ptr)s->active);
  if((active != ((drift_win_rng_request_ptr)NULL))) {
    drift_rng_call_stop(s, active);
  }
  if((s->callbacks != 0)) {
    drift_rng_call_irq(s, 0);
  }
  if((s->powered != 0)) {
    drift_rng_call_power(s, 0);
  } else {
    int32_t n = drift_rng_native_close(s);
    if((n < 0)) {
      runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_native_error(n));
    }
  }
  if(((drift_rng_native_state(s) != 0) || (drift_rng_dma_active(s) != 0))) {
    if((s->error == 0)) {
      runtime_types_record_47Aim8M22hNeuoKr2_fail(s, runtime_types_record_47Aim8M22hNeuoKr2_STATE);
    }
    s->phase = 4U;
    return s->error;
  }
  s->powered = 0U;
  if((s->prepared != 0)) {
    drift_rng_call_release(s);
    s->prepared = 0U;
  }
  if(((s->configured & 2) != 0)) {
    drift_rng_cleanup_device(s);
  }
  if(((s->configured & 1) != 0)) {
    drift_rng_cleanup_driver(s);
  }
  if((drift_rng_dma(s)->data != ((nluint8_arr0_ptr)NULL))) {
    drift_rng_free_dma(s);
  }
  s->pool_used = 0U;
  s->pool_quarantined = 0U;
  s->latch_ready = 0U;
  s->latch_cookie = (void*)NULL;
  s->dpc = 0U;
  s->irq_latch = 0U;
  s->active = (void*)NULL;
  s->callbacks = 0U;
  if((s->error != 0)) {
    s->phase = 4U;
    return s->error;
  }
  s->phase = 0U;
  return 0;
}

