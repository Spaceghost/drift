/* Independent USB2 hub class requests. SPDX-License-Identifier: MIT */
#include "drift_usb_hub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks;if (!(x)) { fprintf(stderr,"usb hub:%d: %s\n",__LINE__,#x);exit(1); } } while (0)
typedef struct model {
    uint64_t time,reset_at;
    unsigned ports,connected,powered,reset_port,resets,clears,children,requests;
    unsigned speed,think,power_mode,delay,stall,short_desc,bad_desc,short_status,
             stuck_reset,disconnect,overcurrent,last_port,connection_change,chatter,tail_missing;
} model;
static uint64_t read_reg(void *p,uint32_t r,uint32_t o,uint32_t w) { (void)p;(void)r;(void)o;(void)w;return 0; }
static void write_reg(void *p,uint32_t r,uint32_t o,uint32_t w,uint64_t v) { (void)p;(void)r;(void)o;(void)w;(void)v; }
static int32_t alloc_dma(void *p,uint32_t s,uint32_t a,drift_dma *b) { (void)p;(void)s;(void)a;(void)b;return -1; }
static void release_dma(void *p,drift_dma *b) { (void)p;(void)b; }
static void sync_dma(void *p,drift_dma *b,uint32_t o,uint32_t n,uint32_t d) { (void)p;(void)b;(void)o;(void)n;(void)d; }
static uint64_t now(void *p) { return ((model *)p)->time; }
static void wait_us(void *p,uint32_t us) { ((model *)p)->time+=us; }
int32_t drift_usb_control(drift_usb *d,uint8_t type,uint8_t request,uint16_t value,
                         uint16_t index,uint8_t *data,uint32_t size,uint32_t *got)
{
    model *m=d->hw->context;
    ++m->requests;*got=0;
    CHECK(d->slot==1 && d->kind==5 && d->state==1);
    if (m->stall)return -32;
    if (type==0xa0) {
        CHECK(request==6 && value==0x2900 && !index && size==9 && data);
        memset(data,0,9);data[0]=(uint8_t)(7+2*((m->ports+8)/8)-m->tail_missing);data[1]=0x29;
        data[2]=(uint8_t)m->ports;data[3]=(uint8_t)(m->power_mode|(m->think<<5));data[5]=(uint8_t)m->delay;
        if (m->bad_desc)data[1]=0x2a;
        *got=m->short_desc?m->short_desc:(data[0]<9?data[0]:9);return 0;
    }
    CHECK(index>=1 && index<=m->ports);
    m->last_port=index;
    if (type==0x23) {
        CHECK(size==0 && data==NULL && (request==1 || request==3));
        if (request==3 && value==8) { m->powered|=1u<<index;return 0; }
        if (request==3 && value==4) {
            CHECK(m->power_mode==2 || m->powered==(1u<<(m->ports+1))-2);
            CHECK(m->time>=m->delay*2000+100000);
            m->reset_port=index;m->reset_at=m->time+20000;++m->resets;return 0;
        }
        CHECK(request==1 && value>=16 && value<=20);if (value==16)m->connection_change=0;++m->clears;return 0;
    }
    CHECK(type==0xa3 && request==0 && !value && size==4 && data);
    {
        uint32_t status=0;
        if (index==m->connected && !(m->disconnect && m->time>=100000)) {
            status=1|0x100;if (m->connection_change || m->chatter)status|=1u<<16;
            if (m->overcurrent)status|=8;
            if (m->reset_port==index) {
                if (m->stuck_reset || m->time<m->reset_at)status|=16;
                else {
                    status|=2|(1u<<20);
                    if (m->speed==2)status|=0x200;
                    if (m->speed==3)status|=0x400;
                    if (m->speed==4)status|=0x600;
                }
            }
        }
        for (unsigned i=0;i<4;++i)data[i]=(uint8_t)(status>>(i*8));
        *got=m->short_status?3:4;return 0;
    }
}
int32_t drift_usb_hub_child(drift_usb *d,uint8_t port,uint8_t speed,uint8_t ports,uint8_t think)
{
    model *m=d->hw->context;
    CHECK(port==m->connected && speed==m->speed && ports==m->ports && think==m->think);
    CHECK(m->resets==1 && m->clears==2 && m->time>=m->reset_at);
    ++m->children;d->slot=2;d->kind=1;return 0;
}
static drift_usb setup(model *m,drift_hw *h)
{
    memset(m,0,sizeof(*m));m->ports=4;m->connected=2;m->speed=3;m->think=2;m->delay=2;m->connection_change=1;
    *h=(drift_hw){m,read_reg,write_reg,alloc_dma,release_dma,sync_dma,now,wait_us};
    drift_usb d={0};d.hw=h;d.state=1;d.kind=5;d.slot=1;return d;
}
int main(void)
{
    model m;drift_hw h;drift_usb d=setup(&m,&h);
    CHECK(drift_usb_attach_hub(NULL,0)==-22);
    CHECK(drift_usb_attach_hub(&d,0)==0 && m.children==1 && m.last_port==2);
    CHECK(drift_usb_attach_hub(&d,0)==-22);
    d=setup(&m,&h);CHECK(drift_usb_attach_hub(&d,2)==0 && m.children==1);
    d=setup(&m,&h);CHECK(drift_usb_attach_hub(&d,5)==-95 && !m.children);
    d=setup(&m,&h);d.parent_slot=1;CHECK(drift_usb_attach_hub(&d,0)==-22 && !m.requests);
    d=setup(&m,&h);m.connected=0;CHECK(drift_usb_attach_hub(&d,0)==-19 && !m.resets);
    d=setup(&m,&h);CHECK(drift_usb_attach_hub(&d,1)==-19 && !m.resets);
    d=setup(&m,&h);m.stall=1;CHECK(drift_usb_attach_hub(&d,0)==-32);
    d=setup(&m,&h);m.short_desc=6;CHECK(drift_usb_attach_hub(&d,0)==-5);
    d=setup(&m,&h);m.short_desc=7;CHECK(drift_usb_attach_hub(&d,0)==-5);
    d=setup(&m,&h);m.short_desc=8;CHECK(drift_usb_attach_hub(&d,0)==-5);
    d=setup(&m,&h);m.bad_desc=1;CHECK(drift_usb_attach_hub(&d,0)==-5);
    d=setup(&m,&h);m.ports=16;CHECK(drift_usb_attach_hub(&d,0)==-95);
    d=setup(&m,&h);m.short_status=1;CHECK(drift_usb_attach_hub(&d,0)==-5 && !m.resets);
    d=setup(&m,&h);m.overcurrent=1;CHECK(drift_usb_attach_hub(&d,0)==-5 && !m.resets);
    d=setup(&m,&h);m.disconnect=1;CHECK(drift_usb_attach_hub(&d,0)==-19 && !m.resets);
    d=setup(&m,&h);m.stuck_reset=1;CHECK(drift_usb_attach_hub(&d,0)==-110 && !m.children);
    d=setup(&m,&h);m.power_mode=2;CHECK(drift_usb_attach_hub(&d,0)==0 && !m.powered);
    d=setup(&m,&h);m.ports=15;m.connected=15;CHECK(drift_usb_attach_hub(&d,0)==0);
    /* QEMU8.2.2's8-port descriptor is10bytes, with a complete2-byte
     * removable bitmap and one omitted unused power-mask byte. */
    d=setup(&m,&h);m.ports=8;m.tail_missing=1;CHECK(drift_usb_attach_hub(&d,0)==0);
    d=setup(&m,&h);m.ports=8;m.tail_missing=2;CHECK(drift_usb_attach_hub(&d,0)==0);
    d=setup(&m,&h);m.ports=8;m.tail_missing=3;CHECK(drift_usb_attach_hub(&d,0)==-5);
    d=setup(&m,&h);m.tail_missing=1;CHECK(drift_usb_attach_hub(&d,0)==0);
    d=setup(&m,&h);m.speed=1;CHECK(drift_usb_attach_hub(&d,0)==0);
    d=setup(&m,&h);m.speed=2;CHECK(drift_usb_attach_hub(&d,0)==0);
    d=setup(&m,&h);m.chatter=1;CHECK(drift_usb_attach_hub(&d,0)==-110 && !m.resets);
    d=setup(&m,&h);m.power_mode=3;CHECK(drift_usb_attach_hub(&d,0)==-95 && !m.resets);
    d=setup(&m,&h);m.speed=4;CHECK(drift_usb_attach_hub(&d,0)==-5 && !m.children);
    printf("usb hub: %u class request/descriptor/reset checks passed\n",checks);return 0;
}
