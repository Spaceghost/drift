/* Actual unchanged Linux source, using the independent noncoherent ring model.
 * SPDX-License-Identifier: MIT */
#define main drift_existing_native_model_main
#include "../../../tests/test_virtio.c"
#undef main
#include "virtio_rng_compat.h"
#include <pthread.h>

static unsigned alloc_failure,ida_failure,vq_failure,registration_failure;
static unsigned ready_failure,add_failure,kick_failure;
void *__real_drift_lx_vrng_alloc(size_t,int);
int __real_drift_lx_vrng_ida_alloc(struct ida *,int);
struct virtqueue *__real_drift_lx_vrng_find_vq(struct virtio_device *,char *);
int __real_drift_lx_vrng_register(struct hwrng *);
void *__wrap_drift_lx_vrng_alloc(size_t n,int flags) { return alloc_failure?NULL:__real_drift_lx_vrng_alloc(n,flags); }
int __wrap_drift_lx_vrng_ida_alloc(struct ida *a,int flags) { return ida_failure?-28:__real_drift_lx_vrng_ida_alloc(a,flags); }
struct virtqueue *__wrap_drift_lx_vrng_find_vq(struct virtio_device *d,char *name) { return vq_failure?ERR_PTR(-12):__real_drift_lx_vrng_find_vq(d,name); }
int __wrap_drift_lx_vrng_register(struct hwrng *rng) { return registration_failure?-12:__real_drift_lx_vrng_register(rng); }
int32_t __real_drift_virtio_raw_device_ready(drift_virtio *);
int32_t __real_drift_virtio_raw_add(drift_virtio *,uint32_t,drift_virtio_raw_transfer *,drift_dma *,uint32_t,uint32_t,uint32_t,void *);
int32_t __real_drift_virtio_raw_kick(drift_virtio *,uint32_t);
int32_t __wrap_drift_virtio_raw_device_ready(drift_virtio *d) { return ready_failure?-5:__real_drift_virtio_raw_device_ready(d); }
int32_t __wrap_drift_virtio_raw_add(drift_virtio *d,uint32_t q,drift_virtio_raw_transfer *t,drift_dma *b,uint32_t o,uint32_t n,uint32_t dir,void *c) { return add_failure?-6:__real_drift_virtio_raw_add(d,q,t,b,o,n,dir,c); }
int32_t __wrap_drift_virtio_raw_kick(drift_virtio *d,uint32_t q) { return kick_failure?-5:__real_drift_virtio_raw_kick(d,q); }

static int rng_raw_open(drift_virtio *d,drift_hw *h,model *m)
{
    return m->transport?drift_virtio_raw_open_pci(d,h,0,0x100,1,0x200,4,2,0x300,4,1):drift_virtio_raw_open_mmio(d,h,0,4,1);
}
static void expected(const uint8_t *p,unsigned n,unsigned offset)
{
    for(unsigned i=0;i<n;++i)CHECK(p[i]==(uint8_t)(((i+offset)%64)*17+9));
}
static void ordinary(unsigned transport)
{
    model *m=malloc(sizeof(*m));CHECK(m);drift_hw h;drift_virtio d={0};drift_linux_virtio_rng s={0};
    uint8_t bytes[512];init(m,&h,transport,4);CHECK(rng_raw_open(&d,&h,m)==0);
    CHECK(m->status==11 && d.state==4 && m->allocations==1);
    CHECK(drift_linux_virtio_rng_open(&s,&d)==0);
    m->external_mask|=1u<<s.payload.token;m->external_release_allowed|=1u<<s.payload.token;
    CHECK(s.phase==2 && d.state==5 && s.registered && s.id_owned && s.transfer.active);
    CHECK(m->allocations==2 && m->notifications==1 && s.payload.address>UINT32_MAX);
    CHECK(strcmp(((struct hwrng *)s.hwrng)->name,"virtio_rng.0")==0);
    CHECK(drift_linux_virtio_rng_open(&s,&d)==-16);
    CHECK(drift_linux_virtio_rng_read(&s,NULL,1,0)==-22);
    CHECK(drift_linux_virtio_rng_read(&s,bytes,0,0)==-22);
    CHECK(drift_linux_virtio_rng_read(&s,bytes,4097,0)==-22);
    CHECK(drift_linux_virtio_rng_read(&s,bytes,1,2)==-22);
    memset(bytes,0xcc,sizeof(bytes));
    CHECK(drift_linux_virtio_rng_read(&s,bytes,7,0)==7);expected(bytes,7,0);CHECK(bytes[7]==0xcc);
    CHECK(!s.transfer.active && m->notifications==1);
    CHECK(drift_linux_virtio_rng_read(&s,bytes,57,0)==57);expected(bytes,57,7);
    CHECK(s.transfer.active && m->notifications==2);
    CHECK(drift_linux_virtio_rng_service(&s)==1);
    CHECK(drift_linux_virtio_rng_service(&s)==0);
    CHECK(drift_linux_virtio_rng_read(&s,bytes,128,1)==128);expected(bytes,128,0);
    CHECK(s.transfer.active && m->payload_acquires>=3);
    CHECK(drift_linux_virtio_rng_cancel(&s)==0);
    CHECK(drift_linux_virtio_rng_read(&s,bytes,1,0)==1);expected(bytes,1,0);
    CHECK(drift_linux_virtio_rng_read(&s,bytes,32,1)==-4);
    CHECK(!m->releases);
    CHECK(drift_linux_virtio_rng_read(&s,bytes,32,1)==32);expected(bytes,32,1);
    CHECK(drift_linux_virtio_rng_freeze(&s)==0);
    CHECK(s.phase==4 && !s.allocated && !s.id_owned && !s.payload.data && d.state==0);
    CHECK(m->allocations==m->releases);
    CHECK(rng_raw_open(&d,&h,m)==0);
    CHECK(drift_linux_virtio_rng_restore(&s,&d)==0);
    CHECK(s.phase==2 && s.registered && s.transfer.active);
    CHECK(drift_linux_virtio_rng_read(&s,bytes,128,1)==128);expected(bytes,128,0);
    CHECK(drift_linux_virtio_rng_close(&s)==0);
    CHECK(!s.phase && !s.allocated && !s.id_owned && !s.payload.data && !s.transfer.active && d.state==0);
    CHECK(m->allocations==m->releases);
    CHECK(drift_linux_virtio_rng_close(&s)==0);
    CHECK(drift_linux_virtio_rng_read(&s,bytes,8,0)==-19);
    free(m);
}
static void delayed_and_failed(unsigned which)
{
    model *m=malloc(sizeof(*m));CHECK(m);drift_hw h;drift_virtio d={0};drift_linux_virtio_rng s={0};uint8_t bytes[128];
    init(m,&h,0,4);CHECK(rng_raw_open(&d,&h,m)==0);d.timeout_us=30;
    m->drop=which>=4;m->bad_id=which==0;m->bad_length=which==1;m->bad_index=which==2;m->zero_length=which==3;
    m->reset_stuck=which==5;
    CHECK(drift_linux_virtio_rng_open(&s,&d)==0);
    if(which>=4)CHECK(drift_linux_virtio_rng_read(&s,bytes,16,0)==0);
    CHECK(drift_linux_virtio_rng_read(&s,bytes,16,1)==(which>=4?-110:-5));
    CHECK(s.allocated && s.id_owned && s.payload.data);
    if(which==5){
        CHECK(s.phase==5 && d.state==3 && !m->releases && s.transfer.active);
        CHECK(drift_linux_virtio_rng_close(&s)==-110);
        CHECK(s.phase==5 && s.free_requested && s.id_free_requested && !s.registered && s.allocated && s.id_owned && !m->releases);
        m->reset_stuck=0;
    }else CHECK(d.state==0 && m->releases==1 && !s.transfer.active);
    CHECK(drift_linux_virtio_rng_close(&s)==0);
    CHECK(!s.phase && !s.allocated && !s.id_owned && !s.payload.data && m->allocations==m->releases);
    free(m);
}
static void probe_failures(void)
{
    for(unsigned fault=0;fault<6;++fault){
        model *m=malloc(sizeof(*m));CHECK(m);drift_hw h;drift_virtio d={0};drift_linux_virtio_rng s={0};
        init(m,&h,0,4);CHECK(rng_raw_open(&d,&h,m)==0);d.timeout_us=30;
        alloc_failure=fault==0;ida_failure=fault==1;vq_failure=fault==2;registration_failure=fault==3;
        m->fail_alloc=fault==4?2:0;m->reset_stuck=fault==5;
        if(fault==5)registration_failure=1;
        CHECK(drift_linux_virtio_rng_open(&s,&d)<0);
        alloc_failure=ida_failure=vq_failure=registration_failure=0;
        if(fault==5){CHECK(s.phase==5 && d.state==3 && s.allocated && s.id_owned && s.payload.data && !m->releases);m->reset_stuck=0;CHECK(drift_linux_virtio_rng_close(&s)==0);}
        CHECK(!s.phase && !s.allocated && !s.id_owned && !s.payload.data && !d.state && m->allocations==m->releases);
        free(m);
    }
}
static void ignored_source_errors(void)
{
    for(unsigned fault=0;fault<3;++fault){
        model *m=malloc(sizeof(*m));CHECK(m);drift_hw h;drift_virtio d={0};drift_linux_virtio_rng s={0};
        init(m,&h,0,4);CHECK(rng_raw_open(&d,&h,m)==0);
        ready_failure=fault==0;add_failure=fault==1;kick_failure=fault==2;
        CHECK(drift_linux_virtio_rng_open(&s,&d)<0);
        ready_failure=add_failure=kick_failure=0;
        CHECK(!s.phase && !s.allocated && !s.id_owned && !s.payload.data && !s.transfer.active && !d.state);
        CHECK(!m->notifications && m->allocations==m->releases);
        free(m);
    }
}
static void freeze_restore_errors(void)
{
    for(unsigned fault=0;fault<3;++fault){
        model *m=malloc(sizeof(*m));CHECK(m);drift_hw h;drift_virtio d={0};drift_linux_virtio_rng s={0};
        init(m,&h,0,4);CHECK(rng_raw_open(&d,&h,m)==0);d.timeout_us=30;
        CHECK(drift_linux_virtio_rng_open(&s,&d)==0);
        if(fault==0){
            m->reset_stuck=1;CHECK(drift_linux_virtio_rng_freeze(&s)==-110);
            CHECK(s.phase==5 && s.allocated && s.id_owned && s.payload.data && !m->releases);
            m->reset_stuck=0;CHECK(drift_linux_virtio_rng_close(&s)==0);
        }else{
            CHECK(drift_linux_virtio_rng_freeze(&s)==0 && s.phase==4);
            CHECK(rng_raw_open(&d,&h,m)==0);d.timeout_us=30;registration_failure=1;m->reset_stuck=fault==2;
            CHECK(drift_linux_virtio_rng_restore(&s,&d)<0);registration_failure=0;
            if(fault==2){
                CHECK(s.phase==5 && s.allocated && s.id_owned && s.payload.data && !s.registered);
                m->reset_stuck=0;CHECK(drift_linux_virtio_rng_close(&s)==0);
            }
        }
        CHECK(!s.phase && !s.allocated && !s.id_owned && !s.payload.data && !d.state && m->allocations==m->releases);
        free(m);
    }
}
static void id_pool(void)
{
    model *m[17];drift_hw h[17];drift_virtio d[17]={{0}};drift_linux_virtio_rng s[17]={{0}};
    for(unsigned i=0;i<17;++i){
        m[i]=malloc(sizeof(*m[i]));CHECK(m[i]);init(m[i],&h[i],0,4);CHECK(rng_raw_open(&d[i],&h[i],m[i])==0);
        int32_t r=drift_linux_virtio_rng_open(&s[i],&d[i]);
        if(i<16)CHECK(r==0 && s[i].index==i && s[i].id_owned);
        else CHECK(r==-28 && !s[i].phase && !s[i].allocated && !d[i].state);
    }
    CHECK(drift_linux_virtio_rng_close(&s[7])==0);
    CHECK(rng_raw_open(&d[16],&h[16],m[16])==0);
    CHECK(drift_linux_virtio_rng_open(&s[16],&d[16])==0 && s[16].index==7);
    CHECK(strcmp(((struct hwrng *)s[16].hwrng)->name,"virtio_rng.7")==0);
    for(unsigned i=0;i<17;++i){CHECK(drift_linux_virtio_rng_close(&s[i])==0);CHECK(m[i]->allocations==m[i]->releases);free(m[i]);}
}
static drift_linux_virtio_rng *cancel_state;
static int32_t nested_result,thread_result;
static void *cancel_thread(void *unused)
{
    (void)unused;thread_result=drift_linux_virtio_rng_service(cancel_state);
    CHECK(drift_linux_virtio_rng_cancel(cancel_state)==0);return NULL;
}
static void cancel_wait(void *context,uint32_t duration)
{
    uint8_t data[16];nested_result=drift_linux_virtio_rng_read(cancel_state,data,sizeof(data),0);
    pthread_t thread;CHECK(!pthread_create(&thread,NULL,cancel_thread,NULL));CHECK(!pthread_join(thread,NULL));
    wait_us(context,duration);
}
static void cancellation(void)
{
    model *m=malloc(sizeof(*m));CHECK(m);drift_hw h;drift_virtio d={0};drift_linux_virtio_rng s={0};uint8_t out[16];
    init(m,&h,0,4);h.wait_us=cancel_wait;CHECK(rng_raw_open(&d,&h,m)==0);m->drop=1;
    CHECK(drift_linux_virtio_rng_open(&s,&d)==0);cancel_state=&s;
    CHECK(drift_linux_virtio_rng_read(&s,out,sizeof(out),1)==-4);
    CHECK(nested_result==-16 && thread_result==-16 && s.transfer.active && !m->releases && !s.error);
    h.wait_us=wait_us;m->drop=0;service(m,0);
    CHECK(drift_linux_virtio_rng_read(&s,out,sizeof(out),1)==16);expected(out,16,0);
    CHECK(drift_linux_virtio_rng_close(&s)==0 && m->allocations==m->releases);
    free(m);cancel_state=NULL;
}
int main(void)
{
    ordinary(0);ordinary(1);
    for(unsigned fault=0;fault<6;++fault)delayed_and_failed(fault);
    probe_failures();ignored_source_errors();freeze_restore_errors();id_pool();cancellation();
    printf("Linux virtio-rng: %u checks passed (actual source, async SG/DMA, read, lifecycle, failure and cancellation)\n",checks);
    return 0;
}
