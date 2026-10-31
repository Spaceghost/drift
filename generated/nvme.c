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
#include "drift_nvme.h"
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
typedef struct nlniltype {} nlniltype;
typedef struct nlniltype nltype;
typedef uint8_t* nluint8_arr0_ptr;
static uint32_t drift_nvme_get16(nluint8_arr0_ptr p, uint32_t n);
static uint32_t drift_nvme_get32(nluint8_arr0_ptr p, uint32_t n);
static uint64_t drift_nvme_get64(nluint8_arr0_ptr p, uint32_t n);
static void drift_nvme_put16(nluint8_arr0_ptr p, uint32_t n, uint32_t v);
static void drift_nvme_put32(nluint8_arr0_ptr p, uint32_t n, uint32_t v);
static void drift_nvme_put64(nluint8_arr0_ptr p, uint32_t n, uint64_t v);
static void drift_nvme_zero(nluint8_arr0_ptr p, uint32_t start, uint32_t size);
typedef drift_nvme* drift_nvme_ptr;
static uint32_t drift_nvme_reg(drift_nvme_ptr d, uint32_t offset);
static void drift_nvme_wr(drift_nvme_ptr d, uint32_t offset, uint32_t value);
static int32_t drift_nvme_ready(drift_nvme_ptr d, uint32_t wanted);
static void drift_nvme_release(drift_nvme_ptr d);
static int32_t drift_nvme_stop(drift_nvme_ptr d);
static int32_t drift_nvme_fatal(drift_nvme_ptr d, int32_t result);
typedef drift_dma* drift_dma_ptr;
static int32_t drift_nvme_allocate(drift_nvme_ptr d, drift_dma_ptr buffer);
static int32_t drift_nvme_command(drift_nvme_ptr d, uint32_t qid, uint32_t opcode, uint32_t nsid, uint64_t prp, uint32_t cdw10, uint32_t cdw11, uint32_t cdw12);
typedef drift_hw* drift_hw_ptr;
NELUA_CEXPORT int32_t drift_nvme_init(drift_nvme_ptr d, drift_hw_ptr bus, uint32_t region, uint32_t nsid, uint32_t timeout);
static NELUA_INLINE uint32_t nelua_shl_nluint32(uint32_t a, int32_t b);
static int32_t drift_nvme_transfer(drift_nvme_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr buffer, bool write);
NELUA_CEXPORT int32_t drift_nvme_read(drift_nvme_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr buffer);
NELUA_CEXPORT int32_t drift_nvme_write(drift_nvme_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr buffer);
NELUA_CEXPORT int32_t drift_nvme_flush(drift_nvme_ptr d);
NELUA_CEXPORT int32_t drift_nvme_close(drift_nvme_ptr d);
/* ------------------------------ DEFINITIONS ------------------------------- */
uint32_t drift_nvme_get16(nluint8_arr0_ptr p, uint32_t n) {
  return ((uint32_t)p[n] | ((uint32_t)p[(n + 1)] << 8));
}
uint32_t drift_nvme_get32(nluint8_arr0_ptr p, uint32_t n) {
  return (drift_nvme_get16(p, n) | (drift_nvme_get16(p, (n + 2)) << 16));
}
uint64_t drift_nvme_get64(nluint8_arr0_ptr p, uint32_t n) {
  return ((uint64_t)drift_nvme_get32(p, n) | ((uint64_t)drift_nvme_get32(p, (n + 4)) << 32));
}
void drift_nvme_put16(nluint8_arr0_ptr p, uint32_t n, uint32_t v) {
  uint8_t _asgntmp_1 = (uint8_t)(v & 255);
  uint8_t _asgntmp_2 = (uint8_t)((v >> 8) & 255);
  p[n] = _asgntmp_1;
  p[(n + 1)] = _asgntmp_2;
}
void drift_nvme_put32(nluint8_arr0_ptr p, uint32_t n, uint32_t v) {
  drift_nvme_put16(p, n, v);
  drift_nvme_put16(p, (n + 2), (v >> 16));
}
void drift_nvme_put64(nluint8_arr0_ptr p, uint32_t n, uint64_t v) {
  drift_nvme_put32(p, n, (uint32_t)(v & 0xffffffffLL));
  drift_nvme_put32(p, (n + 4), (uint32_t)(v >> 32));
}
void drift_nvme_zero(nluint8_arr0_ptr p, uint32_t start, uint32_t size) {
  for(uint32_t n = 0U, _end = size; n < _end; n += 1) {
    p[(start + n)] = 0U;
  }
}
uint32_t drift_nvme_reg(drift_nvme_ptr d, uint32_t offset) {
  return (uint32_t)drift_hw_read(d->hw, d->region, offset, 4U);
}
void drift_nvme_wr(drift_nvme_ptr d, uint32_t offset, uint32_t value) {
  drift_hw_write(d->hw, d->region, offset, 4U, (uint64_t)value);
}
int32_t drift_nvme_ready(drift_nvme_ptr d, uint32_t wanted) {
  uint64_t start = drift_hw_time(d->hw);
  while(true) {
    uint32_t status = drift_nvme_reg(d, 0x1cU);
    if(((status & 1) == wanted)) {
      return 0;
    }
    if(((wanted != 0) && ((status & 2) != 0))) {
      return -5;
    }
    if(((drift_hw_time(d->hw) - start) >= (uint64_t)d->reset_us)) {
      return -4;
    }
    drift_hw_wait(d->hw, 10U);
  }
}
void drift_nvme_release(drift_nvme_ptr d) {
  if((d->data.data != ((nluint8_arr0_ptr)NULL))) {
    drift_hw_release(d->hw, (&d->data));
    d->data.data = ((nluint8_arr0_ptr)NULL);
  }
  if((d->icq.data != ((nluint8_arr0_ptr)NULL))) {
    drift_hw_release(d->hw, (&d->icq));
    d->icq.data = ((nluint8_arr0_ptr)NULL);
  }
  if((d->isq.data != ((nluint8_arr0_ptr)NULL))) {
    drift_hw_release(d->hw, (&d->isq));
    d->isq.data = ((nluint8_arr0_ptr)NULL);
  }
  if((d->acq.data != ((nluint8_arr0_ptr)NULL))) {
    drift_hw_release(d->hw, (&d->acq));
    d->acq.data = ((nluint8_arr0_ptr)NULL);
  }
  if((d->asq.data != ((nluint8_arr0_ptr)NULL))) {
    drift_hw_release(d->hw, (&d->asq));
    d->asq.data = ((nluint8_arr0_ptr)NULL);
  }
}
int32_t drift_nvme_stop(drift_nvme_ptr d) {
  drift_nvme_wr(d, 0x14U, (drift_nvme_reg(d, 0x14U) & 4294967294ULL));
  int32_t result = drift_nvme_ready(d, 0U);
  if((result != 0)) {
    d->state = 3U;
    return result;
  }
  drift_nvme_release(d);
  d->state = 0U;
  return 0;
}
int32_t drift_nvme_fatal(drift_nvme_ptr d, int32_t result) {
  if((drift_nvme_stop(d) == 0)) {
    d->state = 2U;
  }
  return result;
}
int32_t drift_nvme_allocate(drift_nvme_ptr d, drift_dma_ptr buffer) {
  if((drift_hw_dma(d->hw, 4096U, 4096U, buffer) != 0)) {
    return -3;
  }
  if(((((buffer->data == ((nluint8_arr0_ptr)NULL)) || (buffer->size < 4096)) || ((buffer->address & 4095) != 0)) || (buffer->address > 0xfffffffffffff000ULL))) {
    if((buffer->data != ((nluint8_arr0_ptr)NULL))) {
      drift_hw_release(d->hw, buffer);
      buffer->data = ((nluint8_arr0_ptr)NULL);
    }
    return -3;
  }
  drift_nvme_zero(buffer->data, 0U, 4096U);
  drift_hw_sync(d->hw, buffer, 0U, 4096U, 3U);
  return 0;
}
int32_t drift_nvme_command(drift_nvme_ptr d, uint32_t qid, uint32_t opcode, uint32_t nsid, uint64_t prp, uint32_t cdw10, uint32_t cdw11, uint32_t cdw12) {
  drift_dma_ptr sq = (&d->asq);
  drift_dma_ptr cq = (&d->acq);
  if((qid != 0)) {
    drift_dma_ptr _asgntmp_1 = (&d->isq);
    drift_dma_ptr _asgntmp_2 = (&d->icq);
    sq = _asgntmp_1;
    cq = _asgntmp_2;
  }
  uint32_t slot = (d->sq_tail[qid] * 64);
  drift_nvme_zero(sq->data, slot, 64U);
  d->cid = ((d->cid + 1) & 65535);
  drift_nvme_put16(sq->data, slot, opcode);
  drift_nvme_put16(sq->data, (slot + 2), d->cid);
  drift_nvme_put32(sq->data, (slot + 4), nsid);
  drift_nvme_put64(sq->data, (slot + 24), prp);
  drift_nvme_put32(sq->data, (slot + 40), cdw10);
  drift_nvme_put32(sq->data, (slot + 44), cdw11);
  drift_nvme_put32(sq->data, (slot + 48), cdw12);
  drift_hw_sync(d->hw, sq, slot, 64U, 1U);
  d->sq_tail[qid] = ((d->sq_tail[qid] + 1) % d->depth);
  drift_nvme_wr(d, (0x1000 + ((qid * 2) * d->stride)), d->sq_tail[qid]);
  uint64_t start = drift_hw_time(d->hw);
  while(true) {
    uint32_t at = (d->cq_head[qid] * 16);
    drift_hw_sync(d->hw, cq, at, 16U, 2U);
    uint32_t status = drift_nvme_get16(cq->data, (at + 14));
    if(((status & 1) == d->phase[qid])) {
      d->last_status = (status >> 1);
      d->last_result = drift_nvme_get32(cq->data, at);
      if((((drift_nvme_get16(cq->data, (at + 12)) != d->cid) || (drift_nvme_get16(cq->data, (at + 10)) != qid)) || (drift_nvme_get16(cq->data, (at + 8)) != d->sq_tail[qid]))) {
        return drift_nvme_fatal(d, -6);
      }
      d->cq_head[qid] = (d->cq_head[qid] + 1);
      if((d->cq_head[qid] == d->depth)) {
        d->cq_head[qid] = 0U;
        d->phase[qid] = (d->phase[qid] ^ 1);
      }
      drift_nvme_wr(d, (0x1000 + (((qid * 2) + 1) * d->stride)), d->cq_head[qid]);
      if((d->last_status != 0)) {
        return -5;
      }
      return 0;
    }
    if(((drift_nvme_reg(d, 0x1cU) & 2) != 0)) {
      return drift_nvme_fatal(d, -5);
    }
    if(((drift_hw_time(d->hw) - start) >= (uint64_t)d->timeout_us)) {
      return drift_nvme_fatal(d, -4);
    }
    drift_hw_wait(d->hw, 10U);
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
int32_t drift_nvme_init(drift_nvme_ptr d, drift_hw_ptr bus, uint32_t region, uint32_t nsid, uint32_t timeout) {
  if((((((d == ((drift_nvme_ptr)NULL)) || (drift_hw_valid(bus) == 0)) || (nsid == 0)) || (nsid == 0xffffffffLL)) || (timeout == 0))) {
    return -1;
  }
  if((d->state != 0)) {
    return -7;
  }
  drift_hw_ptr _asgntmp_1 = bus;
  uint32_t _asgntmp_2 = region;
  uint32_t _asgntmp_3 = nsid;
  uint32_t _asgntmp_4 = timeout;
  d->hw = _asgntmp_1;
  d->region = _asgntmp_2;
  d->nsid = _asgntmp_3;
  d->timeout_us = _asgntmp_4;
  uint64_t cap = drift_hw_read(bus, region, 0U, 8U);
  d->reset_us = ((uint32_t)(((cap >> 24) & 255) + 1) * 500000);
  if((((((cap >> 37) & 1) == 0) || (((cap >> 48) & 15) != 0)) || ((cap & 65535) < 1))) {
    return -2;
  }
  d->depth = 8U;
  if(((cap & 65535) < 7)) {
    d->depth = (uint32_t)((cap & 65535) + 1);
  }
  d->stride = nelua_shl_nluint32(4U, (uint32_t)((cap >> 32) & 15));
  if((drift_nvme_stop(d) != 0)) {
    return -4;
  }
  uint32_t _asgntmp_5 = 0U;
  uint32_t _asgntmp_6 = 0U;
  uint32_t _asgntmp_7 = 0U;
  uint32_t _asgntmp_8 = 0U;
  d->sq_tail[0] = _asgntmp_5;
  d->sq_tail[1] = _asgntmp_6;
  d->cq_head[0] = _asgntmp_7;
  d->cq_head[1] = _asgntmp_8;
  uint32_t _asgntmp_9 = 1U;
  uint32_t _asgntmp_10 = 1U;
  uint32_t _asgntmp_11 = 0U;
  uint32_t _asgntmp_12 = 0U;
  uint32_t _asgntmp_13 = 0U;
  d->phase[0] = _asgntmp_9;
  d->phase[1] = _asgntmp_10;
  d->cid = _asgntmp_11;
  d->last_status = _asgntmp_12;
  d->last_result = _asgntmp_13;
  if((((((drift_nvme_allocate(d, (&d->asq)) != 0) || (drift_nvme_allocate(d, (&d->acq)) != 0)) || (drift_nvme_allocate(d, (&d->isq)) != 0)) || (drift_nvme_allocate(d, (&d->icq)) != 0)) || (drift_nvme_allocate(d, (&d->data)) != 0))) {
    drift_nvme_release(d);
    d->state = 2U;
    return -3;
  }
  drift_nvme_wr(d, 0xcU, 0xffffffffULL);
  drift_nvme_wr(d, 0x24U, ((d->depth - 1) | ((d->depth - 1) << 16)));
  drift_hw_write(bus, region, 0x28U, 8U, d->asq.address);
  drift_hw_write(bus, region, 0x30U, 8U, d->acq.address);
  drift_nvme_wr(d, 0x14U, 0x460001U);
  int32_t result = drift_nvme_ready(d, 1U);
  if((result != 0)) {
    return drift_nvme_fatal(d, result);
  }
  d->state = 1U;
  result = drift_nvme_command(d, 0U, 6U, 0U, d->data.address, 1U, 0U, 0U);
  if((result != 0)) {
    return drift_nvme_fatal(d, result);
  }
  drift_hw_sync(bus, (&d->data), 0U, 4096U, 2U);
  if((((((drift_nvme_get32(d->data.data, 516U) == 0) || ((d->data.data[512] & 15) > 6)) || ((d->data.data[512] >> 4) < 6)) || ((d->data.data[513] & 15) > 4)) || ((d->data.data[513] >> 4) < 4))) {
    return drift_nvme_fatal(d, -2);
  }
  drift_nvme_zero(d->data.data, 0U, 4096U);
  drift_hw_sync(bus, (&d->data), 0U, 4096U, 3U);
  result = drift_nvme_command(d, 0U, 6U, nsid, d->data.address, 0U, 0U, 0U);
  if((result != 0)) {
    return drift_nvme_fatal(d, result);
  }
  drift_hw_sync(bus, (&d->data), 0U, 4096U, 2U);
  d->blocks = drift_nvme_get64(d->data.data, 0U);
  uint32_t format = (uint32_t)(d->data.data[26] & 15);
  if(((((d->blocks == 0) || (format > (uint32_t)d->data.data[25])) || ((d->data.data[26] & 240U) != 0)) || ((d->data.data[29] & 7) != 0))) {
    return drift_nvme_fatal(d, -2);
  }
  uint32_t entry = (128 + (format * 4));
  uint32_t shift = (uint32_t)d->data.data[(entry + 2)];
  if((((drift_nvme_get16(d->data.data, entry) != 0) || (shift < 9)) || (shift > 12))) {
    return drift_nvme_fatal(d, -2);
  }
  d->block_size = nelua_shl_nluint32(1U, shift);
  result = drift_nvme_command(d, 0U, 5U, 0U, d->icq.address, (1 | ((d->depth - 1) << 16)), 1U, 0U);
  if((result != 0)) {
    return drift_nvme_fatal(d, result);
  }
  result = drift_nvme_command(d, 0U, 1U, 0U, d->isq.address, (1 | ((d->depth - 1) << 16)), 0x10001U, 0U);
  if((result != 0)) {
    return drift_nvme_fatal(d, result);
  }
  return 0;
}
int32_t drift_nvme_transfer(drift_nvme_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr buffer, bool write) {
  if((((d == ((drift_nvme_ptr)NULL)) || (buffer == ((nluint8_arr0_ptr)NULL))) || (count == 0))) {
    return -1;
  }
  if((d->state != 1)) {
    return -7;
  }
  if((((count > (4096 / (double)d->block_size)) || (lba >= d->blocks)) || ((uint64_t)count > (d->blocks - lba)))) {
    return -1;
  }
  uint32_t size = (count * d->block_size);
  if(write) {
    for(uint32_t i = 0U, _end = size; i < _end; i += 1) {
      d->data.data[i] = buffer[i];
    }
    drift_hw_sync(d->hw, (&d->data), 0U, size, 1U);
  } else {
    drift_hw_sync(d->hw, (&d->data), 0U, size, 3U);
  }
  uint32_t opcode = 2U;
  if(write) {
    opcode = 1U;
  }
  int32_t result = drift_nvme_command(d, 1U, opcode, d->nsid, d->data.address, (uint32_t)(lba & 0xffffffffLL), (uint32_t)(lba >> 32), (count - 1));
  if((result != 0)) {
    return result;
  }
  if((!write)) {
    drift_hw_sync(d->hw, (&d->data), 0U, size, 2U);
    for(uint32_t i = 0U, _end = size; i < _end; i += 1) {
      buffer[i] = d->data.data[i];
    }
  }
  return 0;
}
int32_t drift_nvme_read(drift_nvme_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr buffer) {
  return drift_nvme_transfer(d, lba, count, buffer, false);
}
int32_t drift_nvme_write(drift_nvme_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr buffer) {
  return drift_nvme_transfer(d, lba, count, buffer, true);
}
int32_t drift_nvme_flush(drift_nvme_ptr d) {
  if((d == ((drift_nvme_ptr)NULL))) {
    return -1;
  }
  if((d->state != 1)) {
    return -7;
  }
  return drift_nvme_command(d, 1U, 0U, d->nsid, 0U, 0U, 0U, 0U);
}
int32_t drift_nvme_close(drift_nvme_ptr d) {
  if((d == ((drift_nvme_ptr)NULL))) {
    return -1;
  }
  if((d->hw == ((drift_hw_ptr)NULL))) {
    return 0;
  }
  return drift_nvme_stop(d);
}

