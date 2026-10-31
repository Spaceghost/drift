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
#include "drift_usb.h"
#include <stdint.h>
/* Macro used sign that a type punning cast may alias (related to strict aliasing). */
#ifdef __GNUC__
  #define NELUA_MAYALIAS __attribute__((may_alias))
#else
  #define NELUA_MAYALIAS
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
/* ------------------------------ DECLARATIONS ------------------------------ */
typedef struct nlniltype {} nlniltype;
typedef struct nlniltype nltype;
typedef drift_usb* drift_usb_ptr;
typedef uint32_t* nluint32_ptr;
static int32_t drift_usb_hub_status(drift_usb_ptr d, uint32_t port, nluint32_ptr out);
typedef struct NELUA_MAYALIAS nluint8_arr4 {uint8_t v[4];} nluint8_arr4;
typedef union NELUA_MAYALIAS nluint8_arr4_cast {nluint8_arr4 a; uint8_t p[4];} nluint8_arr4_cast;
typedef uint8_t* nluint8_arr0_ptr;
static int32_t drift_usb_hub_feature(drift_usb_ptr d, uint32_t port, uint16_t value, bool set);
NELUA_CEXPORT int32_t drift_usb_attach_hub(drift_usb_ptr d, uint32_t port);
typedef struct NELUA_MAYALIAS nluint8_arr9 {uint8_t v[9];} nluint8_arr9;
typedef union NELUA_MAYALIAS nluint8_arr9_cast {nluint8_arr9 a; uint8_t p[9];} nluint8_arr9_cast;
static NELUA_INLINE bool nelua_eq_nlint64_nluint32(int64_t a, uint32_t b);
static NELUA_INLINE uint32_t nelua_shl_nluint32(uint32_t a, int32_t b);
/* ------------------------------ DEFINITIONS ------------------------------- */
int32_t drift_usb_hub_status(drift_usb_ptr d, uint32_t port, nluint32_ptr out) {
  nluint8_arr4 bytes = (nluint8_arr4){0};
  uint32_t got = 0U;
  int32_t e = drift_usb_control(d, 0xa3U, 0U, 0U, (uint16_t)port, ((nluint8_arr0_ptr)(&bytes.v[0])), 4U, (&got));
  if((e != 0)) {
    return e;
  }
  if((got != 4)) {
    return -5;
  }
  (*out) = ((((uint32_t)bytes.v[0] | ((uint32_t)bytes.v[1] << 8)) | ((uint32_t)bytes.v[2] << 16)) | ((uint32_t)bytes.v[3] << 24));
  return 0;
}
int32_t drift_usb_hub_feature(drift_usb_ptr d, uint32_t port, uint16_t value, bool set) {
  uint32_t got = 0U;
  uint8_t request = 1U;
  if(set) {
    request = 3U;
  }
  return drift_usb_control(d, 0x23U, request, value, (uint16_t)port, ((nluint8_arr0_ptr)NULL), 0U, (&got));
}
bool nelua_eq_nlint64_nluint32(int64_t a, uint32_t b) {
  return (uint64_t)a == (uint64_t)b && a >= 0;
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
int32_t drift_usb_attach_hub(drift_usb_ptr d, uint32_t port) {
  if(((((((d == ((drift_usb_ptr)NULL)) || (d->state != 1)) || (d->slot == 0)) || (d->kind != 5)) || (d->parent_slot != 0)) || (drift_hw_valid(d->hw) == 0))) {
    return -22;
  }
  nluint8_arr9 descriptor = (nluint8_arr9){0};
  uint32_t got = 0U;
  int32_t e = drift_usb_control(d, 0xa0U, 6U, 0x2900U, 0U, ((nluint8_arr0_ptr)(&descriptor.v[0])), 9U, (&got));
  if((e != 0)) {
    return e;
  }
  if((((got < 7) || (got > 9)) || (descriptor.v[1] != 0x29))) {
    return -5;
  }
  uint32_t ports = (uint32_t)descriptor.v[2];
  uint32_t characteristics = ((uint32_t)descriptor.v[3] | ((uint32_t)descriptor.v[4] << 8));
  if(((((ports == 0) || (ports > 15)) || (port > ports)) || ((characteristics & 3) == 3))) {
    return -95;
  }
  uint32_t bitmap = ((ports + 8) / 8);
  uint32_t mandatory = (7 + bitmap);
  uint32_t declared = (uint32_t)descriptor.v[0];
  int64_t prefix = (int64_t)declared;
  if((prefix > 9)) {
    prefix = 9;
  }
  if(((((declared < mandatory) || (declared > (7 + (2 * bitmap)))) || (!nelua_eq_nlint64_nluint32(prefix, got))) || (got < mandatory))) {
    return -5;
  }
  if(((characteristics & 3) != 2)) {
    for(uint32_t i = 1U, _end = ports; i <= _end; i += 1) {
      e = drift_usb_hub_feature(d, i, 8U, true);
      if((e != 0)) {
        return e;
      }
    }
  }
  drift_hw_wait(d->hw, ((uint32_t)descriptor.v[5] * 2000));
  if((port == 0)) {
    for(uint32_t i = 1U, _end = ports; i <= _end; i += 1) {
      uint32_t value = 0U;
      e = drift_usb_hub_status(d, i, (&value));
      if((e != 0)) {
        return e;
      }
      if(((value & 1) != 0)) {
        port = i;
        break;
      }
    }
  }
  if((port == 0)) {
    return -19;
  }
  uint32_t value = 0U;
  uint64_t debounce_start = drift_hw_time(d->hw);
  uint64_t stable = debounce_start;
  while(true) {
    e = drift_usb_hub_status(d, port, (&value));
    if((e != 0)) {
      return e;
    }
    if(((value & 1) == 0)) {
      return -19;
    }
    if(((value & 8) != 0)) {
      return -5;
    }
    if(((value & 65536U) != 0)) {
      e = drift_usb_hub_feature(d, port, 16U, false);
      if((e != 0)) {
        return e;
      }
      stable = drift_hw_time(d->hw);
    }
    if(((drift_hw_time(d->hw) - stable) >= 100000)) {
      break;
    }
    if(((drift_hw_time(d->hw) - debounce_start) >= 2000000)) {
      return -110;
    }
    drift_hw_wait(d->hw, 10000U);
  }
  e = drift_usb_hub_feature(d, port, 4U, true);
  if((e != 0)) {
    return e;
  }
  uint64_t start = drift_hw_time(d->hw);
  while(true) {
    e = drift_usb_hub_status(d, port, (&value));
    if((e != 0)) {
      return e;
    }
    if(((value & 1) == 0)) {
      return -19;
    }
    if(((value & 8) != 0)) {
      return -5;
    }
    if(((value & 0x13) == 3)) {
      break;
    }
    if(((drift_hw_time(d->hw) - start) >= 1000000)) {
      return -110;
    }
    drift_hw_wait(d->hw, 10000U);
  }
  for(uint32_t bit = 0U; bit <= 4U; bit += 1) {
    if(((value & nelua_shl_nluint32(1U, (16 + bit))) != 0)) {
      e = drift_usb_hub_feature(d, port, (uint16_t)(16 + bit), false);
      if((e != 0)) {
        return e;
      }
    }
  }
  uint8_t speed = 1U;
  if(((value & 0x200) != 0)) {
    speed = 2U;
  }
  if(((value & 0x400) != 0)) {
    if((speed == 2)) {
      return -5;
    }
    speed = 3U;
  }
  return drift_usb_hub_child(d, (uint8_t)port, speed, (uint8_t)ports, (uint8_t)((characteristics >> 5) & 3));
}

