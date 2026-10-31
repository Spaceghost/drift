/* Typed bindings; functional framework state belongs to compat.nelua.
 * SPDX-License-Identifier: MIT */
#ifndef DRIFT_WINDOWS_VIORNG_PROFILE_H
#define DRIFT_WINDOWS_VIORNG_PROFILE_H
#include "drift_windows_viorng.h"
#include "upstream/viorng.h"
typedef struct virtqueue drift_rng_virtqueue;
typedef struct VirtIOBufferDescriptor drift_rng_sg;
typedef struct drift_rng_metadata {
    WDF_DRIVER_CONFIG driver;
    WDF_OBJECT_ATTRIBUTES driver_attrs,device_attrs;
    WDF_PNPPOWER_EVENT_CALLBACKS power;
    WDF_INTERRUPT_CONFIG interrupt;
    WDF_IO_QUEUE_CONFIG queue;
    uint32_t io_type,interface_id;
} drift_rng_metadata;
_Static_assert(sizeof(DEVICE_CONTEXT)<=sizeof(((drift_win_rng *)0)->context_storage)-32,"context storage");
_Static_assert(sizeof(drift_rng_metadata)<=sizeof(((drift_win_rng *)0)->config_storage),"callback storage");
_Static_assert(sizeof(READ_BUFFER_ENTRY)<=sizeof(((drift_win_rng *)0)->entry_storage[0]),"pool storage");
_Static_assert(_Alignof(DEVICE_CONTEXT)<=_Alignof(drift_win_rng),"context alignment");
drift_win_rng *drift_win_rng_current(void);
static inline drift_rng_metadata *drift_rng_meta(drift_win_rng *s){return (drift_rng_metadata *)s->config_storage;}
static inline DEVICE_CONTEXT *drift_rng_context(drift_win_rng *s){return (DEVICE_CONTEXT *)s->context_storage;}
static inline drift_virtio *drift_rng_native(drift_win_rng *s){return &s->transport;}
static inline drift_virtio_raw_transfer *drift_rng_transfer(drift_win_rng *s){return &s->transfer;}
static inline drift_dma *drift_rng_dma(drift_win_rng *s){return &s->common;}
static inline drift_win_rng_request **drift_rng_pending(drift_win_rng *s){return s->pending;}
static inline drift_win_rng_request **drift_rng_notifications(drift_win_rng *s){return s->notifications;}
static inline void *drift_rng_entry(drift_win_rng *s,uint32_t i){return i<16?s->entry_storage[i]:NULL;}
static inline int32_t drift_rng_entry_index(drift_win_rng *s,void *p){for(unsigned i=0;i<16;++i)if(p==s->entry_storage[i])return i;return -1;}
static inline void drift_rng_entry_clear(drift_win_rng *s,uint32_t i){for(unsigned j=0;j<4;++j)s->entry_storage[i][j]=0;}
static inline int32_t drift_rng_host_valid(drift_win_rng_host *h){return h&&h->completed;}
static inline int32_t drift_rng_hw_valid(drift_win_rng *s){return drift_hw_valid(s->hw);}
static inline void drift_rng_notify(drift_win_rng *s,drift_win_rng_request *r){s->host->completed(s->host->context,r);}
static inline drift_win_rng_object *drift_rng_object(drift_win_rng *s,uint32_t k){
    switch(k){case 1:return &s->driver;case 2:return &s->device;case 3:return &s->queue;case 4:return &s->interrupt;case 5:return &s->spinlock;case 6:return &s->init;case 7:return &s->driver_object;default:return NULL;}
}
static inline void drift_rng_storage_init(drift_win_rng *s){
    for(unsigned i=1;i<=7;++i){drift_win_rng_object *o=drift_rng_object(s,i);o->owner=s;o->kind=i;}
    for(unsigned i=0;i<64;++i)s->context_storage[i]=0;
    for(unsigned i=0;i<128;++i)s->config_storage[i]=0;
    for(unsigned i=0;i<16;++i){s->pending[i]=NULL;s->notifications[i]=NULL;drift_rng_entry_clear(s,i);}
}
static inline int32_t drift_rng_driver_meta(drift_win_rng *s,void *config,void *attrs){
    WDF_DRIVER_CONFIG *c=config;WDF_OBJECT_ATTRIBUTES *a=attrs;
    if(!c||c->Size!=sizeof(*c)||!c->EvtDriverDeviceAdd||!a||a->Size!=sizeof(*a)||!a->EvtCleanupCallback)return 0;
    drift_rng_meta(s)->driver=*c;drift_rng_meta(s)->driver_attrs=*a;return 1;
}
static inline int32_t drift_rng_power_meta(drift_win_rng *s,void *config){
    WDF_PNPPOWER_EVENT_CALLBACKS *c=config;
    if(!c||c->Size!=sizeof(*c)||!c->EvtDevicePrepareHardware||!c->EvtDeviceReleaseHardware||!c->EvtDeviceD0Entry||!c->EvtDeviceD0Exit)return 0;
    drift_rng_meta(s)->power=*c;return 1;
}
static inline void drift_rng_set_iotype(drift_win_rng *s,uint32_t t){drift_rng_meta(s)->io_type=t;}
static inline int32_t drift_rng_device_meta(drift_win_rng *s,void *attrs){
    WDF_OBJECT_ATTRIBUTES *a=attrs;
    if(!a||a->Size!=sizeof(*a)||a->ContextSize!=sizeof(DEVICE_CONTEXT)||!a->EvtCleanupCallback||drift_rng_meta(s)->io_type!=WdfDeviceIoDirect)return 0;
    drift_rng_meta(s)->device_attrs=*a;return 1;
}
static inline void drift_rng_device_result(drift_win_rng *s,WDFDEVICE_INIT **i,WDFDEVICE *d){*i=NULL;*d=&s->device;}
static inline int32_t drift_rng_interrupt_meta(drift_win_rng *s,void *config){
    WDF_INTERRUPT_CONFIG *c=config;
    if(!c||c->Size!=sizeof(*c)||!c->EvtInterruptIsr||!c->EvtInterruptDpc||!c->EvtInterruptEnable||!c->EvtInterruptDisable)return 0;
    drift_rng_meta(s)->interrupt=*c;return 1;
}
static inline int32_t drift_rng_lock_meta(drift_win_rng *s,void *attrs){WDF_OBJECT_ATTRIBUTES *a=attrs;return a&&a->Size==sizeof(*a)&&a->ParentObject==&s->device;}
static inline int32_t drift_rng_interface_meta(drift_win_rng *s,void *id){GUID *g=id;if(!g||g->Data1!=0x2489fc19||g->Data2!=0xd0fd||g->Data3!=0x4950)return 0;drift_rng_meta(s)->interface_id=1;return 1;}
static inline int32_t drift_rng_queue_meta(drift_win_rng *s,void *config){WDF_IO_QUEUE_CONFIG *c=config;if(!c||c->Size!=sizeof(*c)||c->DispatchType!=WdfIoQueueDispatchSequential||c->AllowZeroLengthRequests||!c->EvtIoRead||!c->EvtIoStop)return 0;drift_rng_meta(s)->queue=*c;return 1;}
static inline int32_t drift_rng_irq_info_valid(void *info){WDF_INTERRUPT_INFO *i=info;return i&&i->Size==sizeof(*i);}
static inline void drift_rng_irq_info(void *info){((WDF_INTERRUPT_INFO *)info)->MessageSignaled=FALSE;}
static inline void drift_rng_set_cancel(drift_win_rng_request *r,EVT_WDF_REQUEST_CANCEL *f){r->cancel=f;}
static inline void drift_rng_call_cancel(drift_win_rng_request *r){r->cancel(r);}
static inline void drift_rng_output(drift_win_rng_request *r,void *result,size_t *length){*(void **)result=r->output;if(length)*length=r->capacity;}
static inline int32_t drift_rng_tag_valid(uint32_t tag){return tag==VIRT_RNG_MEMORY_TAG;}
static inline size_t drift_rng_entry_size(void){return sizeof(READ_BUFFER_ENTRY);}
static inline void drift_rng_bind_vdevice(VIRTIO_WDF_DRIVER *v,drift_win_rng *s){v->owner=s;v->VIODevice.owner=s;}
static inline struct virtqueue *drift_rng_vq(drift_win_rng *s){return (struct virtqueue *)&s->context_storage[60];}
static inline void drift_rng_init_vq(drift_win_rng *s,struct virtqueue **out){struct virtqueue *q=drift_rng_vq(s);q->owner=s;q->index=0;*out=q;}
static inline int32_t drift_rng_queue_param(drift_win_rng *s,void *p){VIRTIO_WDF_QUEUE_PARAM *x=p;return x&&x->Interrupt==&s->interrupt;}
static inline int32_t drift_rng_native_open(drift_win_rng *s){if(s->mode==1)return drift_virtio_raw_open_mmio(&s->transport,s->hw,s->region,4,1);if(s->mode==2)return drift_virtio_raw_open_function(&s->transport,s->hw,s->pci,s->bdf,s->bars,4,1);return -1;}
static inline int32_t drift_rng_native_ready(drift_win_rng *s){return drift_virtio_raw_device_ready(&s->transport);}
static inline int32_t drift_rng_native_close(drift_win_rng *s){return drift_virtio_close(&s->transport);}
static inline uint32_t drift_rng_native_state(drift_win_rng *s){return s->transport.state;}
static inline uint32_t drift_rng_dma_active(drift_win_rng *s){return s->transfer.active;}
static inline void *drift_rng_dma_cookie(drift_win_rng *s){return s->transfer.cookie;}
static inline int32_t drift_rng_native_add(drift_win_rng *s,uint32_t n,void *cookie){return drift_virtio_raw_add(&s->transport,0,&s->transfer,&s->common,0,n,DRIFT_DMA_FROM_DEVICE,cookie);}
static inline int32_t drift_rng_native_kick(drift_win_rng *s){return drift_virtio_raw_kick(&s->transport,0);}
static inline int32_t drift_rng_native_get(drift_win_rng *s,void **cookie,uint32_t *n){return drift_virtio_raw_get(&s->transport,0,&s->transfer,cookie,n);}
static inline int32_t drift_rng_alloc_dma(drift_win_rng *s){return drift_hw_dma(s->hw,4096,4096,&s->common);}
static inline void drift_rng_free_dma(drift_win_rng *s){drift_hw_release(s->hw,&s->common);s->common=(drift_dma){0};}
static inline int32_t drift_rng_call_entry(drift_win_rng *s){UNICODE_STRING r={0};return DriverEntry(&s->driver_object,&r);}
static inline int32_t drift_rng_call_add(drift_win_rng *s){return drift_rng_meta(s)->driver.EvtDriverDeviceAdd(&s->driver,&s->init);}
static inline int32_t drift_rng_call_prepare(drift_win_rng *s){return drift_rng_meta(s)->power.EvtDevicePrepareHardware(&s->device,&s->init,&s->init);}
static inline int32_t drift_rng_call_power(drift_win_rng *s,int32_t on){return on?drift_rng_meta(s)->power.EvtDeviceD0Entry(&s->device,WdfPowerDeviceD3):drift_rng_meta(s)->power.EvtDeviceD0Exit(&s->device,WdfPowerDeviceD3Final);}
static inline int32_t drift_rng_call_release(drift_win_rng *s){return drift_rng_meta(s)->power.EvtDeviceReleaseHardware(&s->device,&s->init);}
static inline int32_t drift_rng_call_irq(drift_win_rng *s,int32_t enable){return enable?drift_rng_meta(s)->interrupt.EvtInterruptEnable(&s->interrupt,&s->device):drift_rng_meta(s)->interrupt.EvtInterruptDisable(&s->interrupt,&s->device);}
static inline int32_t drift_rng_call_isr(drift_win_rng *s){return drift_rng_meta(s)->interrupt.EvtInterruptIsr(&s->interrupt,0);}
static inline void drift_rng_call_dpc(drift_win_rng *s){drift_rng_meta(s)->interrupt.EvtInterruptDpc(&s->interrupt,&s->device);}
static inline void drift_rng_call_read(drift_win_rng *s,drift_win_rng_request *r){drift_rng_meta(s)->queue.EvtIoRead(&s->queue,r,r->length);}
static inline void drift_rng_call_stop(drift_win_rng *s,drift_win_rng_request *r){drift_rng_meta(s)->queue.EvtIoStop(&s->queue,r,WdfRequestStopActionPurge);}
static inline void drift_rng_cleanup_device(drift_win_rng *s){drift_rng_meta(s)->device_attrs.EvtCleanupCallback(&s->device);}
static inline void drift_rng_cleanup_driver(drift_win_rng *s){drift_rng_meta(s)->driver_attrs.EvtCleanupCallback(&s->driver);}
#endif
