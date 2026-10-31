/* Explicit lifecycle around unchanged GPL-2.0-or-later Linux source.
 * This wrapper is MIT; the combined import is not an MIT-only artifact. */
#include "virtio_rng_compat.h"
#include <stdatomic.h>
static atomic_flag gate=ATOMIC_FLAG_INIT;
static drift_linux_virtio_rng *active;
drift_linux_virtio_rng *drift_lx_vrng_current(void) { return active; }
#include "upstream/virtio-rng.c"
_Static_assert(sizeof(struct virtrng_info)<=sizeof(((drift_linux_virtio_rng *)0)->source_slot),"source slot");
_Static_assert(_Alignof(struct virtrng_info)<=_Alignof(drift_linux_virtio_rng),"source alignment");
static struct virtrng_info *source(drift_linux_virtio_rng *s) { return (void *)s->source_slot; }
uint8_t *drift_lx_vrng_source_data(drift_linux_virtio_rng *s) { return source(s)->data; }
uint32_t drift_lx_vrng_source_capacity(drift_linux_virtio_rng *s) { return sizeof(source(s)->data); }
struct completion *drift_lx_vrng_source_completion(drift_linux_virtio_rng *s) { return &source(s)->have_data; }
struct hwrng *drift_lx_vrng_source_hwrng(drift_linux_virtio_rng *s) { return &source(s)->hwrng; }
uint32_t drift_lx_vrng_source_size(void) { return sizeof(struct virtrng_info); }
void drift_lx_vrng_callback(drift_linux_virtio_rng *s)
{
    struct virtqueue *q=drift_vrng_queue_slot(s);
    if(q->owner!=s || q->vdev!=drift_vrng_device_slot(s) || !q->callback){if(!s->error)s->error=-5;return;}
    q->callback(q);
}
static const struct virtio_config_ops config={drift_lx_vrng_del_vqs};
static int32_t enter(drift_linux_virtio_rng *s)
{
    if(!s)return -22;
    if(atomic_flag_test_and_set_explicit(&gate,memory_order_acquire))return -16;
    active=s;return 0;
}
static int32_t leave(int32_t result)
{
    active=NULL;atomic_flag_clear_explicit(&gate,memory_order_release);return result;
}
static int32_t cleanup(drift_linux_virtio_rng *s,uint32_t final_phase)
{
    if(s->allocated && !s->free_requested){
        s->phase=3;
        drift_vrng_driver->remove(drift_vrng_device_slot(s));
    }
    int32_t result=drift_lx_vrng_quiesce(s);
    if(result)return result;
    drift_vrng_device_slot(s)->priv=NULL;
    s->phase=final_phase;s->error=0;s->read_active=0;s->read_output=NULL;s->read_length=0;
    return 0;
}
static int32_t bind(drift_linux_virtio_rng *s,drift_virtio *d,bool restore)
{
    int32_t result=drift_lx_vrng_prepare(s,d);
    if(result)return result;
    struct virtio_device *v=drift_vrng_device_slot(s);
    v->owner=s;v->config=&config;
    result=restore?drift_vrng_driver->restore(v):drift_vrng_driver->probe(v);
    if(!restore && !result && !s->error)drift_vrng_driver->scan(v);
    if(s->error)result=s->error;
    if(!result && (!s->registered || !s->allocated || !s->queue_created || source(s)->hwrng_removed))result=-5;
    if(result){
        int32_t quiesced=cleanup(s,0);
        return quiesced?quiesced:result;
    }
    s->phase=2;return 0;
}
int32_t drift_linux_virtio_rng_open(drift_linux_virtio_rng *s,drift_virtio *d)
{
    int32_t r=enter(s);if(r)return r;
    if(s->phase)return leave(-16);
    return leave(bind(s,d,false));
}
int32_t drift_linux_virtio_rng_service(drift_linux_virtio_rng *s)
{
    int32_t r=enter(s);if(r)return r;
    if(s->phase!=2)return leave(-19);
    return leave(drift_lx_vrng_pump(s));
}
int32_t drift_linux_virtio_rng_read(drift_linux_virtio_rng *s,uint8_t *out,uint32_t n,int32_t wait)
{
    int32_t r=enter(s);if(r)return r;
    if(s->phase!=2 || !s->registered)return leave(-19);
    if(!out || !n || n>4096 || (wait!=0 && wait!=1))return leave(-22);
    if(s->error)return leave(s->error);
    if(wait && drift_vrng_cancel_take(s))return leave(-4);
    s->read_output=out;s->read_length=n;s->read_active=1;
    s->read_started=drift_hw_time(s->transport->hw);
    r=drift_lx_vrng_pump(s);
    if(r>=0)r=source(s)->hwrng.read(&source(s)->hwrng,out,n,wait!=0);
    if(s->error)r=s->error;
    s->read_active=0;s->read_output=NULL;s->read_length=0;
    return leave(r);
}
int32_t drift_linux_virtio_rng_cancel(drift_linux_virtio_rng *s)
{
    if(!s)return -22;
    __atomic_store_n(&s->cancel_requested,1,__ATOMIC_RELEASE);return 0;
}
int32_t drift_linux_virtio_rng_close(drift_linux_virtio_rng *s)
{
    int32_t r=enter(s);if(r)return r;
    if(!s->phase)return leave(0);
    if(s->phase==4){s->phase=0;return leave(0);}
    return leave(cleanup(s,0));
}
int32_t drift_linux_virtio_rng_freeze(drift_linux_virtio_rng *s)
{
    int32_t r=enter(s);if(r)return r;
    if(s->phase!=2)return leave(-19);
    s->phase=3;
    r=drift_vrng_driver->freeze(drift_vrng_device_slot(s));
    if(s->error)r=s->error;
    int32_t stopped=drift_lx_vrng_quiesce(s);
    if(stopped)return leave(stopped);
    drift_vrng_device_slot(s)->priv=NULL;s->phase=4;s->error=0;
    return leave(r);
}
int32_t drift_linux_virtio_rng_restore(drift_linux_virtio_rng *s,drift_virtio *d)
{
    int32_t r=enter(s);if(r)return r;
    if(s->phase!=4)return leave(-19);
    return leave(bind(s,d,true));
}
