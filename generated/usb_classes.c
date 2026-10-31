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
#include "drift_usb.h"
#include <stdint.h>
#include "drift_usb_classes.h"
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
static uint32_t drift_usb_classes_get16(nluint8_arr0_ptr p, uint32_t o);
static int32_t drift_usb_classes_hex(uint8_t c);
typedef drift_usb_cdc* drift_usb_cdc_ptr;
static int32_t drift_usb_classes_mac_string(drift_usb_cdc_ptr c, uint32_t index);
typedef struct NELUA_MAYALIAS nluint8_arr256 {uint8_t v[256];} nluint8_arr256;
typedef union NELUA_MAYALIAS nluint8_arr256_cast {nluint8_arr256 a; uint8_t p[256];} nluint8_arr256_cast;
static bool drift_usb_classes_ready(drift_usb_cdc_ptr c);
typedef drift_usb* drift_usb_ptr;
NELUA_CEXPORT int32_t drift_usb_cdc_open(drift_usb_cdc_ptr c, drift_usb_ptr d, uint32_t kind, uint32_t interface);
NELUA_CEXPORT int32_t drift_usb_acm_configure(drift_usb_cdc_ptr c, uint32_t baud, uint32_t stop, uint32_t parity, uint32_t bits, uint32_t lines);
typedef struct NELUA_MAYALIAS nluint8_arr7 {uint8_t v[7];} nluint8_arr7;
typedef union NELUA_MAYALIAS nluint8_arr7_cast {nluint8_arr7 a; uint8_t p[7];} nluint8_arr7_cast;
static NELUA_INLINE uint32_t nelua_shr_nluint32(uint32_t a, int32_t b);
NELUA_CEXPORT int32_t drift_usb_cdc_read(drift_usb_cdc_ptr c, nluint8_arr0_ptr data, uint32_t capacity);
NELUA_CEXPORT int32_t drift_usb_cdc_write(drift_usb_cdc_ptr c, nluint8_arr0_ptr data, uint32_t size);
NELUA_CEXPORT int32_t drift_usb_cdc_close(drift_usb_cdc_ptr c);
/* ------------------------------ DEFINITIONS ------------------------------- */
uint32_t drift_usb_classes_get16(nluint8_arr0_ptr p, uint32_t o) {
  return ((uint32_t)p[o] | ((uint32_t)p[(o + 1)] << 8));
}
int32_t drift_usb_classes_hex(uint8_t c) {
  if(((c >= 48) && (c <= 57))) {
    return ((int32_t)c - 48);
  }
  if(((c >= 65) && (c <= 70))) {
    return ((int32_t)c - 55);
  }
  if(((c >= 97) && (c <= 102))) {
    return ((int32_t)c - 87);
  }
  return -1;
}
int32_t drift_usb_classes_mac_string(drift_usb_cdc_ptr c, uint32_t index) {
  nluint8_arr256 data = (nluint8_arr256){0};
  uint32_t got = 0U;
  int32_t result = drift_usb_control(c->device, 0x80U, 6U, 0x300U, 0U, ((nluint8_arr0_ptr)(&data.v[0])), 255U, (&got));
  if((result != 0)) {
    return result;
  }
  if((((((got < 4) || (data.v[1] != 3)) || (data.v[0] < 4)) || (data.v[0] > got)) || ((data.v[0] & 1) != 0))) {
    return -71;
  }
  uint32_t language = drift_usb_classes_get16(((nluint8_arr0_ptr)(&data.v[0])), 2U);
  result = drift_usb_control(c->device, 0x80U, 6U, (uint16_t)(0x300 | index), (uint16_t)language, ((nluint8_arr0_ptr)(&data.v[0])), 255U, (&got));
  if((result != 0)) {
    return result;
  }
  if((((got < 26) || (data.v[0] != 26)) || (data.v[1] != 3))) {
    return -71;
  }
  for(uint32_t i = 0U; i <= 5U; i += 1) {
    int32_t a = drift_usb_classes_hex(data.v[(2 + (i * 4))]);
    int32_t b = drift_usb_classes_hex(data.v[(4 + (i * 4))]);
    if(((((a < 0) || (b < 0)) || (data.v[(3 + (i * 4))] != 0)) || (data.v[(5 + (i * 4))] != 0))) {
      return -71;
    }
    c->mac[i] = (uint8_t)((a * 16) + b);
  }
  if(((c->mac[0] & 1) != 0)) {
    return -71;
  }
  uint8_t nonzero = 0U;
  for(uint32_t i = 0U; i <= 5U; i += 1) {
    nonzero = (nonzero | c->mac[i]);
  }
  if((nonzero == 0)) {
    return -71;
  }
  return 0;
}
bool drift_usb_classes_ready(drift_usb_cdc_ptr c) {
  return ((((c != ((drift_usb_cdc_ptr)NULL)) && (c->state == 1)) && (c->device != ((drift_usb_ptr)NULL))) && (c->device->state == 1));
}
int32_t drift_usb_cdc_open(drift_usb_cdc_ptr c, drift_usb_ptr d, uint32_t kind, uint32_t interface) {
  if((((((c == ((drift_usb_cdc_ptr)NULL)) || (d == ((drift_usb_ptr)NULL))) || (c->state != 0)) || (d->state != 1)) || ((kind != 1) && (kind != 2)))) {
    return -22;
  }
  uint32_t total = d->total_config;
  if((((((total < 9) || (total > 1024)) || (d->config[0] < 9)) || (d->config[1] != 2)) || (drift_usb_classes_get16(((nluint8_arr0_ptr)(&d->config[0])), 2U) != total))) {
    return -71;
  }
  bool chosen = false;
  bool current = false;
  uint32_t master = 0U;
  uint32_t slave = 0xffffffffULL;
  uint32_t mac_index = 0U;
  uint32_t maximum = 0U;
  uint32_t acm = 0U;
  bool union_seen = false;
  bool functional_seen = false;
  uint32_t at = 0U;
  while((at < total)) {
    uint32_t n = (uint32_t)d->config[at];
    if(((n < 2) || (n > (total - at)))) {
      return -71;
    }
    uint8_t t = d->config[(at + 1)];
    if((t == 4)) {
      if((n < 9)) {
        return -71;
      }
      current = false;
      if((((((!chosen) && (d->config[(at + 3)] == 0)) && (d->config[(at + 5)] == 2)) && ((((kind == 1) && (d->config[(at + 6)] == 2)) && (d->config[(at + 7)] <= 6)) || (((kind == 2) && (d->config[(at + 6)] == 6)) && (d->config[(at + 7)] == 0)))) && ((interface == 0xffffffffLL) || (d->config[(at + 2)] == interface)))) {
        master = (uint32_t)d->config[(at + 2)];
        chosen = true;
        current = true;
      }
    } else if((current && (t == 0x24))) {
      if((n < 3)) {
        return -71;
      }
      uint8_t subtype = d->config[(at + 2)];
      if((subtype == 6)) {
        if(((((n != 5) || union_seen) || (d->config[(at + 3)] != master)) || (d->config[(at + 4)] == master))) {
          return -71;
        }
        slave = (uint32_t)d->config[(at + 4)];
        union_seen = true;
      } else if(((kind == 1) && (subtype == 2))) {
        if(((n != 4) || functional_seen)) {
          return -71;
        }
        acm = (uint32_t)d->config[(at + 3)];
        functional_seen = true;
      } else if(((kind == 2) && (subtype == 15))) {
        if(((n != 13) || functional_seen)) {
          return -71;
        }
        mac_index = (uint32_t)d->config[(at + 3)];
        maximum = drift_usb_classes_get16(((nluint8_arr0_ptr)(&d->config[0])), (at + 8));
        functional_seen = true;
      }
    }
    at = (at + n);
  }
  if((((!chosen) || (!union_seen)) || (!functional_seen))) {
    return -95;
  }
  if(((kind == 1) && ((acm & 2) == 0))) {
    return -95;
  }
  if(((kind == 2) && (((mac_index == 0) || (maximum < 14)) || (maximum > 16384)))) {
    return -95;
  }
  at = 0U;
  current = false;
  bool selected = false;
  uint32_t input = 0U;
  uint32_t output = 0U;
  uint32_t packet = 0U;
  uint32_t alt = 0U;
  uint32_t expected = 0U;
  uint32_t count = 0U;
  bool valid = true;
  while((at <= total)) {
    uint32_t n = 0U;
    uint8_t t = 4U;
    if((at < total)) {
      n = (uint32_t)d->config[at];
      t = d->config[(at + 1)];
    }
    if((t == 4)) {
      if(((((((current && valid) && (expected == 2)) && (count == 2)) && (input != 0)) && (output != 0)) && (!selected))) {
        c->endpoint_in = input;
        c->endpoint_out = output;
        c->out_packet_size = packet;
        c->alternate = alt;
        selected = true;
      }
      current = false;
      input = 0U;
      output = 0U;
      packet = 0U;
      count = 0U;
      valid = true;
      if((((((at < total) && (d->config[(at + 2)] == slave)) && (d->config[(at + 5)] == 10)) && (d->config[(at + 6)] == 0)) && (d->config[(at + 7)] == 0))) {
        current = true;
        alt = (uint32_t)d->config[(at + 3)];
        expected = (uint32_t)d->config[(at + 4)];
      }
    } else if((current && (t == 5))) {
      if((n < 7)) {
        return -71;
      }
      count = (count + 1);
      uint32_t endpoint = (uint32_t)d->config[(at + 2)];
      if(((((((endpoint & 0x70) != 0) || ((endpoint & 15) == 0)) || ((d->config[(at + 3)] & 3) != 2)) || ((drift_usb_classes_get16(((nluint8_arr0_ptr)(&d->config[0])), (at + 4)) & 0x7ff) == 0)) || (drift_usb_classes_get16(((nluint8_arr0_ptr)(&d->config[0])), (at + 4)) > 512))) {
        valid = false;
      }
      if(((endpoint & 128) != 0)) {
        if((input != 0)) {
          valid = false;
        }
        input = endpoint;
      } else {
        if((output != 0)) {
          valid = false;
        }
        output = endpoint;
        packet = (drift_usb_classes_get16(((nluint8_arr0_ptr)(&d->config[0])), (at + 4)) & 0x7ff);
      }
    }
    if((at == total)) {
      break;
    }
    at = (at + n);
  }
  if((!selected)) {
    return -95;
  }
  c->device = d;
  c->kind = kind;
  c->control_interface = master;
  c->data_interface = slave;
  c->maximum_segment = ((kind == 2) ? maximum : 16384U);
  for(uint32_t i = 0U; i <= 5U; i += 1) {
    c->mac[i] = 0U;
  }
  if((c->alternate != 0)) {
    int32_t result = drift_usb_set_interface(d, (uint8_t)slave, (uint8_t)c->alternate);
    if((result != 0)) {
      return result;
    }
  }
  if((kind == 2)) {
    int32_t result = drift_usb_classes_mac_string(c, mac_index);
    if((result != 0)) {
      return result;
    }
    uint32_t got = 0U;
    result = drift_usb_control(d, 0x21U, 0x43U, 12U, (uint16_t)master, ((nluint8_arr0_ptr)NULL), 0U, (&got));
    if((result != 0)) {
      return result;
    }
    if((got != 0)) {
      return -71;
    }
  }
  c->state = 1U;
  return 0;
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
int32_t drift_usb_acm_configure(drift_usb_cdc_ptr c, uint32_t baud, uint32_t stop, uint32_t parity, uint32_t bits, uint32_t lines) {
  if((((((((!drift_usb_classes_ready(c)) || (c->kind != 1)) || (baud == 0)) || (stop > 2)) || (parity > 4)) || (((bits < 5) || (bits > 8)) && (bits != 16))) || (lines > 3))) {
    return -22;
  }
  nluint8_arr7 data = (nluint8_arr7){0};
  for(uint32_t i = 0U; i <= 3U; i += 1) {
    data.v[i] = (uint8_t)nelua_shr_nluint32(baud, (8 * i));
  }
  data.v[4] = (uint8_t)stop;
  data.v[5] = (uint8_t)parity;
  data.v[6] = (uint8_t)bits;
  uint32_t got = 0U;
  int32_t result = drift_usb_control(c->device, 0x21U, 0x20U, 0U, (uint16_t)c->control_interface, ((nluint8_arr0_ptr)(&data.v[0])), 7U, (&got));
  if((result != 0)) {
    return result;
  }
  if((got != 7)) {
    return -71;
  }
  result = drift_usb_control(c->device, 0x21U, 0x22U, (uint16_t)lines, (uint16_t)c->control_interface, ((nluint8_arr0_ptr)NULL), 0U, (&got));
  if((result != 0)) {
    return result;
  }
  if((got != 0)) {
    return -71;
  }
  return 0;
}
int32_t drift_usb_cdc_read(drift_usb_cdc_ptr c, nluint8_arr0_ptr data, uint32_t capacity) {
  if(((((!drift_usb_classes_ready(c)) || (data == ((nluint8_arr0_ptr)NULL))) || (capacity == 0)) || (capacity > 16384))) {
    return -22;
  }
  if(((c->kind == 2) && (capacity < c->maximum_segment))) {
    return -22;
  }
  uint32_t got = 0U;
  int32_t result = drift_usb_transfer(c->device, (uint8_t)c->endpoint_in, data, capacity, (&got));
  if((result == -11)) {
    return 0;
  }
  if((result != 0)) {
    return result;
  }
  if(((got > capacity) || (((c->kind == 2) && (got != 0)) && ((got < 14) || (got > c->maximum_segment))))) {
    return -71;
  }
  return (int32_t)got;
}
int32_t drift_usb_cdc_write(drift_usb_cdc_ptr c, nluint8_arr0_ptr data, uint32_t size) {
  if((((((!drift_usb_classes_ready(c)) || (data == ((nluint8_arr0_ptr)NULL))) || (size == 0)) || (size > c->maximum_segment)) || ((c->kind == 2) && (size < 14)))) {
    return -22;
  }
  uint32_t got = 0U;
  int32_t result = drift_usb_transfer(c->device, (uint8_t)c->endpoint_out, data, size, (&got));
  if((result != 0)) {
    return result;
  }
  if((got != size)) {
    return -71;
  }
  if(((c->kind == 2) && ((size % c->out_packet_size) == 0))) {
    result = drift_usb_transfer(c->device, (uint8_t)c->endpoint_out, ((nluint8_arr0_ptr)NULL), 0U, (&got));
    if((result != 0)) {
      return result;
    }
    if((got != 0)) {
      return -71;
    }
  }
  return (int32_t)size;
}
int32_t drift_usb_cdc_close(drift_usb_cdc_ptr c) {
  if((c == ((drift_usb_cdc_ptr)NULL))) {
    return -22;
  }
  if((c->state == 0)) {
    return 0;
  }
  int32_t result = 0;
  if(drift_usb_classes_ready(c)) {
    uint32_t got = 0U;
    result = drift_usb_control(c->device, 0x21U, ((c->kind == 1) ? 0x22U : 0x43U), 0U, (uint16_t)c->control_interface, ((nluint8_arr0_ptr)NULL), 0U, (&got));
    if(((result == 0) && (got != 0))) {
      result = -71;
    }
  } else {
    result = -22;
  }
  c->state = 0U;
  return result;
}

