/* Independent register/DMA models for actual Ethernet commands.
 * SPDX-License-Identifier: MIT */
#include "drift_ethernet.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct allocation { uint8_t *cpu,*device; uint64_t address; uint32_t size; int live; } allocation;
typedef struct model {
    uint8_t regs[0x6000];
    allocation alloc[8];
    uint32_t kind, count, freed, sync_to, sync_from, transmits, received, expected_size;
    uint32_t rx_producer;
    uint32_t reset_stuck, tx_stuck, tx_error, fail_alloc, high_dma, bad_alignment;
    uint64_t clock;
    uint8_t packet[2048], mac[6];
    int pending;
} model;
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); abort(); } } while (0)
static uint64_t le(const uint8_t *p,unsigned bytes) { uint64_t x=0; for(unsigned i=0;i<bytes;i++)x|=(uint64_t)p[i]<<(8*i); return x; }
static void put(uint8_t*p,unsigned bytes,uint64_t x) { for(unsigned i=0;i<bytes;i++)p[i]=(uint8_t)(x>>(8*i)); }
static uint8_t *dma_at(model*m,uint64_t a,unsigned n) {
    for(unsigned i=0;i<m->count;i++) { allocation*b=&m->alloc[i];
        if(b->live&&a>=b->address&&a-b->address<=b->size&&n<=b->size-(a-b->address))return b->device+(a-b->address);
    }
    CHECK(!"DMA outside live allocation");return NULL;
}
static uint64_t rd(void*ctx,uint32_t region,uint32_t off,uint32_t width) {
    model*m=ctx; CHECK(region==7&&off+width<=sizeof(m->regs));
    if(m->kind==2&&off==0x37&&!(m->regs[0x37]&0x10))return (m->regs[0x37]&~1u)|(m->pending?0:1);
    return le(m->regs+off,width);
}
static void wr(void*ctx,uint32_t region,uint32_t off,uint32_t width,uint64_t value) {
    model*m=ctx; CHECK(region==7&&off+width<=sizeof(m->regs)); put(m->regs+off,width,value);
    if(m->kind==1&&off==0&&(value&0x04000000)&&!m->reset_stuck) {
        put(m->regs,4,value&~0x04000000u);put(m->regs+0x100,4,0);put(m->regs+0x400,4,0);
    }
    if(m->kind==2&&off==0x37&&(value&0x10)&&!m->reset_stuck) { m->regs[0x37]=0;m->pending=0; }
    if(m->kind==1&&off==0x3818) {
        uint32_t head=(uint32_t)le(m->regs+0x3810,4);
        uint64_t ring=le(m->regs+0x3800,4)|(le(m->regs+0x3804,4)<<32);
        if (value==head) return;
        CHECK(value==((head+1)&15));
        uint8_t*d=dma_at(m,ring+head*16,16); unsigned n=(unsigned)le(d+8,2);
        CHECK(d[11]==0x0b&&n>=60&&n<=1518);
        memcpy(m->packet,dma_at(m,le(d,8),n),n);m->expected_size=n;m->transmits++;
        if(!m->tx_stuck) { d[12]=(uint8_t)(1|(m->tx_error?2:0)); put(m->regs+0x3810,4,value); }
    }
    if(m->kind==2&&off>=0x10&&off<=0x1c&&width==4) {
        unsigned slot=(off-0x10)/4,n=(unsigned)value&0x1fff;
        CHECK(n>=60&&n<=1518);
        memcpy(m->packet,dma_at(m,le(m->regs+0x20+slot*4,4),n),n);
        m->expected_size=n;m->transmits++;
        if(!m->tx_stuck)put(m->regs+off,4,value|(m->tx_error?0x40000000:0xa000));
    }
    if(m->kind==2&&off==0x38)m->pending=0;
    if(m->kind==2&&off==0x3e)put(m->regs+off,2,0);
}
static int32_t alloc_dma(void*ctx,uint32_t n,uint32_t align,drift_dma*out) {
    model*m=ctx;
    if(m->fail_alloc&&m->count+1==m->fail_alloc)return -1;
    CHECK(m->count<8);
    allocation*b=&m->alloc[m->count++];
    b->cpu=calloc(1,n);b->device=calloc(1,n);CHECK(b->cpu&&b->device);
    b->size=n;b->live=1;b->address=(m->high_dma?UINT64_C(0x100000000):UINT64_C(0x100000))+(uint64_t)(m->count-1)*0x10000;
    CHECK((b->address&(align-1))==0);
    if(m->bad_alignment)b->address++;
    out->data=b->cpu;out->address=b->address;out->size=n;out->token=m->count;
    return 0;
}
static void release_dma(void*ctx,drift_dma*b) {
    model*m=ctx;CHECK(b->token&&b->token<=m->count);
    allocation*a=&m->alloc[b->token-1];CHECK(a->live);
    if(m->kind==1)CHECK(le(m->regs+0x100,4)==0&&le(m->regs+0x400,4)==0);
    else CHECK(!(m->regs[0x37]&0x0c));
    a->live=0;free(a->cpu);free(a->device);m->freed++;
}
static void sync_dma(void*ctx,drift_dma*b,uint32_t at,uint32_t n,uint32_t direction) {
    model*m=ctx;CHECK(b->token&&b->token<=m->count&&at<=b->size&&n<=b->size-at);
    allocation*a=&m->alloc[b->token-1];CHECK(a->live);
    if(direction==DRIFT_DMA_TO_DEVICE||direction==DRIFT_DMA_BIDIRECTIONAL) { memcpy(a->device+at,a->cpu+at,n);m->sync_to++; }
    else if(direction==DRIFT_DMA_FROM_DEVICE) { memcpy(a->cpu+at,a->device+at,n);m->sync_from++; }
    else CHECK(!"unknown DMA sync direction");
}
static uint64_t now(void*ctx) { return ((model*)ctx)->clock; }
static void wait_us(void*ctx,uint32_t us) { ((model*)ctx)->clock+=us; }
static drift_hw setup(model*m,unsigned kind) {
    memset(m,0,sizeof(*m));m->kind=kind;
    const uint8_t mac[]={0x52,0x54,0,0x12,0x34,0x56};memcpy(m->mac,mac,6);
    if(kind==1) { memcpy(m->regs+0x5400,mac,6);m->regs[0x5407]=0x80; }
    else memcpy(m->regs,mac,6);
    drift_hw hw={m,rd,wr,alloc_dma,release_dma,sync_dma,now,wait_us};return hw;
}
static void inject(model*m,const uint8_t*packet,unsigned n,unsigned bad) {
    if(m->kind==1) {
        uint64_t ring=le(m->regs+0x2800,4)|(le(m->regs+0x2804,4)<<32);
        uint8_t*desc=dma_at(m,ring+m->rx_producer*16,16);
        if(n<=2048)memcpy(dma_at(m,le(desc,8),n),packet,n);
        put(desc+8,2,n);desc[12]=bad==2?1:3;desc[13]=bad==1?1:0;
        m->rx_producer=(m->rx_producer+1)%16;
    } else {
        uint8_t*p=dma_at(m,le(m->regs+0x30,4)+m->rx_producer,n+8);
        put(p,2,bad==1?5:1);put(p+2,2,n+4);memcpy(p+4,packet,n);memset(p+4+n,0,4);m->pending=1;
        m->rx_producer=((m->rx_producer+4+n+4+3)/4*4)%8192;
    }
    m->received++;
}
static void normal(unsigned kind) {
    model m;drift_hw hw=setup(&m,kind);drift_ethernet d={0};uint8_t frame[2048],out[2048];uint32_t length;
    for(unsigned i=0;i<sizeof(frame);i++)frame[i]=(uint8_t)(i*7);
    CHECK(drift_eth_init(&d,&hw,7,kind,1000)==0);CHECK(d.state==1);CHECK(!memcmp(d.mac,m.mac,6));
    CHECK(drift_eth_init(&d,&hw,7,kind,1000)==-2);
    CHECK(drift_eth_rx(&d,out,sizeof(out),&length)==1&&length==0);
    CHECK(drift_eth_tx(&d,frame,13)==-1);CHECK(drift_eth_tx(&d,frame,1519)==-1);
    for(unsigned i=0;i<40;i++) {
        CHECK(drift_eth_tx(&d,frame,14+i)==0);CHECK(m.expected_size==60);
        CHECK(!memcmp(frame,m.packet,14+i));for(unsigned j=14+i;j<60;j++)CHECK(m.packet[j]==0);
    }
    CHECK(drift_eth_tx(&d,frame,1518)==0&&m.expected_size==1518);CHECK(!memcmp(frame,m.packet,1518));
    for(unsigned i=0;i<40;i++) {
        inject(&m,frame,1518,0);
        CHECK(drift_eth_rx(&d,out,100,&length)==-6&&length==0);
        CHECK(drift_eth_rx(&d,out,sizeof(out),&length)==0&&length==1518);CHECK(!memcmp(frame,out,1518));
        CHECK(drift_eth_rx(&d,out,sizeof(out),&length)==1);
    }
    CHECK(m.sync_to>0&&m.sync_from>0);
    CHECK(drift_eth_close(&d)==0&&m.freed==m.count);CHECK(drift_eth_close(&d)==0);
    CHECK(drift_eth_tx(&d,frame,60)==-2);CHECK(drift_eth_rx(&d,out,sizeof(out),&length)==-2);
}
static void errors(unsigned kind) {
    for(unsigned mode=0;mode<7;mode++) {
        model m;drift_hw hw=setup(&m,kind);drift_ethernet d={0};uint8_t frame[2048]={0};uint32_t n;
        if(mode==0) {
            m.reset_stuck=1;CHECK(drift_eth_init(&d,&hw,7,kind,100)==-4);CHECK(m.count==0);m.reset_stuck=0;CHECK(drift_eth_close(&d)==0);continue;
        }
        CHECK(drift_eth_init(&d,&hw,7,kind,100)==0);
        if(mode==1||mode==2) {
            m.tx_stuck=mode==1;m.tx_error=mode==2;
            CHECK(drift_eth_tx(&d,frame,60)==(mode==1?-4:-5));CHECK(d.state==-1);CHECK(m.freed==0);
        } else if(mode==3) {
            inject(&m,frame,60,1);CHECK(drift_eth_rx(&d,frame,sizeof(frame),&n)==-5);
        } else if(mode==4&&kind==1) {
            inject(&m,frame,60,2);CHECK(drift_eth_rx(&d,frame,sizeof(frame),&n)==-5);
        } else if(mode==5) {
            m.tx_stuck=1;m.reset_stuck=1;CHECK(drift_eth_tx(&d,frame,60)==-4);CHECK(drift_eth_close(&d)==-4&&m.freed==0);
            m.reset_stuck=0;
        } else {
            inject(&m,frame,1530,0);CHECK(drift_eth_rx(&d,frame,sizeof(frame),&n)==-5);
        }
        CHECK(drift_eth_close(&d)==0&&m.freed==m.count);
    }
    for(unsigned fail=1;fail<=(kind==1?4:2);fail++) {
        model m;drift_hw hw=setup(&m,kind);drift_ethernet d={0};m.fail_alloc=fail;
        CHECK(drift_eth_init(&d,&hw,7,kind,100)==-3);CHECK(d.state==0&&m.freed==m.count);
    }
    model m;drift_hw hw=setup(&m,kind);drift_ethernet d={0};m.high_dma=1;
    CHECK(drift_eth_init(&d,&hw,7,kind,100)==(kind==1?0:-3));CHECK(drift_eth_close(&d)==0);CHECK(m.freed==m.count);
    hw=setup(&m,kind);memset(&d,0,sizeof(d));m.bad_alignment=1;
    CHECK(drift_eth_init(&d,&hw,7,kind,100)==-3&&m.freed==m.count);
}
int main(void) {
    CHECK(drift_eth_init(NULL,NULL,0,0,0)==-1);CHECK(drift_eth_close(NULL)==-1);
    normal(1);normal(2);errors(1);errors(2);
    printf("ethernet: %u checks passed (82540EM + RTL8139 models)\n",checks);
    return 0;
}
