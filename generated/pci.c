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
#include "drift_pci.h"
#include <stdint.h>
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
typedef drift_pci* drift_pci_ptr;
typedef drift_pci_device* drift_pci_device_ptr;
NELUA_CEXPORT int32_t drift_pci_find(drift_pci_ptr p, uint32_t first, uint32_t last, uint32_t vendor, uint32_t device, uint32_t index, drift_pci_device_ptr out);
typedef uint32_t* nluint32_arr0_ptr;
NELUA_CEXPORT int32_t drift_pci_cap(drift_pci_ptr p, uint32_t bdf, uint32_t id, uint32_t index, nluint32_arr0_ptr out);
static NELUA_INLINE uint64_t nelua_shl_nluint64(uint64_t a, int64_t b);
typedef drift_pci_bar* drift_pci_bar_ptr;
NELUA_CEXPORT int32_t drift_pci_get_bar(drift_pci_ptr p, uint32_t bdf, uint32_t index, drift_pci_bar_ptr out);
NELUA_CEXPORT int32_t drift_pci_enable(drift_pci_ptr p, uint32_t bdf, uint32_t mask);
/* ------------------------------ DEFINITIONS ------------------------------- */
int32_t drift_pci_find(drift_pci_ptr p, uint32_t first, uint32_t last, uint32_t vendor, uint32_t device, uint32_t index, drift_pci_device_ptr out) {
  if(((((((drift_pci_valid(p) == 0) || (out == ((drift_pci_device_ptr)NULL))) || (first > last)) || (last > 255)) || (vendor > 65535)) || (device > 65535))) {
    return -1;
  }
  uint32_t bus = first;
  while((bus <= last)) {
    uint32_t slot = 0U;
    while((slot < 32)) {
      uint32_t base = ((bus << 8) | (slot << 3));
      uint32_t id = drift_pci_read(p, base, 0U);
      if(((id & 65535) != 65535)) {
        uint32_t count = 1U;
        if(((drift_pci_read(p, base, 12U) & 0x800000) != 0)) {
          count = 8U;
        }
        uint32_t fn = 0U;
        while((fn < count)) {
          uint32_t bdf = (base | fn);
          id = drift_pci_read(p, bdf, 0U);
          if(((((id & 65535) != 65535) && ((vendor == 65535) || ((id & 65535) == vendor))) && ((device == 65535) || ((id >> 16) == device)))) {
            if((index == 0)) {
              out->bdf = bdf;
              out->vendor = (id & 65535);
              out->device = (id >> 16);
              out->class_code = (drift_pci_read(p, bdf, 8U) >> 8);
              return 0;
            }
            index = (index - 1);
          }
          fn = (fn + 1);
        }
      }
      slot = (slot + 1);
    }
    bus = (bus + 1);
  }
  return 1;
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
int32_t drift_pci_cap(drift_pci_ptr p, uint32_t bdf, uint32_t id, uint32_t index, nluint32_arr0_ptr out) {
  if(((((drift_pci_valid(p) == 0) || (bdf > 65535)) || (id > 255)) || (out == ((nluint32_arr0_ptr)NULL)))) {
    return -1;
  }
  if(((drift_pci_read(p, bdf, 4U) & 0x100000) == 0)) {
    return 1;
  }
  if((((drift_pci_read(p, bdf, 12U) >> 16) & 127) > 1)) {
    return -2;
  }
  uint32_t offset = (drift_pci_read(p, bdf, 52U) & 255);
  uint64_t seen = 0U;
  while((offset != 0)) {
    if((((offset < 64) || (offset > 252)) || ((offset & 3) != 0))) {
      return -2;
    }
    uint64_t bit = nelua_shl_nluint64(1U, (offset >> 2));
    if(((seen & bit) != 0)) {
      return -2;
    }
    seen = (seen | bit);
    uint32_t cap = drift_pci_read(p, bdf, offset);
    if(((cap & 255) == id)) {
      if((index == 0)) {
        out[0] = offset;
        return 0;
      }
      index = (index - 1);
    }
    offset = ((cap >> 8) & 255);
  }
  return 1;
}
int32_t drift_pci_get_bar(drift_pci_ptr p, uint32_t bdf, uint32_t index, drift_pci_bar_ptr out) {
  if(((((drift_pci_valid(p) == 0) || (bdf > 65535)) || (index > 5)) || (out == ((drift_pci_bar_ptr)NULL)))) {
    return -1;
  }
  uint32_t kind = ((drift_pci_read(p, bdf, 12U) >> 16) & 127);
  uint32_t limit = 6U;
  if((kind == 1)) {
    limit = 2U;
  } else if((kind != 0)) {
    return -2;
  }
  if((index >= limit)) {
    return -1;
  }
  uint32_t i = 0U;
  while((i < limit)) {
    uint32_t value = drift_pci_read(p, bdf, (16 + (i * 4)));
    uint32_t width = 32U;
    uint32_t io = (value & 1);
    if((io == 0)) {
      if(((value & 6) == 4)) {
        width = 64U;
      } else if(((value & 6) != 0)) {
        return -2;
      }
    }
    if((i == index)) {
      if(((value == 0) || (value == 0xffffffffLL))) {
        return -2;
      }
      out->io = io;
      out->width = width;
      out->prefetch = 0U;
      if((io != 0)) {
        out->address = (uint64_t)(value & 0xfffffffcLL);
      } else {
        out->address = (uint64_t)(value & 0xfffffff0LL);
        out->prefetch = ((value >> 3) & 1);
        if((width == 64)) {
          if(((i + 1) >= limit)) {
            return -2;
          }
          out->address = (out->address | ((uint64_t)drift_pci_read(p, bdf, (20 + (i * 4))) << 32));
        }
      }
      return 0;
    }
    if((width == 64)) {
      if(((i + 1) == index)) {
        return -1;
      }
      i = (i + 2);
    } else {
      i = (i + 1);
    }
  }
  return -1;
}
int32_t drift_pci_enable(drift_pci_ptr p, uint32_t bdf, uint32_t mask) {
  if((((drift_pci_valid(p) == 0) || (bdf > 65535)) || ((mask & 4294967288ULL) != 0))) {
    return -1;
  }
  if(((drift_pci_read(p, bdf, 0U) & 65535) == 65535)) {
    return 1;
  }
  drift_pci_write(p, bdf, 4U, ((drift_pci_read(p, bdf, 4U) & 65535) | mask));
  return 0;
}

