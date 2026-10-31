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
#include "pcspkr_compat.h"
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
/* ------------------------------ DECLARATIONS ------------------------------ */
typedef drift_linux_pcspkr* drift_linux_pcspkr_ptr;
static void drift_linux_pcspkr_fail(drift_linux_pcspkr_ptr s, int32_t error_1);
static drift_linux_pcspkr_ptr drift_linux_pcspkr_active(void);
typedef drift_linux_pcspkr_host* drift_linux_pcspkr_host_ptr;
NELUA_CEXPORT int32_t drift_lx_prepare(drift_linux_pcspkr_ptr s, drift_linux_pcspkr_host_ptr h);
typedef uint8_t* nluint8_arr0_ptr;
NELUA_CEXPORT int32_t drift_lx_finish(drift_linux_pcspkr_ptr s, int32_t result);
typedef drift_pcspkr_input* drift_pcspkr_input_ptr;
NELUA_CEXPORT drift_pcspkr_input_ptr drift_lx_input_allocate_device(void);
NELUA_CEXPORT int32_t drift_lx_input_register_device(drift_pcspkr_input_ptr d);
NELUA_CEXPORT void drift_lx_input_free_device(drift_pcspkr_input_ptr d);
NELUA_CEXPORT void drift_lx_input_unregister_device(drift_pcspkr_input_ptr d);
NELUA_CEXPORT unsigned long drift_lx_lock(void);
NELUA_CEXPORT void drift_lx_unlock(unsigned long flags);
NELUA_CEXPORT uint8_t drift_lx_inb(uint16_t port);
NELUA_CEXPORT void drift_lx_outb(uint8_t value, uint16_t port);
NELUA_CEXPORT void drift_lx_delay(void);
/* ------------------------------ DEFINITIONS ------------------------------- */
void drift_linux_pcspkr_fail(drift_linux_pcspkr_ptr s, int32_t error_1) {
  if(((s != ((drift_linux_pcspkr_ptr)NULL)) && (s->error == 0))) {
    s->error = error_1;
  }
}
drift_linux_pcspkr_ptr drift_linux_pcspkr_active(void) {
  drift_linux_pcspkr_ptr s = drift_lx_current();
  if(((s == ((drift_linux_pcspkr_ptr)NULL)) || (s->host == ((drift_linux_pcspkr_host_ptr)NULL)))) {
    return ((drift_linux_pcspkr_ptr)NULL);
  }
  return s;
}
int32_t drift_lx_prepare(drift_linux_pcspkr_ptr s, drift_linux_pcspkr_host_ptr h) {
  if(((s == ((drift_linux_pcspkr_ptr)NULL)) || (drift_lx_host_valid(h) == 0))) {
    return -22;
  }
  if(((((s->phase != 0) || (s->allocated != 0)) || (s->registered != 0)) || (s->locked != 0))) {
    return -16;
  }
  s->host = h;
  s->error = 0;
  s->phase = 1U;
  s->irq_flags = 0U;
  nluint8_arr0_ptr p = drift_lx_storage(s);
  for(uint32_t i = 0U; i < 320U; i += 1) {
    p[i] = 0U;
  }
  return 0;
}
int32_t drift_lx_finish(drift_linux_pcspkr_ptr s, int32_t result) {
  if((s->error != 0)) {
    return s->error;
  }
  return result;
}
drift_pcspkr_input_ptr drift_lx_input_allocate_device(void) {
  drift_linux_pcspkr_ptr s = drift_linux_pcspkr_active();
  if((((s == ((drift_linux_pcspkr_ptr)NULL)) || (s->phase != 1)) || (s->allocated != 0))) {
    return ((drift_pcspkr_input_ptr)NULL);
  }
  nluint8_arr0_ptr p = drift_lx_storage(s);
  for(uint32_t i = 0U; i < 256U; i += 1) {
    p[i] = 0U;
  }
  s->allocated = 1U;
  drift_pcspkr_input_ptr d = drift_lx_slot(s);
  drift_lx_set_owner(d, s);
  return d;
}
int32_t drift_lx_input_register_device(drift_pcspkr_input_ptr d) {
  drift_linux_pcspkr_ptr s = drift_linux_pcspkr_active();
  if(((((s == ((drift_linux_pcspkr_ptr)NULL)) || (d == ((drift_pcspkr_input_ptr)NULL))) || (s->allocated == 0)) || (s->registered != 0))) {
    return -22;
  }
  if(((drift_lx_is_owned(d, s) == 0) || (drift_lx_metadata_valid(d, s) == 0))) {
    return -22;
  }
  s->registered = 1U;
  return 0;
}
void drift_lx_input_free_device(drift_pcspkr_input_ptr d) {
  drift_linux_pcspkr_ptr s = drift_linux_pcspkr_active();
  if((s == ((drift_linux_pcspkr_ptr)NULL))) {
    return;
  }
  if((((d == ((drift_pcspkr_input_ptr)NULL)) || (s->allocated == 0)) || (s->registered != 0))) {
    drift_linux_pcspkr_fail(s, -22);
    return;
  }
  if((drift_lx_is_owned(d, s) == 0)) {
    drift_linux_pcspkr_fail(s, -22);
    return;
  }
  s->allocated = 0U;
}
void drift_lx_input_unregister_device(drift_pcspkr_input_ptr d) {
  drift_linux_pcspkr_ptr s = drift_linux_pcspkr_active();
  if((s == ((drift_linux_pcspkr_ptr)NULL))) {
    return;
  }
  if((((d == ((drift_pcspkr_input_ptr)NULL)) || (s->allocated == 0)) || (s->registered == 0))) {
    drift_linux_pcspkr_fail(s, -22);
    return;
  }
  if((drift_lx_is_owned(d, s) == 0)) {
    drift_linux_pcspkr_fail(s, -22);
    return;
  }
  s->registered = 0U;
  s->allocated = 0U;
}
unsigned long drift_lx_lock(void) {
  drift_linux_pcspkr_ptr s = drift_linux_pcspkr_active();
  unsigned long flags = 0U;
  if((s == ((drift_linux_pcspkr_ptr)NULL))) {
    return 0U;
  }
  if((s->error != 0)) {
    return 0U;
  }
  if((s->locked != 0)) {
    drift_linux_pcspkr_fail(s, -16);
    return 0U;
  }
  int32_t result = drift_lx_host_lock(s->host, (&flags));
  if((result != 0)) {
    drift_linux_pcspkr_fail(s, ((result < 0) ? result : -5));
    return 0U;
  }
  s->locked = 1U;
  s->irq_flags = flags;
  return flags;
}
void drift_lx_unlock(unsigned long flags) {
  drift_linux_pcspkr_ptr s = drift_linux_pcspkr_active();
  if((s == ((drift_linux_pcspkr_ptr)NULL))) {
    return;
  }
  if((s->locked == 0)) {
    if((s->error == 0)) {
      drift_linux_pcspkr_fail(s, -22);
    }
    return;
  }
  int32_t result = drift_lx_host_unlock(s->host, flags);
  if((result != 0)) {
    drift_linux_pcspkr_fail(s, ((result < 0) ? result : -5));
    return;
  }
  s->locked = 0U;
}
uint8_t drift_lx_inb(uint16_t port) {
  drift_linux_pcspkr_ptr s = drift_linux_pcspkr_active();
  if((s == ((drift_linux_pcspkr_ptr)NULL))) {
    return 0U;
  }
  if((s->error != 0)) {
    return 0U;
  }
  if(((s->locked == 0) || (port != 0x61))) {
    drift_linux_pcspkr_fail(s, -22);
    return 0U;
  }
  int32_t result = drift_lx_host_read(s->host, port);
  if(((result < 0) || (result > 255))) {
    drift_linux_pcspkr_fail(s, ((result < 0) ? result : -5));
    return 0U;
  }
  return (uint8_t)result;
}
void drift_lx_outb(uint8_t value, uint16_t port) {
  drift_linux_pcspkr_ptr s = drift_linux_pcspkr_active();
  if((s == ((drift_linux_pcspkr_ptr)NULL))) {
    return;
  }
  if((s->error != 0)) {
    return;
  }
  if(((s->locked == 0) || (((port != 0x42) && (port != 0x43)) && (port != 0x61)))) {
    drift_linux_pcspkr_fail(s, -22);
    return;
  }
  int32_t result = drift_lx_host_write(s->host, port, value);
  if((result != 0)) {
    drift_linux_pcspkr_fail(s, ((result < 0) ? result : -5));
  }
}
void drift_lx_delay(void) {
  drift_linux_pcspkr_ptr s = drift_linux_pcspkr_active();
  if((s == ((drift_linux_pcspkr_ptr)NULL))) {
    return;
  }
  if((s->error != 0)) {
    return;
  }
  if((s->locked == 0)) {
    drift_linux_pcspkr_fail(s, -22);
    return;
  }
  int32_t result = drift_lx_host_wait(s->host);
  if((result != 0)) {
    drift_linux_pcspkr_fail(s, ((result < 0) ? result : -5));
  }
}

