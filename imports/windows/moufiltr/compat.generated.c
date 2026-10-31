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
#include "wdf_compat.h"
#include <stddef.h>
#include <stdbool.h>
/* Macro used to export C functions. */
#ifdef _WIN32
  #define NELUA_CEXPORT __declspec(dllexport)
#elif defined(__GNUC__)
  #define NELUA_CEXPORT __attribute__((visibility("default")))
#else
  #define NELUA_CEXPORT
#endif
/* ------------------------------ DECLARATIONS ------------------------------ */
static int32_t drift_windows_moufiltr_INVALID = -1073741811;
static int32_t drift_windows_moufiltr_INVALID_STATE = -1073741436;
static int32_t drift_windows_moufiltr_INVALID_HANDLE = -1073741816;
static int32_t drift_windows_moufiltr_BUSY = -2147483631;
static int32_t drift_windows_moufiltr_TOO_SMALL = -1073741789;
static int32_t drift_windows_moufiltr_PENDING = 0x103;
typedef drift_win_moufiltr* drift_win_moufiltr_ptr;
typedef drift_win_mou_object* drift_win_mou_object_ptr;
static drift_win_moufiltr_ptr drift_windows_moufiltr_owner(drift_win_mou_object_ptr o, uint32_t k);
typedef drift_win_mou_request* drift_win_mou_request_ptr;
static bool drift_windows_moufiltr_valid(drift_win_mou_request_ptr r);
static int32_t drift_windows_moufiltr_finish_request(drift_win_mou_request_ptr r, int32_t status, uintptr_t information);
typedef drift_win_mou_host* drift_win_mou_host_ptr;
NELUA_CEXPORT int32_t drift_mou_prepare(drift_win_moufiltr_ptr s, drift_win_mou_host_ptr h);
NELUA_CEXPORT int32_t drift_mou_open_finish(drift_win_moufiltr_ptr s, int32_t status);
typedef drift_win_mou_object_ptr* drift_win_mou_object_ptr_arr0_ptr;
NELUA_CEXPORT int32_t WdfDriverCreate(drift_win_mou_object_ptr o, void* registry, void* attrs, void* config, drift_win_mou_object_ptr_arr0_ptr result);
NELUA_CEXPORT void WdfFdoInitSetFilter(drift_win_mou_object_ptr i);
NELUA_CEXPORT void WdfDeviceInitSetDeviceType(drift_win_mou_object_ptr i, uint32_t t);
NELUA_CEXPORT int32_t WdfDeviceCreate(drift_win_mou_object_ptr_arr0_ptr i, void* attrs, drift_win_mou_object_ptr_arr0_ptr result);
NELUA_CEXPORT int32_t WdfIoQueueCreate(drift_win_mou_object_ptr d, void* config, void* attrs, drift_win_mou_object_ptr_arr0_ptr result);
NELUA_CEXPORT drift_win_mou_object_ptr WdfIoQueueGetDevice(drift_win_mou_object_ptr q);
NELUA_CEXPORT drift_win_mou_object_ptr WdfDeviceGetIoTarget(drift_win_mou_object_ptr d);
NELUA_CEXPORT drift_win_mou_object_ptr WdfDeviceWdmGetDeviceObject(drift_win_mou_object_ptr d);
NELUA_CEXPORT drift_win_mou_object_ptr WdfWdmDeviceGetWdfDeviceHandle(drift_win_mou_object_ptr d);
NELUA_CEXPORT void* drift_wdf_context(drift_win_mou_object_ptr d);
typedef size_t* nlcsize_ptr;
NELUA_CEXPORT int32_t WdfRequestRetrieveInputBuffer(drift_win_mou_request_ptr r, size_t minimum, void* result, nlcsize_ptr length);
typedef void** nlpointer_ptr;
NELUA_CEXPORT int32_t WdfRequestGetStatus(drift_win_mou_request_ptr r);
NELUA_CEXPORT void WdfRequestComplete(drift_win_mou_request_ptr r, int32_t status);
NELUA_CEXPORT uint8_t WdfRequestSend(drift_win_mou_request_ptr r, drift_win_mou_object_ptr target, void* options);
NELUA_CEXPORT int32_t drift_mou_begin(drift_win_moufiltr_ptr s, drift_win_mou_request_ptr r, uint32_t code, void* input, size_t length);
NELUA_CEXPORT int32_t drift_mou_end(drift_win_mou_request_ptr r);
NELUA_CEXPORT int32_t drift_windows_moufiltr_complete(drift_win_mou_request_ptr r, int32_t status, uintptr_t information);
NELUA_CEXPORT int32_t drift_windows_moufiltr_request_reset(drift_win_mou_request_ptr r);
NELUA_CEXPORT int32_t drift_windows_moufiltr_close(drift_win_moufiltr_ptr s);
/* ------------------------------ DEFINITIONS ------------------------------- */
drift_win_moufiltr_ptr drift_windows_moufiltr_owner(drift_win_mou_object_ptr o, uint32_t k) {
  if((((o == ((drift_win_mou_object_ptr)NULL)) || (o->owner == ((drift_win_moufiltr_ptr)NULL))) || (o->kind != k))) {
    return ((drift_win_moufiltr_ptr)NULL);
  }
  if((o != drift_mou_object(o->owner, k))) {
    return ((drift_win_moufiltr_ptr)NULL);
  }
  return o->owner;
}
bool drift_windows_moufiltr_valid(drift_win_mou_request_ptr r) {
  return (((r != ((drift_win_mou_request_ptr)NULL)) && (r->owner != ((drift_win_moufiltr_ptr)NULL))) && (r->owner->phase == 4));
}
int32_t drift_windows_moufiltr_finish_request(drift_win_mou_request_ptr r, int32_t status, uintptr_t information) {
  if((((!drift_windows_moufiltr_valid(r)) || ((r->phase != 1) && (r->phase != 2))) || (r->completions != 0))) {
    return drift_windows_moufiltr_INVALID_STATE;
  }
  if((status == drift_windows_moufiltr_PENDING)) {
    return drift_windows_moufiltr_INVALID;
  }
  drift_win_moufiltr_ptr s = r->owner;
  if((s->outstanding == 0)) {
    return drift_windows_moufiltr_INVALID_STATE;
  }
  r->phase = 3U;
  r->status = status;
  r->information = information;
  r->completions = 1U;
  s->outstanding = (s->outstanding - 1);
  s->depth = (s->depth + 1);
  r->dispatching = (r->dispatching + 1);
  drift_mou_notify(s, r);
  r->dispatching = (r->dispatching - 1);
  s->depth = (s->depth - 1);
  return 0;
}
int32_t drift_mou_prepare(drift_win_moufiltr_ptr s, drift_win_mou_host_ptr h) {
  if(((s == ((drift_win_moufiltr_ptr)NULL)) || (drift_mou_host_valid(h) == 0))) {
    return drift_windows_moufiltr_INVALID;
  }
  if((((s->phase != 0) || (s->outstanding != 0)) || (s->depth != 0))) {
    return drift_windows_moufiltr_BUSY;
  }
  s->host = h;
  s->phase = 1U;
  s->filter = 0U;
  s->device_type = 0U;
  s->error = 0;
  drift_mou_object_init(s);
  return 0;
}
int32_t drift_mou_open_finish(drift_win_moufiltr_ptr s, int32_t status) {
  if((s->error != 0)) {
    status = s->error;
  }
  if(((status >= 0) && (s->phase != 4))) {
    status = drift_windows_moufiltr_INVALID_STATE;
  }
  if((status < 0)) {
    s->phase = 0U;
    s->host = ((drift_win_mou_host_ptr)NULL);
    drift_mou_object_init(s);
  }
  return status;
}
int32_t WdfDriverCreate(drift_win_mou_object_ptr o, void* registry, void* attrs, void* config, drift_win_mou_object_ptr_arr0_ptr result) {
  drift_win_moufiltr_ptr s = drift_windows_moufiltr_owner(o, 6U);
  if(((((s == ((drift_win_moufiltr_ptr)NULL)) || (s->phase != 1)) || (attrs != (void*)NULL)) || (drift_mou_config_valid(config) == 0))) {
    return drift_windows_moufiltr_INVALID;
  }
  drift_mou_save_config(s, config);
  s->phase = 2U;
  if((result != ((drift_win_mou_object_ptr_arr0_ptr)NULL))) {
    result[0] = drift_mou_object(s, 1U);
  }
  return 0;
}
void WdfFdoInitSetFilter(drift_win_mou_object_ptr i) {
  drift_win_moufiltr_ptr s = drift_windows_moufiltr_owner(i, 5U);
  if((s == ((drift_win_moufiltr_ptr)NULL))) {
    return;
  }
  if((s->phase != 2)) {
    s->error = drift_windows_moufiltr_INVALID_STATE;
    return;
  }
  s->filter = 1U;
}
void WdfDeviceInitSetDeviceType(drift_win_mou_object_ptr i, uint32_t t) {
  drift_win_moufiltr_ptr s = drift_windows_moufiltr_owner(i, 5U);
  if((s == ((drift_win_moufiltr_ptr)NULL))) {
    return;
  }
  if(((s->phase != 2) || (t != 15))) {
    s->error = drift_windows_moufiltr_INVALID;
    return;
  }
  s->device_type = t;
}
int32_t WdfDeviceCreate(drift_win_mou_object_ptr_arr0_ptr i, void* attrs, drift_win_mou_object_ptr_arr0_ptr result) {
  if(((i == ((drift_win_mou_object_ptr_arr0_ptr)NULL)) || (result == ((drift_win_mou_object_ptr_arr0_ptr)NULL)))) {
    return drift_windows_moufiltr_INVALID;
  }
  drift_win_moufiltr_ptr s = drift_windows_moufiltr_owner(i[0], 5U);
  if((((((s == ((drift_win_moufiltr_ptr)NULL)) || (s->phase != 2)) || (s->filter != 1)) || (s->device_type != 15)) || (s->error != 0))) {
    return drift_windows_moufiltr_INVALID_STATE;
  }
  if((drift_mou_attributes_valid(attrs) == 0)) {
    return drift_windows_moufiltr_INVALID;
  }
  drift_mou_device_result(i, result, s);
  s->phase = 3U;
  return 0;
}
int32_t WdfIoQueueCreate(drift_win_mou_object_ptr d, void* config, void* attrs, drift_win_mou_object_ptr_arr0_ptr result) {
  drift_win_moufiltr_ptr s = drift_windows_moufiltr_owner(d, 2U);
  if(((s == ((drift_win_moufiltr_ptr)NULL)) || (s->phase != 3))) {
    return drift_windows_moufiltr_INVALID_STATE;
  }
  if(((drift_mou_queue_valid(config) == 0) || (attrs != (void*)NULL))) {
    return drift_windows_moufiltr_INVALID;
  }
  drift_mou_save_queue(s, config);
  s->phase = 4U;
  if((result != ((drift_win_mou_object_ptr_arr0_ptr)NULL))) {
    result[0] = drift_mou_object(s, 3U);
  }
  return 0;
}
drift_win_mou_object_ptr WdfIoQueueGetDevice(drift_win_mou_object_ptr q) {
  drift_win_moufiltr_ptr s = drift_windows_moufiltr_owner(q, 3U);
  if(((s == ((drift_win_moufiltr_ptr)NULL)) || (s->phase != 4))) {
    return ((drift_win_mou_object_ptr)NULL);
  }
  return drift_mou_object(s, 2U);
}
drift_win_mou_object_ptr WdfDeviceGetIoTarget(drift_win_mou_object_ptr d) {
  drift_win_moufiltr_ptr s = drift_windows_moufiltr_owner(d, 2U);
  if(((s == ((drift_win_moufiltr_ptr)NULL)) || (s->phase != 4))) {
    return ((drift_win_mou_object_ptr)NULL);
  }
  return drift_mou_object(s, 4U);
}
drift_win_mou_object_ptr WdfDeviceWdmGetDeviceObject(drift_win_mou_object_ptr d) {
  drift_win_moufiltr_ptr s = drift_windows_moufiltr_owner(d, 2U);
  if(((s == ((drift_win_moufiltr_ptr)NULL)) || (s->phase != 4))) {
    return ((drift_win_mou_object_ptr)NULL);
  }
  return drift_mou_object(s, 7U);
}
drift_win_mou_object_ptr WdfWdmDeviceGetWdfDeviceHandle(drift_win_mou_object_ptr d) {
  drift_win_moufiltr_ptr s = drift_windows_moufiltr_owner(d, 7U);
  if(((s == ((drift_win_moufiltr_ptr)NULL)) || (s->phase != 4))) {
    return ((drift_win_mou_object_ptr)NULL);
  }
  return drift_mou_object(s, 2U);
}
void* drift_wdf_context(drift_win_mou_object_ptr d) {
  drift_win_moufiltr_ptr s = drift_windows_moufiltr_owner(d, 2U);
  if(((s == ((drift_win_moufiltr_ptr)NULL)) || (s->phase != 4))) {
    return (void*)NULL;
  }
  return drift_mou_extension(s);
}
int32_t WdfRequestRetrieveInputBuffer(drift_win_mou_request_ptr r, size_t minimum, void* result, nlcsize_ptr length) {
  if((((!drift_windows_moufiltr_valid(r)) || (r->phase != 1)) || (result == (void*)NULL))) {
    return drift_windows_moufiltr_INVALID_STATE;
  }
  if((r->input_length < minimum)) {
    return drift_windows_moufiltr_TOO_SMALL;
  }
  if((drift_mou_buffer_valid(r) == 0)) {
    return drift_windows_moufiltr_INVALID;
  }
  drift_mou_buffer_result(r, (nlpointer_ptr)result, length);
  return 0;
}
int32_t WdfRequestGetStatus(drift_win_mou_request_ptr r) {
  if((!drift_windows_moufiltr_valid(r))) {
    return drift_windows_moufiltr_INVALID_HANDLE;
  }
  return r->status;
}
void WdfRequestComplete(drift_win_mou_request_ptr r, int32_t status) {
  if((!drift_windows_moufiltr_valid(r))) {
    return;
  }
  if((r->phase != 1)) {
    r->error = drift_windows_moufiltr_INVALID_STATE;
    return;
  }
  int32_t result = drift_windows_moufiltr_finish_request(r, status, 0U);
  if((result != 0)) {
    r->error = result;
  }
}
uint8_t WdfRequestSend(drift_win_mou_request_ptr r, drift_win_mou_object_ptr target, void* options) {
  if((!drift_windows_moufiltr_valid(r))) {
    return 0U;
  }
  drift_win_moufiltr_ptr s = drift_windows_moufiltr_owner(target, 4U);
  if(((((s == ((drift_win_moufiltr_ptr)NULL)) || (s != r->owner)) || (r->phase != 1)) || (drift_mou_send_valid(options) == 0))) {
    r->status = drift_windows_moufiltr_INVALID;
    return 0U;
  }
  r->phase = 2U;
  int32_t status = drift_mou_lower_send(s, r);
  if((status < 0)) {
    if((r->phase != 2)) {
      r->error = drift_windows_moufiltr_INVALID_STATE;
      return 1U;
    }
    r->phase = 1U;
    r->status = status;
    return 0U;
  }
  return 1U;
}
int32_t drift_mou_begin(drift_win_moufiltr_ptr s, drift_win_mou_request_ptr r, uint32_t code, void* input, size_t length) {
  if((((s == ((drift_win_moufiltr_ptr)NULL)) || (r == ((drift_win_mou_request_ptr)NULL))) || (s->phase != 4))) {
    return drift_windows_moufiltr_INVALID_STATE;
  }
  if((((r->phase != 0) || (r->dispatching != 0)) || (r->owner != ((drift_win_moufiltr_ptr)NULL)))) {
    return drift_windows_moufiltr_BUSY;
  }
  if(((s->outstanding >= 64) || (s->depth >= 64))) {
    return drift_windows_moufiltr_BUSY;
  }
  r->owner = s;
  r->phase = 1U;
  r->dispatching = 1U;
  r->completions = 0U;
  r->control = code;
  r->status = drift_windows_moufiltr_PENDING;
  r->error = 0;
  r->input = input;
  r->input_length = length;
  r->information = 0U;
  s->outstanding = (s->outstanding + 1);
  s->depth = (s->depth + 1);
  return 0;
}
int32_t drift_mou_end(drift_win_mou_request_ptr r) {
  if(((!drift_windows_moufiltr_valid(r)) || (r->dispatching != 1))) {
    return drift_windows_moufiltr_INVALID_STATE;
  }
  r->dispatching = 0U;
  r->owner->depth = (r->owner->depth - 1);
  if((r->error != 0)) {
    return r->error;
  }
  if((r->phase == 2)) {
    return drift_windows_moufiltr_PENDING;
  }
  if((r->phase == 3)) {
    return r->status;
  }
  return drift_windows_moufiltr_INVALID_STATE;
}
int32_t drift_windows_moufiltr_complete(drift_win_mou_request_ptr r, int32_t status, uintptr_t information) {
  if(((r == ((drift_win_mou_request_ptr)NULL)) || (r->phase != 2))) {
    return drift_windows_moufiltr_INVALID_STATE;
  }
  if((!drift_windows_moufiltr_valid(r))) {
    return drift_windows_moufiltr_INVALID_STATE;
  }
  return drift_windows_moufiltr_finish_request(r, status, information);
}
int32_t drift_windows_moufiltr_request_reset(drift_win_mou_request_ptr r) {
  if((r == ((drift_win_mou_request_ptr)NULL))) {
    return drift_windows_moufiltr_INVALID;
  }
  if(((r->phase != 3) || (r->dispatching != 0))) {
    return drift_windows_moufiltr_BUSY;
  }
  r->owner = ((drift_win_moufiltr_ptr)NULL);
  r->phase = 0U;
  r->completions = 0U;
  r->error = 0;
  r->status = 0;
  r->control = 0U;
  r->input = (void*)NULL;
  r->input_length = 0U;
  r->information = 0U;
  return 0;
}
int32_t drift_windows_moufiltr_close(drift_win_moufiltr_ptr s) {
  if(((s == ((drift_win_moufiltr_ptr)NULL)) || (s->phase != 4))) {
    return drift_windows_moufiltr_INVALID_STATE;
  }
  if(((s->outstanding != 0) || (s->depth != 0))) {
    return drift_windows_moufiltr_BUSY;
  }
  s->phase = 0U;
  s->host = ((drift_win_mou_host_ptr)NULL);
  s->filter = 0U;
  s->device_type = 0U;
  s->error = 0;
  drift_mou_object_init(s);
  return 0;
}

