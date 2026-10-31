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
#include "drift_virtio.h"
#include "drift_nvme.h"
#include "drift_ahci.h"
#include "drift_usb.h"
#include "drift_ethernet.h"
#include "drift_usb_classes.h"
#include <stdbool.h>
#include <stdint.h>
/* Macro used to export C functions. */
#ifdef _WIN32
  #define NELUA_CEXPORT __declspec(dllexport)
#elif defined(__GNUC__)
  #define NELUA_CEXPORT __attribute__((visibility("default")))
#else
  #define NELUA_CEXPORT
#endif
#include "drift_io.h"
#include <stddef.h>
/* ------------------------------ DECLARATIONS ------------------------------ */
static bool drift_io_geometry(uint64_t blocks, uint32_t size);
static int32_t drift_io_normalized(int32_t status);
typedef drift_block* drift_block_ptr;
NELUA_CEXPORT int32_t drift_block_bind(drift_block_ptr d, uint32_t kind, void* driver);
typedef drift_virtio* drift_virtio_ptr;
typedef drift_nvme* drift_nvme_ptr;
typedef drift_ahci* drift_ahci_ptr;
typedef drift_usb* drift_usb_ptr;
static bool drift_io_block_live(drift_block_ptr d);
typedef uint8_t* nluint8_arr0_ptr;
static int32_t drift_io_io(drift_block_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr buffer, uint32_t bytes, bool writing);
typedef uint8_t* nluint8_ptr;
NELUA_CEXPORT int32_t drift_block_read(drift_block_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr p, uint32_t bytes);
NELUA_CEXPORT int32_t drift_block_write(drift_block_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr p, uint32_t bytes);
NELUA_CEXPORT int32_t drift_block_flush(drift_block_ptr d);
typedef drift_net* drift_net_ptr;
NELUA_CEXPORT int32_t drift_net_bind(drift_net_ptr d, uint32_t kind, void* driver);
typedef drift_ethernet* drift_ethernet_ptr;
typedef drift_usb_cdc* drift_usb_cdc_ptr;
static bool drift_io_net_live(drift_net_ptr d);
NELUA_CEXPORT int32_t drift_net_send(drift_net_ptr d, nluint8_arr0_ptr frame, uint32_t bytes);
NELUA_CEXPORT int32_t drift_net_receive(drift_net_ptr d, nluint8_arr0_ptr frame, uint32_t capacity);
/* ------------------------------ DEFINITIONS ------------------------------- */
bool drift_io_geometry(uint64_t blocks, uint32_t size) {
  return ((((blocks != 0) && (size >= 512)) && (size <= 4096)) && ((size & (size - 1)) == 0));
}
int32_t drift_io_normalized(int32_t status) {
  if((status > 0)) {
    return -1004;
  }
  return status;
}
int32_t drift_block_bind(drift_block_ptr d, uint32_t kind, void* driver) {
  if((d == ((drift_block_ptr)NULL))) {
    return -1000;
  }
  uint32_t _asgntmp_1 = 0U;
  void* _asgntmp_2 = (void*)NULL;
  uint64_t _asgntmp_3 = 0U;
  uint32_t _asgntmp_4 = 0U;
  uint32_t _asgntmp_5 = 0U;
  uint32_t _asgntmp_6 = 0U;
  d->kind = _asgntmp_1;
  d->driver = (void*)_asgntmp_2;
  d->blocks = _asgntmp_3;
  d->sector_size = _asgntmp_4;
  d->max_sectors = _asgntmp_5;
  d->completed = _asgntmp_6;
  if((driver == (void*)NULL)) {
    return -1000;
  }
  uint64_t blocks = 0U;
  uint32_t size = 0U;
  uint32_t limit = 0U;
  if((kind == 1)) {
    drift_virtio_ptr p = ((drift_virtio_ptr)driver);
    if(((p->state != 2) || (p->device_id != 2))) {
      return -1001;
    }
    uint64_t _asgntmp_7 = p->capacity;
    uint32_t _asgntmp_8 = 512U;
    uint32_t _asgntmp_9 = 65536U;
    blocks = _asgntmp_7;
    size = _asgntmp_8;
    limit = _asgntmp_9;
  } else if((kind == 2)) {
    drift_nvme_ptr p = ((drift_nvme_ptr)driver);
    if((p->state != 1)) {
      return -1001;
    }
    uint64_t _asgntmp_10 = p->blocks;
    uint32_t _asgntmp_11 = p->block_size;
    uint32_t _asgntmp_12 = 4096U;
    blocks = _asgntmp_10;
    size = _asgntmp_11;
    limit = _asgntmp_12;
  } else if((kind == 3)) {
    drift_ahci_ptr p = ((drift_ahci_ptr)driver);
    if((p->state != 1)) {
      return -1001;
    }
    uint64_t _asgntmp_13 = p->sectors;
    uint32_t _asgntmp_14 = p->sector_size;
    uint32_t _asgntmp_15 = 65536U;
    blocks = _asgntmp_13;
    size = _asgntmp_14;
    limit = _asgntmp_15;
  } else if((kind == 4)) {
    drift_usb_ptr p = ((drift_usb_ptr)driver);
    if((((p->state != 1) || (p->slot == 0)) || (p->kind != 3))) {
      return -1001;
    }
    int32_t result = drift_usb_storage_info(p, (&blocks), (&size));
    if((result != 0)) {
      return drift_io_normalized(result);
    }
    if(((p->blocks != blocks) || (p->sector_size != size))) {
      return -1004;
    }
    limit = 16384U;
  } else {
    return -1003;
  }
  if(((!drift_io_geometry(blocks, size)) || (size > limit))) {
    return -1004;
  }
  uint32_t _asgntmp_16 = kind;
  void* _asgntmp_17 = driver;
  uint64_t _asgntmp_18 = blocks;
  uint32_t _asgntmp_19 = size;
  uint32_t _asgntmp_20 = (limit / size);
  d->kind = _asgntmp_16;
  d->driver = _asgntmp_17;
  d->blocks = _asgntmp_18;
  d->sector_size = _asgntmp_19;
  d->max_sectors = _asgntmp_20;
  return 0;
}
bool drift_io_block_live(drift_block_ptr d) {
  if(((d->driver == (void*)NULL) || (!drift_io_geometry(d->blocks, d->sector_size)))) {
    return false;
  }
  if((d->kind == 1)) {
    drift_virtio_ptr p = ((drift_virtio_ptr)d->driver);
    return (((((p->state == 2) && (p->device_id == 2)) && (p->capacity == d->blocks)) && (d->sector_size == 512)) && (d->max_sectors == 128));
  } else if((d->kind == 2)) {
    drift_nvme_ptr p = ((drift_nvme_ptr)d->driver);
    return ((((p->state == 1) && (p->blocks == d->blocks)) && (p->block_size == d->sector_size)) && (d->max_sectors == (4096 / d->sector_size)));
  } else if((d->kind == 3)) {
    drift_ahci_ptr p = ((drift_ahci_ptr)d->driver);
    return ((((p->state == 1) && (p->sectors == d->blocks)) && (p->sector_size == d->sector_size)) && (d->max_sectors == (65536 / d->sector_size)));
  } else if((d->kind == 4)) {
    drift_usb_ptr p = ((drift_usb_ptr)d->driver);
    return ((((((p->state == 1) && (p->slot != 0)) && (p->kind == 3)) && (p->blocks == d->blocks)) && (p->sector_size == d->sector_size)) && (d->max_sectors == (16384 / d->sector_size)));
  }
  return false;
}
int32_t drift_io_io(drift_block_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr buffer, uint32_t bytes, bool writing) {
  if((d == ((drift_block_ptr)NULL))) {
    return -1000;
  }
  d->completed = 0U;
  if(((buffer == ((nluint8_arr0_ptr)NULL)) || (count == 0))) {
    return -1000;
  }
  if((!drift_io_block_live(d))) {
    return -1001;
  }
  if((((lba >= d->blocks) || ((uint64_t)count > (d->blocks - lba))) || (count > (bytes / d->sector_size)))) {
    return -1002;
  }
  while((d->completed < count)) {
    uint32_t n = (count - d->completed);
    if((n > d->max_sectors)) {
      n = d->max_sectors;
    }
    uint64_t at = (lba + d->completed);
    nluint8_ptr data = (&buffer[(d->completed * d->sector_size)]);
    int32_t result = 0;
    if((d->kind == 1)) {
      result = drift_virtio_block(((drift_virtio_ptr)d->driver), at, n, ((nluint8_arr0_ptr)data), (int32_t)(writing ? 1 : 0));
    } else if((d->kind == 2)) {
      if(writing) {
        result = drift_nvme_write(((drift_nvme_ptr)d->driver), at, n, ((nluint8_arr0_ptr)data));
      } else {
        result = drift_nvme_read(((drift_nvme_ptr)d->driver), at, n, ((nluint8_arr0_ptr)data));
      }
    } else if((d->kind == 3)) {
      if(writing) {
        result = drift_ahci_write(((drift_ahci_ptr)d->driver), at, n, ((nluint8_arr0_ptr)data));
      } else {
        result = drift_ahci_read(((drift_ahci_ptr)d->driver), at, n, ((nluint8_arr0_ptr)data));
      }
    } else {
      if(writing) {
        result = drift_usb_storage_write(((drift_usb_ptr)d->driver), at, n, ((nluint8_arr0_ptr)data));
      } else {
        result = drift_usb_storage_read(((drift_usb_ptr)d->driver), at, n, ((nluint8_arr0_ptr)data));
      }
    }
    if((result != 0)) {
      return drift_io_normalized(result);
    }
    d->completed = (d->completed + n);
  }
  return 0;
}
int32_t drift_block_read(drift_block_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr p, uint32_t bytes) {
  return drift_io_io(d, lba, count, p, bytes, false);
}
int32_t drift_block_write(drift_block_ptr d, uint64_t lba, uint32_t count, nluint8_arr0_ptr p, uint32_t bytes) {
  return drift_io_io(d, lba, count, p, bytes, true);
}
int32_t drift_block_flush(drift_block_ptr d) {
  if((d == ((drift_block_ptr)NULL))) {
    return -1000;
  }
  d->completed = 0U;
  if((!drift_io_block_live(d))) {
    return -1001;
  }
  int32_t result = 0;
  if((d->kind == 1)) {
    result = drift_virtio_flush(((drift_virtio_ptr)d->driver));
  } else if((d->kind == 2)) {
    result = drift_nvme_flush(((drift_nvme_ptr)d->driver));
  } else if((d->kind == 3)) {
    result = drift_ahci_flush(((drift_ahci_ptr)d->driver));
  } else {
    result = drift_usb_storage_flush(((drift_usb_ptr)d->driver));
  }
  return drift_io_normalized(result);
}
int32_t drift_net_bind(drift_net_ptr d, uint32_t kind, void* driver) {
  if((d == ((drift_net_ptr)NULL))) {
    return -1000;
  }
  uint32_t _asgntmp_1 = 0U;
  void* _asgntmp_2 = (void*)NULL;
  uint32_t _asgntmp_3 = 0U;
  d->kind = _asgntmp_1;
  d->driver = (void*)_asgntmp_2;
  d->max_frame = _asgntmp_3;
  for(uint32_t i = 0U; i <= 5U; i += 1) {
    d->mac[i] = 0U;
  }
  if((driver == (void*)NULL)) {
    return -1000;
  }
  nluint8_arr0_ptr mac = NULL;
  uint32_t maximum = 0U;
  if((kind == 1)) {
    drift_virtio_ptr p = ((drift_virtio_ptr)driver);
    if(((p->state != 2) || (p->device_id != 1))) {
      return -1001;
    }
    nluint8_arr0_ptr _asgntmp_4 = ((nluint8_arr0_ptr)(&p->mac[0]));
    uint32_t _asgntmp_5 = 2036U;
    mac = ((nluint8_arr0_ptr)_asgntmp_4);
    maximum = _asgntmp_5;
  } else if((kind == 2)) {
    drift_ethernet_ptr p = ((drift_ethernet_ptr)driver);
    if((p->state != 1)) {
      return -1001;
    }
    nluint8_arr0_ptr _asgntmp_6 = ((nluint8_arr0_ptr)(&p->mac[0]));
    uint32_t _asgntmp_7 = 1518U;
    mac = ((nluint8_arr0_ptr)_asgntmp_6);
    maximum = _asgntmp_7;
  } else if((kind == 3)) {
    drift_usb_cdc_ptr p = ((drift_usb_cdc_ptr)driver);
    if(((((p->state != 1) || (p->kind != 2)) || (p->device == ((drift_usb_ptr)NULL))) || (p->device->state != 1))) {
      return -1001;
    }
    nluint8_arr0_ptr _asgntmp_8 = ((nluint8_arr0_ptr)(&p->mac[0]));
    uint32_t _asgntmp_9 = p->maximum_segment;
    mac = ((nluint8_arr0_ptr)_asgntmp_8);
    maximum = _asgntmp_9;
  } else {
    return -1003;
  }
  if((((maximum < 14) || (maximum > 16384)) || ((mac[0] & 1) != 0))) {
    return -1004;
  }
  uint8_t bits = 0U;
  for(uint32_t i = 0U; i <= 5U; i += 1) {
    bits = (bits | mac[i]);
  }
  if((bits == 0)) {
    return -1004;
  }
  for(uint32_t i = 0U; i <= 5U; i += 1) {
    d->mac[i] = mac[i];
  }
  uint32_t _asgntmp_10 = kind;
  void* _asgntmp_11 = driver;
  uint32_t _asgntmp_12 = maximum;
  d->kind = _asgntmp_10;
  d->driver = _asgntmp_11;
  d->max_frame = _asgntmp_12;
  return 0;
}
bool drift_io_net_live(drift_net_ptr d) {
  if((((d->driver == (void*)NULL) || (d->max_frame < 14)) || (d->max_frame > 16384))) {
    return false;
  }
  nluint8_arr0_ptr mac = NULL;
  if((d->kind == 1)) {
    drift_virtio_ptr p = ((drift_virtio_ptr)d->driver);
    if((((p->state != 2) || (p->device_id != 1)) || (d->max_frame != 2036))) {
      return false;
    }
    mac = ((nluint8_arr0_ptr)(&p->mac[0]));
  } else if((d->kind == 2)) {
    drift_ethernet_ptr p = ((drift_ethernet_ptr)d->driver);
    if(((p->state != 1) || (d->max_frame != 1518))) {
      return false;
    }
    mac = ((nluint8_arr0_ptr)(&p->mac[0]));
  } else if((d->kind == 3)) {
    drift_usb_cdc_ptr p = ((drift_usb_cdc_ptr)d->driver);
    if((((((p->state != 1) || (p->kind != 2)) || (p->device == ((drift_usb_ptr)NULL))) || (p->device->state != 1)) || (d->max_frame != p->maximum_segment))) {
      return false;
    }
    mac = ((nluint8_arr0_ptr)(&p->mac[0]));
  } else {
    return false;
  }
  for(uint32_t i = 0U; i <= 5U; i += 1) {
    if((d->mac[i] != mac[i])) {
      return false;
    }
  }
  return true;
}
int32_t drift_net_send(drift_net_ptr d, nluint8_arr0_ptr frame, uint32_t bytes) {
  if(((d == ((drift_net_ptr)NULL)) || (frame == ((nluint8_arr0_ptr)NULL)))) {
    return -1000;
  }
  if((!drift_io_net_live(d))) {
    return -1001;
  }
  if(((bytes < 14) || (bytes > d->max_frame))) {
    return -1002;
  }
  int32_t result = 0;
  if((d->kind == 1)) {
    result = drift_virtio_send(((drift_virtio_ptr)d->driver), frame, bytes);
  } else if((d->kind == 2)) {
    result = drift_eth_tx(((drift_ethernet_ptr)d->driver), frame, bytes);
  } else {
    result = drift_usb_cdc_write(((drift_usb_cdc_ptr)d->driver), frame, bytes);
    if((result == (int32_t)bytes)) {
      return 0;
    }
    if((result >= 0)) {
      return -1004;
    }
    return result;
  }
  return drift_io_normalized(result);
}
int32_t drift_net_receive(drift_net_ptr d, nluint8_arr0_ptr frame, uint32_t capacity) {
  if(((d == ((drift_net_ptr)NULL)) || (frame == ((nluint8_arr0_ptr)NULL)))) {
    return -1000;
  }
  if((!drift_io_net_live(d))) {
    return -1001;
  }
  if((capacity < d->max_frame)) {
    return -1002;
  }
  int32_t result = 0;
  if((d->kind == 1)) {
    result = drift_virtio_receive(((drift_virtio_ptr)d->driver), frame, d->max_frame);
  } else if((d->kind == 2)) {
    uint32_t length = 0U;
    result = drift_eth_rx(((drift_ethernet_ptr)d->driver), frame, d->max_frame, (&length));
    if((result == 1)) {
      if((length != 0)) {
        return -1004;
      }
      return 0;
    }
    if((result > 0)) {
      return -1004;
    }
    if((result == 0)) {
      if((((length < 14) || (length > d->max_frame)) || (length > capacity))) {
        return -1004;
      }
      return (int32_t)length;
    }
    return result;
  } else {
    result = drift_usb_cdc_read(((drift_usb_cdc_ptr)d->driver), frame, d->max_frame);
  }
  if(((result > 0) && (((result < 14) || ((uint32_t)result > d->max_frame)) || ((uint32_t)result > capacity)))) {
    return -1004;
  }
  return result;
}

