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
#include "drift_ethernet.h"
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
#include <stddef.h>
/* Macro used to export C functions. */
#ifdef _WIN32
  #define NELUA_CEXPORT __declspec(dllexport)
#elif defined(__GNUC__)
  #define NELUA_CEXPORT __attribute__((visibility("default")))
#else
  #define NELUA_CEXPORT
#endif
/* ------------------------------ DECLARATIONS ------------------------------ */
typedef struct nlniltype {} nlniltype;
typedef struct nlniltype nltype;
typedef drift_ethernet* drift_ethernet_ptr;
static uint32_t drift_ethernet_read(drift_ethernet_ptr d, uint32_t off, uint32_t width);
static void drift_ethernet_write(drift_ethernet_ptr d, uint32_t off, uint32_t width, uint32_t value);
typedef uint8_t* nluint8_arr0_ptr;
static uint32_t drift_ethernet_get16(nluint8_arr0_ptr p, uint32_t at);
static void drift_ethernet_put16(nluint8_arr0_ptr p, uint32_t at, uint32_t value);
static void drift_ethernet_put64(nluint8_arr0_ptr p, uint32_t at, uint64_t value);
static NELUA_INLINE uint64_t nelua_shr_nluint64(uint64_t a, int64_t b);
typedef drift_dma* drift_dma_ptr;
static void drift_ethernet_clear(drift_dma_ptr buffer);
static bool drift_ethernet_expired(drift_ethernet_ptr d, uint64_t start);
static int32_t drift_ethernet_reset(drift_ethernet_ptr d);
static void drift_ethernet_free_buffers(drift_ethernet_ptr d);
static bool drift_ethernet_allocate(drift_ethernet_ptr d, drift_dma_ptr b, uint32_t size, uint32_t alignment);
typedef drift_hw* drift_hw_ptr;
NELUA_CEXPORT int32_t drift_eth_init(drift_ethernet_ptr d, drift_hw_ptr bus, uint32_t region, uint32_t kind, uint32_t timeout_us);
static NELUA_INLINE uint32_t nelua_shr_nluint32(uint32_t a, int32_t b);
NELUA_CEXPORT int32_t drift_eth_close(drift_ethernet_ptr d);
static int32_t drift_ethernet_stop_fault(drift_ethernet_ptr d, int32_t error_1);
NELUA_CEXPORT int32_t drift_eth_tx(drift_ethernet_ptr d, nluint8_arr0_ptr frame, uint32_t length);
static void drift_ethernet_recycle(drift_ethernet_ptr d);
typedef uint32_t* nluint32_arr0_ptr;
NELUA_CEXPORT int32_t drift_eth_rx(drift_ethernet_ptr d, nluint8_arr0_ptr frame, uint32_t capacity, nluint32_arr0_ptr length);
/* ------------------------------ DEFINITIONS ------------------------------- */
uint32_t drift_ethernet_read(drift_ethernet_ptr d, uint32_t off, uint32_t width) {
  return (uint32_t)drift_hw_read(d->hw, d->region, off, width);
}
void drift_ethernet_write(drift_ethernet_ptr d, uint32_t off, uint32_t width, uint32_t value) {
  drift_hw_write(d->hw, d->region, off, width, (uint64_t)value);
}
uint32_t drift_ethernet_get16(nluint8_arr0_ptr p, uint32_t at) {
  return ((uint32_t)p[at] | ((uint32_t)p[(at + 1)] << 8));
}
void drift_ethernet_put16(nluint8_arr0_ptr p, uint32_t at, uint32_t value) {
  uint8_t _asgntmp_1 = (uint8_t)(value & 255);
  uint8_t _asgntmp_2 = (uint8_t)((value >> 8) & 255);
  p[at] = _asgntmp_1;
  p[(at + 1)] = _asgntmp_2;
}
uint64_t nelua_shr_nluint64(uint64_t a, int64_t b) {
  if(NELUA_LIKELY(b >= 0 && b < 64)) {
    return (uint64_t)a >> b;
  } else if(NELUA_UNLIKELY(b < 0 && b > -64)) {
    return (uint64_t)a << -b;
  } else {
    return 0;
  }
}
void drift_ethernet_put64(nluint8_arr0_ptr p, uint32_t at, uint64_t value) {
  for(uint32_t i = 0U; i <= 7U; i += 1) {
    p[(at + i)] = (uint8_t)(nelua_shr_nluint64(value, (i * 8)) & 255);
  }
}
void drift_ethernet_clear(drift_dma_ptr buffer) {
  for(uint32_t i = 0U, _end = buffer->size; i < _end; i += 1) {
    buffer->data[i] = 0U;
  }
}
bool drift_ethernet_expired(drift_ethernet_ptr d, uint64_t start) {
  return ((drift_hw_time(d->hw) - start) >= d->timeout_us);
}
int32_t drift_ethernet_reset(drift_ethernet_ptr d) {
  uint64_t start = drift_hw_time(d->hw);
  if((d->kind == 1)) {
    drift_ethernet_write(d, 0xd8U, 4U, 0xffffffffULL);
    drift_ethernet_write(d, 0x100U, 4U, 0U);
    drift_ethernet_write(d, 0x400U, 4U, 0U);
    drift_ethernet_write(d, 0U, 4U, (drift_ethernet_read(d, 0U, 4U) | 0x4000000));
    while(((drift_ethernet_read(d, 0U, 4U) & 0x4000000) != 0)) {
      if(drift_ethernet_expired(d, start)) {
        d->state = -1;
        return -4;
      }
      drift_hw_wait(d->hw, 10U);
    }
    drift_hw_wait(d->hw, 1000U);
    drift_ethernet_write(d, 0xd8U, 4U, 0xffffffffULL);
  } else {
    drift_ethernet_write(d, 0x3cU, 2U, 0U);
    drift_ethernet_write(d, 0x37U, 1U, 0x10U);
    while(((drift_ethernet_read(d, 0x37U, 1U) & 0x10) != 0)) {
      if(drift_ethernet_expired(d, start)) {
        d->state = -1;
        return -4;
      }
      drift_hw_wait(d->hw, 10U);
    }
  }
  return 0;
}
void drift_ethernet_free_buffers(drift_ethernet_ptr d) {
  if((d->rx_ring.data != ((nluint8_arr0_ptr)NULL))) {
    drift_hw_release(d->hw, (&d->rx_ring));
  }
  if((d->tx_ring.data != ((nluint8_arr0_ptr)NULL))) {
    drift_hw_release(d->hw, (&d->tx_ring));
  }
  if((d->rx_data.data != ((nluint8_arr0_ptr)NULL))) {
    drift_hw_release(d->hw, (&d->rx_data));
  }
  if((d->tx_data.data != ((nluint8_arr0_ptr)NULL))) {
    drift_hw_release(d->hw, (&d->tx_data));
  }
  drift_dma _asgntmp_1 = (drift_dma){0};
  drift_dma _asgntmp_2 = (drift_dma){0};
  drift_dma _asgntmp_3 = (drift_dma){0};
  drift_dma _asgntmp_4 = (drift_dma){0};
  d->rx_ring = _asgntmp_1;
  d->tx_ring = _asgntmp_2;
  d->rx_data = _asgntmp_3;
  d->tx_data = _asgntmp_4;
}
bool drift_ethernet_allocate(drift_ethernet_ptr d, drift_dma_ptr b, uint32_t size, uint32_t alignment) {
  if((drift_hw_dma(d->hw, size, alignment, b) != 0)) {
    return false;
  }
  if(((((b->data == ((nluint8_arr0_ptr)NULL)) || (b->size < size)) || ((b->address & (alignment - 1)) != 0)) || (b->address > (0xffffffffffffffffULL - size)))) {
    return false;
  }
  if(((d->kind == 2) && ((b->address + size) > 0x100000000ULL))) {
    return false;
  }
  drift_ethernet_clear(b);
  return true;
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
int32_t drift_eth_init(drift_ethernet_ptr d, drift_hw_ptr bus, uint32_t region, uint32_t kind, uint32_t timeout_us) {
  if((((d == ((drift_ethernet_ptr)NULL)) || (drift_hw_valid(bus) == 0)) || (timeout_us == 0))) {
    return -1;
  }
  if(((kind != 1) && (kind != 2))) {
    return -7;
  }
  if((((((d->state != 0) || (d->rx_ring.data != ((nluint8_arr0_ptr)NULL))) || (d->tx_ring.data != ((nluint8_arr0_ptr)NULL))) || (d->rx_data.data != ((nluint8_arr0_ptr)NULL))) || (d->tx_data.data != ((nluint8_arr0_ptr)NULL)))) {
    return -2;
  }
  drift_hw_ptr _asgntmp_1 = bus;
  uint32_t _asgntmp_2 = region;
  uint32_t _asgntmp_3 = kind;
  uint32_t _asgntmp_4 = timeout_us;
  d->hw = _asgntmp_1;
  d->region = _asgntmp_2;
  d->kind = _asgntmp_3;
  d->timeout_us = _asgntmp_4;
  uint32_t _asgntmp_5 = 0U;
  uint32_t _asgntmp_6 = 0U;
  int32_t _asgntmp_7 = 2;
  d->rx_next = _asgntmp_5;
  d->tx_next = _asgntmp_6;
  d->state = (int32_t)_asgntmp_7;
  if((drift_ethernet_reset(d) != 0)) {
    return -4;
  }
  if((kind == 1)) {
    if(((((!drift_ethernet_allocate(d, (&d->rx_ring), 256U, 128U)) || (!drift_ethernet_allocate(d, (&d->tx_ring), 256U, 128U))) || (!drift_ethernet_allocate(d, (&d->rx_data), 32768U, 2048U))) || (!drift_ethernet_allocate(d, (&d->tx_data), 2048U, 16U)))) {
      drift_ethernet_free_buffers(d);
      d->state = 0;
      return -3;
    }
    uint32_t low = drift_ethernet_read(d, 0x5400U, 4U);
    uint32_t high = drift_ethernet_read(d, 0x5404U, 4U);
    for(uint32_t i = 0U; i <= 3U; i += 1) {
      d->mac[i] = (uint8_t)(nelua_shr_nluint32(low, (i * 8)) & 255);
    }
    uint8_t _asgntmp_8 = (uint8_t)(high & 255);
    uint8_t _asgntmp_9 = (uint8_t)((high >> 8) & 255);
    d->mac[4] = _asgntmp_8;
    d->mac[5] = _asgntmp_9;
    for(uint32_t i = 0U; i <= 15U; i += 1) {
      drift_ethernet_put64(d->rx_ring.data, (i * 16), (d->rx_data.address + ((uint64_t)i * 2048)));
      d->tx_ring.data[((i * 16) + 12)] = 1U;
    }
    drift_hw_sync(d->hw, (&d->rx_data), 0U, 32768U, 3U);
    drift_hw_sync(d->hw, (&d->rx_ring), 0U, 256U, 3U);
    drift_hw_sync(d->hw, (&d->tx_ring), 0U, 256U, 3U);
    drift_ethernet_write(d, 0U, 4U, (drift_ethernet_read(d, 0U, 4U) | 0x40));
    for(uint32_t i = 0U; i <= 127U; i += 1) {
      drift_ethernet_write(d, (0x5200 + (i * 4)), 4U, 0U);
    }
    drift_ethernet_write(d, 0x5404U, 4U, (high | 0x80000000LL));
    drift_ethernet_write(d, 0x2800U, 4U, (uint32_t)(d->rx_ring.address & 0xffffffffLL));
    drift_ethernet_write(d, 0x2804U, 4U, (uint32_t)(d->rx_ring.address >> 32));
    drift_ethernet_write(d, 0x2808U, 4U, 256U);
    drift_ethernet_write(d, 0x2810U, 4U, 0U);
    drift_ethernet_write(d, 0x2818U, 4U, 15U);
    drift_ethernet_write(d, 0x3800U, 4U, (uint32_t)(d->tx_ring.address & 0xffffffffLL));
    drift_ethernet_write(d, 0x3804U, 4U, (uint32_t)(d->tx_ring.address >> 32));
    drift_ethernet_write(d, 0x3808U, 4U, 256U);
    drift_ethernet_write(d, 0x3810U, 4U, 0U);
    drift_ethernet_write(d, 0x3818U, 4U, 0U);
    drift_ethernet_write(d, 0x410U, 4U, 0x60200aU);
    drift_ethernet_write(d, 0x400U, 4U, 0x400faU);
    drift_ethernet_write(d, 0x100U, 4U, 0x4008002U);
  } else {
    if(((!drift_ethernet_allocate(d, (&d->rx_data), 10256U, 256U)) || (!drift_ethernet_allocate(d, (&d->tx_data), 8192U, 256U)))) {
      drift_ethernet_free_buffers(d);
      d->state = 0;
      return -3;
    }
    for(uint32_t i = 0U; i <= 5U; i += 1) {
      d->mac[i] = (uint8_t)drift_ethernet_read(d, i, 1U);
    }
    drift_hw_sync(d->hw, (&d->rx_data), 0U, 10256U, 3U);
    drift_ethernet_write(d, 0x52U, 1U, 0U);
    drift_ethernet_write(d, 0x30U, 4U, (uint32_t)d->rx_data.address);
    for(uint32_t i = 0U; i <= 3U; i += 1) {
      drift_ethernet_write(d, (0x20 + (i * 4)), 4U, (uint32_t)(d->tx_data.address + ((uint64_t)i * 2048)));
    }
    drift_ethernet_write(d, 0x38U, 2U, 0xfff0U);
    drift_ethernet_write(d, 0x3eU, 2U, 0xffffU);
    drift_ethernet_write(d, 0x37U, 1U, 0xcU);
    drift_ethernet_write(d, 0x40U, 4U, 0x3000700U);
    drift_ethernet_write(d, 0x8U, 4U, 0xffffffffULL);
    drift_ethernet_write(d, 0xcU, 4U, 0xffffffffULL);
    drift_ethernet_write(d, 0x44U, 4U, 0x78eU);
  }
  d->state = 1;
  return 0;
}
int32_t drift_eth_close(drift_ethernet_ptr d) {
  if((d == ((drift_ethernet_ptr)NULL))) {
    return -1;
  }
  if((d->state == 0)) {
    return 0;
  }
  if((drift_hw_valid(d->hw) == 0)) {
    return -1;
  }
  if((drift_ethernet_reset(d) != 0)) {
    return -4;
  }
  drift_ethernet_free_buffers(d);
  d->state = 0;
  return 0;
}
int32_t drift_ethernet_stop_fault(drift_ethernet_ptr d, int32_t error_1) {
  drift_ethernet_reset(d);
  d->state = -1;
  return error_1;
}
int32_t drift_eth_tx(drift_ethernet_ptr d, nluint8_arr0_ptr frame, uint32_t length) {
  if(((((d == ((drift_ethernet_ptr)NULL)) || (frame == ((nluint8_arr0_ptr)NULL))) || (length < 14)) || (length > 1518))) {
    return -1;
  }
  if((d->state != 1)) {
    return -2;
  }
  int64_t send = (int64_t)length;
  if((send < 60)) {
    send = 60;
  }
  uint32_t slot = d->tx_next;
  uint32_t at = 0U;
  if((d->kind == 2)) {
    at = (slot * 2048);
  }
  for(uint32_t i = 0U, _end = (uint32_t)send; i < _end; i += 1) {
    if((i < length)) {
      d->tx_data.data[(at + i)] = frame[i];
    } else {
      d->tx_data.data[(at + i)] = 0U;
    }
  }
  drift_hw_sync(d->hw, (&d->tx_data), at, (uint32_t)send, 1U);
  if((d->kind == 1)) {
    uint32_t desc = (slot * 16);
    drift_ethernet_put64(d->tx_ring.data, desc, d->tx_data.address);
    for(uint32_t i = 8U; i <= 15U; i += 1) {
      d->tx_ring.data[(desc + i)] = 0U;
    }
    drift_ethernet_put16(d->tx_ring.data, (desc + 8), (uint32_t)send);
    d->tx_ring.data[(desc + 11)] = 0xbU;
    drift_hw_sync(d->hw, (&d->tx_ring), desc, 16U, 1U);
    drift_ethernet_write(d, 0x3818U, 4U, ((slot + 1) & 15));
    uint64_t start = drift_hw_time(d->hw);
    while(true) {
      drift_hw_sync(d->hw, (&d->tx_ring), desc, 16U, 2U);
      if(((d->tx_ring.data[(desc + 12)] & 1) != 0)) {
        break;
      }
      if(drift_ethernet_expired(d, start)) {
        return drift_ethernet_stop_fault(d, -4);
      }
      drift_hw_wait(d->hw, 10U);
    }
    if(((d->tx_ring.data[(desc + 12)] & 0xe) != 0)) {
      return drift_ethernet_stop_fault(d, -5);
    }
    d->tx_next = ((slot + 1) & 15);
  } else {
    drift_ethernet_write(d, (0x10 + (slot * 4)), 4U, (uint32_t)send);
    uint64_t start = drift_hw_time(d->hw);
    while(true) {
      uint32_t status = drift_ethernet_read(d, (0x10 + (slot * 4)), 4U);
      if(((status & 0xc0004000LL) != 0)) {
        return drift_ethernet_stop_fault(d, -5);
      }
      if(((status & 0x8000) != 0)) {
        break;
      }
      if(drift_ethernet_expired(d, start)) {
        return drift_ethernet_stop_fault(d, -4);
      }
      drift_hw_wait(d->hw, 10U);
    }
    d->tx_next = ((slot + 1) & 3);
  }
  return 0;
}
void drift_ethernet_recycle(drift_ethernet_ptr d) {
  uint32_t desc = (d->rx_next * 16);
  drift_ethernet_put64(d->rx_ring.data, desc, (d->rx_data.address + ((uint64_t)d->rx_next * 2048)));
  for(uint32_t i = 8U; i <= 15U; i += 1) {
    d->rx_ring.data[(desc + i)] = 0U;
  }
  drift_hw_sync(d->hw, (&d->rx_ring), desc, 16U, 1U);
  drift_ethernet_write(d, 0x2818U, 4U, d->rx_next);
  d->rx_next = ((d->rx_next + 1) & 15);
}
int32_t drift_eth_rx(drift_ethernet_ptr d, nluint8_arr0_ptr frame, uint32_t capacity, nluint32_arr0_ptr length) {
  if((((d == ((drift_ethernet_ptr)NULL)) || (frame == ((nluint8_arr0_ptr)NULL))) || (length == ((nluint32_arr0_ptr)NULL)))) {
    return -1;
  }
  length[0] = 0U;
  if((d->state != 1)) {
    return -2;
  }
  if((d->kind == 1)) {
    uint32_t desc = (d->rx_next * 16);
    drift_hw_sync(d->hw, (&d->rx_ring), desc, 16U, 2U);
    uint8_t status = d->rx_ring.data[(desc + 12)];
    if(((status & 1) == 0)) {
      return 1;
    }
    uint32_t size = drift_ethernet_get16(d->rx_ring.data, (desc + 8));
    if((((((status & 2) == 0) || (d->rx_ring.data[(desc + 13)] != 0)) || (size < 14)) || (size > 1518))) {
      drift_ethernet_recycle(d);
      return -5;
    }
    if((size > capacity)) {
      return -6;
    }
    uint32_t offset = (d->rx_next * 2048);
    drift_hw_sync(d->hw, (&d->rx_data), offset, size, 2U);
    for(uint32_t i = 0U, _end = size; i < _end; i += 1) {
      frame[i] = d->rx_data.data[(offset + i)];
    }
    length[0] = size;
    drift_ethernet_recycle(d);
  } else {
    if(((drift_ethernet_read(d, 0x3eU, 2U) & 0x8050) != 0)) {
      return drift_ethernet_stop_fault(d, -5);
    }
    if(((drift_ethernet_read(d, 0x37U, 1U) & 1) != 0)) {
      return 1;
    }
    drift_hw_sync(d->hw, (&d->rx_data), 0U, d->rx_data.size, 2U);
    uint32_t at = d->rx_next;
    uint32_t flags = drift_ethernet_get16(d->rx_data.data, at);
    uint32_t size = drift_ethernet_get16(d->rx_data.data, (at + 2));
    if((((((flags & 1) == 0) || ((flags & 0x3e) != 0)) || (size < 18)) || (size > 1522))) {
      return drift_ethernet_stop_fault(d, -5);
    }
    if(((size - 4) > capacity)) {
      return -6;
    }
    for(uint32_t i = 0U, _end = (size - 4); i < _end; i += 1) {
      frame[i] = d->rx_data.data[((at + 4) + i)];
    }
    length[0] = (size - 4);
    d->rx_next = (((at + size) + 7) & 0x1ffc);
    drift_ethernet_write(d, 0x38U, 2U, ((d->rx_next - 16) & 0xffff));
    drift_ethernet_write(d, 0x3eU, 2U, 0x1U);
  }
  return 0;
}

