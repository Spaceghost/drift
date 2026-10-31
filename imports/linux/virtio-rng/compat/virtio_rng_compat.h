/* Closed Linux source API for the pinned Virtio RNG driver. SPDX-License-Identifier: MIT */
#ifndef DRIFT_VIRTIO_RNG_COMPAT_H
#define DRIFT_VIRTIO_RNG_COMPAT_H
#include "drift_linux_virtio_rng.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
_Static_assert(sizeof(unsigned long)==sizeof(void *), "Linux source requires LP64 or ILP32; LLP64 is unsupported");
#define ENOMEM 12
#define ENODEV 19
#define GFP_KERNEL 0
#define SMP_CACHE_BYTES 64
#define KBUILD_MODNAME "virtio_rng"
#define VIRTIO_ID_RNG 4
#define VIRTIO_DEV_ANY_ID UINT32_MAX
typedef uint8_t u8;
struct completion { unsigned done; };
struct ida { uint32_t bitmap; };
struct scatterlist { void *data; unsigned length; };
struct hwrng {
    const char *name;
    int (*read)(struct hwrng *,void *,size_t,bool);
    void (*cleanup)(struct hwrng *);
    unsigned long priv;
};
struct virtio_device;
struct virtqueue { struct virtio_device *vdev; void (*callback)(struct virtqueue *); drift_linux_virtio_rng *owner; };
struct virtio_config_ops { void (*del_vqs)(struct virtio_device *); };
struct virtio_device { void *priv; const struct virtio_config_ops *config; drift_linux_virtio_rng *owner; };
struct virtio_device_id { uint32_t device,vendor; };
struct virtio_driver {
    struct { const char *name; } driver;
    const struct virtio_device_id *id_table;
    int (*probe)(struct virtio_device *);
    void (*remove)(struct virtio_device *);
    void (*scan)(struct virtio_device *);
    int (*freeze)(struct virtio_device *);
    int (*restore)(struct virtio_device *);
};
typedef struct hwrng drift_vrng_hwrng;
typedef struct ida drift_vrng_ida;
typedef struct completion drift_vrng_completion;
typedef struct scatterlist drift_vrng_sg;
typedef struct virtqueue drift_vrng_vq;
typedef struct virtio_device drift_vrng_device;
_Static_assert(sizeof(struct virtio_device)<=sizeof(((drift_linux_virtio_rng *)0)->device_slot),"vdev slot");
_Static_assert(sizeof(struct virtqueue)<=sizeof(((drift_linux_virtio_rng *)0)->queue_slot),"vq slot");

drift_linux_virtio_rng *drift_lx_vrng_current(void);
void *drift_lx_vrng_alloc(size_t,int);
void drift_lx_vrng_free(void *);
int drift_lx_vrng_ida_alloc(struct ida *,int);
void drift_lx_vrng_ida_free(struct ida *,int);
int drift_lx_vrng_name(char *,char *,int);
void *drift_lx_vrng_memcpy(void *,void *,size_t);
void drift_lx_vrng_completion_init(struct completion *);
void drift_lx_vrng_complete(struct completion *);
int drift_lx_vrng_wait(struct completion *);
struct virtqueue *drift_lx_vrng_find_vq(struct virtio_device *,char *);
void drift_lx_vrng_ready(struct virtio_device *);
int drift_lx_vrng_add(struct virtqueue *,struct scatterlist *,unsigned,void *,int);
bool drift_lx_vrng_kick(struct virtqueue *);
void *drift_lx_vrng_get(struct virtqueue *,unsigned *);
int drift_lx_vrng_register(struct hwrng *);
void drift_lx_vrng_unregister(struct hwrng *);
void drift_lx_vrng_reset(struct virtio_device *);
void drift_lx_vrng_del_vqs(struct virtio_device *);
int32_t drift_lx_vrng_prepare(drift_linux_virtio_rng *,drift_virtio *);
int32_t drift_lx_vrng_pump(drift_linux_virtio_rng *);
int32_t drift_lx_vrng_quiesce(drift_linux_virtio_rng *);
void drift_lx_vrng_reclaim(drift_linux_virtio_rng *);

/* Source-layout accessors are defined after the unchanged source is included. */
uint8_t *drift_lx_vrng_source_data(drift_linux_virtio_rng *);
uint32_t drift_lx_vrng_source_capacity(drift_linux_virtio_rng *);
struct completion *drift_lx_vrng_source_completion(drift_linux_virtio_rng *);
struct hwrng *drift_lx_vrng_source_hwrng(drift_linux_virtio_rng *);
uint32_t drift_lx_vrng_source_size(void);
void drift_lx_vrng_callback(drift_linux_virtio_rng *);
static inline struct virtio_device *drift_vrng_device_slot(drift_linux_virtio_rng *s) { return (void *)s->device_slot; }
static inline struct virtqueue *drift_vrng_queue_slot(drift_linux_virtio_rng *s) { return (void *)s->queue_slot; }
static inline uint8_t *drift_vrng_source_slot(drift_linux_virtio_rng *s) { return (void *)s->source_slot; }
static inline uint8_t *drift_vrng_device_bytes(drift_linux_virtio_rng *s) { return (void *)s->device_slot; }
static inline uint8_t *drift_vrng_queue_bytes(drift_linux_virtio_rng *s) { return (void *)s->queue_slot; }
static inline drift_hw *drift_vrng_hw(drift_linux_virtio_rng *s) { return s->transport->hw; }
static inline int32_t drift_vrng_transport_valid(drift_virtio *d) { return d && d->state==4 && d->device_id==4 && d->capacity==1 && drift_hw_valid(d->hw); }
static inline int32_t drift_vrng_transport_closed(drift_linux_virtio_rng *s) { return s->transport && s->transport->state==0; }
static inline int32_t drift_vrng_transport_failed(drift_linux_virtio_rng *s) { return s->transport && s->transport->state==3; }
static inline void drift_vrng_set_callback(struct virtqueue *q,struct virtio_device *v,drift_linux_virtio_rng *s,void (*cb)(struct virtqueue *)) { q->vdev=v; q->owner=s; q->callback=cb; }
static inline void drift_vrng_sg_data(struct scatterlist *sg,void **p,uint32_t *n) { *p=sg->data;*n=sg->length; }
static inline int32_t drift_vrng_cancel_take(drift_linux_virtio_rng *s) { return __atomic_exchange_n(&s->cancel_requested,0,__ATOMIC_ACQ_REL)!=0; }
static inline void drift_vrng_cancel_clear(drift_linux_virtio_rng *s) { __atomic_store_n(&s->cancel_requested,0,__ATOMIC_RELEASE); }
static inline uint32_t drift_vrng_timeout(drift_linux_virtio_rng *s) { return s->transport->timeout_us; }

#define DEFINE_IDA(name) struct ida name={0}
#define ida_alloc drift_lx_vrng_ida_alloc
#define ida_free drift_lx_vrng_ida_free
#define kzalloc drift_lx_vrng_alloc
#define kfree drift_lx_vrng_free
#define sprintf drift_lx_vrng_name
#define memcpy(dst,src,n) drift_lx_vrng_memcpy((dst),(void *)(src),(n))
#define min_t(type,a,b) ((type)(a)<(type)(b)?(type)(a):(type)(b))
#define init_completion drift_lx_vrng_completion_init
#define reinit_completion drift_lx_vrng_completion_init
#define complete drift_lx_vrng_complete
#define wait_for_completion_killable drift_lx_vrng_wait
static inline void sg_init_one(struct scatterlist *sg,void *data,unsigned length) { sg->data=data;sg->length=length; }
static inline struct virtqueue *virtio_find_single_vq(struct virtio_device *d,void (*cb)(struct virtqueue *),const char *name) {
    if(!cb)return (void *)(intptr_t)-22;
    struct virtqueue *q=drift_lx_vrng_find_vq(d,(char *)name);
    if((uintptr_t)q<(uintptr_t)-4095){q->vdev=d;q->owner=d->owner;q->callback=cb;}
    return q;
}
#define virtio_device_ready drift_lx_vrng_ready
#define virtqueue_add_inbuf drift_lx_vrng_add
#define virtqueue_kick drift_lx_vrng_kick
#define virtqueue_get_buf drift_lx_vrng_get
#define hwrng_register drift_lx_vrng_register
#define hwrng_unregister drift_lx_vrng_unregister
#define virtio_reset_device drift_lx_vrng_reset
#define ERR_PTR(error) ((void *)(intptr_t)(error))
#define PTR_ERR(pointer) ((int)(intptr_t)(pointer))
#define IS_ERR(pointer) ((uintptr_t)(pointer)>=(uintptr_t)-4095)
#define smp_store_release(p,value) __atomic_store_n((p),(value),__ATOMIC_RELEASE)
#define smp_load_acquire(p) __atomic_load_n((p),__ATOMIC_ACQUIRE)
#define pm_sleep_ptr(fn) (fn)
#define module_virtio_driver(value) static struct virtio_driver *const drift_vrng_driver=&(value)
#define MODULE_DEVICE_TABLE(kind,table) static const void *const drift_vrng_ids __attribute__((unused))=(table)
#define MODULE_DESCRIPTION(value) static const char drift_vrng_description[] __attribute__((unused))=value
#define MODULE_LICENSE(value) static const char drift_vrng_license[] __attribute__((unused))=value
#endif
