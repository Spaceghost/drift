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
#include <stdbool.h>
#include "drift.h"
/* Macro used to export C functions. */
#ifdef _WIN32
  #define NELUA_CEXPORT __declspec(dllexport)
#elif defined(__GNUC__)
  #define NELUA_CEXPORT __attribute__((visibility("default")))
#else
  #define NELUA_CEXPORT
#endif
#include <stddef.h>
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
typedef uint8_t* nluint8_arr0_ptr;
static uint32_t drift_le32(nluint8_arr0_ptr p, uint32_t n);
static bool drift_span(uint32_t total, uint32_t start, uint32_t count);
typedef drift_vm* drift_vm_ptr;
static int32_t drift_trap(drift_vm_ptr v, int32_t error_1);
NELUA_CEXPORT int32_t drift_load(drift_vm_ptr v, nluint8_arr0_ptr p, uint32_t size, uint32_t grants, uint64_t instance);
NELUA_CEXPORT int32_t drift_run(drift_vm_ptr v, uint32_t fuel);
static NELUA_INLINE uint32_t nelua_shl_nluint32(uint32_t a, int32_t b);
static NELUA_INLINE uint32_t nelua_shr_nluint32(uint32_t a, int32_t b);
NELUA_CEXPORT int32_t drift_complete(drift_vm_ptr v, uint64_t instance, uint32_t ticket, int32_t status, nluint8_arr0_ptr data, uint32_t size);
/* ------------------------------ DEFINITIONS ------------------------------- */
uint32_t drift_le32(nluint8_arr0_ptr p, uint32_t n) {
  return ((((uint32_t)p[n] | ((uint32_t)p[(n + 1)] << 8)) | ((uint32_t)p[(n + 2)] << 16)) | ((uint32_t)p[(n + 3)] << 24));
}
bool drift_span(uint32_t total, uint32_t start, uint32_t count) {
  return ((start <= total) && (count <= (total - start)));
}
int32_t drift_trap(drift_vm_ptr v, int32_t error_1) {
  v->state = error_1;
  return error_1;
}
int32_t drift_load(drift_vm_ptr v, nluint8_arr0_ptr p, uint32_t size, uint32_t grants, uint64_t instance) {
  if((v == ((drift_vm_ptr)NULL))) {
    return -9;
  }
  v->state = -1;
  if((((p == ((nluint8_arr0_ptr)NULL)) || (size < 32)) || (instance == 0))) {
    return -1;
  }
  if((((((p[0] != 68) || (p[1] != 82)) || (p[2] != 70)) || (p[3] != 84)) || (drift_le32(p, 4U) != 1))) {
    return -1;
  }
  uint32_t count = drift_le32(p, 8U);
  uint32_t memory = drift_le32(p, 12U);
  uint32_t data = drift_le32(p, 16U);
  uint32_t caps = drift_le32(p, 20U);
  uint32_t entry = drift_le32(p, 24U);
  uint32_t bus = drift_le32(p, 28U);
  if(((((((((count == 0) || (count > 1024)) || (memory == 0)) || (memory > 4096)) || (data > memory)) || (entry >= count)) || (bus > 3)) || ((caps & 4294967288ULL) != 0))) {
    return -1;
  }
  if((size != ((32 + (count * 8)) + data))) {
    return -1;
  }
  if(((caps & grants) != caps)) {
    return -4;
  }
  if((((caps & 1) != 0) && (bus == 0))) {
    return -1;
  }
  uint32_t i = 0U;
  while((i < count)) {
    uint32_t off = (32 + (i * 8));
    uint8_t op = p[off];
    uint8_t a = p[(off + 1)];
    uint8_t b = p[(off + 2)];
    uint8_t c = p[(off + 3)];
    uint32_t imm = drift_le32(p, (off + 4));
    if(((((op > 22) || (a >= 16)) || (b >= 16)) || (c >= 16))) {
      return -2;
    }
    if((((op >= 16) && (op <= 19)) && (imm >= count))) {
      return -2;
    }
    if(((op == 20) && ((imm >= 16) || ((caps & 1) == 0)))) {
      return -4;
    }
    if(((op == 21) && ((caps & 2) == 0))) {
      return -4;
    }
    if(((op == 22) && ((caps & 4) == 0))) {
      return -4;
    }
    uint8_t _asgntmp_1 = op;
    uint8_t _asgntmp_2 = a;
    uint8_t _asgntmp_3 = b;
    uint8_t _asgntmp_4 = c;
    v->code[i].op = _asgntmp_1;
    v->code[i].a = _asgntmp_2;
    v->code[i].b = _asgntmp_3;
    v->code[i].c = _asgntmp_4;
    v->code[i].imm = imm;
    i = (i + 1);
  }
  i = 0U;
  while((i < 16)) {
    v->r[i] = 0U;
    i = (i + 1);
  }
  i = 0U;
  while((i < 4096)) {
    if((i < data)) {
      v->memory[i] = p[((32 + (count * 8)) + i)];
    } else {
      v->memory[i] = 0U;
    }
    i = (i + 1);
  }
  uint64_t _asgntmp_5 = instance;
  uint64_t _asgntmp_6 = 0U;
  uint32_t _asgntmp_7 = 0U;
  v->instance = _asgntmp_5;
  v->steps = _asgntmp_6;
  v->sequence = _asgntmp_7;
  uint32_t _asgntmp_8 = count;
  uint32_t _asgntmp_9 = memory;
  uint32_t _asgntmp_10 = entry;
  v->code_len = _asgntmp_8;
  v->memory_len = _asgntmp_9;
  v->pc = _asgntmp_10;
  uint32_t _asgntmp_11 = caps;
  uint32_t _asgntmp_12 = bus;
  int32_t _asgntmp_13 = 0;
  v->caps = _asgntmp_11;
  v->bus = _asgntmp_12;
  v->state = (int32_t)_asgntmp_13;
  uint64_t _asgntmp_14 = instance;
  uint32_t _asgntmp_15 = 0U;
  uint32_t _asgntmp_16 = 0U;
  v->effect.instance = _asgntmp_14;
  v->effect.ticket = _asgntmp_15;
  v->effect.kind = _asgntmp_16;
  uint32_t _asgntmp_17 = 0U;
  uint32_t _asgntmp_18 = 0U;
  uint32_t _asgntmp_19 = 0U;
  uint32_t _asgntmp_20 = 0U;
  v->effect.tx = _asgntmp_17;
  v->effect.tx_len = _asgntmp_18;
  v->effect.rx = _asgntmp_19;
  v->effect.rx_len = _asgntmp_20;
  uint32_t _asgntmp_21 = 0U;
  uint32_t _asgntmp_22 = 0U;
  uint32_t _asgntmp_23 = 0U;
  uint32_t _asgntmp_24 = 0U;
  v->effect.tag = _asgntmp_21;
  v->effect.x = _asgntmp_22;
  v->effect.y = _asgntmp_23;
  v->effect.z = _asgntmp_24;
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
uint32_t nelua_shr_nluint32(uint32_t a, int32_t b) {
  if(NELUA_LIKELY(b >= 0 && b < 32)) {
    return (uint32_t)a >> b;
  } else if(NELUA_UNLIKELY(b < 0 && b > -32)) {
    return (uint32_t)a << -b;
  } else {
    return 0;
  }
}
int32_t drift_run(drift_vm_ptr v, uint32_t fuel) {
  if((v == ((drift_vm_ptr)NULL))) {
    return -9;
  }
  if((v->state != 0)) {
    return v->state;
  }
  while((fuel > 0)) {
    if((v->pc >= v->code_len)) {
      return drift_trap(v, -3);
    }
    drift_insn ins = v->code[v->pc];
    uint8_t a = ins.a;
    uint8_t b = ins.b;
    uint8_t c = ins.c;
    uint32_t imm = ins.imm;
    uint8_t op = ins.op;
    uint32_t _asgntmp_1 = (v->pc + 1);
    uint64_t _asgntmp_2 = (v->steps + 1);
    uint32_t _asgntmp_3 = (fuel - 1);
    v->pc = _asgntmp_1;
    v->steps = _asgntmp_2;
    fuel = _asgntmp_3;
    if((op == 0)) {
      v->state = 3;
      return 3;
    } else if((op == 1)) {
      v->r[a] = imm;
    } else if((op == 2)) {
      v->r[a] = v->r[b];
    } else if((op == 3)) {
      v->r[a] = (v->r[b] + v->r[c]);
    } else if((op == 4)) {
      v->r[a] = (v->r[b] - v->r[c]);
    } else if((op == 5)) {
      v->r[a] = (v->r[b] * v->r[c]);
    } else if((op == 6)) {
      if((v->r[c] == 0)) {
        return drift_trap(v, -7);
      }
      v->r[a] = (v->r[b] / v->r[c]);
    } else if((op == 7)) {
      v->r[a] = (v->r[b] & v->r[c]);
    } else if((op == 8)) {
      v->r[a] = (v->r[b] | v->r[c]);
    } else if((op == 9)) {
      v->r[a] = (v->r[b] ^ v->r[c]);
    } else if((op == 10)) {
      v->r[a] = nelua_shl_nluint32(v->r[b], (v->r[c] & 31));
    } else if((op == 11)) {
      v->r[a] = nelua_shr_nluint32(v->r[b], (v->r[c] & 31));
    } else if(((op >= 12) && (op <= 15))) {
      uint32_t base = v->r[b];
      int64_t width = 1;
      if((op >= 14)) {
        width = 4;
      }
      if(((!drift_span(v->memory_len, base, imm)) || (!drift_span(v->memory_len, (base + imm), (uint32_t)width)))) {
        return drift_trap(v, -3);
      }
      uint32_t addr = (base + imm);
      if((op == 12)) {
        v->r[a] = (uint32_t)v->memory[addr];
      } else if((op == 13)) {
        v->memory[addr] = (uint8_t)(v->r[a] & 255);
      } else if((op == 14)) {
        v->r[a] = drift_le32(((nluint8_arr0_ptr)(&v->memory[0])), addr);
      } else {
        uint32_t value = v->r[a];
        v->memory[addr] = (uint8_t)(value & 255);
        v->memory[(addr + 1)] = (uint8_t)((value >> 8) & 255);
        v->memory[(addr + 2)] = (uint8_t)((value >> 16) & 255);
        v->memory[(addr + 3)] = (uint8_t)(value >> 24);
      }
    } else if((op == 16)) {
      v->pc = imm;
    } else if((op == 17)) {
      if((v->r[a] == v->r[b])) {
        v->pc = imm;
      }
    } else if((op == 18)) {
      if((v->r[a] != v->r[b])) {
        v->pc = imm;
      }
    } else if((op == 19)) {
      if((v->r[a] < v->r[b])) {
        v->pc = imm;
      }
    } else if(((op >= 20) && (op <= 22))) {
      if((v->sequence == 0xffffffffULL)) {
        return drift_trap(v, -8);
      }
      uint32_t _asgntmp_4 = 0U;
      uint32_t _asgntmp_5 = 0U;
      uint32_t _asgntmp_6 = 0U;
      uint32_t _asgntmp_7 = 0U;
      v->effect.tx = _asgntmp_4;
      v->effect.tx_len = _asgntmp_5;
      v->effect.rx = _asgntmp_6;
      v->effect.rx_len = _asgntmp_7;
      uint32_t _asgntmp_8 = 0U;
      uint32_t _asgntmp_9 = 0U;
      uint32_t _asgntmp_10 = 0U;
      uint32_t _asgntmp_11 = 0U;
      v->effect.tag = _asgntmp_8;
      v->effect.x = _asgntmp_9;
      v->effect.y = _asgntmp_10;
      v->effect.z = _asgntmp_11;
      if((op == 20)) {
        uint32_t tx = v->r[a];
        uint32_t txlen = v->r[b];
        uint32_t rx = v->r[c];
        uint32_t rxlen = v->r[imm];
        if(((!drift_span(v->memory_len, tx, txlen)) || (!drift_span(v->memory_len, rx, rxlen)))) {
          return drift_trap(v, -3);
        }
        uint32_t _asgntmp_12 = tx;
        uint32_t _asgntmp_13 = txlen;
        uint32_t _asgntmp_14 = rx;
        uint32_t _asgntmp_15 = rxlen;
        v->effect.tx = _asgntmp_12;
        v->effect.tx_len = _asgntmp_13;
        v->effect.rx = _asgntmp_14;
        v->effect.rx_len = _asgntmp_15;
        v->effect.kind = 1U;
      } else if((op == 21)) {
        uint32_t _asgntmp_16 = 2U;
        uint32_t _asgntmp_17 = imm;
        v->effect.kind = _asgntmp_16;
        v->effect.tag = _asgntmp_17;
        uint32_t _asgntmp_18 = v->r[a];
        uint32_t _asgntmp_19 = v->r[b];
        uint32_t _asgntmp_20 = v->r[c];
        v->effect.x = _asgntmp_18;
        v->effect.y = _asgntmp_19;
        v->effect.z = _asgntmp_20;
      } else {
        uint32_t _asgntmp_21 = 3U;
        uint32_t _asgntmp_22 = v->r[a];
        v->effect.kind = _asgntmp_21;
        v->effect.x = _asgntmp_22;
      }
      v->sequence = (v->sequence + 1);
      uint64_t _asgntmp_23 = v->instance;
      uint32_t _asgntmp_24 = v->sequence;
      v->effect.instance = _asgntmp_23;
      v->effect.ticket = _asgntmp_24;
      v->state = 1;
      return 1;
    } else {
      return drift_trap(v, -2);
    }
  }
  return 2;
}
int32_t drift_complete(drift_vm_ptr v, uint64_t instance, uint32_t ticket, int32_t status, nluint8_arr0_ptr data, uint32_t size) {
  if((v == ((drift_vm_ptr)NULL))) {
    return -9;
  }
  if((v->state != 1)) {
    return -5;
  }
  if(((v->instance != instance) || (v->effect.ticket != ticket))) {
    return -6;
  }
  if((((status > 0) || ((status < 0) && (size != 0))) || ((size != 0) && (data == ((nluint8_arr0_ptr)NULL))))) {
    return -9;
  }
  if(((v->effect.kind != 1) && (size != 0))) {
    return -9;
  }
  if((size > v->effect.rx_len)) {
    return -3;
  }
  uint32_t i = 0U;
  uintptr_t dst = (uintptr_t)(&v->memory[v->effect.rx]);
  uintptr_t src = (uintptr_t)data;
  if(((dst > src) && ((dst - src) < size))) {
    i = size;
    while((i > 0)) {
      i = (i - 1);
      v->memory[(v->effect.rx + i)] = data[i];
    }
  } else {
    while((i < size)) {
      v->memory[(v->effect.rx + i)] = data[i];
      i = (i + 1);
    }
  }
  uint32_t _asgntmp_1 = (uint32_t)status;
  uint32_t _asgntmp_2 = size;
  int32_t _asgntmp_3 = 0;
  v->r[0] = _asgntmp_1;
  v->r[1] = _asgntmp_2;
  v->state = (int32_t)_asgntmp_3;
  return 0;
}

