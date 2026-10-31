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
#include "drift_hw.h"
#define NELUA_NIL (nlniltype){}
#include "virtio_rng_compat.h"
#include <stdint.h>
#include <stddef.h>
/* Macro used to export C functions. */
#ifdef _WIN32
  #define NELUA_CEXPORT __declspec(dllexport)
#elif defined(__GNUC__)
  #define NELUA_CEXPORT __attribute__((visibility("default")))
#else
  #define NELUA_CEXPORT
#endif
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
#include <stdbool.h>
/* ------------------------------ DECLARATIONS ------------------------------ */
typedef struct nlniltype {} nlniltype;
typedef struct nlniltype nltype;
typedef drift_linux_virtio_rng* drift_linux_virtio_rng_ptr;
static void drift_linux_virtio_rng_fail(drift_linux_virtio_rng_ptr s, int32_t e);
static int32_t drift_linux_virtio_rng_native_error(int32_t e);
typedef uint8_t* nluint8_arr0_ptr;
static void drift_linux_virtio_rng_clear(nluint8_arr0_ptr p, uint32_t n);
typedef drift_virtio* drift_virtio_ptr;
NELUA_CEXPORT int32_t drift_lx_vrng_prepare(drift_linux_virtio_rng_ptr s, drift_virtio_ptr d);
NELUA_CEXPORT void* drift_lx_vrng_alloc(size_t n, int flags);
NELUA_CEXPORT void drift_lx_vrng_free(void* p);
typedef drift_vrng_ida* drift_vrng_ida_ptr;
NELUA_CEXPORT int drift_lx_vrng_ida_alloc(drift_vrng_ida_ptr a, int flags);
static NELUA_INLINE uint32_t nelua_shl_nluint32(uint32_t a, int32_t b);
NELUA_CEXPORT void drift_lx_vrng_ida_free(drift_vrng_ida_ptr a, int index);
NELUA_CEXPORT int drift_lx_vrng_name(char* out, char* format, int index);
typedef char* nlcchar_arr0_ptr;
NELUA_CEXPORT void* drift_lx_vrng_memcpy(void* dst, void* src, size_t n);
typedef drift_vrng_completion* drift_vrng_completion_ptr;
NELUA_CEXPORT void drift_lx_vrng_completion_init(drift_vrng_completion_ptr c);
NELUA_CEXPORT void drift_lx_vrng_complete(drift_vrng_completion_ptr c);
typedef drift_vrng_vq* drift_vrng_vq_ptr;
typedef drift_vrng_device* drift_vrng_device_ptr;
NELUA_CEXPORT drift_vrng_vq_ptr drift_lx_vrng_find_vq(drift_vrng_device_ptr d, char* name);
NELUA_CEXPORT void drift_lx_vrng_ready(drift_vrng_device_ptr d);
typedef drift_vrng_sg* drift_vrng_sg_ptr;
NELUA_CEXPORT int drift_lx_vrng_add(drift_vrng_vq_ptr q, drift_vrng_sg_ptr sg, unsigned int count, void* cookie, int flags);
NELUA_CEXPORT bool drift_lx_vrng_kick(drift_vrng_vq_ptr q);
typedef unsigned int* nlcuint_ptr;
NELUA_CEXPORT void* drift_lx_vrng_get(drift_vrng_vq_ptr q, nlcuint_ptr length);
NELUA_CEXPORT int32_t drift_lx_vrng_pump(drift_linux_virtio_rng_ptr s);
NELUA_CEXPORT int drift_lx_vrng_wait(drift_vrng_completion_ptr c);
typedef drift_vrng_hwrng* drift_vrng_hwrng_ptr;
NELUA_CEXPORT int drift_lx_vrng_register(drift_vrng_hwrng_ptr r);
NELUA_CEXPORT void drift_lx_vrng_unregister(drift_vrng_hwrng_ptr r);
NELUA_CEXPORT void drift_lx_vrng_reset(drift_vrng_device_ptr d);
NELUA_CEXPORT void drift_lx_vrng_del_vqs(drift_vrng_device_ptr d);
NELUA_CEXPORT void drift_lx_vrng_reclaim(drift_linux_virtio_rng_ptr s);
NELUA_CEXPORT int32_t drift_lx_vrng_quiesce(drift_linux_virtio_rng_ptr s);
/* ------------------------------ DEFINITIONS ------------------------------- */
void drift_linux_virtio_rng_fail(drift_linux_virtio_rng_ptr s, int32_t e) {
  if(((s != ((drift_linux_virtio_rng_ptr)NULL)) && (s->error == 0))) {
    s->error = ((e < 0) ? e : -5);
  }
}
int32_t drift_linux_virtio_rng_native_error(int32_t e) {
  if((e == -2)) {
    return -110;
  } else if((e == -6)) {
    return -12;
  } else {
    return -5;
  }
}
void drift_linux_virtio_rng_clear(nluint8_arr0_ptr p, uint32_t n) {
  for(uint32_t i = 0U, _end = n; i < _end; i += 1) {
    p[i] = 0U;
  }
}
int32_t drift_lx_vrng_prepare(drift_linux_virtio_rng_ptr s, drift_virtio_ptr d) {
  if(((s == ((drift_linux_virtio_rng_ptr)NULL)) || (drift_vrng_transport_valid(d) == 0))) {
    return -22;
  }
  if((((((s->phase != 0) && (s->phase != 4)) || (s->allocated != 0)) || (s->payload.data != ((nluint8_arr0_ptr)NULL))) || (s->transfer.active != 0))) {
    return -16;
  }
  s->transport = d;
  s->phase = 1U;
  s->error = 0;
  s->queue_created = 0U;
  s->registered = 0U;
  s->id_owned = 0U;
  s->free_requested = 0U;
  s->id_free_requested = 0U;
  s->completion_latched = 0U;
  s->read_active = 0U;
  s->hwrng = (void*)NULL;
  s->ida = (void*)NULL;
  s->source_buffer = (void*)NULL;
  s->completed_cookie = (void*)NULL;
  s->completed_length = 0U;
  s->read_output = ((nluint8_arr0_ptr)NULL);
  s->read_length = 0U;
  drift_linux_virtio_rng_clear(drift_vrng_source_slot(s), 2048U);
  drift_linux_virtio_rng_clear(drift_vrng_device_bytes(s), 128U);
  drift_linux_virtio_rng_clear(drift_vrng_queue_bytes(s), 128U);
  drift_vrng_cancel_clear(s);
  return 0;
}
void* drift_lx_vrng_alloc(size_t n, int flags) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if((((((s == ((drift_linux_virtio_rng_ptr)NULL)) || (flags != 0)) || (n != drift_lx_vrng_source_size())) || (n > 2048)) || (s->allocated != 0))) {
    return (void*)NULL;
  }
  drift_linux_virtio_rng_clear(drift_vrng_source_slot(s), 2048U);
  s->allocated = 1U;
  s->free_requested = 0U;
  return (void*)drift_vrng_source_slot(s);
}
void drift_lx_vrng_free(void* p) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if((s == ((drift_linux_virtio_rng_ptr)NULL))) {
    return;
  }
  if((((s->allocated == 0) || (p != (void*)drift_vrng_source_slot(s))) || (s->free_requested != 0))) {
    drift_linux_virtio_rng_fail(s, -22);
    return;
  }
  s->free_requested = 1U;
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
int drift_lx_vrng_ida_alloc(drift_vrng_ida_ptr a, int flags) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if(((((s == ((drift_linux_virtio_rng_ptr)NULL)) || (a == ((drift_vrng_ida_ptr)NULL))) || (flags != 0)) || (s->id_owned != 0))) {
    return -22;
  }
  for(uint32_t i = 0U; i < 16U; i += 1) {
    if(((a->bitmap & nelua_shl_nluint32(1U, i)) == 0)) {
      a->bitmap = (a->bitmap | nelua_shl_nluint32(1U, i));
      s->ida = (void*)a;
      s->index = i;
      s->id_owned = 1U;
      s->id_free_requested = 0U;
      return (int)i;
    }
  }
  return -28;
}
void drift_lx_vrng_ida_free(drift_vrng_ida_ptr a, int index) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if((s == ((drift_linux_virtio_rng_ptr)NULL))) {
    return;
  }
  if(((((((a == ((drift_vrng_ida_ptr)NULL)) || (s->id_owned == 0)) || (s->ida != (void*)a)) || (index < 0)) || ((uint32_t)index != s->index)) || (s->id_free_requested != 0))) {
    drift_linux_virtio_rng_fail(s, -22);
    return;
  }
  s->id_free_requested = 1U;
}
int drift_lx_vrng_name(char* out, char* format, int index) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  char* expected = (char*)"virtio_rng.%d";
  char* prefix = (char*)"virtio_rng.";
  if((((((s == ((drift_linux_virtio_rng_ptr)NULL)) || (out == (char*)NULL)) || (format == (char*)NULL)) || (index < 0)) || (index > 15))) {
    drift_linux_virtio_rng_fail(s, -22);
    return -22;
  }
  nlcchar_arr0_ptr o = ((nlcchar_arr0_ptr)out);
  nlcchar_arr0_ptr f = ((nlcchar_arr0_ptr)format);
  nlcchar_arr0_ptr e = ((nlcchar_arr0_ptr)expected);
  nlcchar_arr0_ptr p = ((nlcchar_arr0_ptr)prefix);
  for(uint32_t i = 0U; i <= 13U; i += 1) {
    if((f[i] != e[i])) {
      drift_linux_virtio_rng_fail(s, -22);
      return -22;
    }
  }
  for(uint32_t i = 0U; i < 11U; i += 1) {
    o[i] = p[i];
  }
  if((index >= 10)) {
    o[11] = 49;
    o[12] = (char)((48 + index) - 10);
    o[13] = 0;
    return 13;
  }
  o[11] = (char)(48 + index);
  o[12] = 0;
  return 12;
}
void* drift_lx_vrng_memcpy(void* dst, void* src, size_t n) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if(((((s == ((drift_linux_virtio_rng_ptr)NULL)) || (dst == (void*)NULL)) || (src == (void*)NULL)) || (n > 4096))) {
    drift_linux_virtio_rng_fail(s, -22);
    return (void*)NULL;
  }
  size_t da = (size_t)dst;
  size_t sa = (size_t)src;
  size_t db = (size_t)s->read_output;
  size_t sb = (size_t)drift_lx_vrng_source_data(s);
  if((((((((s->read_active == 0) || (da < db)) || (sa < sb)) || ((da - db) > s->read_length)) || ((sa - sb) > drift_lx_vrng_source_capacity(s))) || (n > (s->read_length - (da - db)))) || (n > (drift_lx_vrng_source_capacity(s) - (sa - sb))))) {
    drift_linux_virtio_rng_fail(s, -22);
    return (void*)NULL;
  }
  nluint8_arr0_ptr d = ((nluint8_arr0_ptr)dst);
  nluint8_arr0_ptr p = ((nluint8_arr0_ptr)src);
  for(size_t i = 0U, _end = n; i < _end; i += 1) {
    d[i] = p[i];
  }
  return dst;
}
void drift_lx_vrng_completion_init(drift_vrng_completion_ptr c) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if(((s == ((drift_linux_virtio_rng_ptr)NULL)) || (c != drift_lx_vrng_source_completion(s)))) {
    drift_linux_virtio_rng_fail(s, -22);
    return;
  }
  c->done = 0U;
}
void drift_lx_vrng_complete(drift_vrng_completion_ptr c) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if((((s == ((drift_linux_virtio_rng_ptr)NULL)) || (c != drift_lx_vrng_source_completion(s))) || (c->done == 0xffffffffULL))) {
    drift_linux_virtio_rng_fail(s, -22);
    return;
  }
  c->done = (c->done + 1);
}
drift_vrng_vq_ptr drift_lx_vrng_find_vq(drift_vrng_device_ptr d, char* name) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if(((((s == ((drift_linux_virtio_rng_ptr)NULL)) || (d != drift_vrng_device_slot(s))) || (name == (char*)NULL)) || (s->queue_created != 0))) {
    drift_linux_virtio_rng_fail(s, -22);
    return ((drift_vrng_vq_ptr)-22);
  }
  nlcchar_arr0_ptr text = ((nlcchar_arr0_ptr)name);
  if(((((((text[0] != 105) || (text[1] != 110)) || (text[2] != 112)) || (text[3] != 117)) || (text[4] != 116)) || (text[5] != 0))) {
    drift_linux_virtio_rng_fail(s, -22);
    return ((drift_vrng_vq_ptr)-22);
  }
  uint32_t n = drift_lx_vrng_source_capacity(s);
  if((((n == 0) || (n > 4096)) || (drift_hw_dma(drift_vrng_hw(s), n, 4096U, (&s->payload)) != 0))) {
    drift_linux_virtio_rng_fail(s, -12);
    return ((drift_vrng_vq_ptr)-12);
  }
  if((((s->payload.data == ((nluint8_arr0_ptr)NULL)) || (s->payload.size < n)) || (s->payload.address > (0xffffffffffffffffULL - n)))) {
    drift_linux_virtio_rng_fail(s, -12);
    return ((drift_vrng_vq_ptr)-12);
  }
  drift_linux_virtio_rng_clear(s->payload.data, n);
  s->queue_created = 1U;
  return drift_vrng_queue_slot(s);
}
void drift_lx_vrng_ready(drift_vrng_device_ptr d) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if((((s == ((drift_linux_virtio_rng_ptr)NULL)) || (d != drift_vrng_device_slot(s))) || (s->queue_created == 0))) {
    drift_linux_virtio_rng_fail(s, -22);
    return;
  }
  int32_t r = drift_virtio_raw_device_ready(s->transport);
  if((r != 0)) {
    drift_linux_virtio_rng_fail(s, drift_linux_virtio_rng_native_error(r));
  }
}
int drift_lx_vrng_add(drift_vrng_vq_ptr q, drift_vrng_sg_ptr sg, unsigned int count, void* cookie, int flags) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if((s == ((drift_linux_virtio_rng_ptr)NULL))) {
    return -22;
  }
  if((s->error != 0)) {
    return (int)s->error;
  }
  if(((((((q != drift_vrng_queue_slot(s)) || (sg == ((drift_vrng_sg_ptr)NULL))) || (count != 1)) || (flags != 0)) || (s->queue_created == 0)) || (s->completion_latched != 0))) {
    drift_linux_virtio_rng_fail(s, -22);
    return -22;
  }
  void* p = (void*)NULL;
  uint32_t n = 0U;
  drift_vrng_sg_data(sg, (&p), (&n));
  if((((p != (void*)drift_lx_vrng_source_data(s)) || (cookie != p)) || (n != drift_lx_vrng_source_capacity(s)))) {
    drift_linux_virtio_rng_fail(s, -22);
    return -22;
  }
  s->source_buffer = p;
  int32_t r = drift_virtio_raw_add(s->transport, 0U, (&s->transfer), (&s->payload), 0U, n, 2U, cookie);
  if((r != 0)) {
    drift_linux_virtio_rng_fail(s, drift_linux_virtio_rng_native_error(r));
    return (int)s->error;
  }
  return 0;
}
bool drift_lx_vrng_kick(drift_vrng_vq_ptr q) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if(((s == ((drift_linux_virtio_rng_ptr)NULL)) || (q != drift_vrng_queue_slot(s)))) {
    drift_linux_virtio_rng_fail(s, -22);
    return false;
  }
  if((s->error != 0)) {
    return false;
  }
  int32_t r = drift_virtio_raw_kick(s->transport, 0U);
  if((r != 0)) {
    drift_linux_virtio_rng_fail(s, drift_linux_virtio_rng_native_error(r));
    return false;
  }
  return true;
}
void* drift_lx_vrng_get(drift_vrng_vq_ptr q, nlcuint_ptr length) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if((((s == ((drift_linux_virtio_rng_ptr)NULL)) || (q != drift_vrng_queue_slot(s))) || (length == (nlcuint_ptr)NULL))) {
    drift_linux_virtio_rng_fail(s, -22);
    return (void*)NULL;
  }
  if((s->completion_latched == 0)) {
    return (void*)NULL;
  }
  (*length) = (unsigned int)s->completed_length;
  void* p = s->completed_cookie;
  s->completion_latched = 0U;
  s->completed_cookie = (void*)NULL;
  s->completed_length = 0U;
  return p;
}
int32_t drift_lx_vrng_pump(drift_linux_virtio_rng_ptr s) {
  if((((s != drift_lx_vrng_current()) || (s->phase == 0)) || (s->queue_created == 0))) {
    return -22;
  }
  if((s->error != 0)) {
    return s->error;
  }
  if((s->completion_latched != 0)) {
    drift_linux_virtio_rng_fail(s, -5);
    return s->error;
  }
  void* p = (void*)NULL;
  uint32_t n = 0U;
  int32_t r = drift_virtio_raw_get(s->transport, 0U, (&s->transfer), (&p), (&n));
  if((r < 0)) {
    drift_linux_virtio_rng_fail(s, drift_linux_virtio_rng_native_error(r));
    if((drift_vrng_transport_failed(s) != 0)) {
      s->phase = 5U;
    }
    return s->error;
  }
  if((r == 0)) {
    return 0;
  }
  if((((p != s->source_buffer) || (n == 0)) || (n > drift_lx_vrng_source_capacity(s)))) {
    drift_linux_virtio_rng_fail(s, -5);
    if((drift_virtio_close(s->transport) != 0)) {
      s->phase = 5U;
    }
    return s->error;
  }
  nluint8_arr0_ptr out = drift_lx_vrng_source_data(s);
  for(uint32_t i = 0U, _end = n; i < _end; i += 1) {
    out[i] = s->payload.data[i];
  }
  s->completed_cookie = p;
  s->completed_length = n;
  s->completion_latched = 1U;
  drift_lx_vrng_callback(s);
  if((s->error != 0)) {
    return s->error;
  }
  if((s->completion_latched != 0)) {
    drift_linux_virtio_rng_fail(s, -5);
    return s->error;
  }
  return 1;
}
int drift_lx_vrng_wait(drift_vrng_completion_ptr c) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if((((s == ((drift_linux_virtio_rng_ptr)NULL)) || (c != drift_lx_vrng_source_completion(s))) || (s->read_active == 0))) {
    drift_linux_virtio_rng_fail(s, -22);
    return -22;
  }
  while((c->done == 0)) {
    if((drift_vrng_cancel_take(s) != 0)) {
      return -4;
    }
    if((s->error != 0)) {
      return (int)s->error;
    }
    if(((drift_hw_time(drift_vrng_hw(s)) - s->read_started) >= drift_vrng_timeout(s))) {
      drift_linux_virtio_rng_fail(s, -110);
      if((drift_virtio_close(s->transport) != 0)) {
        s->phase = 5U;
      }
      return (int)s->error;
    }
    int32_t r = drift_lx_vrng_pump(s);
    if((r < 0)) {
      return (int)r;
    }
    if((c->done == 0)) {
      drift_hw_wait(drift_vrng_hw(s), 10U);
    }
  }
  c->done = (c->done - 1);
  return 0;
}
int drift_lx_vrng_register(drift_vrng_hwrng_ptr r) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if(((((s == ((drift_linux_virtio_rng_ptr)NULL)) || (r != drift_lx_vrng_source_hwrng(s))) || (s->registered != 0)) || (s->error != 0))) {
    return -22;
  }
  s->hwrng = (void*)r;
  s->registered = 1U;
  return 0;
}
void drift_lx_vrng_unregister(drift_vrng_hwrng_ptr r) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if((((s == ((drift_linux_virtio_rng_ptr)NULL)) || (s->registered == 0)) || (s->hwrng != (void*)r))) {
    drift_linux_virtio_rng_fail(s, -22);
    return;
  }
  s->registered = 0U;
  s->hwrng = (void*)NULL;
}
void drift_lx_vrng_reset(drift_vrng_device_ptr d) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if(((s == ((drift_linux_virtio_rng_ptr)NULL)) || (d != drift_vrng_device_slot(s)))) {
    drift_linux_virtio_rng_fail(s, -22);
    return;
  }
  int32_t r = drift_virtio_close(s->transport);
  if((r != 0)) {
    drift_linux_virtio_rng_fail(s, drift_linux_virtio_rng_native_error(r));
    s->phase = 5U;
  }
}
void drift_lx_vrng_del_vqs(drift_vrng_device_ptr d) {
  drift_linux_virtio_rng_ptr s = drift_lx_vrng_current();
  if(((s == ((drift_linux_virtio_rng_ptr)NULL)) || (d != drift_vrng_device_slot(s)))) {
    drift_linux_virtio_rng_fail(s, -22);
    return;
  }
  if((drift_vrng_transport_closed(s) != 0)) {
    s->queue_created = 0U;
    s->completion_latched = 0U;
  }
}
void drift_lx_vrng_reclaim(drift_linux_virtio_rng_ptr s) {
  if((((s != drift_lx_vrng_current()) || (drift_vrng_transport_closed(s) == 0)) || (s->transfer.active != 0))) {
    drift_linux_virtio_rng_fail(s, -16);
    return;
  }
  if((s->payload.data != ((nluint8_arr0_ptr)NULL))) {
    drift_hw_release(drift_vrng_hw(s), (&s->payload));
    s->payload.data = ((nluint8_arr0_ptr)NULL);
  }
  if((s->id_owned != 0)) {
    drift_vrng_ida_ptr a = ((drift_vrng_ida_ptr)s->ida);
    if((((a == ((drift_vrng_ida_ptr)NULL)) || (s->index >= 16)) || ((a->bitmap & nelua_shl_nluint32(1U, s->index)) == 0))) {
      drift_linux_virtio_rng_fail(s, -22);
      return;
    }
    a->bitmap = (a->bitmap & (~nelua_shl_nluint32(1U, s->index)));
    s->id_owned = 0U;
  }
  s->allocated = 0U;
  s->free_requested = 0U;
  s->id_free_requested = 0U;
  s->queue_created = 0U;
  s->registered = 0U;
  s->completion_latched = 0U;
  s->hwrng = (void*)NULL;
  s->ida = (void*)NULL;
  s->source_buffer = (void*)NULL;
  s->completed_cookie = (void*)NULL;
}
int32_t drift_lx_vrng_quiesce(drift_linux_virtio_rng_ptr s) {
  if((s != drift_lx_vrng_current())) {
    return -22;
  }
  int32_t r = drift_virtio_close(s->transport);
  if((r != 0)) {
    s->phase = 5U;
    drift_linux_virtio_rng_fail(s, drift_linux_virtio_rng_native_error(r));
    return drift_linux_virtio_rng_native_error(r);
  }
  drift_lx_vrng_reclaim(s);
  return (int32_t)((((s->transfer.active == 0) && (s->payload.data == ((nluint8_arr0_ptr)NULL))) && (s->allocated == 0)) ? 0 : -5);
}

