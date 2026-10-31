/* Reuse the independent register/DMA oracle, not its class driver tests.
 * SPDX-License-Identifier: MIT */
#define main drift_viorng_native_model_reference_main
#include "../../../tests/test_virtio.c"
#undef main
#include "profile.h"

typedef struct notification_model { drift_win_rng *profile; unsigned count; } notification_model;
static uint32_t fixture_state,fixture_start;
static uint32_t next_fixture(uint32_t x){x^=x<<13;x^=x>>17;x^=x<<5;return x;}
/* Nonperiodic device bytes catch stale repeated 64/256-byte blocks. The
 * register oracle still checks descriptor authority and used-ring ordering. */
static void write_rng_reg(void *context,uint32_t region,uint32_t offset,uint32_t width,uint64_t value)
{
    model *m=context;
    if(region==0&&offset==0x50){
        CHECK(!m->transport&&width==4&&value==0);++m->notifications;
        queue *q=&m->queue[0];CHECK(q->enabled&&m->status==15);
        if(m->drop)return;
        uint8_t *available=device(m,q->avail,20);
        CHECK((uint16_t)(get(available+2,2)-q->seen)==1);
        CHECK(get(available+4+(q->seen&7)*2,2)==0);++q->seen;
        unsigned size;uint8_t *output=descriptor(m,0,0,2,&size);
        CHECK(size>0&&size<=4096);fixture_start=fixture_state;
        for(unsigned i=0;i<size;++i){fixture_state=next_fixture(fixture_state);output[i]=(uint8_t)fixture_state;}
        used(m,0,size);return;
    }
    write_reg(context,region,offset,width,value);
}
static void completed(void *context,drift_win_rng_request *r)
{
    notification_model *n=context;
    CHECK(!n->profile->locked&&r->phase==DRIFT_WIN_RNG_COMPLETE&&r->completions==1&&r->notifying);
    CHECK(drift_windows_viorng_request_reset(r)==DRIFT_WIN_RNG_BUSY);
    CHECK(drift_windows_viorng_close(n->profile)==DRIFT_WIN_RNG_BUSY);
    ++n->count;
}
static void setup(model *m,drift_hw *h,drift_win_rng *s,notification_model *n,drift_win_rng_host *host)
{
    init(m,h,0,4);memset(s,0,sizeof(*s));*n=(notification_model){s,0};
    h->write=write_rng_reg;fixture_state=0x6d2b79f5U;fixture_start=fixture_state;
    *host=(drift_win_rng_host){n,completed};
}
static void check_pattern(const uint8_t *p,size_t n)
{uint32_t expected=fixture_start;for(size_t i=0;i<n;++i){expected=next_fixture(expected);CHECK(p[i]==(uint8_t)expected);}}
static void check_pattern_at(const uint8_t *p,size_t n,uint32_t seed)
{for(size_t i=0;i<n;++i){seed=next_fixture(seed);CHECK(p[i]==(uint8_t)seed);}}
static void test_basic(model *m)
{
    drift_hw h;drift_win_rng s;notification_model notify;drift_win_rng_host host;
    setup(m,&h,&s,&notify,&host);
    CHECK(drift_windows_viorng_open_mmio(&s,&h,0,&host)==0);
    CHECK(s.phase==2&&s.configured==255&&s.transport.state==5&&m->status==15);
    CHECK(m->notifications==0&&m->allocations==2&&s.common.data);
    uint8_t bytes[258];memset(bytes,0xcc,sizeof(bytes));drift_win_rng_request r={0};
    CHECK(drift_windows_viorng_read(&s,&r,bytes+1,256,128)==DRIFT_WIN_RNG_PENDING);
    CHECK(r.phase==2&&r.cancelable&&r.completions==0&&notify.count==0&&s.transfer.active);
    for(unsigned i=0;i<sizeof(bytes);++i)CHECK(bytes[i]==0xcc);
    CHECK(drift_windows_viorng_pump(&s)==1);
    CHECK(r.status==0&&r.information==128&&r.completions==1&&notify.count==1&&!s.transfer.active);
    check_pattern(bytes+1,128);CHECK(bytes[0]==0xcc&&bytes[129]==0xcc&&bytes[257]==0xcc);
    CHECK(drift_windows_viorng_pump(&s)==0&&notify.count==1&&s.pool_used==0);
    CHECK(drift_windows_viorng_request_reset(&r)==0);
    unsigned kicks=m->notifications;
    CHECK(drift_windows_viorng_read(&s,&r,NULL,0,0)==0);
    CHECK(r.status==0&&r.information==0&&r.completions==1&&notify.count==2&&m->notifications==kicks);
    CHECK(drift_windows_viorng_close(&s)==0&&s.phase==0&&m->allocations==m->releases);
    CHECK(drift_windows_viorng_close(&s)==0);
}
static void test_queue_and_cancel(model *m)
{
    drift_hw h;drift_win_rng s;notification_model notify;drift_win_rng_host host;
    setup(m,&h,&s,&notify,&host);CHECK(drift_windows_viorng_open_mmio(&s,&h,0,&host)==0);
    uint8_t a[128],b[128],c[128];memset(a,0xcc,sizeof(a));memset(b,0xcc,sizeof(b));
    drift_win_rng_request first={0},second={0},queued_cancel={0};
    CHECK(drift_windows_viorng_read(&s,&first,a,sizeof(a),128)==DRIFT_WIN_RNG_PENDING);
    CHECK(drift_windows_viorng_read(&s,&second,b,sizeof(b),64)==DRIFT_WIN_RNG_PENDING);
    CHECK(drift_windows_viorng_read(&s,&queued_cancel,c,sizeof(c),32)==DRIFT_WIN_RNG_PENDING);
    CHECK(first.phase==2&&second.phase==1&&queued_cancel.phase==1&&m->notifications==1);
    CHECK(drift_windows_viorng_cancel(&s,&queued_cancel)==0);
    CHECK(queued_cancel.status==DRIFT_WIN_RNG_CANCELLED&&notify.count==1&&s.pending_count==1);
    CHECK(drift_windows_viorng_cancel(&s,&first)==0);
    CHECK(first.status==DRIFT_WIN_RNG_CANCELLED&&first.completions==1&&notify.count==2);
    CHECK(s.transfer.active&&s.common.data&&second.phase==1&&m->notifications==1);
    CHECK(drift_windows_viorng_cancel(&s,&first)<0&&notify.count==2);
    CHECK(drift_windows_viorng_pump(&s)==0);
    CHECK(second.phase==2&&m->notifications==2&&s.transfer.active&&s.pool_used);
    CHECK(drift_windows_viorng_pump(&s)==1);
    CHECK(second.status==0&&second.information==64&&notify.count==3);
    check_pattern(b,64);for(unsigned i=0;i<128;++i)CHECK(a[i]==0xcc);
    CHECK(drift_windows_viorng_close(&s)==0&&m->allocations==m->releases);
}
static void test_bounds_and_source_failures(model *m)
{
    drift_hw h;drift_win_rng s;notification_model notify;drift_win_rng_host host;
    setup(m,&h,&s,&notify,&host);CHECK(drift_windows_viorng_open_mmio(&s,&h,0,&host)==0);
    uint8_t *large=malloc(8194);CHECK(large);memset(large,0xcc,8194);drift_win_rng_request r={0};
    CHECK(drift_windows_viorng_read(&s,&r,large+1,8192,8192)==DRIFT_WIN_RNG_PENDING);
    CHECK(drift_windows_viorng_pump(&s)==1&&r.status==0&&r.information==4096);
    check_pattern(large+1,4096);CHECK(large[0]==0xcc&&large[4097]==0xcc&&large[8193]==0xcc);
    CHECK(drift_windows_viorng_request_reset(&r)==0);
    unsigned kicks=m->notifications;
    CHECK(drift_windows_viorng_read(&s,&r,large,8,64)==STATUS_BUFFER_TOO_SMALL);
    CHECK(r.completions==1&&m->notifications==kicks&&notify.count==2);
    CHECK(drift_windows_viorng_request_reset(&r)==0);
    CHECK(drift_windows_viorng_read(&s,&r,NULL,64,64)==STATUS_INVALID_PARAMETER);
    CHECK(r.completions==1&&m->notifications==kicks&&notify.count==3);
    CHECK(drift_windows_viorng_close(&s)==0&&m->allocations==m->releases);free(large);
    /* Preserve the upstream D0Entry quirk: DMA allocation failure does not
     * change its SUCCESS return, but its subsequent read must fail honestly. */
    setup(m,&h,&s,&notify,&host);m->fail_alloc=2;
    CHECK(drift_windows_viorng_open_mmio(&s,&h,0,&host)==0);
    CHECK(!s.common.data&&s.phase==2&&m->allocations==1);
    memset(&r,0,sizeof(r));uint8_t bytes[16];
    CHECK(drift_windows_viorng_read(&s,&r,bytes,sizeof(bytes),sizeof(bytes))==STATUS_INSUFFICIENT_RESOURCES);
    CHECK(r.completions==1&&notify.count==1&&m->notifications==0);
    CHECK(drift_windows_viorng_close(&s)==0&&m->allocations==m->releases);
    setup(m,&h,&s,&notify,&host);m->fail_alloc=1;
    CHECK(drift_windows_viorng_open_mmio(&s,&h,0,&host)<0);
    CHECK(s.phase==0&&m->allocations==m->releases);
}
static void test_bad_completion(model *m,unsigned malformed,unsigned stuck)
{
    drift_hw h;drift_win_rng s;notification_model notify;drift_win_rng_host host;
    setup(m,&h,&s,&notify,&host);CHECK(drift_windows_viorng_open_mmio(&s,&h,0,&host)==0);
    uint8_t bytes[64];memset(bytes,0xcc,sizeof(bytes));drift_win_rng_request r={0};
    if(malformed==1)m->bad_id=1;else if(malformed==2)m->bad_length=1;else m->bad_index=1;
    m->reset_stuck=stuck;
    CHECK(drift_windows_viorng_read(&s,&r,bytes,sizeof(bytes),sizeof(bytes))==DRIFT_WIN_RNG_PENDING);
    CHECK(drift_windows_viorng_pump(&s)<0&&r.completions==0);
    for(unsigned i=0;i<sizeof(bytes);++i)CHECK(bytes[i]==0xcc);
    if(stuck){
        CHECK(s.transfer.active&&s.common.data&&m->releases==0);
        CHECK(drift_windows_viorng_close(&s)<0);
        CHECK(s.phase!=0&&s.common.data&&s.pool_used&&m->releases==0);
        CHECK(r.completions==1&&r.status==DRIFT_WIN_RNG_CANCELLED&&notify.count==1);
        m->reset_stuck=0;
    }
    CHECK(drift_windows_viorng_close(&s)==0);
    CHECK(s.phase==0&&m->allocations==m->releases&&s.pool_used==0&&!s.common.data);
    CHECK(r.completions==1&&r.status==DRIFT_WIN_RNG_CANCELLED&&notify.count==1);
}
static void test_timeout_and_remove(model *m)
{
    drift_hw h;drift_win_rng s;notification_model notify;drift_win_rng_host host;
    setup(m,&h,&s,&notify,&host);CHECK(drift_windows_viorng_open_mmio(&s,&h,0,&host)==0);
    m->drop=1;uint8_t bytes[128];drift_win_rng_request a={0},b={0};
    CHECK(drift_windows_viorng_read(&s,&a,bytes,128,128)==DRIFT_WIN_RNG_PENDING);
    CHECK(drift_windows_viorng_read(&s,&b,bytes,128,128)==DRIFT_WIN_RNG_PENDING);
    CHECK(drift_windows_viorng_pump(&s)==0);
    m->time+=s.transport.timeout_us;
    CHECK(drift_windows_viorng_pump(&s)<0);
    CHECK(drift_windows_viorng_close(&s)==0);
    CHECK(a.status==DRIFT_WIN_RNG_CANCELLED&&b.status==DRIFT_WIN_RNG_CANCELLED);
    CHECK(a.completions==1&&b.completions==1&&notify.count==2&&m->allocations==m->releases);
}
static void test_callback_gating(model *m)
{
    drift_hw h;drift_win_rng s;notification_model notify;drift_win_rng_host host;
    setup(m,&h,&s,&notify,&host);CHECK(drift_windows_viorng_open_mmio(&s,&h,0,&host)==0);
    CHECK(VirtRngEvtInterruptIsr(&s.interrupt,0)==FALSE&&s.dpc==0);
    CHECK(VirtRngEvtInterruptDisable(&s.interrupt,&s.device)==0);
    uint8_t bytes[64];memset(bytes,0xcc,sizeof(bytes));drift_win_rng_request r={0};
    CHECK(drift_windows_viorng_read(&s,&r,bytes,64,64)==DRIFT_WIN_RNG_PENDING);
    CHECK(drift_windows_viorng_pump(&s)==0);
    CHECK(s.latch_ready==1&&s.transfer.active==0&&r.phase==2&&notify.count==0);
    for(unsigned i=0;i<64;++i)CHECK(bytes[i]==0xcc);
    CHECK(VirtRngEvtInterruptEnable(&s.interrupt,&s.device)==0);
    CHECK(drift_windows_viorng_pump(&s)==1&&r.status==0&&r.information==64);
    check_pattern(bytes,64);CHECK(s.latch_ready==0&&s.dpc==0&&s.pool_used==0);
    CHECK(drift_windows_viorng_close(&s)==0&&m->allocations==m->releases);
}
static void test_queue_limit(model *m)
{
    drift_hw h;drift_win_rng s;notification_model notify;drift_win_rng_host host;
    setup(m,&h,&s,&notify,&host);CHECK(drift_windows_viorng_open_mmio(&s,&h,0,&host)==0);
    m->drop=1;uint8_t bytes[16][16];drift_win_rng_request r[17]={{0}};
    for(unsigned i=0;i<16;++i)CHECK(drift_windows_viorng_read(&s,&r[i],bytes[i],16,16)==DRIFT_WIN_RNG_PENDING);
    CHECK(s.pending_count==15&&s.active==&r[0]&&m->notifications==1);
    CHECK(drift_windows_viorng_read(&s,&r[16],bytes[0],16,16)==DRIFT_WIN_RNG_BUSY&&r[16].phase==0);
    CHECK(drift_windows_viorng_close(&s)==0&&notify.count==16&&m->allocations==m->releases);
    for(unsigned i=0;i<16;++i)CHECK(r[i].status==DRIFT_WIN_RNG_CANCELLED&&r[i].completions==1);
}
typedef struct chained_notification {
    notification_model base;
    drift_win_rng_request *next;
    uint8_t *output;
    uint32_t seed;
} chained_notification;
static void completed_then_read(void *context,drift_win_rng_request *r)
{
    chained_notification *n=context;completed(&n->base,r);
    if(n->base.count==1){
        CHECK(drift_windows_viorng_read(n->base.profile,n->next,n->output,64,64)==DRIFT_WIN_RNG_PENDING);
        n->seed=fixture_start;
    }
}
static void test_callback_read(model *m)
{
    drift_hw h;drift_win_rng s;notification_model notify;drift_win_rng_host host;
    setup(m,&h,&s,&notify,&host);
    uint8_t first[64],second[64];drift_win_rng_request a={0},b={0};
    chained_notification chain={{&s,0},&b,second,0};
    host.context=&chain;host.completed=completed_then_read;
    CHECK(drift_windows_viorng_open_mmio(&s,&h,0,&host)==0);
    CHECK(drift_windows_viorng_read(&s,&a,first,64,64)==DRIFT_WIN_RNG_PENDING);
    uint32_t first_seed=fixture_start;
    CHECK(drift_windows_viorng_pump(&s)==1&&chain.base.count==1);
    CHECK(a.phase==3&&a.status==0&&a.information==64&&a.completions==1);
    CHECK(b.phase==2&&b.completions==0&&s.transfer.active&&s.depth==0);
    check_pattern_at(first,64,first_seed);
    CHECK(drift_windows_viorng_pump(&s)==1&&chain.base.count==2);
    CHECK(b.phase==3&&b.status==0&&b.information==64&&b.completions==1);
    check_pattern_at(second,64,chain.seed);CHECK(memcmp(first,second,64)!=0);
    CHECK(drift_windows_viorng_close(&s)==0&&m->allocations==m->releases);
}
static void test_two_instances(model *a)
{
    model *b=malloc(sizeof(*b));CHECK(b);
    drift_hw ha,hb;drift_win_rng sa,sb;notification_model na,nb;drift_win_rng_host ca,cb;
    setup(a,&ha,&sa,&na,&ca);setup(b,&hb,&sb,&nb,&cb);
    CHECK(drift_windows_viorng_open_mmio(&sa,&ha,0,&ca)==0);
    CHECK(drift_windows_viorng_open_mmio(&sb,&hb,0,&cb)==0);
    uint8_t ba[128],bb[128];drift_win_rng_request ra={0},rb={0};
    CHECK(drift_windows_viorng_read(&sa,&ra,ba,128,128)==DRIFT_WIN_RNG_PENDING);uint32_t seed_a=fixture_start;
    CHECK(drift_windows_viorng_read(&sb,&rb,bb,128,128)==DRIFT_WIN_RNG_PENDING);uint32_t seed_b=fixture_start;
    CHECK(drift_windows_viorng_cancel(&sb,&ra)==DRIFT_WIN_RNG_INVALID_STATE);
    CHECK(drift_windows_viorng_pump(&sb)==1&&nb.count==1&&na.count==0&&ra.phase==2);
    check_pattern_at(bb,128,seed_b);
    CHECK(drift_windows_viorng_pump(&sa)==1&&na.count==1);
    check_pattern_at(ba,128,seed_a);CHECK(memcmp(ba,bb,128)!=0);
    CHECK(drift_windows_viorng_close(&sa)==0&&drift_windows_viorng_close(&sb)==0);
    CHECK(a->allocations==a->releases&&b->allocations==b->releases);free(b);
}
int main(void)
{
    model *m=malloc(sizeof(*m));CHECK(m);
    test_basic(m);test_queue_and_cancel(m);test_bounds_and_source_failures(m);
    for(unsigned malformed=1;malformed<=3;++malformed){test_bad_completion(m,malformed,0);test_bad_completion(m,malformed,1);}
    test_timeout_and_remove(m);
    test_callback_gating(m);test_queue_limit(m);test_callback_read(m);test_two_instances(m);
    free(m);printf("Windows viorng: %u checks passed (unchanged source, actual split-ring DMA oracle, deferred ISR/DPC, cancellation and quiescence)\n",checks);
    return 0;
}
