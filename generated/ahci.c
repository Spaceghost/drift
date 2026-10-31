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
#include "drift_ahci.h"
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
/* Macro used to export C functions. */
#ifdef _WIN32
  #define NELUA_CEXPORT __declspec(dllexport)
#elif defined(__GNUC__)
  #define NELUA_CEXPORT __attribute__((visibility("default")))
#else
  #define NELUA_CEXPORT
#endif
#include <stddef.h>
/* ------------------------------ DECLARATIONS ------------------------------ */
typedef struct nlniltype {} nlniltype;
typedef struct nlniltype nltype;
typedef drift_ahci* drift_ahci_ptr;
static uint32_t drift_ahci_rd(drift_ahci_ptr d, uint32_t offset);
static void drift_ahci_wr(drift_ahci_ptr d, uint32_t offset, uint32_t value);
static uint32_t drift_ahci_reg(drift_ahci_ptr d, uint32_t offset);
typedef uint8_t* nluint8_arr0_ptr;
static uint32_t drift_ahci_load16(nluint8_arr0_ptr p, uint32_t off);
static void drift_ahci_put32(nluint8_arr0_ptr p, uint32_t off, uint32_t value);
static NELUA_INLINE uint32_t nelua_shr_nluint32(uint32_t a, int32_t b);
static void drift_ahci_clear(nluint8_arr0_ptr p, uint32_t off, uint32_t size);
static bool drift_ahci_expired(drift_ahci_ptr d, uint64_t start);
static int32_t drift_ahci_wait_bits(drift_ahci_ptr d, uint32_t offset, uint32_t mask, uint32_t want);
static int32_t drift_ahci_stop(drift_ahci_ptr d);
static int32_t drift_ahci_failed(drift_ahci_ptr d, int32_t status);
static int32_t drift_ahci_command(drift_ahci_ptr d, uint8_t opcode, uint64_t lba, uint32_t count, uint32_t bytes, bool writing);
NELUA_CEXPORT int32_t drift_ahci_close(drift_ahci_ptr d);
typedef drift_hw* drift_hw_ptr;
NELUA_CEXPORT int32_t drift_ahci_init(drift_ahci_ptr d, drift_hw_ptr bus, uint32_t region, uint32_t port, uint32_t timeout_us);
static NELUA_INLINE uint32_t nelua_shl_nluint32(uint32_t a, int32_t b);
static NELUA_INLINE uint64_t nelua_shl_nluint64(uint64_t a, int64_t b);
static int32_t drift_ahci_transfer(drift_ahci_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr buffer, bool writing);
NELUA_CEXPORT int32_t drift_ahci_read(drift_ahci_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr buffer);
NELUA_CEXPORT int32_t drift_ahci_write(drift_ahci_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr buffer);
NELUA_CEXPORT int32_t drift_ahci_flush(drift_ahci_ptr d);
/* ------------------------------ DEFINITIONS ------------------------------- */
uint32_t drift_ahci_rd(drift_ahci_ptr d, uint32_t offset) {
  return (uint32_t)drift_hw_read(d->hw, d->region, offset, 4U);
}
void drift_ahci_wr(drift_ahci_ptr d, uint32_t offset, uint32_t value) {
  drift_hw_write(d->hw, d->region, offset, 4U, (uint64_t)value);
}
uint32_t drift_ahci_reg(drift_ahci_ptr d, uint32_t offset) {
  return ((0x100 + (d->port * 0x80)) + offset);
}
uint32_t drift_ahci_load16(nluint8_arr0_ptr p, uint32_t off) {
  return ((uint32_t)p[off] | ((uint32_t)p[(off + 1)] << 8));
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
void drift_ahci_put32(nluint8_arr0_ptr p, uint32_t off, uint32_t value) {
  for(uint32_t i = 0U; i <= 3U; i += 1) {
    p[(off + i)] = (uint8_t)nelua_shr_nluint32(value, (i * 8));
  }
}
void drift_ahci_clear(nluint8_arr0_ptr p, uint32_t off, uint32_t size) {
  for(uint32_t i = 0U, _end = size; i < _end; i += 1) {
    p[(off + i)] = 0U;
  }
}
bool drift_ahci_expired(drift_ahci_ptr d, uint64_t start) {
  return ((drift_hw_time(d->hw) - start) >= (uint64_t)d->timeout_us);
}
int32_t drift_ahci_wait_bits(drift_ahci_ptr d, uint32_t offset, uint32_t mask, uint32_t want) {
  uint64_t start = drift_hw_time(d->hw);
  while(((drift_ahci_rd(d, offset) & mask) != want)) {
    if(drift_ahci_expired(d, start)) {
      return -5;
    }
    drift_hw_wait(d->hw, 10U);
  }
  return 0;
}
int32_t drift_ahci_stop(drift_ahci_ptr d) {
  uint32_t off = drift_ahci_reg(d, 0x18U);
  drift_ahci_wr(d, off, (drift_ahci_rd(d, off) & 4294967294ULL));
  if((drift_ahci_wait_bits(d, off, 0x8000U, 0U) != 0)) {
    return -5;
  }
  drift_ahci_wr(d, off, (drift_ahci_rd(d, off) & 4294967279ULL));
  return drift_ahci_wait_bits(d, off, 0x4000U, 0U);
}
int32_t drift_ahci_failed(drift_ahci_ptr d, int32_t status) {
  d->state = 2;
  return status;
}
int32_t drift_ahci_command(drift_ahci_ptr d, uint8_t opcode, uint64_t lba, uint32_t count, uint32_t bytes, bool writing) {
  if((d->state != 1)) {
    return -4;
  }
  if(((drift_ahci_rd(d, drift_ahci_reg(d, 0x38U)) != 0) || (drift_ahci_rd(d, drift_ahci_reg(d, 0x34U)) != 0))) {
    return drift_ahci_failed(d, -4);
  }
  uint64_t start = drift_hw_time(d->hw);
  while(((drift_ahci_rd(d, drift_ahci_reg(d, 0x20U)) & 0x88) != 0)) {
    if(drift_ahci_expired(d, start)) {
      return drift_ahci_failed(d, -5);
    }
    drift_hw_wait(d->hw, 10U);
  }
  nluint8_arr0_ptr p = d->memory.data;
  drift_ahci_clear(p, 0U, 32U);
  drift_ahci_clear(p, 1280U, 144U);
  uint32_t flags = 5U;
  if(writing) {
    flags = (flags | 0x40);
  }
  if((bytes != 0)) {
    flags = (flags | 0x10000);
  }
  drift_ahci_put32(p, 0U, flags);
  uint64_t table_1 = (d->memory.address + 1280);
  drift_ahci_put32(p, 8U, (uint32_t)table_1);
  drift_ahci_put32(p, 12U, (uint32_t)(table_1 >> 32));
  uint8_t _asgntmp_1 = 0x27U;
  uint8_t _asgntmp_2 = 0x80U;
  uint8_t _asgntmp_3 = opcode;
  p[1280] = (uint8_t)_asgntmp_1;
  p[1281] = (uint8_t)_asgntmp_2;
  p[1282] = _asgntmp_3;
  uint8_t _asgntmp_4 = (uint8_t)lba;
  uint8_t _asgntmp_5 = (uint8_t)(lba >> 8);
  uint8_t _asgntmp_6 = (uint8_t)(lba >> 16);
  p[1284] = _asgntmp_4;
  p[1285] = _asgntmp_5;
  p[1286] = _asgntmp_6;
  p[1287] = 0x40U;
  uint8_t _asgntmp_7 = (uint8_t)(lba >> 24);
  uint8_t _asgntmp_8 = (uint8_t)(lba >> 32);
  uint8_t _asgntmp_9 = (uint8_t)(lba >> 40);
  p[1288] = _asgntmp_7;
  p[1289] = _asgntmp_8;
  p[1290] = _asgntmp_9;
  uint8_t _asgntmp_10 = (uint8_t)count;
  uint8_t _asgntmp_11 = (uint8_t)(count >> 8);
  p[1292] = _asgntmp_10;
  p[1293] = _asgntmp_11;
  if((bytes != 0)) {
    uint64_t address = (d->memory.address + 4096);
    drift_ahci_put32(p, 1408U, (uint32_t)address);
    drift_ahci_put32(p, 1412U, (uint32_t)(address >> 32));
    drift_ahci_put32(p, 1420U, ((bytes - 1) | 0x80000000ULL));
    drift_hw_sync(d->hw, (&d->memory), 4096U, bytes, (uint32_t)(writing ? 1 : 2));
  }
  drift_hw_sync(d->hw, (&d->memory), 0U, 4096U, 1U);
  drift_ahci_wr(d, drift_ahci_reg(d, 0x10U), 0xffffffffULL);
  drift_ahci_wr(d, drift_ahci_reg(d, 0x38U), 1U);
  start = drift_hw_time(d->hw);
  while(true) {
    uint32_t status = drift_ahci_rd(d, drift_ahci_reg(d, 0x10U));
    if(((status & 0x7d000000U) != 0)) {
      return drift_ahci_failed(d, -6);
    }
    if(((drift_ahci_rd(d, drift_ahci_reg(d, 0x38U)) & 1) == 0)) {
      break;
    }
    if(drift_ahci_expired(d, start)) {
      return drift_ahci_failed(d, -5);
    }
    drift_hw_wait(d->hw, 10U);
  }
  if(((drift_ahci_rd(d, drift_ahci_reg(d, 0x20U)) & 0x89) != 0)) {
    return drift_ahci_failed(d, -6);
  }
  drift_hw_sync(d->hw, (&d->memory), 0U, 32U, 2U);
  if((bytes != 0)) {
    uint32_t got = ((((uint32_t)p[4] | ((uint32_t)p[5] << 8)) | ((uint32_t)p[6] << 16)) | ((uint32_t)p[7] << 24));
    if((got != bytes)) {
      return drift_ahci_failed(d, -6);
    }
    if((!writing)) {
      drift_hw_sync(d->hw, (&d->memory), 4096U, bytes, 2U);
    }
  }
  return 0;
}
int32_t drift_ahci_close(drift_ahci_ptr d) {
  if((d == ((drift_ahci_ptr)NULL))) {
    return -1;
  }
  if((d->state == 0)) {
    return 0;
  }
  if((drift_hw_valid(d->hw) == 0)) {
    return -1;
  }
  if((drift_ahci_stop(d) != 0)) {
    return drift_ahci_failed(d, -5);
  }
  if((d->memory.data != ((nluint8_arr0_ptr)NULL))) {
    drift_hw_release(d->hw, (&d->memory));
    d->memory.data = ((nluint8_arr0_ptr)NULL);
    uint64_t _asgntmp_1 = 0U;
    uint32_t _asgntmp_2 = 0U;
    uint64_t _asgntmp_3 = 0U;
    d->memory.address = _asgntmp_1;
    d->memory.size = _asgntmp_2;
    d->memory.token = _asgntmp_3;
  }
  int32_t _asgntmp_4 = 0;
  uint64_t _asgntmp_5 = 0U;
  uint32_t _asgntmp_6 = 0U;
  uint32_t _asgntmp_7 = 0U;
  d->state = (int32_t)_asgntmp_4;
  d->sectors = _asgntmp_5;
  d->sector_size = _asgntmp_6;
  d->flush_command = _asgntmp_7;
  return 0;
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
uint64_t nelua_shl_nluint64(uint64_t a, int64_t b) {
  if(NELUA_LIKELY(b >= 0 && b < 64)) {
    return ((uint64_t)a) << b;
  } else if(NELUA_UNLIKELY(b < 0 && b > -64)) {
    return (uint64_t)a >> -b;
  } else {
    return 0;
  }
}
int32_t drift_ahci_init(drift_ahci_ptr d, drift_hw_ptr bus, uint32_t region, uint32_t port, uint32_t timeout_us) {
  if(((((d == ((drift_ahci_ptr)NULL)) || (drift_hw_valid(bus) == 0)) || (port >= 32)) || (timeout_us == 0))) {
    return -1;
  }
  if((d->state != 0)) {
    return -4;
  }
  drift_hw_ptr _asgntmp_1 = bus;
  uint32_t _asgntmp_2 = region;
  uint32_t _asgntmp_3 = port;
  uint32_t _asgntmp_4 = timeout_us;
  d->hw = _asgntmp_1;
  d->region = _asgntmp_2;
  d->port = _asgntmp_3;
  d->timeout_us = _asgntmp_4;
  d->cap = drift_ahci_rd(d, 0U);
  if(((port > (d->cap & 31)) || ((drift_ahci_rd(d, 0xcU) & nelua_shl_nluint32(1U, port)) == 0))) {
    return -3;
  }
  if(((drift_ahci_rd(d, 0x24U) & 1) != 0)) {
    drift_ahci_wr(d, 0x28U, (drift_ahci_rd(d, 0x28U) | 2));
    if((drift_ahci_wait_bits(d, 0x28U, 0x11U, 0U) != 0)) {
      return -5;
    }
  }
  drift_ahci_wr(d, 4U, (drift_ahci_rd(d, 4U) | 0x80000000ULL));
  if(((drift_ahci_rd(d, 4U) & 0x80000000ULL) == 0)) {
    return -2;
  }
  d->state = 2;
  if((drift_ahci_stop(d) != 0)) {
    return -5;
  }
  if(((drift_ahci_rd(d, drift_ahci_reg(d, 0x28U)) & 0xf0f) != 0x103)) {
    return -3;
  }
  if((drift_hw_dma(bus, 69632U, 4096U, (&d->memory)) != 0)) {
    return -7;
  }
  if((((((d->memory.data == ((nluint8_arr0_ptr)NULL)) || (d->memory.size < 69632)) || ((d->memory.address & 4095) != 0)) || (d->memory.address > 18446744073709481984ULL)) || (((d->cap & 0x80000000ULL) == 0) && ((d->memory.address + 69631) > 0xffffffffULL)))) {
    if((d->memory.data != ((nluint8_arr0_ptr)NULL))) {
      drift_hw_release(bus, (&d->memory));
    }
    d->memory.data = ((nluint8_arr0_ptr)NULL);
    return -7;
  }
  drift_ahci_clear(d->memory.data, 0U, 69632U);
  drift_hw_sync(bus, (&d->memory), 0U, 69632U, 3U);
  drift_ahci_wr(d, drift_ahci_reg(d, 0U), (uint32_t)d->memory.address);
  drift_ahci_wr(d, drift_ahci_reg(d, 4U), (uint32_t)(d->memory.address >> 32));
  drift_ahci_wr(d, drift_ahci_reg(d, 8U), (uint32_t)(d->memory.address + 1024));
  drift_ahci_wr(d, drift_ahci_reg(d, 12U), (uint32_t)((d->memory.address + 1024) >> 32));
  drift_ahci_wr(d, drift_ahci_reg(d, 0x14U), 0U);
  drift_ahci_wr(d, drift_ahci_reg(d, 0x30U), 0xffffffffULL);
  drift_ahci_wr(d, drift_ahci_reg(d, 0x10U), 0xffffffffULL);
  drift_ahci_wr(d, drift_ahci_reg(d, 0x18U), (drift_ahci_rd(d, drift_ahci_reg(d, 0x18U)) | 0x10));
  drift_ahci_wr(d, drift_ahci_reg(d, 0x18U), (drift_ahci_rd(d, drift_ahci_reg(d, 0x18U)) | 1));
  uint64_t start = drift_hw_time(bus);
  while(((drift_ahci_rd(d, drift_ahci_reg(d, 0x24U)) == 0xffffffffULL) || (drift_ahci_rd(d, drift_ahci_reg(d, 0x24U)) == 0))) {
    if(drift_ahci_expired(d, start)) {
      return -5;
    }
    drift_hw_wait(bus, 10U);
  }
  if((drift_ahci_rd(d, drift_ahci_reg(d, 0x24U)) != 0x101)) {
    return -2;
  }
  d->state = 1;
  int32_t result = drift_ahci_command(d, 0xecU, 0U, 0U, 512U, false);
  if((result != 0)) {
    return result;
  }
  nluint8_arr0_ptr p = d->memory.data;
  uint32_t word83 = drift_ahci_load16(p, 4262U);
  if(((word83 & 0xc400) != 0x4400)) {
    return drift_ahci_failed(d, -2);
  }
  uint64_t capacity = 0U;
  for(uint32_t i = 0U; i <= 3U; i += 1) {
    capacity = (capacity | nelua_shl_nluint64((uint64_t)drift_ahci_load16(p, (4296 + (i * 2))), (i * 16)));
  }
  if(((capacity == 0) || (capacity > 0x1000000000000ULL))) {
    return drift_ahci_failed(d, -2);
  }
  d->sector_size = 512U;
  uint32_t size_word = drift_ahci_load16(p, 4308U);
  if((((size_word & 0xc000) == 0x4000) && ((size_word & 0x1000) != 0))) {
    uint32_t words = (drift_ahci_load16(p, 4330U) | (drift_ahci_load16(p, 4332U) << 16));
    if((words != 2048)) {
      return drift_ahci_failed(d, -2);
    }
    d->sector_size = 4096U;
  }
  d->flush_command = 0U;
  if(((word83 & 0x2000) != 0)) {
    d->flush_command = 0xeaU;
  } else if(((word83 & 0x1000) != 0)) {
    d->flush_command = 0xe7U;
  }
  d->sectors = capacity;
  return 0;
}
int32_t drift_ahci_transfer(drift_ahci_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr buffer, bool writing) {
  if((((d == ((drift_ahci_ptr)NULL)) || (buffer == ((nluint8_arr0_ptr)NULL))) || (count == 0))) {
    return -1;
  }
  if((d->state != 1)) {
    return -4;
  }
  if((((count > (65536 / (double)d->sector_size)) || (lba >= d->sectors)) || ((uint64_t)count > (d->sectors - lba)))) {
    return -1;
  }
  uint32_t bytes = (count * d->sector_size);
  if(writing) {
    for(uint32_t i = 0U, _end = bytes; i < _end; i += 1) {
      d->memory.data[(4096 + i)] = buffer[i];
    }
  }
  int32_t result = drift_ahci_command(d, (uint8_t)(writing ? 0x35 : 0x25), lba, count, bytes, writing);
  if(((result == 0) && (!writing))) {
    for(uint32_t i = 0U, _end = bytes; i < _end; i += 1) {
      buffer[i] = d->memory.data[(4096 + i)];
    }
  }
  return result;
}
int32_t drift_ahci_read(drift_ahci_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr buffer) {
  return drift_ahci_transfer(d, lba, count, buffer, false);
}
int32_t drift_ahci_write(drift_ahci_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr buffer) {
  return drift_ahci_transfer(d, lba, count, buffer, true);
}
int32_t drift_ahci_flush(drift_ahci_ptr d) {
  if((d == ((drift_ahci_ptr)NULL))) {
    return -1;
  }
  if((d->state != 1)) {
    return -4;
  }
  if((d->flush_command == 0)) {
    return -2;
  }
  return drift_ahci_command(d, (uint8_t)d->flush_command, 0U, 0U, 0U, false);
}

