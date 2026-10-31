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
#include "drift_virtio.h"
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
#include "drift_pci.h"
/* ------------------------------ DECLARATIONS ------------------------------ */
typedef struct nlniltype {} nlniltype;
typedef struct nlniltype nltype;
typedef uint8_t* nluint8_arr0_ptr;
static void drift_virtio_put(nluint8_arr0_ptr p, uint32_t off, uint64_t value, uint32_t bytes);
static NELUA_INLINE uint64_t nelua_shr_nluint64(uint64_t a, int64_t b);
static uint64_t drift_virtio_get(nluint8_arr0_ptr p, uint32_t off, uint32_t bytes);
static NELUA_INLINE uint64_t nelua_shl_nluint64(uint64_t a, int64_t b);
static void drift_virtio_clear(nluint8_arr0_ptr p, uint32_t bytes);
typedef drift_virtio* drift_virtio_ptr;
static uint32_t drift_virtio_status(drift_virtio_ptr d);
static void drift_virtio_set_status(drift_virtio_ptr d, uint32_t value);
static bool drift_virtio_reset(drift_virtio_ptr d);
NELUA_CEXPORT int32_t drift_virtio_close(drift_virtio_ptr d);
typedef drift_virtio_raw_transfer* drift_virtio_raw_transfer_ptr;
static int32_t drift_virtio_poison(drift_virtio_ptr d, int32_t error_1);
static uint32_t drift_virtio_feature(drift_virtio_ptr d, uint32_t selector);
static void drift_virtio_accept_feature(drift_virtio_ptr d, uint32_t selector, uint32_t value);
static uint64_t drift_virtio_config(drift_virtio_ptr d, uint32_t offset, uint32_t width);
static uint32_t drift_virtio_generation(drift_virtio_ptr d);
typedef drift_dma* drift_dma_ptr;
static bool drift_virtio_allocate(drift_virtio_ptr d, drift_dma_ptr b, uint32_t size);
static bool drift_virtio_queue_init(drift_virtio_ptr d, uint32_t index, uint32_t capacity);
typedef drift_virtqueue* drift_virtqueue_ptr;
static void drift_virtio_descriptor(drift_virtqueue_ptr q, uint32_t index, uint64_t address, uint32_t length, uint32_t flags, uint32_t next);
static void drift_virtio_publish(drift_virtio_ptr d, uint32_t index);
static void drift_virtio_notify(drift_virtio_ptr d, uint32_t index);
static void drift_virtio_submit(drift_virtio_ptr d, uint32_t index);
static int32_t drift_virtio_poll(drift_virtio_ptr d, uint32_t index, uint32_t maximum);
static int32_t drift_virtio_finish(drift_virtio_ptr d, uint32_t index, uint32_t maximum);
static void drift_virtio_receive_post(drift_virtio_ptr d, uint32_t index);
static void drift_virtio_input_post(drift_virtio_ptr d, uint32_t index);
static int32_t drift_virtio_start(drift_virtio_ptr d, bool ready_now, uint32_t raw_queues);
typedef drift_hw* drift_hw_ptr;
static int32_t drift_virtio_open_mmio_mode(drift_virtio_ptr d, drift_hw_ptr h, uint32_t region, uint32_t id, bool ready_now, uint32_t queues);
NELUA_CEXPORT int32_t drift_virtio_open_mmio(drift_virtio_ptr d, drift_hw_ptr h, uint32_t region, uint32_t id);
static int32_t drift_virtio_open_pci_bounded(drift_virtio_ptr d, drift_hw_ptr h, uint32_t common_region, uint32_t common_offset, uint32_t notify_region, uint32_t notify_offset, uint32_t multiplier, uint32_t device_region, uint32_t device_offset, uint32_t id, uint32_t notify_length, bool ready_now, uint32_t queues);
NELUA_CEXPORT int32_t drift_virtio_open_pci(drift_virtio_ptr d, drift_hw_ptr h, uint32_t common_region, uint32_t common_offset, uint32_t notify_region, uint32_t notify_offset, uint32_t multiplier, uint32_t device_region, uint32_t device_offset, uint32_t id);
typedef drift_pci* drift_pci_ptr;
static bool drift_virtio_memory_bar(drift_pci_ptr p, uint32_t bdf, uint32_t index);
typedef drift_virtio_bar* drift_virtio_bar_arr0_ptr;
static int32_t drift_virtio_open_function_mode(drift_virtio_ptr d, drift_hw_ptr h, drift_pci_ptr p, uint32_t bdf, drift_virtio_bar_arr0_ptr bars, bool ready_now, uint32_t expected, uint32_t queues);
NELUA_CEXPORT int32_t drift_virtio_open_function(drift_virtio_ptr d, drift_hw_ptr h, drift_pci_ptr p, uint32_t bdf, drift_virtio_bar_arr0_ptr bars);
NELUA_CEXPORT int32_t drift_virtio_raw_open_mmio(drift_virtio_ptr d, drift_hw_ptr h, uint32_t region, uint32_t id, uint32_t queues);
NELUA_CEXPORT int32_t drift_virtio_raw_open_pci(drift_virtio_ptr d, drift_hw_ptr h, uint32_t cr, uint32_t co, uint32_t nr, uint32_t no, uint32_t nm, uint32_t dr, uint32_t doff, uint32_t id, uint32_t queues);
NELUA_CEXPORT int32_t drift_virtio_raw_open_function(drift_virtio_ptr d, drift_hw_ptr h, drift_pci_ptr p, uint32_t bdf, drift_virtio_bar_arr0_ptr bars, uint32_t id, uint32_t queues);
NELUA_CEXPORT int32_t drift_virtio_raw_device_ready(drift_virtio_ptr d);
static bool drift_virtio_raw_queue(drift_virtio_ptr d, uint32_t index);
NELUA_CEXPORT int32_t drift_virtio_raw_add(drift_virtio_ptr d, uint32_t index, drift_virtio_raw_transfer_ptr t, drift_dma_ptr mapping, uint32_t offset, uint32_t length, uint32_t direction, void* cookie);
NELUA_CEXPORT int32_t drift_virtio_raw_kick(drift_virtio_ptr d, uint32_t index);
typedef void** nlpointer_ptr;
typedef uint32_t* nluint32_ptr;
NELUA_CEXPORT int32_t drift_virtio_raw_get(drift_virtio_ptr d, uint32_t index, drift_virtio_raw_transfer_ptr t, nlpointer_ptr cookie, nluint32_ptr length);
static bool drift_virtio_ready(drift_virtio_ptr d, uint32_t id);
NELUA_CEXPORT int32_t drift_virtio_block(drift_virtio_ptr d, uint64_t sector, uint32_t count, nluint8_arr0_ptr data, int32_t write);
NELUA_CEXPORT int32_t drift_virtio_flush(drift_virtio_ptr d);
NELUA_CEXPORT int32_t drift_virtio_send(drift_virtio_ptr d, nluint8_arr0_ptr data, uint32_t size);
static int32_t drift_virtio_receive_data(drift_virtio_ptr d, nluint8_arr0_ptr data, uint32_t capacity, uint32_t header);
NELUA_CEXPORT int32_t drift_virtio_receive(drift_virtio_ptr d, nluint8_arr0_ptr data, uint32_t capacity);
NELUA_CEXPORT int32_t drift_virtio_console_write(drift_virtio_ptr d, nluint8_arr0_ptr data, uint32_t size);
NELUA_CEXPORT int32_t drift_virtio_console_read(drift_virtio_ptr d, nluint8_arr0_ptr data, uint32_t capacity);
NELUA_CEXPORT int32_t drift_virtio_entropy(drift_virtio_ptr d, nluint8_arr0_ptr data, uint32_t capacity);
typedef drift_virtio_input_event* drift_virtio_input_event_ptr;
NELUA_CEXPORT int32_t drift_virtio_input(drift_virtio_ptr d, drift_virtio_input_event_ptr event);
static NELUA_INLINE uint32_t nelua_shl_nluint32(uint32_t a, int32_t b);
/* ------------------------------ DEFINITIONS ------------------------------- */
uint64_t nelua_shr_nluint64(uint64_t a, int64_t b) {
  if(NELUA_LIKELY(b >= 0 && b < 64)) {
    return (uint64_t)a >> b;
  } else if(NELUA_UNLIKELY(b < 0 && b > -64)) {
    return (uint64_t)a << -b;
  } else {
    return 0;
  }
}
void drift_virtio_put(nluint8_arr0_ptr p, uint32_t off, uint64_t value, uint32_t bytes) {
  for(uint32_t i = 0U, _end = bytes; i < _end; i += 1) {
    p[(off + i)] = (uint8_t)nelua_shr_nluint64(value, (i * 8));
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
uint64_t drift_virtio_get(nluint8_arr0_ptr p, uint32_t off, uint32_t bytes) {
  uint64_t value = 0U;
  for(uint32_t i = 0U, _end = bytes; i < _end; i += 1) {
    value = (value | nelua_shl_nluint64((uint64_t)p[(off + i)], (i * 8)));
  }
  return value;
}
void drift_virtio_clear(nluint8_arr0_ptr p, uint32_t bytes) {
  for(uint32_t i = 0U, _end = bytes; i < _end; i += 1) {
    p[i] = 0U;
  }
}
uint32_t drift_virtio_status(drift_virtio_ptr d) {
  if((d->transport == 0)) {
    return (uint32_t)drift_hw_read(d->hw, d->region, 0x70U, 4U);
  }
  return (uint32_t)drift_hw_read(d->hw, d->region, (d->common_offset + 20), 1U);
}
void drift_virtio_set_status(drift_virtio_ptr d, uint32_t value) {
  if((d->transport == 0)) {
    drift_hw_write(d->hw, d->region, 0x70U, 4U, (uint64_t)value);
  } else {
    drift_hw_write(d->hw, d->region, (d->common_offset + 20), 1U, (uint64_t)value);
  }
}
bool drift_virtio_reset(drift_virtio_ptr d) {
  drift_virtio_set_status(d, 0U);
  uint64_t start = drift_hw_time(d->hw);
  while((drift_virtio_status(d) != 0)) {
    if(((drift_hw_time(d->hw) - start) >= d->timeout_us)) {
      return false;
    }
    drift_hw_wait(d->hw, 10U);
  }
  return true;
}
int32_t drift_virtio_close(drift_virtio_ptr d) {
  if((d == ((drift_virtio_ptr)NULL))) {
    return -1;
  }
  if((d->state == 0)) {
    return 0;
  }
  if((drift_hw_valid(d->hw) == 0)) {
    return -1;
  }
  if((!drift_virtio_reset(d))) {
    d->state = 3U;
    return -2;
  }
  for(uint32_t i = 0U; i <= 1U; i += 1) {
    if((d->queue[i].raw != ((drift_virtio_raw_transfer_ptr)NULL))) {
      d->queue[i].raw->active = 0U;
      d->queue[i].raw->owner = (void*)NULL;
    }
    if((d->queue[i].buffer.data != ((nluint8_arr0_ptr)NULL))) {
      drift_hw_release(d->hw, (&d->queue[i].buffer));
    }
    if((d->queue[i].ring.data != ((nluint8_arr0_ptr)NULL))) {
      drift_hw_release(d->hw, (&d->queue[i].ring));
    }
    d->queue[i] = (drift_virtqueue){0};
  }
  d->state = 0U;
  return 0;
}
int32_t drift_virtio_poison(drift_virtio_ptr d, int32_t error_1) {
  d->state = 3U;
  drift_virtio_close(d);
  return error_1;
}
uint32_t drift_virtio_feature(drift_virtio_ptr d, uint32_t selector) {
  if((d->transport == 0)) {
    drift_hw_write(d->hw, d->region, 0x14U, 4U, (uint64_t)selector);
    return (uint32_t)drift_hw_read(d->hw, d->region, 0x10U, 4U);
  }
  drift_hw_write(d->hw, d->region, d->common_offset, 4U, (uint64_t)selector);
  return (uint32_t)drift_hw_read(d->hw, d->region, (d->common_offset + 4), 4U);
}
void drift_virtio_accept_feature(drift_virtio_ptr d, uint32_t selector, uint32_t value) {
  if((d->transport == 0)) {
    drift_hw_write(d->hw, d->region, 0x24U, 4U, (uint64_t)selector);
    drift_hw_write(d->hw, d->region, 0x20U, 4U, (uint64_t)value);
  } else {
    drift_hw_write(d->hw, d->region, (d->common_offset + 8), 4U, (uint64_t)selector);
    drift_hw_write(d->hw, d->region, (d->common_offset + 12), 4U, (uint64_t)value);
  }
}
uint64_t drift_virtio_config(drift_virtio_ptr d, uint32_t offset, uint32_t width) {
  if((d->transport == 0)) {
    return drift_hw_read(d->hw, d->region, (0x100 + offset), width);
  }
  return drift_hw_read(d->hw, d->device_region, (d->device_offset + offset), width);
}
uint32_t drift_virtio_generation(drift_virtio_ptr d) {
  if((d->transport == 0)) {
    return (uint32_t)drift_hw_read(d->hw, d->region, 0xfcU, 4U);
  }
  return (uint32_t)drift_hw_read(d->hw, d->region, (d->common_offset + 21), 1U);
}
bool drift_virtio_allocate(drift_virtio_ptr d, drift_dma_ptr b, uint32_t size) {
  b->data = ((nluint8_arr0_ptr)NULL);
  if((drift_hw_dma(d->hw, size, 4096U, b) != 0)) {
    return false;
  }
  if(((((b->data == ((nluint8_arr0_ptr)NULL)) || (b->size < size)) || ((b->address & 4095) != 0)) || (b->address > (0xffffffffffffffffULL - size)))) {
    if((b->data != ((nluint8_arr0_ptr)NULL))) {
      drift_hw_release(d->hw, b);
    }
    b->data = ((nluint8_arr0_ptr)NULL);
    return false;
  }
  drift_virtio_clear(b->data, size);
  return true;
}
bool drift_virtio_queue_init(drift_virtio_ptr d, uint32_t index, uint32_t capacity) {
  drift_virtqueue_ptr q = (&d->queue[index]);
  uint32_t maximum = 0U;
  if((d->transport == 0)) {
    drift_hw_write(d->hw, d->region, 0x30U, 4U, (uint64_t)index);
    if((drift_hw_read(d->hw, d->region, 0x44U, 4U) != 0)) {
      return false;
    }
    maximum = (uint32_t)drift_hw_read(d->hw, d->region, 0x34U, 4U);
  } else {
    drift_hw_write(d->hw, d->region, (d->common_offset + 22), 2U, (uint64_t)index);
    if((drift_hw_read(d->hw, d->region, (d->common_offset + 28), 2U) != 0)) {
      return false;
    }
    maximum = (uint32_t)drift_hw_read(d->hw, d->region, (d->common_offset + 24), 2U);
    uint64_t notification = drift_hw_read(d->hw, d->region, (d->common_offset + 30), 2U);
    uint64_t offset = ((notification * d->notify_multiplier) + d->notify_offset);
    if((((((notification * d->notify_multiplier) + 2) > d->notify_length) || (offset > 0xfffffffdULL)) || ((offset & 1) != 0))) {
      return false;
    }
    q->notify = (uint32_t)offset;
  }
  if(((maximum < 8) || (!drift_virtio_allocate(d, (&q->ring), 4096U)))) {
    return false;
  }
  if(((capacity != 0) && (!drift_virtio_allocate(d, (&q->buffer), capacity)))) {
    return false;
  }
  q->capacity = capacity;
  drift_virtio_put(q->ring.data, 128U, 1U, 2U);
  drift_hw_sync(d->hw, (&q->ring), 0U, 4096U, 3U);
  if((d->transport == 0)) {
    drift_hw_write(d->hw, d->region, 0x38U, 4U, 8U);
    drift_hw_write(d->hw, d->region, 0x80U, 4U, (uint64_t)(uint32_t)q->ring.address);
    drift_hw_write(d->hw, d->region, 0x84U, 4U, (q->ring.address >> 32));
    drift_hw_write(d->hw, d->region, 0x90U, 4U, (uint64_t)(uint32_t)(q->ring.address + 128));
    drift_hw_write(d->hw, d->region, 0x94U, 4U, ((q->ring.address + 128) >> 32));
    drift_hw_write(d->hw, d->region, 0xa0U, 4U, (uint64_t)(uint32_t)(q->ring.address + 256));
    drift_hw_write(d->hw, d->region, 0xa4U, 4U, ((q->ring.address + 256) >> 32));
    drift_hw_write(d->hw, d->region, 0x44U, 4U, 1U);
  } else {
    drift_hw_write(d->hw, d->region, (d->common_offset + 24), 2U, 8U);
    drift_hw_write(d->hw, d->region, (d->common_offset + 26), 2U, 0xffffU);
    for(uint32_t part = 0U; part <= 1U; part += 1) {
      drift_hw_write(d->hw, d->region, ((d->common_offset + 32) + (part * 4)), 4U, (uint64_t)(uint32_t)nelua_shr_nluint64(q->ring.address, (part * 32)));
      drift_hw_write(d->hw, d->region, ((d->common_offset + 40) + (part * 4)), 4U, (uint64_t)(uint32_t)nelua_shr_nluint64((q->ring.address + 128), (part * 32)));
      drift_hw_write(d->hw, d->region, ((d->common_offset + 48) + (part * 4)), 4U, (uint64_t)(uint32_t)nelua_shr_nluint64((q->ring.address + 256), (part * 32)));
    }
    drift_hw_write(d->hw, d->region, (d->common_offset + 28), 2U, 1U);
  }
  return true;
}
void drift_virtio_descriptor(drift_virtqueue_ptr q, uint32_t index, uint64_t address, uint32_t length, uint32_t flags, uint32_t next) {
  drift_virtio_put(q->ring.data, (index * 16), address, 8U);
  drift_virtio_put(q->ring.data, ((index * 16) + 8), (uint64_t)length, 4U);
  drift_virtio_put(q->ring.data, ((index * 16) + 12), (uint64_t)flags, 2U);
  drift_virtio_put(q->ring.data, ((index * 16) + 14), (uint64_t)next, 2U);
}
void drift_virtio_publish(drift_virtio_ptr d, uint32_t index) {
  drift_virtqueue_ptr q = (&d->queue[index]);
  drift_virtio_put(q->ring.data, (132 + (((uint32_t)q->available & 7) * 2)), 0U, 2U);
  q->available = (q->available + 1);
  if((q->buffer.data != ((nluint8_arr0_ptr)NULL))) {
    drift_hw_sync(d->hw, (&q->buffer), 0U, q->capacity, 3U);
  }
  drift_hw_sync(d->hw, (&q->ring), 0U, 128U, 1U);
  drift_hw_sync(d->hw, (&q->ring), 132U, 16U, 1U);
  drift_virtio_put(q->ring.data, 130U, (uint64_t)q->available, 2U);
  drift_hw_sync(d->hw, (&q->ring), 128U, 20U, 1U);
  q->pending = 1U;
}
void drift_virtio_notify(drift_virtio_ptr d, uint32_t index) {
  drift_virtqueue_ptr q = (&d->queue[index]);
  if((d->transport == 0)) {
    drift_hw_write(d->hw, d->region, 0x50U, 4U, (uint64_t)index);
  } else {
    drift_hw_write(d->hw, d->notify_region, q->notify, 2U, (uint64_t)index);
  }
}
void drift_virtio_submit(drift_virtio_ptr d, uint32_t index) {
  drift_virtio_publish(d, index);
  drift_virtio_notify(d, index);
}
int32_t drift_virtio_poll(drift_virtio_ptr d, uint32_t index, uint32_t maximum) {
  drift_virtqueue_ptr q = (&d->queue[index]);
  drift_hw_sync(d->hw, (&q->ring), 256U, 68U, 2U);
  uint16_t used = (uint16_t)drift_virtio_get(q->ring.data, 258U, 2U);
  if((used == q->used)) {
    if(((drift_virtio_status(d) & 0xc0) != 0)) {
      return drift_virtio_poison(d, -5);
    }
    return -7;
  }
  if((((((uint32_t)used - (uint32_t)q->used) & 65535) != 1) || (drift_virtio_get(q->ring.data, (260 + (((uint32_t)q->used & 7) * 8)), 4U) != 0))) {
    return drift_virtio_poison(d, -5);
  }
  uint64_t length = drift_virtio_get(q->ring.data, (264 + (((uint32_t)q->used & 7) * 8)), 4U);
  if((length > maximum)) {
    return drift_virtio_poison(d, -5);
  }
  q->used = used;
  q->pending = 0U;
  if((q->buffer.data != ((nluint8_arr0_ptr)NULL))) {
    drift_hw_sync(d->hw, (&q->buffer), 0U, q->capacity, 2U);
  }
  return (int32_t)length;
}
int32_t drift_virtio_finish(drift_virtio_ptr d, uint32_t index, uint32_t maximum) {
  uint64_t start = drift_hw_time(d->hw);
  while(true) {
    int32_t result = drift_virtio_poll(d, index, maximum);
    if((result != -7)) {
      return result;
    }
    if(((drift_hw_time(d->hw) - start) >= d->timeout_us)) {
      return drift_virtio_poison(d, -2);
    }
    drift_hw_wait(d->hw, 10U);
  }
}
void drift_virtio_receive_post(drift_virtio_ptr d, uint32_t index) {
  drift_virtqueue_ptr q = (&d->queue[index]);
  drift_virtio_descriptor(q, 0U, q->buffer.address, q->capacity, 2U, 0U);
  drift_virtio_submit(d, index);
}
void drift_virtio_input_post(drift_virtio_ptr d, uint32_t index) {
  drift_virtqueue_ptr q = (&d->queue[0]);
  drift_virtio_descriptor(q, index, (q->buffer.address + (index * 8)), 8U, 2U, 0U);
  drift_hw_sync(d->hw, (&q->buffer), (index * 8), 8U, 3U);
  drift_hw_sync(d->hw, (&q->ring), (index * 16), 16U, 1U);
  drift_virtio_put(q->ring.data, (132 + (((uint32_t)q->available & 7) * 2)), (uint64_t)index, 2U);
  q->available = (q->available + 1);
  drift_hw_sync(d->hw, (&q->ring), 132U, 16U, 1U);
  drift_virtio_put(q->ring.data, 130U, (uint64_t)q->available, 2U);
  drift_hw_sync(d->hw, (&q->ring), 128U, 20U, 1U);
  if((d->transport == 0)) {
    drift_hw_write(d->hw, d->region, 0x50U, 4U, 0U);
  } else {
    drift_hw_write(d->hw, d->notify_region, q->notify, 2U, 0U);
  }
}
int32_t drift_virtio_start(drift_virtio_ptr d, bool ready_now, uint32_t raw_queues) {
  for(uint32_t i = 0U; i <= 5U; i += 1) {
    d->mac[i] = 0U;
  }
  if(ready_now) {
    if((((((d->device_id != 1) && (d->device_id != 2)) && (d->device_id != 3)) && (d->device_id != 4)) && (d->device_id != 18))) {
      return drift_virtio_poison(d, -4);
    }
  } else if(((((d->device_id == 0) || (d->device_id > 63)) || (raw_queues == 0)) || (raw_queues > 2))) {
    return drift_virtio_poison(d, -4);
  }
  if((!drift_virtio_reset(d))) {
    d->state = 3U;
    return -2;
  }
  drift_virtio_set_status(d, 1U);
  drift_virtio_set_status(d, 3U);
  if(((drift_virtio_feature(d, 1U) & 1) == 0)) {
    return drift_virtio_poison(d, -4);
  }
  uint32_t offered = drift_virtio_feature(d, 0U);
  d->features = 0U;
  if((ready_now && (d->device_id == 1))) {
    d->features = (offered & 0x20);
  }
  if((ready_now && (d->device_id == 2))) {
    d->features = (offered & 0x220);
  }
  drift_virtio_accept_feature(d, 0U, d->features);
  drift_virtio_accept_feature(d, 1U, 1U);
  drift_virtio_set_status(d, 11U);
  if(((drift_virtio_status(d) & 8) == 0)) {
    return drift_virtio_poison(d, -4);
  }
  if((!ready_now)) {
    for(uint32_t i = 0U, _end = raw_queues; i < _end; i += 1) {
      if((!drift_virtio_queue_init(d, i, 0U))) {
        return drift_virtio_poison(d, -6);
      }
    }
    d->capacity = (uint64_t)raw_queues;
    d->state = 4U;
    return 0;
  }
  uint32_t capacity = 4096U;
  if((d->device_id == 1)) {
    capacity = 2048U;
  } else if((d->device_id == 2)) {
    capacity = 65568U;
  } else if((d->device_id == 18)) {
    capacity = 64U;
  }
  if((!drift_virtio_queue_init(d, 0U, capacity))) {
    return drift_virtio_poison(d, -6);
  }
  if(((d->device_id == 1) || (d->device_id == 3))) {
    if((!drift_virtio_queue_init(d, 1U, capacity))) {
      return drift_virtio_poison(d, -6);
    }
  }
  if(((d->device_id == 2) || ((d->device_id == 1) && ((d->features & 0x20) != 0)))) {
    bool stable = false;
    for(uint32_t attempt = 0U; attempt <= 15U; attempt += 1) {
      uint32_t before = drift_virtio_generation(d);
      if((d->device_id == 2)) {
        d->capacity = (drift_virtio_config(d, 0U, 4U) | (drift_virtio_config(d, 4U, 4U) << 32));
      } else {
        for(uint32_t i = 0U; i <= 5U; i += 1) {
          d->mac[i] = (uint8_t)drift_virtio_config(d, i, 1U);
        }
      }
      if((before == drift_virtio_generation(d))) {
        stable = true;
        break;
      }
    }
    if(((!stable) || ((d->device_id == 2) && (d->capacity == 0)))) {
      return drift_virtio_poison(d, -5);
    }
  }
  drift_virtio_set_status(d, 15U);
  if(((drift_virtio_status(d) & 0xcf) != 15)) {
    return drift_virtio_poison(d, -5);
  }
  d->state = 2U;
  if(((d->device_id == 1) || (d->device_id == 3))) {
    drift_virtio_receive_post(d, 0U);
  } else if((d->device_id == 18)) {
    for(uint32_t i = 0U; i <= 7U; i += 1) {
      drift_virtio_input_post(d, i);
    }
  }
  return 0;
}
int32_t drift_virtio_open_mmio_mode(drift_virtio_ptr d, drift_hw_ptr h, uint32_t region, uint32_t id, bool ready_now, uint32_t queues) {
  if((((d == ((drift_virtio_ptr)NULL)) || (drift_hw_valid(h) == 0)) || (d->state != 0))) {
    return -1;
  }
  if((((drift_hw_read(h, region, 0U, 4U) != 0x74726976) || (drift_hw_read(h, region, 4U, 4U) != 2)) || (drift_hw_read(h, region, 8U, 4U) != id))) {
    return -1;
  }
  d->hw = h;
  d->region = region;
  d->transport = 0U;
  d->device_id = id;
  d->timeout_us = 1000000U;
  d->state = 1U;
  return drift_virtio_start(d, ready_now, queues);
}
int32_t drift_virtio_open_mmio(drift_virtio_ptr d, drift_hw_ptr h, uint32_t region, uint32_t id) {
  return drift_virtio_open_mmio_mode(d, h, region, id, true, 0U);
}
int32_t drift_virtio_open_pci_bounded(drift_virtio_ptr d, drift_hw_ptr h, uint32_t common_region, uint32_t common_offset, uint32_t notify_region, uint32_t notify_offset, uint32_t multiplier, uint32_t device_region, uint32_t device_offset, uint32_t id, uint32_t notify_length, bool ready_now, uint32_t queues) {
  if(((((((((d == ((drift_virtio_ptr)NULL)) || (drift_hw_valid(h) == 0)) || (d->state != 0)) || (common_offset > 0xffffffc0LL)) || (device_offset > 0xfffffeffLL)) || ((common_offset & 3) != 0)) || ((notify_offset & 1) != 0)) || ((id == 2) && ((device_offset & 3) != 0)))) {
    return -1;
  }
  d->hw = h;
  d->region = common_region;
  d->common_offset = common_offset;
  d->notify_region = notify_region;
  d->notify_offset = notify_offset;
  d->notify_multiplier = multiplier;
  d->notify_length = notify_length;
  d->device_region = device_region;
  d->device_offset = device_offset;
  d->transport = 1U;
  d->device_id = id;
  d->timeout_us = 1000000U;
  d->state = 1U;
  return drift_virtio_start(d, ready_now, queues);
}
int32_t drift_virtio_open_pci(drift_virtio_ptr d, drift_hw_ptr h, uint32_t common_region, uint32_t common_offset, uint32_t notify_region, uint32_t notify_offset, uint32_t multiplier, uint32_t device_region, uint32_t device_offset, uint32_t id) {
  return drift_virtio_open_pci_bounded(d, h, common_region, common_offset, notify_region, notify_offset, multiplier, device_region, device_offset, id, (0xffffffffULL - notify_offset), true, 0U);
}
bool drift_virtio_memory_bar(drift_pci_ptr p, uint32_t bdf, uint32_t index) {
  uint32_t i = 0U;
  while((i <= index)) {
    uint32_t raw = drift_pci_read(p, bdf, (16 + (i * 4)));
    if((i == index)) {
      return ((((raw != 0) && (raw != 0xffffffffLL)) && ((raw & 1) == 0)) && (((raw & 6) == 0) || (((raw & 6) == 4) && (i < 5))));
    }
    if((((raw & 1) == 0) && ((raw & 6) == 4))) {
      i = (i + 2);
    } else {
      i = (i + 1);
    }
  }
  return false;
}
int32_t drift_virtio_open_function_mode(drift_virtio_ptr d, drift_hw_ptr h, drift_pci_ptr p, uint32_t bdf, drift_virtio_bar_arr0_ptr bars, bool ready_now, uint32_t expected, uint32_t queues) {
  if(((((((d == ((drift_virtio_ptr)NULL)) || (d->state != 0)) || (drift_hw_valid(h) == 0)) || (drift_pci_valid(p) == 0)) || (bdf > 65535)) || (bars == ((drift_virtio_bar_arr0_ptr)NULL)))) {
    return -1;
  }
  uint32_t identity = drift_pci_read(p, bdf, 0U);
  uint32_t device_id = (identity >> 16);
  if(((((identity & 65535) != 0x1af4) || (device_id < 0x1041)) || (device_id > 0x107f))) {
    return -4;
  }
  device_id = (device_id - 0x1040);
  if(ready_now) {
    if((((((device_id != 1) && (device_id != 2)) && (device_id != 3)) && (device_id != 4)) && (device_id != 18))) {
      return -4;
    }
  } else if((((device_id != expected) || (queues == 0)) || (queues > 2))) {
    return -4;
  }
  if((((drift_pci_read(p, bdf, 12U) >> 16) & 127) != 0)) {
    return -4;
  }
  uint32_t command = drift_pci_read(p, bdf, 4U);
  if(((command & 6) != 6)) {
    return -1;
  }
  if(((command & 0x100000) == 0)) {
    return -4;
  }
  uint32_t common_bar = 0xffffffffULL;
  uint32_t notify_bar = 0xffffffffULL;
  uint32_t device_bar = 0xffffffffULL;
  uint32_t common_offset = 0U;
  uint32_t notify_offset = 0U;
  uint32_t device_offset = 0U;
  uint32_t notify_length = 0U;
  uint32_t multiplier = 0U;
  uint32_t at = (drift_pci_read(p, bdf, 0x34U) & 255);
  uint64_t occupied = 0U;
  while((at != 0)) {
    if((((at < 64) || (at > 252)) || ((at & 3) != 0))) {
      return -5;
    }
    uint64_t bit = nelua_shl_nluint64(1U, (at >> 2));
    if(((occupied & bit) != 0)) {
      return -5;
    }
    occupied = (occupied | bit);
    uint32_t head = drift_pci_read(p, bdf, at);
    if(((head & 255) == 9)) {
      uint32_t length = ((head >> 16) & 255);
      uint32_t kind = (head >> 24);
      if(((length < 16) || (length > (256 - at)))) {
        return -5;
      }
      for(uint32_t offset = 4U, _end = length; offset < _end; offset += 4) {
        bit = nelua_shl_nluint64(1U, ((at + offset) >> 2));
        if(((occupied & bit) != 0)) {
          return -5;
        }
        occupied = (occupied | bit);
      }
      if((((kind == 1) || (kind == 2)) || (kind == 4))) {
        uint32_t bar = (drift_pci_read(p, bdf, (at + 4)) & 255);
        uint32_t offset = drift_pci_read(p, bdf, (at + 8));
        uint32_t span = drift_pci_read(p, bdf, (at + 12));
        if((((((((bar >= 6) || (!drift_virtio_memory_bar(p, bdf, bar))) || (bars[bar].region == 0xffffffffLL)) || (span == 0)) || ((uint64_t)offset > bars[bar].size)) || ((uint64_t)span > (bars[bar].size - (uint64_t)offset))) || (((uint64_t)offset + (uint64_t)span) > 0x100000000ULL))) {
          return -3;
        }
        if((kind == 1)) {
          if((((common_bar != 0xffffffffLL) || (span < 56)) || ((offset & 3) != 0))) {
            return -5;
          }
          common_bar = bar;
          common_offset = offset;
        } else if((kind == 2)) {
          if(((((notify_bar != 0xffffffffLL) || (length < 20)) || (span < 2)) || ((offset & 1) != 0))) {
            return -5;
          }
          notify_bar = bar;
          notify_offset = offset;
          notify_length = span;
          multiplier = drift_pci_read(p, bdf, (at + 16));
        } else {
          if((((device_bar != 0xffffffffLL) || ((ready_now && (device_id == 2)) && ((span < 8) || ((offset & 3) != 0)))) || ((ready_now && (device_id == 1)) && (span < 6)))) {
            return -5;
          }
          device_bar = bar;
          device_offset = offset;
        }
      }
    }
    at = ((head >> 8) & 255);
  }
  if((((common_bar == 0xffffffffLL) || (notify_bar == 0xffffffffLL)) || ((ready_now && ((device_id == 1) || (device_id == 2))) && (device_bar == 0xffffffffLL)))) {
    return -4;
  }
  uint32_t device_region = 0U;
  if((device_bar != 0xffffffffLL)) {
    device_region = bars[device_bar].region;
  }
  return drift_virtio_open_pci_bounded(d, h, bars[common_bar].region, common_offset, bars[notify_bar].region, notify_offset, multiplier, device_region, device_offset, device_id, notify_length, ready_now, queues);
}
int32_t drift_virtio_open_function(drift_virtio_ptr d, drift_hw_ptr h, drift_pci_ptr p, uint32_t bdf, drift_virtio_bar_arr0_ptr bars) {
  return drift_virtio_open_function_mode(d, h, p, bdf, bars, true, 0U, 0U);
}
int32_t drift_virtio_raw_open_mmio(drift_virtio_ptr d, drift_hw_ptr h, uint32_t region, uint32_t id, uint32_t queues) {
  if(((((queues == 0) || (queues > 2)) || (id == 0)) || (id > 63))) {
    return -1;
  }
  return drift_virtio_open_mmio_mode(d, h, region, id, false, queues);
}
int32_t drift_virtio_raw_open_pci(drift_virtio_ptr d, drift_hw_ptr h, uint32_t cr, uint32_t co, uint32_t nr, uint32_t no, uint32_t nm, uint32_t dr, uint32_t doff, uint32_t id, uint32_t queues) {
  if(((((queues == 0) || (queues > 2)) || (id == 0)) || (id > 63))) {
    return -1;
  }
  return drift_virtio_open_pci_bounded(d, h, cr, co, nr, no, nm, dr, doff, id, (0xffffffffULL - no), false, queues);
}
int32_t drift_virtio_raw_open_function(drift_virtio_ptr d, drift_hw_ptr h, drift_pci_ptr p, uint32_t bdf, drift_virtio_bar_arr0_ptr bars, uint32_t id, uint32_t queues) {
  if(((((queues == 0) || (queues > 2)) || (id == 0)) || (id > 63))) {
    return -1;
  }
  return drift_virtio_open_function_mode(d, h, p, bdf, bars, false, id, queues);
}
int32_t drift_virtio_raw_device_ready(drift_virtio_ptr d) {
  if(((((d == ((drift_virtio_ptr)NULL)) || (d->state != 4)) || (d->capacity == 0)) || (d->capacity > 2))) {
    return -1;
  }
  drift_virtio_set_status(d, 15U);
  if(((drift_virtio_status(d) & 0xcf) != 15)) {
    return drift_virtio_poison(d, -5);
  }
  d->state = 5U;
  return 0;
}
bool drift_virtio_raw_queue(drift_virtio_ptr d, uint32_t index) {
  return (((((d != ((drift_virtio_ptr)NULL)) && (d->state == 5)) && (d->capacity > 0)) && (d->capacity <= 2)) && (index < d->capacity));
}
int32_t drift_virtio_raw_add(drift_virtio_ptr d, uint32_t index, drift_virtio_raw_transfer_ptr t, drift_dma_ptr mapping, uint32_t offset, uint32_t length, uint32_t direction, void* cookie) {
  if(((((((!drift_virtio_raw_queue(d, index)) || (t == ((drift_virtio_raw_transfer_ptr)NULL))) || (mapping == ((drift_dma_ptr)NULL))) || (cookie == (void*)NULL)) || (mapping->data == ((nluint8_arr0_ptr)NULL))) || ((direction != 1) && (direction != 2)))) {
    return -1;
  }
  if((((((length == 0) || (offset > mapping->size)) || (length > (mapping->size - offset))) || (mapping->address > (0xffffffffffffffffULL - offset))) || ((mapping->address + offset) > (0xffffffffffffffffULL - length)))) {
    return -3;
  }
  drift_virtqueue_ptr q = (&d->queue[index]);
  if((((q->pending != 0) || (q->raw != ((drift_virtio_raw_transfer_ptr)NULL))) || (t->active != 0))) {
    return -7;
  }
  t->mapping = (*mapping);
  t->cookie = cookie;
  t->owner = (void*)d;
  t->queue = index;
  t->offset = offset;
  t->length = length;
  t->direction = direction;
  t->started_us = drift_hw_time(d->hw);
  t->active = 1U;
  q->raw = t;
  drift_hw_sync(d->hw, (&t->mapping), offset, length, ((direction == 2) ? 3U : 1U));
  drift_virtio_descriptor(q, 0U, (mapping->address + offset), length, ((direction == 2) ? 2U : 0U), 0U);
  drift_virtio_publish(d, index);
  return 0;
}
int32_t drift_virtio_raw_kick(drift_virtio_ptr d, uint32_t index) {
  if((!drift_virtio_raw_queue(d, index))) {
    return -1;
  }
  drift_virtio_raw_transfer_ptr t = d->queue[index].raw;
  if((((((t == ((drift_virtio_raw_transfer_ptr)NULL)) || (t->active == 0)) || (t->owner != (void*)d)) || (t->queue != index)) || (d->queue[index].pending == 0))) {
    return -1;
  }
  drift_virtio_notify(d, index);
  return 0;
}
int32_t drift_virtio_raw_get(drift_virtio_ptr d, uint32_t index, drift_virtio_raw_transfer_ptr t, nlpointer_ptr cookie, nluint32_ptr length) {
  if(((((!drift_virtio_raw_queue(d, index)) || (t == ((drift_virtio_raw_transfer_ptr)NULL))) || (cookie == (nlpointer_ptr)NULL)) || (length == (nluint32_ptr)NULL))) {
    return -1;
  }
  (*cookie) = (void*)NULL;
  (*length) = 0U;
  drift_virtqueue_ptr q = (&d->queue[index]);
  if(((q->raw == ((drift_virtio_raw_transfer_ptr)NULL)) && (t->active == 0))) {
    return 0;
  }
  if((((((q->raw != t) || (t->active == 0)) || (t->owner != (void*)d)) || (t->queue != index)) || (q->pending == 0))) {
    return -1;
  }
  int32_t result = drift_virtio_poll(d, index, ((t->direction == 2) ? t->length : 0U));
  if((result == -7)) {
    if(((drift_hw_time(d->hw) - t->started_us) >= d->timeout_us)) {
      return drift_virtio_poison(d, -2);
    }
    return 0;
  }
  if((result < 0)) {
    return result;
  }
  if((t->direction == 2)) {
    drift_hw_sync(d->hw, (&t->mapping), t->offset, t->length, 2U);
  }
  (*cookie) = t->cookie;
  (*length) = (uint32_t)result;
  q->raw = ((drift_virtio_raw_transfer_ptr)NULL);
  t->active = 0U;
  t->owner = (void*)NULL;
  return 1;
}
bool drift_virtio_ready(drift_virtio_ptr d, uint32_t id) {
  return (((d != ((drift_virtio_ptr)NULL)) && (d->state == 2)) && (d->device_id == id));
}
int32_t drift_virtio_block(drift_virtio_ptr d, uint64_t sector, uint32_t count, nluint8_arr0_ptr data, int32_t write) {
  if((((!drift_virtio_ready(d, 2U)) || (data == ((nluint8_arr0_ptr)NULL))) || ((write != 0) && (write != 1)))) {
    return -1;
  }
  if(((((count == 0) || (count > 128)) || (sector >= d->capacity)) || (count > (d->capacity - sector)))) {
    return -3;
  }
  if(((write != 0) && ((d->features & 0x20) != 0))) {
    return -4;
  }
  drift_virtqueue_ptr q = (&d->queue[0]);
  uint32_t bytes = (count * 512);
  drift_virtio_put(q->buffer.data, 0U, (uint64_t)(uint32_t)write, 4U);
  drift_virtio_put(q->buffer.data, 4U, 0U, 4U);
  drift_virtio_put(q->buffer.data, 8U, sector, 8U);
  q->buffer.data[65552] = 255U;
  if((write != 0)) {
    for(uint32_t i = 0U, _end = bytes; i < _end; i += 1) {
      q->buffer.data[(16 + i)] = data[i];
    }
  }
  drift_virtio_descriptor(q, 0U, q->buffer.address, 16U, 1U, 1U);
  drift_virtio_descriptor(q, 1U, (q->buffer.address + 16), bytes, (1 | ((write == 0) ? 2U : 0U)), 2U);
  drift_virtio_descriptor(q, 2U, (q->buffer.address + 65552), 1U, 2U, 0U);
  drift_virtio_submit(d, 0U);
  int32_t result = drift_virtio_finish(d, 0U, (bytes + 1));
  if((result < 0)) {
    return result;
  }
  if((q->buffer.data[65552] != 0)) {
    return -5;
  }
  if((write == 0)) {
    if(((uint32_t)result != (bytes + 1))) {
      return drift_virtio_poison(d, -5);
    }
    for(uint32_t i = 0U, _end = bytes; i < _end; i += 1) {
      data[i] = q->buffer.data[(16 + i)];
    }
  } else if((result != 1)) {
    return drift_virtio_poison(d, -5);
  }
  return 0;
}
int32_t drift_virtio_flush(drift_virtio_ptr d) {
  if((!drift_virtio_ready(d, 2U))) {
    return -1;
  }
  if(((d->features & 0x200) == 0)) {
    return -4;
  }
  drift_virtqueue_ptr q = (&d->queue[0]);
  drift_virtio_put(q->buffer.data, 0U, 4U, 4U);
  drift_virtio_put(q->buffer.data, 4U, 0U, 4U);
  drift_virtio_put(q->buffer.data, 8U, 0U, 8U);
  q->buffer.data[65552] = 255U;
  drift_virtio_descriptor(q, 0U, q->buffer.address, 16U, 1U, 1U);
  drift_virtio_descriptor(q, 1U, (q->buffer.address + 65552), 1U, 2U, 0U);
  drift_virtio_submit(d, 0U);
  int32_t result = drift_virtio_finish(d, 0U, 1U);
  if((result < 0)) {
    return result;
  }
  if(((result != 1) || (q->buffer.data[65552] != 0))) {
    return -5;
  }
  return 0;
}
int32_t drift_virtio_send(drift_virtio_ptr d, nluint8_arr0_ptr data, uint32_t size) {
  if(((!drift_virtio_ready(d, 1U)) || (data == ((nluint8_arr0_ptr)NULL)))) {
    return -1;
  }
  if(((size < 14) || (size > 2036))) {
    return -3;
  }
  drift_virtqueue_ptr q = (&d->queue[1]);
  drift_virtio_clear(q->buffer.data, 12U);
  for(uint32_t i = 0U, _end = size; i < _end; i += 1) {
    q->buffer.data[(12 + i)] = data[i];
  }
  drift_virtio_descriptor(q, 0U, q->buffer.address, (size + 12), 0U, 0U);
  drift_virtio_submit(d, 1U);
  int32_t result = drift_virtio_finish(d, 1U, 0U);
  if((result < 0)) {
    return result;
  }
  return 0;
}
int32_t drift_virtio_receive_data(drift_virtio_ptr d, nluint8_arr0_ptr data, uint32_t capacity, uint32_t header) {
  drift_virtqueue_ptr q = (&d->queue[0]);
  int32_t result = drift_virtio_poll(d, 0U, q->capacity);
  if((result == -7)) {
    return 0;
  }
  if((result < 0)) {
    return result;
  }
  if(((uint32_t)result < header)) {
    return drift_virtio_poison(d, -5);
  }
  uint32_t length = ((uint32_t)result - header);
  if(((header == 12) && (((q->buffer.data[0] != 0) || (q->buffer.data[1] != 0)) || (length < 14)))) {
    return drift_virtio_poison(d, -5);
  }
  if((length > capacity)) {
    drift_virtio_receive_post(d, 0U);
    return -3;
  }
  for(uint32_t i = 0U, _end = length; i < _end; i += 1) {
    data[i] = q->buffer.data[(header + i)];
  }
  drift_virtio_receive_post(d, 0U);
  return (int32_t)length;
}
int32_t drift_virtio_receive(drift_virtio_ptr d, nluint8_arr0_ptr data, uint32_t capacity) {
  if((((!drift_virtio_ready(d, 1U)) || (data == ((nluint8_arr0_ptr)NULL))) || (capacity == 0))) {
    return -1;
  }
  return drift_virtio_receive_data(d, data, capacity, 12U);
}
int32_t drift_virtio_console_write(drift_virtio_ptr d, nluint8_arr0_ptr data, uint32_t size) {
  if(((!drift_virtio_ready(d, 3U)) || (data == ((nluint8_arr0_ptr)NULL)))) {
    return -1;
  }
  if(((size == 0) || (size > 4096))) {
    return -3;
  }
  drift_virtqueue_ptr q = (&d->queue[1]);
  for(uint32_t i = 0U, _end = size; i < _end; i += 1) {
    q->buffer.data[i] = data[i];
  }
  drift_virtio_descriptor(q, 0U, q->buffer.address, size, 0U, 0U);
  drift_virtio_submit(d, 1U);
  int32_t result = drift_virtio_finish(d, 1U, 0U);
  if((result < 0)) {
    return result;
  }
  return (int32_t)size;
}
int32_t drift_virtio_console_read(drift_virtio_ptr d, nluint8_arr0_ptr data, uint32_t capacity) {
  if((((!drift_virtio_ready(d, 3U)) || (data == ((nluint8_arr0_ptr)NULL))) || (capacity == 0))) {
    return -1;
  }
  return drift_virtio_receive_data(d, data, capacity, 0U);
}
int32_t drift_virtio_entropy(drift_virtio_ptr d, nluint8_arr0_ptr data, uint32_t capacity) {
  if(((!drift_virtio_ready(d, 4U)) || (data == ((nluint8_arr0_ptr)NULL)))) {
    return -1;
  }
  if(((capacity == 0) || (capacity > 4096))) {
    return -3;
  }
  drift_virtqueue_ptr q = (&d->queue[0]);
  drift_virtio_descriptor(q, 0U, q->buffer.address, capacity, 2U, 0U);
  drift_virtio_submit(d, 0U);
  int32_t result = drift_virtio_finish(d, 0U, capacity);
  if((result < 0)) {
    return result;
  }
  if((result == 0)) {
    return drift_virtio_poison(d, -5);
  }
  for(uint32_t i = 0U, _end = (uint32_t)result; i < _end; i += 1) {
    data[i] = q->buffer.data[i];
  }
  return result;
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
int32_t drift_virtio_input(drift_virtio_ptr d, drift_virtio_input_event_ptr event) {
  if(((!drift_virtio_ready(d, 18U)) || (event == ((drift_virtio_input_event_ptr)NULL)))) {
    return -1;
  }
  drift_virtqueue_ptr q = (&d->queue[0]);
  drift_hw_sync(d->hw, (&q->ring), 256U, 68U, 2U);
  uint16_t produced = (uint16_t)drift_virtio_get(q->ring.data, 258U, 2U);
  uint32_t count = (((uint32_t)produced - (uint32_t)q->used) & 65535);
  if((count == 0)) {
    if(((drift_virtio_status(d) & 0xc0) != 0)) {
      return drift_virtio_poison(d, -5);
    }
    return 0;
  }
  if((count > 8)) {
    return drift_virtio_poison(d, -5);
  }
  uint32_t seen = 0U;
  for(uint32_t i = 0U, _end = count; i < _end; i += 1) {
    uint32_t entry = (260 + ((((uint32_t)q->used + i) & 7) * 8));
    uint64_t id = drift_virtio_get(q->ring.data, entry, 4U);
    if((((id >= 8) || (drift_virtio_get(q->ring.data, (entry + 4), 4U) != 8)) || ((seen & nelua_shl_nluint32(1U, (uint32_t)id)) != 0))) {
      return drift_virtio_poison(d, -5);
    }
    seen = (seen | nelua_shl_nluint32(1U, (uint32_t)id));
  }
  uint32_t slot = (260 + (((uint32_t)q->used & 7) * 8));
  uint64_t id = drift_virtio_get(q->ring.data, slot, 4U);
  uint32_t offset = ((uint32_t)id * 8);
  drift_hw_sync(d->hw, (&q->buffer), offset, 8U, 2U);
  event->type = (uint16_t)drift_virtio_get(q->buffer.data, offset, 2U);
  event->code = (uint16_t)drift_virtio_get(q->buffer.data, (offset + 2), 2U);
  event->value = (uint32_t)drift_virtio_get(q->buffer.data, (offset + 4), 4U);
  q->used = (q->used + 1);
  drift_virtio_input_post(d, (uint32_t)id);
  return 1;
}

