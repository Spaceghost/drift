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
#include <stdint.h>
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
#include "drift_usb.h"
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
/* Macro used sign that a type punning cast may alias (related to strict aliasing). */
#ifdef __GNUC__
  #define NELUA_MAYALIAS __attribute__((may_alias))
#else
  #define NELUA_MAYALIAS
#endif
/* ------------------------------ DECLARATIONS ------------------------------ */
typedef struct nlniltype {} nlniltype;
typedef struct nlniltype nltype;
typedef uint8_t* nluint8_arr0_ptr;
static uint32_t drift_usb_get(nluint8_arr0_ptr p, uint32_t o);
static void drift_usb_put(nluint8_arr0_ptr p, uint32_t o, uint32_t v);
static NELUA_INLINE uint32_t nelua_shr_nluint32(uint32_t a, int32_t b);
static void drift_usb_put64(nluint8_arr0_ptr p, uint32_t o, uint64_t v);
typedef drift_usb* drift_usb_ptr;
static uint32_t drift_usb_rd(drift_usb_ptr d, uint32_t o);
static void drift_usb_wr(drift_usb_ptr d, uint32_t o, uint32_t v);
static void drift_usb_wr64(drift_usb_ptr d, uint32_t o, uint64_t v);
static void drift_usb_sync(drift_usb_ptr d, uint32_t o, uint32_t n, uint32_t direction);
static void drift_usb_clear(drift_usb_ptr d, uint32_t o, uint32_t n);
static int32_t drift_usb_waitreg(drift_usb_ptr d, uint32_t o, uint32_t mask, uint32_t value);
static int32_t drift_usb_stop(drift_usb_ptr d);
static int32_t drift_usb_poison(drift_usb_ptr d, int32_t e);
static void drift_usb_trb(drift_usb_ptr d, uint32_t o, uint64_t p, uint32_t status, uint32_t control);
static uint64_t drift_usb_enqueue(drift_usb_ptr d, uint32_t index, uint64_t p, uint32_t status, uint32_t control);
typedef uint32_t* nluint32_ptr;
static int32_t drift_usb_event(drift_usb_ptr d, uint32_t kind, uint64_t target, uint32_t endpoint, uint32_t request, uint64_t short_at, nluint32_ptr actual);
static int32_t drift_usb_command(drift_usb_ptr d, uint64_t p, uint32_t c);
NELUA_CEXPORT int32_t drift_usb_close(drift_usb_ptr d);
typedef drift_hw* drift_hw_ptr;
NELUA_CEXPORT int32_t drift_usb_open(drift_usb_ptr d, drift_hw_ptr h, uint32_t region);
static bool drift_usb_ready(drift_usb_ptr d);
NELUA_CEXPORT int32_t drift_usb_control(drift_usb_ptr d, uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, nluint8_arr0_ptr buffer, uint32_t length, nluint32_ptr received);
NELUA_CEXPORT int32_t drift_usb_transfer(drift_usb_ptr d, uint8_t endpoint, nluint8_arr0_ptr buffer, uint32_t length, nluint32_ptr received);
NELUA_CEXPORT int32_t drift_usb_recover(drift_usb_ptr d, uint8_t endpoint);
static uint32_t drift_usb_little16(nluint8_arr0_ptr p, uint32_t o);
static int32_t drift_usb_enumerate(drift_usb_ptr d);
typedef struct NELUA_MAYALIAS nluint8_arr1024 {uint8_t v[1024];} nluint8_arr1024;
typedef union NELUA_MAYALIAS nluint8_arr1024_cast {nluint8_arr1024 a; uint8_t p[1024];} nluint8_arr1024_cast;
static NELUA_INLINE uint32_t nelua_shl_nluint32(uint32_t a, int32_t b);
static int32_t drift_usb_attach_root(drift_usb_ptr d, uint32_t port, uint32_t index);
NELUA_CEXPORT int32_t drift_usb_attach(drift_usb_ptr d, uint32_t port);
NELUA_CEXPORT int32_t drift_usb_attach_config(drift_usb_ptr d, uint32_t port, uint32_t index);
NELUA_CEXPORT int32_t drift_usb_hid(drift_usb_ptr d, nluint8_arr0_ptr report, uint32_t capacity, nluint32_ptr received);
static uint32_t drift_usb_be32(nluint8_arr0_ptr p, uint32_t o);
static void drift_usb_big32(nluint8_arr0_ptr p, uint32_t o, uint32_t v);
static int32_t drift_usb_storage_reset(drift_usb_ptr d);
static int32_t drift_usb_bot(drift_usb_ptr d, nluint8_arr0_ptr command_bytes, uint32_t command_length, nluint8_arr0_ptr data, uint32_t length, bool input);
typedef struct NELUA_MAYALIAS nluint8_arr31 {uint8_t v[31];} nluint8_arr31;
typedef union NELUA_MAYALIAS nluint8_arr31_cast {nluint8_arr31 a; uint8_t p[31];} nluint8_arr31_cast;
typedef struct NELUA_MAYALIAS nluint8_arr13 {uint8_t v[13];} nluint8_arr13;
typedef union NELUA_MAYALIAS nluint8_arr13_cast {nluint8_arr13 a; uint8_t p[13];} nluint8_arr13_cast;
typedef uint64_t* nluint64_ptr;
NELUA_CEXPORT int32_t drift_usb_storage_info(drift_usb_ptr d, nluint64_ptr blocks, nluint32_ptr sector_size);
typedef struct NELUA_MAYALIAS nluint8_arr10 {uint8_t v[10];} nluint8_arr10;
typedef union NELUA_MAYALIAS nluint8_arr10_cast {nluint8_arr10 a; uint8_t p[10];} nluint8_arr10_cast;
typedef struct NELUA_MAYALIAS nluint8_arr18 {uint8_t v[18];} nluint8_arr18;
typedef union NELUA_MAYALIAS nluint8_arr18_cast {nluint8_arr18 a; uint8_t p[18];} nluint8_arr18_cast;
static int32_t drift_usb_storage_io(drift_usb_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr data, bool write);
NELUA_CEXPORT int32_t drift_usb_storage_read(drift_usb_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr data);
NELUA_CEXPORT int32_t drift_usb_storage_write(drift_usb_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr data);
NELUA_CEXPORT int32_t drift_usb_storage_flush(drift_usb_ptr d);
NELUA_CEXPORT int32_t drift_usb_set_interface(drift_usb_ptr d, uint8_t number_1, uint8_t alternate);
NELUA_CEXPORT int32_t drift_usb_hub_child(drift_usb_ptr d, uint8_t hub_port, uint8_t speed, uint8_t hub_ports, uint8_t tt_think);
/* ------------------------------ DEFINITIONS ------------------------------- */
uint32_t drift_usb_get(nluint8_arr0_ptr p, uint32_t o) {
  return ((((uint32_t)p[o] | ((uint32_t)p[(o + 1)] << 8)) | ((uint32_t)p[(o + 2)] << 16)) | ((uint32_t)p[(o + 3)] << 24));
}
uint32_t nelua_shr_nluint32(uint32_t a, int32_t b) {
  if(NELUA_LIKELY(b >= 0 && b < 32)) {
    return (uint32_t)a >> b;
  } else if(NELUA_UNLIKELY(b < 0 && b > -32)) {
    return (uint32_t)a << -b;
  } else {
    return 0;
  }
}
void drift_usb_put(nluint8_arr0_ptr p, uint32_t o, uint32_t v) {
  for(uint32_t i = 0U; i <= 3U; i += 1) {
    p[(o + i)] = (uint8_t)(nelua_shr_nluint32(v, (i * 8)) & 255);
  }
}
void drift_usb_put64(nluint8_arr0_ptr p, uint32_t o, uint64_t v) {
  drift_usb_put(p, o, (uint32_t)(v & 0xffffffffLL));
  drift_usb_put(p, (o + 4), (uint32_t)(v >> 32));
}
uint32_t drift_usb_rd(drift_usb_ptr d, uint32_t o) {
  return (uint32_t)drift_hw_read(d->hw, d->region, o, 4U);
}
void drift_usb_wr(drift_usb_ptr d, uint32_t o, uint32_t v) {
  drift_hw_write(d->hw, d->region, o, 4U, (uint64_t)v);
}
void drift_usb_wr64(drift_usb_ptr d, uint32_t o, uint64_t v) {
  drift_usb_wr(d, o, (uint32_t)(v & 0xffffffffLL));
  drift_usb_wr(d, (o + 4), (uint32_t)(v >> 32));
}
void drift_usb_sync(drift_usb_ptr d, uint32_t o, uint32_t n, uint32_t direction) {
  drift_hw_sync(d->hw, (&d->arena), o, n, direction);
}
void drift_usb_clear(drift_usb_ptr d, uint32_t o, uint32_t n) {
  for(uint32_t i = 0U, _end = n; i < _end; i += 1) {
    d->arena.data[(o + i)] = 0U;
  }
}
int32_t drift_usb_waitreg(drift_usb_ptr d, uint32_t o, uint32_t mask, uint32_t value) {
  uint64_t start = drift_hw_time(d->hw);
  while(((drift_usb_rd(d, o) & mask) != value)) {
    if(((drift_hw_time(d->hw) - start) >= 1000000)) {
      return -110;
    }
    drift_hw_wait(d->hw, 100U);
  }
  return 0;
}
int32_t drift_usb_stop(drift_usb_ptr d) {
  drift_usb_wr(d, d->op, (drift_usb_rd(d, d->op) & 4294967294ULL));
  if((drift_usb_waitreg(d, (d->op + 4), 1U, 1U) != 0)) {
    d->state = 3U;
    return -110;
  }
  d->state = 2U;
  return 0;
}
int32_t drift_usb_poison(drift_usb_ptr d, int32_t e) {
  d->error = (uint32_t)e;
  drift_usb_stop(d);
  return e;
}
void drift_usb_trb(drift_usb_ptr d, uint32_t o, uint64_t p, uint32_t status, uint32_t control) {
  drift_usb_put64(d->arena.data, o, p);
  drift_usb_put(d->arena.data, (o + 8), status);
  drift_usb_put(d->arena.data, (o + 12), control);
  drift_usb_sync(d, o, 16U, 1U);
}
uint64_t drift_usb_enqueue(drift_usb_ptr d, uint32_t index, uint64_t p, uint32_t status, uint32_t control) {
  uint32_t base = d->ring_offset[index];
  uint32_t at = (base + (d->ring_index[index] * 16));
  drift_usb_trb(d, at, p, status, (control | d->ring_cycle[index]));
  d->ring_index[index] = (d->ring_index[index] + 1);
  if((d->ring_index[index] == 255)) {
    drift_usb_trb(d, (base + 4080), (d->arena.address + base), 0U, (6146 | d->ring_cycle[index]));
    d->ring_index[index] = 0U;
    d->ring_cycle[index] = (d->ring_cycle[index] ^ 1);
  }
  return (d->arena.address + at);
}
int32_t drift_usb_event(drift_usb_ptr d, uint32_t kind, uint64_t target, uint32_t endpoint, uint32_t request, uint64_t short_at, nluint32_ptr actual) {
  if((((kind == 32) && (endpoint > 1)) && (d->completion[endpoint] != 0))) {
    int32_t result = d->completion[endpoint];
    d->completion[endpoint] = 0;
    if((actual != (nluint32_ptr)NULL)) {
      (*actual) = d->completed_length[endpoint];
    }
    if((result == 1)) {
      return 0;
    }
    return result;
  }
  uint64_t start = drift_hw_time(d->hw);
  uint32_t short_length = request;
  while(true) {
    if(((drift_hw_time(d->hw) - start) >= 1000000)) {
      if(((((kind == 32) && (endpoint > 1)) && ((endpoint & 1) != 0)) && (((d->kind == 1) || (d->kind == 2)) || (d->kind == 4)))) {
        return -11;
      }
      return drift_usb_poison(d, -110);
    }
    if(((d->port != 0) && ((drift_usb_rd(d, ((d->op + 0x400) + ((d->port - 1) * 16))) & 1) == 0))) {
      return drift_usb_poison(d, -19);
    }
    uint32_t at = (8192 + (d->event_index * 16));
    drift_usb_sync(d, at, 16U, 2U);
    uint32_t control = drift_usb_get(d->arena.data, (at + 12));
    if(((control & 1) == d->event_cycle)) {
      uint32_t status = drift_usb_get(d->arena.data, (at + 8));
      uint64_t parameter = ((uint64_t)drift_usb_get(d->arena.data, at) | ((uint64_t)drift_usb_get(d->arena.data, (at + 4)) << 32));
      uint32_t etype = ((control >> 10) & 63);
      d->event_index = (d->event_index + 1);
      if((d->event_index == 256)) {
        d->event_index = 0U;
        d->event_cycle = (d->event_cycle ^ 1);
      }
      drift_usb_wr64(d, (d->runtime + 0x38), (((d->arena.address + 8192) + (d->event_index * 16)) | 8));
      uint32_t ev_endpoint = ((control >> 16) & 31);
      if((((etype == 32) && (ev_endpoint > 1)) && ((kind != 32) || (ev_endpoint != endpoint)))) {
        uint32_t code = (status >> 24);
        if(((((((control >> 24) != d->slot) || (d->pending_count[ev_endpoint] == 0)) || (parameter != d->pending_pointer[ev_endpoint])) || (d->completion[ev_endpoint] != 0)) || ((status & 0xffffff) > d->pending_count[ev_endpoint]))) {
          return drift_usb_poison(d, -5);
        }
        if(((code == 1) || (code == 13))) {
          d->completion[ev_endpoint] = 1;
          d->completed_length[ev_endpoint] = (d->pending_count[ev_endpoint] - (status & 0xffffff));
        } else if((code == 6)) {
          d->completion[ev_endpoint] = -32;
          d->halted[ev_endpoint] = 1U;
        } else {
          return drift_usb_poison(d, -5);
        }
      } else if((etype == kind)) {
        uint32_t code = (status >> 24);
        if((kind == 33)) {
          if((((parameter & 18446744073709551600ULL) != target) || (code != 1))) {
            return drift_usb_poison(d, -5);
          }
          d->slot = ((control >> 24) & 255);
          if((actual != (nluint32_ptr)NULL)) {
            (*actual) = 0U;
          }
          return 0;
        }
        if((((((control >> 16) & 31) != endpoint) || ((control >> 24) != d->slot)) || ((status & 0xffffff) > request))) {
          return drift_usb_poison(d, -5);
        }
        if(((parameter != target) && (parameter != short_at))) {
          return drift_usb_poison(d, -5);
        }
        if((code == 6)) {
          d->halted[endpoint] = 1U;
          return -32;
        }
        if(((code != 1) && (code != 13))) {
          return drift_usb_poison(d, -5);
        }
        if((code == 13)) {
          short_length = (request - (status & 0xffffff));
        }
        if((parameter == target)) {
          if((actual != (nluint32_ptr)NULL)) {
            (*actual) = short_length;
          }
          return 0;
        }
      } else if((etype != 34)) {
        return drift_usb_poison(d, -5);
      }
    } else {
      if(((drift_hw_time(d->hw) - start) >= 1000000)) {
        if(((((kind == 32) && (endpoint > 1)) && ((endpoint & 1) != 0)) && (((d->kind == 1) || (d->kind == 2)) || (d->kind == 4)))) {
          return -11;
        }
        return drift_usb_poison(d, -110);
      }
      drift_hw_wait(d->hw, 100U);
    }
  }
}
int32_t drift_usb_command(drift_usb_ptr d, uint64_t p, uint32_t c) {
  uint64_t at = drift_usb_enqueue(d, 0U, p, 0U, c);
  drift_usb_wr(d, d->doorbells, 0U);
  uint32_t expected = ((c >> 24) & 255);
  int32_t result = drift_usb_event(d, 33U, at, 0U, 0U, 0U, (nluint32_ptr)NULL);
  if((((result == 0) && (((c >> 10) & 63) != 9)) && (d->slot != expected))) {
    return drift_usb_poison(d, -5);
  }
  return result;
}
int32_t drift_usb_close(drift_usb_ptr d) {
  if(((d == ((drift_usb_ptr)NULL)) || (d->hw == ((drift_hw_ptr)NULL)))) {
    return -22;
  }
  if((d->arena.data == ((nluint8_arr0_ptr)NULL))) {
    d->state = 0U;
    return 0;
  }
  if((drift_usb_stop(d) != 0)) {
    return -110;
  }
  drift_hw_release(d->hw, (&d->arena));
  d->arena.data = ((nluint8_arr0_ptr)NULL);
  d->state = 0U;
  return 0;
}
int32_t drift_usb_open(drift_usb_ptr d, drift_hw_ptr h, uint32_t region) {
  if(((d == ((drift_usb_ptr)NULL)) || (drift_hw_valid(h) == 0))) {
    return -22;
  }
  if((d->state != 0)) {
    return -16;
  }
  d->hw = h;
  d->region = region;
  d->slot = 0U;
  d->port = 0U;
  d->kind = 0U;
  d->error = 0U;
  d->device_context = 20480U;
  d->route = 0U;
  d->parent_slot = 0U;
  d->parent_port = 0U;
  d->hub_speed = 0U;
  d->config_index = 0U;
  d->op = (uint32_t)drift_hw_read(h, region, 0U, 1U);
  if(((d->op < 0x20) || (d->op > 0x80))) {
    return -95;
  }
  uint32_t version = (drift_usb_rd(d, 0U) >> 16);
  if(((version < 0x96) || (version > 0x120))) {
    return -95;
  }
  uint32_t hcs = drift_usb_rd(d, 4U);
  d->ports = (hcs >> 24);
  if(((d->ports == 0) || ((hcs & 255) == 0))) {
    return -95;
  }
  uint32_t hcc = drift_usb_rd(d, 0x10U);
  d->context_size = 32U;
  if(((hcc & 4) != 0)) {
    d->context_size = 64U;
  }
  uint32_t scratch_word = drift_usb_rd(d, 8U);
  uint32_t scratch = (((scratch_word >> 27) & 31) | (((scratch_word >> 21) & 31) << 5));
  uint32_t arena_size = 262144U;
  if((scratch != 0)) {
    arena_size = (((270336 + (scratch * 4096)) + 65535) & 4294901760ULL);
  }
  d->runtime = (drift_usb_rd(d, 0x18U) & 4294967264ULL);
  d->doorbells = (drift_usb_rd(d, 0x14U) & 4294967292ULL);
  if(((((d->runtime < 0x20) || (d->runtime > 0x100000)) || (d->doorbells < 0x20)) || (d->doorbells > 0x100000))) {
    return -95;
  }
  if((drift_usb_stop(d) != 0)) {
    return -110;
  }
  drift_usb_wr(d, d->op, 2U);
  if(((drift_usb_waitreg(d, d->op, 2U, 0U) != 0) || (drift_usb_waitreg(d, (d->op + 4), 2048U, 0U) != 0))) {
    return -110;
  }
  if(((drift_usb_rd(d, (d->op + 8)) & 1) == 0)) {
    d->state = 0U;
    return -95;
  }
  if((drift_hw_dma(h, arena_size, 65536U, (&d->arena)) != 0)) {
    d->state = 0U;
    return -12;
  }
  if((((((d->arena.data == ((nluint8_arr0_ptr)NULL)) || (d->arena.size < arena_size)) || ((d->arena.address & 65535) != 0)) || (d->arena.address > (0xffffffffffffffffULL - (uint64_t)arena_size))) || (((hcc & 1) == 0) && ((d->arena.address > 0xffffffffULL) || ((uint64_t)arena_size > (0x100000000ULL - d->arena.address)))))) {
    if((d->arena.data != ((nluint8_arr0_ptr)NULL))) {
      drift_hw_release(h, (&d->arena));
    }
    d->arena.data = ((nluint8_arr0_ptr)NULL);
    d->state = 0U;
    return -22;
  }
  drift_usb_clear(d, 0U, arena_size);
  for(uint32_t i = 0U; i <= 31U; i += 1) {
    d->ring_offset[i] = 0U;
    d->ring_index[i] = 0U;
    d->ring_cycle[i] = 1U;
    d->pending_count[i] = 0U;
    d->completion[i] = 0;
    d->halted[i] = 0U;
  }
  d->ring_offset[0] = 4096U;
  d->ring_offset[1] = 24576U;
  d->event_index = 0U;
  d->event_cycle = 1U;
  d->tag = 0U;
  d->blocks = 0U;
  d->sector_size = 0U;
  drift_usb_put64(d->arena.data, 12288U, (d->arena.address + 8192));
  drift_usb_put(d->arena.data, 12296U, 256U);
  if((scratch != 0)) {
    drift_usb_put64(d->arena.data, 0U, (d->arena.address + 262144));
    for(uint32_t i = 0U, _end = scratch; i < _end; i += 1) {
      drift_usb_put64(d->arena.data, (262144 + (i * 8)), ((d->arena.address + 270336) + (i * 4096)));
    }
  }
  drift_usb_sync(d, 0U, arena_size, 1U);
  drift_usb_wr64(d, (d->op + 0x30), d->arena.address);
  drift_usb_wr64(d, (d->op + 0x18), ((d->arena.address + 4096) | 1));
  uint32_t slots = 1U;
  if(((hcs & 255) >= 2)) {
    slots = 2U;
  }
  drift_usb_wr(d, (d->op + 0x38), slots);
  drift_usb_wr(d, (d->runtime + 0x28), 1U);
  drift_usb_wr64(d, (d->runtime + 0x30), (d->arena.address + 12288));
  drift_usb_wr64(d, (d->runtime + 0x38), (d->arena.address + 8192));
  drift_usb_wr(d, (d->runtime + 0x20), 0U);
  drift_usb_wr(d, d->op, 1U);
  if((drift_usb_waitreg(d, (d->op + 4), 1U, 0U) != 0)) {
    return drift_usb_poison(d, -110);
  }
  d->state = 1U;
  return 0;
}
bool drift_usb_ready(drift_usb_ptr d) {
  return (((d != ((drift_usb_ptr)NULL)) && (d->state == 1)) && (d->slot != 0));
}
int32_t drift_usb_control(drift_usb_ptr d, uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, nluint8_arr0_ptr buffer, uint32_t length, nluint32_ptr received) {
  if(((((!drift_usb_ready(d)) || (received == (nluint32_ptr)NULL)) || (length > 16384)) || ((length > 0) && (buffer == ((nluint8_arr0_ptr)NULL))))) {
    return -22;
  }
  (*received) = 0U;
  bool input = ((request_type & 128) != 0);
  if((!input)) {
    for(uint32_t i = 0U, _end = length; i < _end; i += 1) {
      d->arena.data[(65536 + i)] = buffer[i];
    }
  }
  if(input) {
    drift_usb_sync(d, 65536U, length, 3U);
  } else {
    drift_usb_sync(d, 65536U, length, 1U);
  }
  uint64_t setup = (((((uint64_t)request_type | ((uint64_t)request << 8)) | ((uint64_t)value << 16)) | ((uint64_t)index << 32)) | ((uint64_t)length << 48));
  uint32_t transfer = 0U;
  if((length > 0)) {
    if(input) {
      transfer = 3U;
    } else {
      transfer = 2U;
    }
  }
  drift_usb_enqueue(d, 1U, setup, 8U, (2112 | (transfer << 16)));
  uint64_t data_at = 0U;
  if((length > 0)) {
    uint32_t direction = 0U;
    if(input) {
      direction = 65536U;
    }
    data_at = drift_usb_enqueue(d, 1U, (d->arena.address + 65536), length, ((3072 | direction) | 4));
  }
  uint32_t direction = 65536U;
  if((input && (length > 0))) {
    direction = 0U;
  }
  uint64_t last = drift_usb_enqueue(d, 1U, 0U, 0U, ((4096 | direction) | 32));
  drift_usb_wr(d, (d->doorbells + (d->slot * 4)), 1U);
  int32_t result = drift_usb_event(d, 32U, last, 1U, length, data_at, received);
  if((result != 0)) {
    return result;
  }
  if(input) {
    drift_usb_sync(d, 65536U, (*received), 2U);
    for(uint32_t i = 0U, _end = (*received); i < _end; i += 1) {
      buffer[i] = d->arena.data[(65536 + i)];
    }
  }
  return 0;
}
int32_t drift_usb_transfer(drift_usb_ptr d, uint8_t endpoint, nluint8_arr0_ptr buffer, uint32_t length, nluint32_ptr received) {
  uint32_t ep = ((uint32_t)(endpoint & 15) * 2);
  if(((endpoint & 128) != 0)) {
    ep = (ep + 1);
  }
  if(((((((((!drift_usb_ready(d)) || (received == (nluint32_ptr)NULL)) || (ep < 2)) || (ep > 31)) || (d->ring_offset[ep] == 0)) || (length > 16384)) || ((length > 0) && (buffer == ((nluint8_arr0_ptr)NULL)))) || ((length == 0) && ((endpoint & 128) != 0)))) {
    return -22;
  }
  (*received) = 0U;
  bool input = ((endpoint & 128) != 0);
  uint64_t last = 0U;
  uint32_t data_offset = (81920 + (((d->ring_offset[ep] - 28672) / 4096) * 16384));
  if((d->pending_count[ep] != 0)) {
    if((d->pending_count[ep] != length)) {
      return -16;
    }
    last = d->pending_pointer[ep];
  } else {
    if((!input)) {
      for(uint32_t i = 0U, _end = length; i < _end; i += 1) {
        d->arena.data[(data_offset + i)] = buffer[i];
      }
    }
    if(input) {
      drift_usb_sync(d, data_offset, length, 3U);
    } else {
      drift_usb_sync(d, data_offset, length, 1U);
    }
    last = drift_usb_enqueue(d, ep, (d->arena.address + data_offset), length, 1060U);
    d->pending_count[ep] = length;
    d->pending_pointer[ep] = last;
    drift_usb_wr(d, (d->doorbells + (d->slot * 4)), ep);
  }
  int32_t result = drift_usb_event(d, 32U, last, ep, length, last, received);
  if((result != -11)) {
    d->pending_count[ep] = 0U;
  }
  if((result != 0)) {
    return result;
  }
  if(input) {
    drift_usb_sync(d, data_offset, (*received), 2U);
    for(uint32_t i = 0U, _end = (*received); i < _end; i += 1) {
      buffer[i] = d->arena.data[(data_offset + i)];
    }
  }
  return 0;
}
int32_t drift_usb_recover(drift_usb_ptr d, uint8_t endpoint) {
  uint32_t ep = ((uint32_t)(endpoint & 15) * 2);
  if(((endpoint & 128) != 0)) {
    ep = (ep + 1);
  }
  if(((((!drift_usb_ready(d)) || (ep < 2)) || (ep > 31)) || (d->ring_offset[ep] == 0))) {
    return -22;
  }
  if((d->halted[ep] == 0)) {
    return 0;
  }
  int32_t result = drift_usb_command(d, 0U, ((14336 | (ep << 16)) | (d->slot << 24)));
  if((result != 0)) {
    return result;
  }
  uint64_t p = (((d->arena.address + d->ring_offset[ep]) + (d->ring_index[ep] * 16)) | (uint64_t)d->ring_cycle[ep]);
  result = drift_usb_command(d, p, ((16384 | (ep << 16)) | (d->slot << 24)));
  if((result == 0)) {
    d->halted[ep] = 0U;
    d->pending_count[ep] = 0U;
    d->completion[ep] = 0;
  }
  return result;
}
uint32_t drift_usb_little16(nluint8_arr0_ptr p, uint32_t o) {
  return ((uint32_t)p[o] | ((uint32_t)p[(o + 1)] << 8));
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
int32_t drift_usb_enumerate(drift_usb_ptr d) {
  int32_t result = drift_usb_command(d, 0U, 9216U);
  if((result != 0)) {
    return result;
  }
  if(((((d->slot == 0) || (d->slot > 2)) || ((d->parent_slot == 0) && (d->slot != 1))) || (d->slot == d->parent_slot))) {
    return drift_usb_poison(d, -5);
  }
  drift_usb_put64(d->arena.data, (d->slot * 8), (d->arena.address + d->device_context));
  drift_usb_clear(d, 16384U, 4096U);
  drift_usb_put(d->arena.data, 16388U, 3U);
  uint32_t slot = (16384 + d->context_size);
  drift_usb_put(d->arena.data, slot, ((d->route | (d->speed << 20)) | 134217728));
  drift_usb_put(d->arena.data, (slot + 4), (d->port << 16));
  if((((d->parent_slot != 0) && (d->hub_speed == 3)) && (d->speed != 3))) {
    drift_usb_put(d->arena.data, (slot + 8), (d->parent_slot | (d->parent_port << 8)));
  }
  uint32_t ep = (slot + d->context_size);
  d->max_packet = 8U;
  if((d->speed == 3)) {
    d->max_packet = 64U;
  }
  drift_usb_put(d->arena.data, (ep + 4), (38 | (d->max_packet << 16)));
  drift_usb_put64(d->arena.data, (ep + 8), ((d->arena.address + d->ring_offset[1]) | 1));
  drift_usb_put(d->arena.data, (ep + 16), 8U);
  drift_usb_sync(d, 0U, (8 * (d->slot + 1)), 1U);
  drift_usb_sync(d, 16384U, 4096U, 1U);
  result = drift_usb_command(d, (d->arena.address + 16384), (11264 | (d->slot << 24)));
  if((result != 0)) {
    return result;
  }
  nluint8_arr1024 data = (nluint8_arr1024){0};
  uint32_t count = 0U;
  result = drift_usb_control(d, 128U, 6U, 0x100U, 0U, ((nluint8_arr0_ptr)(&data.v[0])), 8U, (&count));
  if((result != 0)) {
    return result;
  }
  if((((count != 8) || (data.v[0] != 18)) || (data.v[1] != 1))) {
    return drift_usb_poison(d, -5);
  }
  uint32_t packet = (uint32_t)data.v[7];
  if(((((packet != 8) && (packet != 16)) && (packet != 32)) && (packet != 64))) {
    return drift_usb_poison(d, -95);
  }
  if((packet != d->max_packet)) {
    d->max_packet = packet;
    drift_usb_clear(d, 16384U, 4096U);
    drift_usb_put(d->arena.data, 16388U, 2U);
    drift_usb_put(d->arena.data, ((16384 + (2 * d->context_size)) + 4), (38 | (packet << 16)));
    drift_usb_sync(d, 16384U, 4096U, 1U);
    result = drift_usb_command(d, (d->arena.address + 16384), (13312 | (d->slot << 24)));
    if((result != 0)) {
      return result;
    }
  }
  result = drift_usb_control(d, 128U, 6U, 0x100U, 0U, ((nluint8_arr0_ptr)(&data.v[0])), 18U, (&count));
  if((result != 0)) {
    return result;
  }
  if(((count != 18) || (data.v[17] == 0))) {
    return drift_usb_poison(d, -5);
  }
  if((d->config_index >= (uint32_t)data.v[17])) {
    return drift_usb_poison(d, -95);
  }
  result = drift_usb_control(d, 128U, 6U, (uint16_t)(0x200 | d->config_index), 0U, ((nluint8_arr0_ptr)(&data.v[0])), 9U, (&count));
  if((result != 0)) {
    return result;
  }
  if((((count != 9) || (data.v[0] != 9)) || (data.v[1] != 2))) {
    return drift_usb_poison(d, -5);
  }
  uint32_t total = drift_usb_little16(((nluint8_arr0_ptr)(&data.v[0])), 2U);
  if((((total < 9) || (total > 1024)) || (data.v[5] == 0))) {
    return drift_usb_poison(d, -95);
  }
  result = drift_usb_control(d, 128U, 6U, (uint16_t)(0x200 | d->config_index), 0U, ((nluint8_arr0_ptr)(&d->config[0])), total, (&count));
  if((result != 0)) {
    return result;
  }
  if((count != total)) {
    return drift_usb_poison(d, -5);
  }
  d->total_config = total;
  d->configuration = (uint32_t)d->config[5];
  d->kind = 4U;
  d->endpoint_in = 0U;
  d->endpoint_out = 0U;
  drift_usb_clear(d, 16384U, 4096U);
  drift_usb_sync(d, d->device_context, d->context_size, 2U);
  for(uint32_t i = 0U, _end = d->context_size; i < _end; i += 1) {
    d->arena.data[((16384 + d->context_size) + i)] = d->arena.data[(d->device_context + i)];
  }
  uint32_t mask = 1U;
  uint32_t highest = 1U;
  uint32_t rings = 0U;
  bool selected = false;
  bool current = false;
  uint32_t alt = 0U;
  uint32_t at = 9U;
  while((at < total)) {
    if(((total - at) < 2)) {
      return drift_usb_poison(d, -5);
    }
    uint32_t n = (uint32_t)d->config[at];
    uint32_t kind = (uint32_t)d->config[(at + 1)];
    if(((n < 2) || (n > (total - at)))) {
      return drift_usb_poison(d, -5);
    }
    if((kind == 4)) {
      if((n < 9)) {
        return drift_usb_poison(d, -5);
      }
      alt = (uint32_t)d->config[(at + 3)];
      current = false;
      if(((alt == 0) && (!selected))) {
        uint32_t class = (uint32_t)d->config[(at + 5)];
        uint32_t sub = (uint32_t)d->config[(at + 6)];
        uint32_t protocol = (uint32_t)d->config[(at + 7)];
        if((((class == 3) && (sub == 1)) && ((protocol == 1) || (protocol == 2)))) {
          d->kind = protocol;
          selected = true;
          current = true;
        } else if((((class == 8) && (sub == 6)) && (protocol == 0x50))) {
          d->kind = 3U;
          selected = true;
          current = true;
        } else if(((class == 9) && (protocol <= 1))) {
          d->kind = 5U;
          selected = true;
          current = true;
        }
        if(current) {
          d->interface_number = (uint32_t)d->config[(at + 2)];
        }
      }
    } else if(((kind == 5) && (alt == 0))) {
      if(((n < 7) || (rings == 8))) {
        return drift_usb_poison(d, -95);
      }
      uint32_t address = (uint32_t)d->config[(at + 2)];
      uint32_t attributes = ((uint32_t)d->config[(at + 3)] & 3);
      uint32_t packet_1 = drift_usb_little16(((nluint8_arr0_ptr)(&d->config[0])), (at + 4));
      uint32_t endpoint = ((address & 15) * 2);
      bool input = ((address & 128) != 0);
      if(input) {
        endpoint = (endpoint + 1);
      }
      if(((((((endpoint < 2) || ((address & 0x70) != 0)) || (d->ring_offset[endpoint] != 0)) || (packet_1 == 0)) || (packet_1 > 1024)) || ((attributes != 2) && (attributes != 3)))) {
        return drift_usb_poison(d, -95);
      }
      uint32_t base = (28672 + (rings * 4096));
      rings = (rings + 1);
      d->ring_offset[endpoint] = base;
      uint32_t interval = 0U;
      if((attributes == 3)) {
        uint32_t raw = (uint32_t)d->config[(at + 6)];
        if((raw == 0)) {
          return drift_usb_poison(d, -95);
        }
        if((d->speed == 3)) {
          if((raw > 16)) {
            return drift_usb_poison(d, -95);
          }
          interval = (raw - 1);
        } else {
          interval = 3U;
          uint32_t v = 1U;
          while((v < raw)) {
            v = (v << 1);
            interval = (interval + 1);
          }
        }
      }
      uint32_t etype = attributes;
      if(input) {
        etype = (etype + 4);
      }
      uint32_t context = (16384 + ((endpoint + 1) * d->context_size));
      drift_usb_put(d->arena.data, context, (interval << 16));
      drift_usb_put(d->arena.data, (context + 4), ((6 | (etype << 3)) | (packet_1 << 16)));
      drift_usb_put64(d->arena.data, (context + 8), ((d->arena.address + base) | 1));
      uint32_t metrics = packet_1;
      if((attributes == 3)) {
        metrics = (metrics | (packet_1 << 16));
      }
      drift_usb_put(d->arena.data, (context + 16), metrics);
      mask = (mask | nelua_shl_nluint32(1U, endpoint));
      if((endpoint > highest)) {
        highest = endpoint;
      }
      if(current) {
        if(input) {
          d->endpoint_in = address;
        } else {
          d->endpoint_out = address;
        }
      }
    }
    at = (at + n);
  }
  drift_usb_put(d->arena.data, 16388U, mask);
  uint32_t slotflags = drift_usb_get(d->arena.data, (16384 + d->context_size));
  drift_usb_put(d->arena.data, (16384 + d->context_size), ((slotflags & 0x7ffffff) | (highest << 27)));
  drift_usb_sync(d, 16384U, 4096U, 1U);
  if((rings != 0)) {
    result = drift_usb_command(d, (d->arena.address + 16384), (12288 | (d->slot << 24)));
    if((result != 0)) {
      return result;
    }
  }
  result = drift_usb_control(d, 0U, 9U, (uint16_t)d->configuration, 0U, ((nluint8_arr0_ptr)NULL), 0U, (&count));
  if((result != 0)) {
    return result;
  }
  if(((d->kind == 1) || (d->kind == 2))) {
    if((d->endpoint_in == 0)) {
      return drift_usb_poison(d, -95);
    }
    result = drift_usb_control(d, 0x21U, 11U, 0U, (uint16_t)d->interface_number, ((nluint8_arr0_ptr)NULL), 0U, (&count));
    if((result != 0)) {
      return result;
    }
  } else if((d->kind == 3)) {
    if(((d->endpoint_in == 0) || (d->endpoint_out == 0))) {
      return drift_usb_poison(d, -95);
    }
  }
  return 0;
}
int32_t drift_usb_attach_root(drift_usb_ptr d, uint32_t port, uint32_t index) {
  if((((((d == ((drift_usb_ptr)NULL)) || (d->state != 1)) || (d->slot != 0)) || (port > d->ports)) || (index > 255))) {
    return -22;
  }
  d->config_index = index;
  if((port == 0)) {
    for(uint32_t i = 1U, _end = d->ports; i <= _end; i += 1) {
      if(((drift_usb_rd(d, ((d->op + 0x400) + ((i - 1) * 16))) & 1) != 0)) {
        port = i;
        break;
      }
    }
  }
  if((port == 0)) {
    return -19;
  }
  uint32_t portreg = ((d->op + 0x400) + ((port - 1) * 16));
  uint32_t value = drift_usb_rd(d, portreg);
  if(((value & 1) == 0)) {
    return -19;
  }
  if(((value & 2) == 0)) {
    drift_usb_wr(d, portreg, ((value & 0xe00) | 0x210));
    if((drift_usb_waitreg(d, portreg, 16U, 0U) != 0)) {
      return -110;
    }
    value = drift_usb_rd(d, portreg);
  }
  d->speed = ((value >> 10) & 15);
  if(((((value & 3) != 3) || (d->speed == 0)) || (d->speed > 3))) {
    return -95;
  }
  d->port = port;
  return drift_usb_enumerate(d);
}
int32_t drift_usb_attach(drift_usb_ptr d, uint32_t port) {
  return drift_usb_attach_root(d, port, 0U);
}
int32_t drift_usb_attach_config(drift_usb_ptr d, uint32_t port, uint32_t index) {
  return drift_usb_attach_root(d, port, index);
}
int32_t drift_usb_hid(drift_usb_ptr d, nluint8_arr0_ptr report, uint32_t capacity, nluint32_ptr received) {
  if(((!drift_usb_ready(d)) || ((d->kind != 1) && (d->kind != 2)))) {
    return -22;
  }
  uint32_t n = 8U;
  if((d->kind == 2)) {
    n = 3U;
  }
  if((capacity < n)) {
    return -22;
  }
  int32_t result = drift_usb_transfer(d, (uint8_t)d->endpoint_in, report, n, received);
  if(((result == 0) && ((*received) != n))) {
    return -5;
  }
  return result;
}
uint32_t drift_usb_be32(nluint8_arr0_ptr p, uint32_t o) {
  return (((((uint32_t)p[o] << 24) | ((uint32_t)p[(o + 1)] << 16)) | ((uint32_t)p[(o + 2)] << 8)) | (uint32_t)p[(o + 3)]);
}
void drift_usb_big32(nluint8_arr0_ptr p, uint32_t o, uint32_t v) {
  for(uint32_t i = 0U; i <= 3U; i += 1) {
    p[(o + i)] = (uint8_t)(nelua_shr_nluint32(v, ((3 - i) * 8)) & 255);
  }
}
int32_t drift_usb_storage_reset(drift_usb_ptr d) {
  uint32_t n = 0U;
  int32_t e = drift_usb_control(d, 0x21U, 0xffU, 0U, (uint16_t)d->interface_number, ((nluint8_arr0_ptr)NULL), 0U, (&n));
  if((e != 0)) {
    return e;
  }
  e = drift_usb_control(d, 2U, 1U, 0U, (uint16_t)d->endpoint_in, ((nluint8_arr0_ptr)NULL), 0U, (&n));
  if((e != 0)) {
    return e;
  }
  e = drift_usb_control(d, 2U, 1U, 0U, (uint16_t)d->endpoint_out, ((nluint8_arr0_ptr)NULL), 0U, (&n));
  if((e != 0)) {
    return e;
  }
  e = drift_usb_recover(d, (uint8_t)d->endpoint_in);
  if((e != 0)) {
    return e;
  }
  return drift_usb_recover(d, (uint8_t)d->endpoint_out);
}
int32_t drift_usb_bot(drift_usb_ptr d, nluint8_arr0_ptr command_bytes, uint32_t command_length, nluint8_arr0_ptr data, uint32_t length, bool input) {
  if((((((!drift_usb_ready(d)) || (d->kind != 3)) || (command_length == 0)) || (command_length > 16)) || (length > 16384))) {
    return -22;
  }
  nluint8_arr31 wrapper = (nluint8_arr31){0};
  nluint8_arr13 status = (nluint8_arr13){0};
  for(uint32_t i = 0U; i <= 30U; i += 1) {
    wrapper.v[i] = 0U;
  }
  d->tag = (d->tag + 1);
  if((d->tag == 0)) {
    d->tag = 1U;
  }
  drift_usb_put(((nluint8_arr0_ptr)(&wrapper.v[0])), 0U, 0x43425355U);
  drift_usb_put(((nluint8_arr0_ptr)(&wrapper.v[0])), 4U, d->tag);
  drift_usb_put(((nluint8_arr0_ptr)(&wrapper.v[0])), 8U, length);
  if(input) {
    wrapper.v[12] = 128U;
  }
  wrapper.v[14] = (uint8_t)command_length;
  for(uint32_t i = 0U, _end = command_length; i < _end; i += 1) {
    wrapper.v[(15 + i)] = command_bytes[i];
  }
  uint32_t got = 0U;
  int32_t result = drift_usb_transfer(d, (uint8_t)d->endpoint_out, ((nluint8_arr0_ptr)(&wrapper.v[0])), 31U, (&got));
  if(((result != 0) || (got != 31))) {
    if((d->state == 1)) {
      drift_usb_storage_reset(d);
    }
    return -5;
  }
  if((length > 0)) {
    uint32_t endpoint = d->endpoint_out;
    if(input) {
      endpoint = d->endpoint_in;
    }
    result = drift_usb_transfer(d, (uint8_t)endpoint, data, length, (&got));
    if((result != 0)) {
      if((d->state == 1)) {
        drift_usb_storage_reset(d);
      }
      return result;
    }
    if((got != length)) {
      drift_usb_storage_reset(d);
      return -5;
    }
  }
  result = drift_usb_transfer(d, (uint8_t)d->endpoint_in, ((nluint8_arr0_ptr)(&status.v[0])), 13U, (&got));
  if((result != 0)) {
    if((d->state == 1)) {
      drift_usb_storage_reset(d);
    }
    return result;
  }
  if((((((got != 13) || (drift_usb_get(((nluint8_arr0_ptr)(&status.v[0])), 0U) != 0x53425355)) || (drift_usb_get(((nluint8_arr0_ptr)(&status.v[0])), 4U) != d->tag)) || (drift_usb_get(((nluint8_arr0_ptr)(&status.v[0])), 8U) != 0)) || (status.v[12] > 1))) {
    drift_usb_storage_reset(d);
    return -5;
  }
  if((status.v[12] != 0)) {
    return -5;
  }
  return 0;
}
int32_t drift_usb_storage_info(drift_usb_ptr d, nluint64_ptr blocks, nluint32_ptr sector_size) {
  if(((((!drift_usb_ready(d)) || (blocks == (nluint64_ptr)NULL)) || (sector_size == (nluint32_ptr)NULL)) || (d->kind != 3))) {
    return -22;
  }
  nluint8_arr10 command_bytes = (nluint8_arr10){0};
  nluint8_arr18 reply = (nluint8_arr18){0};
  for(uint32_t i = 0U; i <= 9U; i += 1) {
    command_bytes.v[i] = 0U;
  }
  command_bytes.v[0] = 0x25U;
  int32_t result = drift_usb_bot(d, ((nluint8_arr0_ptr)(&command_bytes.v[0])), 10U, ((nluint8_arr0_ptr)(&reply.v[0])), 8U, true);
  if(((result != 0) && (d->state == 1))) {
    command_bytes.v[0] = 3U;
    command_bytes.v[4] = 18U;
    drift_usb_bot(d, ((nluint8_arr0_ptr)(&command_bytes.v[0])), 6U, ((nluint8_arr0_ptr)(&reply.v[0])), 18U, true);
    command_bytes.v[0] = 0x25U;
    command_bytes.v[4] = 0U;
    result = drift_usb_bot(d, ((nluint8_arr0_ptr)(&command_bytes.v[0])), 10U, ((nluint8_arr0_ptr)(&reply.v[0])), 8U, true);
  }
  if((result != 0)) {
    return result;
  }
  uint32_t last = drift_usb_be32(((nluint8_arr0_ptr)(&reply.v[0])), 0U);
  uint32_t bytes = drift_usb_be32(((nluint8_arr0_ptr)(&reply.v[0])), 4U);
  if(((last == 0xffffffffLL) || ((bytes != 512) && (bytes != 4096)))) {
    return -95;
  }
  d->blocks = ((uint64_t)last + 1);
  d->sector_size = bytes;
  (*blocks) = d->blocks;
  (*sector_size) = bytes;
  return 0;
}
int32_t drift_usb_storage_io(drift_usb_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr data, bool write) {
  if((((((((((!drift_usb_ready(d)) || (d->kind != 3)) || (d->sector_size == 0)) || (data == ((nluint8_arr0_ptr)NULL))) || (count == 0)) || (count > (16384 / d->sector_size))) || (lba >= d->blocks)) || ((uint64_t)count > (d->blocks - lba))) || (lba > 0xffffffffLL))) {
    return -22;
  }
  nluint8_arr10 command_bytes = (nluint8_arr10){0};
  for(uint32_t i = 0U; i <= 9U; i += 1) {
    command_bytes.v[i] = 0U;
  }
  command_bytes.v[0] = 0x28U;
  if(write) {
    command_bytes.v[0] = 0x2aU;
  }
  drift_usb_big32(((nluint8_arr0_ptr)(&command_bytes.v[0])), 2U, (uint32_t)lba);
  command_bytes.v[7] = (uint8_t)(count >> 8);
  command_bytes.v[8] = (uint8_t)(count & 255);
  return drift_usb_bot(d, ((nluint8_arr0_ptr)(&command_bytes.v[0])), 10U, data, (count * d->sector_size), (!write));
}
int32_t drift_usb_storage_read(drift_usb_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr data) {
  return drift_usb_storage_io(d, lba, count, data, false);
}
int32_t drift_usb_storage_write(drift_usb_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr data) {
  return drift_usb_storage_io(d, lba, count, data, true);
}
int32_t drift_usb_storage_flush(drift_usb_ptr d) {
  nluint8_arr10 command_bytes = (nluint8_arr10){0};
  for(uint32_t i = 0U; i <= 9U; i += 1) {
    command_bytes.v[i] = 0U;
  }
  command_bytes.v[0] = 0x35U;
  return drift_usb_bot(d, ((nluint8_arr0_ptr)(&command_bytes.v[0])), 10U, ((nluint8_arr0_ptr)NULL), 0U, false);
}
int32_t drift_usb_set_interface(drift_usb_ptr d, uint8_t number_1, uint8_t alternate) {
  if((!drift_usb_ready(d))) {
    return -22;
  }
  for(uint32_t i = 2U; i <= 31U; i += 1) {
    if((d->pending_count[i] != 0)) {
      return -16;
    }
  }
  bool found = false;
  bool current = false;
  bool target = false;
  uint32_t old_mask = 0U;
  uint32_t new_mask = 0U;
  uint32_t at = 9U;
  while((at < d->total_config)) {
    if(((d->total_config - at) < 2)) {
      return -5;
    }
    uint32_t n = (uint32_t)d->config[at];
    uint32_t kind = (uint32_t)d->config[(at + 1)];
    if(((n < 2) || (n > (d->total_config - at)))) {
      return -5;
    }
    if((kind == 4)) {
      if((n < 9)) {
        return -5;
      }
      current = (d->config[(at + 2)] == number_1);
      target = (current && (d->config[(at + 3)] == alternate));
      if(target) {
        found = true;
      }
    } else if(((kind == 5) && current)) {
      if((n < 7)) {
        return -5;
      }
      uint32_t address = (uint32_t)d->config[(at + 2)];
      uint32_t ep = ((address & 15) * 2);
      if(((address & 128) != 0)) {
        ep = (ep + 1);
      }
      if(((ep < 2) || (ep > 31))) {
        return -95;
      }
      if((d->ring_offset[ep] != 0)) {
        old_mask = (old_mask | nelua_shl_nluint32(1U, ep));
      }
    }
    at = (at + n);
  }
  if((!found)) {
    return -95;
  }
  drift_usb_clear(d, 16384U, 4096U);
  drift_usb_sync(d, d->device_context, d->context_size, 2U);
  for(uint32_t i = 0U, _end = d->context_size; i < _end; i += 1) {
    d->arena.data[((16384 + d->context_size) + i)] = d->arena.data[(d->device_context + i)];
  }
  for(uint32_t ep = 2U; ep <= 31U; ep += 1) {
    if(((old_mask & nelua_shl_nluint32(1U, ep)) != 0)) {
      d->ring_offset[ep] = 0U;
    }
  }
  current = false;
  at = 9U;
  while((at < d->total_config)) {
    uint32_t n = (uint32_t)d->config[at];
    uint32_t kind = (uint32_t)d->config[(at + 1)];
    if((kind == 4)) {
      current = ((d->config[(at + 2)] == number_1) && (d->config[(at + 3)] == alternate));
    } else if(((kind == 5) && current)) {
      uint32_t address = (uint32_t)d->config[(at + 2)];
      uint32_t endpoint = ((address & 15) * 2);
      bool input = ((address & 128) != 0);
      if(input) {
        endpoint = (endpoint + 1);
      }
      uint32_t attributes = ((uint32_t)d->config[(at + 3)] & 3);
      uint32_t packet = drift_usb_little16(((nluint8_arr0_ptr)(&d->config[0])), (at + 4));
      if(((((((address & 0x70) != 0) || (d->ring_offset[endpoint] != 0)) || (packet == 0)) || (packet > 1024)) || ((attributes != 2) && (attributes != 3)))) {
        return drift_usb_poison(d, -95);
      }
      uint32_t base = 0U;
      for(uint32_t i = 0U; i <= 7U; i += 1) {
        uint32_t candidate = (28672 + (i * 4096));
        bool used = false;
        for(uint32_t ep = 2U; ep <= 31U; ep += 1) {
          if((d->ring_offset[ep] == candidate)) {
            used = true;
          }
        }
        if((!used)) {
          base = candidate;
          break;
        }
      }
      if((base == 0)) {
        return drift_usb_poison(d, -95);
      }
      d->ring_offset[endpoint] = base;
      d->ring_index[endpoint] = 0U;
      d->ring_cycle[endpoint] = 1U;
      drift_usb_clear(d, base, 4096U);
      drift_usb_sync(d, base, 4096U, 1U);
      uint32_t interval = 0U;
      if((attributes == 3)) {
        uint32_t raw = (uint32_t)d->config[(at + 6)];
        if((raw == 0)) {
          return drift_usb_poison(d, -95);
        }
        if((d->speed == 3)) {
          if((raw > 16)) {
            return drift_usb_poison(d, -95);
          }
          interval = (raw - 1);
        } else {
          interval = 3U;
          uint32_t v = 1U;
          while((v < raw)) {
            v = (v << 1);
            interval = (interval + 1);
          }
        }
      }
      uint32_t etype = attributes;
      if(input) {
        etype = (etype + 4);
      }
      uint32_t context = (16384 + ((endpoint + 1) * d->context_size));
      drift_usb_put(d->arena.data, context, (interval << 16));
      drift_usb_put(d->arena.data, (context + 4), ((6 | (etype << 3)) | (packet << 16)));
      drift_usb_put64(d->arena.data, (context + 8), ((d->arena.address + base) | 1));
      uint32_t metrics = packet;
      if((attributes == 3)) {
        metrics = (metrics | (packet << 16));
      }
      drift_usb_put(d->arena.data, (context + 16), metrics);
      new_mask = (new_mask | nelua_shl_nluint32(1U, endpoint));
    }
    at = (at + n);
  }
  uint32_t highest = 1U;
  for(uint32_t ep = 2U; ep <= 31U; ep += 1) {
    if((d->ring_offset[ep] != 0)) {
      highest = ep;
    }
  }
  uint32_t slotflags = drift_usb_get(d->arena.data, (16384 + d->context_size));
  drift_usb_put(d->arena.data, (16384 + d->context_size), ((slotflags & 0x7ffffff) | (highest << 27)));
  drift_usb_put(d->arena.data, 16384U, old_mask);
  drift_usb_put(d->arena.data, 16388U, (new_mask | 1));
  drift_usb_sync(d, 16384U, 4096U, 1U);
  int32_t result = drift_usb_command(d, (d->arena.address + 16384), (12288 | (d->slot << 24)));
  if((result != 0)) {
    return result;
  }
  uint32_t got = 0U;
  return drift_usb_control(d, 1U, 11U, (uint16_t)alternate, (uint16_t)number_1, ((nluint8_arr0_ptr)NULL), 0U, (&got));
}
int32_t drift_usb_hub_child(drift_usb_ptr d, uint8_t hub_port, uint8_t speed, uint8_t hub_ports, uint8_t tt_think) {
  if(((((((((((!drift_usb_ready(d)) || (d->kind != 5)) || (d->parent_slot != 0)) || (hub_port == 0)) || (hub_port > 15)) || (hub_ports == 0)) || (hub_port > hub_ports)) || (speed < 1)) || (speed > 3)) || (tt_think > 3))) {
    return -22;
  }
  if(((drift_usb_rd(d, 4U) & 255) < 2)) {
    return -95;
  }
  for(uint32_t i = 2U; i <= 31U; i += 1) {
    if((d->pending_count[i] != 0)) {
      return -16;
    }
  }
  uint32_t parent = d->slot;
  drift_usb_clear(d, 16384U, 4096U);
  drift_usb_sync(d, d->device_context, d->context_size, 2U);
  for(uint32_t i = 0U, _end = d->context_size; i < _end; i += 1) {
    d->arena.data[((16384 + d->context_size) + i)] = d->arena.data[(d->device_context + i)];
  }
  uint32_t slot = (16384 + d->context_size);
  drift_usb_put(d->arena.data, slot, (((drift_usb_get(d->arena.data, slot) & 0x7ffffff) | 67108864) | 134217728));
  drift_usb_put(d->arena.data, (slot + 4), ((drift_usb_get(d->arena.data, (slot + 4)) & 0xffffff) | ((uint32_t)hub_ports << 24)));
  drift_usb_put(d->arena.data, (slot + 8), ((uint32_t)tt_think << 16));
  drift_usb_put(d->arena.data, 16388U, 1U);
  drift_usb_sync(d, 16384U, 4096U, 1U);
  int32_t result = drift_usb_command(d, (d->arena.address + 16384), (13312 | (parent << 24)));
  if((result != 0)) {
    return result;
  }
  uint32_t drop = 0U;
  for(uint32_t ep = 2U; ep <= 31U; ep += 1) {
    if((d->ring_offset[ep] != 0)) {
      drop = (drop | nelua_shl_nluint32(1U, ep));
    }
  }
  drift_usb_put(d->arena.data, 16384U, drop);
  drift_usb_put(d->arena.data, 16388U, 1U);
  drift_usb_sync(d, 16384U, 4096U, 1U);
  result = drift_usb_command(d, (d->arena.address + 16384), (12288 | (parent << 24)));
  if((result != 0)) {
    return result;
  }
  for(uint32_t ep = 2U; ep <= 31U; ep += 1) {
    d->ring_offset[ep] = 0U;
    d->ring_index[ep] = 0U;
    d->ring_cycle[ep] = 1U;
    d->pending_count[ep] = 0U;
    d->completion[ep] = 0;
  }
  d->hub_speed = d->speed;
  d->speed = (uint32_t)speed;
  d->parent_slot = parent;
  d->parent_port = (uint32_t)hub_port;
  d->route = (uint32_t)hub_port;
  d->slot = 0U;
  d->config_index = 0U;
  d->device_context = 221184U;
  d->ring_offset[1] = 225280U;
  d->ring_index[1] = 0U;
  d->ring_cycle[1] = 1U;
  drift_usb_clear(d, 221184U, 8192U);
  drift_usb_sync(d, 221184U, 8192U, 1U);
  return drift_usb_enumerate(d);
}

