/* Independent DMA/register model for the native driver. SPDX-License-Identifier: MIT */
#include "drift_nvme.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"nvme test:%d: %s\n",__LINE__,#x); exit(1); } } while (0)
typedef struct model {
    uint8_t cpu[5][4096], device[5][4096], stored[4096];
    uint64_t addresses[5], cap, asq, acq, isq, icq, now, blocks, last_lba, stored_lba;
    unsigned allocated, released, live[5], fail_alloc, bad_alloc, enabled, stuck_reset,
             stuck_enable, drop, corrupt_cid, corrupt_qid, corrupt_head, error_opcode,
             error_once, cq_tail[2], cq_phase[2], sq_head[2], depth, aqa, block_shift,
             identify_bad, metadata, protection, fail_identify, commands, reads, writes,
             flushes, sync_to, sync_from, stored_size, nsid, doorbell_stride;
} model;
static model m;

static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0]|p[1]<<8); }
static uint32_t u32(const uint8_t *p) { return (uint32_t)u16(p)|(uint32_t)u16(p+2)<<16; }
static uint64_t u64(const uint8_t *p) { return (uint64_t)u32(p)|(uint64_t)u32(p+4)<<32; }
static void p16(uint8_t *p,uint32_t n) { p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8); }
static void p32(uint8_t *p,uint32_t n) { p16(p,n);p16(p+2,n>>16); }
static void p64(uint8_t *p,uint64_t n) { p32(p,(uint32_t)n);p32(p+4,(uint32_t)(n>>32)); }
static uint8_t *dma_at(model *c,uint64_t address,unsigned size)
{
    for (unsigned i=0;i<c->allocated;++i) {
        if (address>=c->addresses[i] && address-c->addresses[i]<=4096 &&
            size<=4096-(address-c->addresses[i])) {
            CHECK(c->live[i]); return c->device[i]+(size_t)(address-c->addresses[i]);
        }
    }
    CHECK(!"device requested unmapped DMA");return NULL;
}
static void execute(model *c,unsigned qid,unsigned tail)
{
    uint64_t sq=qid?c->isq:c->asq, cq=qid?c->icq:c->acq;
    uint8_t *cmd=dma_at(c,sq+c->sq_head[qid]*64,64);
    unsigned op=cmd[0], cid=u16(cmd+2), nsid=u32(cmd+4), status=0;
    uint64_t prp=u64(cmd+24);
    CHECK(c->enabled && tail<c->depth);
    CHECK(cmd[1]==0 && u64(cmd+32)==0);
    ++c->commands;
    if (c->drop) return;
    if (c->error_once && op==c->error_opcode) { status=2;c->error_once=0; }
    else if (!qid) {
        if (op==6) {
            uint8_t *data=dma_at(c,prp,4096);
            memset(data,0,4096);
            if (u32(cmd+40)==1) {
                CHECK(nsid==0); data[512]=c->identify_bad?0x77:0x66; data[513]=0x44;
                p32(data+516,1);
            } else {
                CHECK(u32(cmd+40)==0 && nsid==c->nsid);
                p64(data,c->blocks);p64(data+8,c->blocks);
                data[128]=c->metadata?8:0;data[130]=(uint8_t)c->block_shift;
                data[29]=(uint8_t)c->protection;
            }
        } else if (op==5) {
            CHECK(nsid==0 && (u32(cmd+40)&65535)==1 && u32(cmd+44)==1);
            CHECK((u32(cmd+40)>>16)==c->depth-1); c->icq=prp;
        } else if (op==1) {
            CHECK(c->icq && nsid==0 && (u32(cmd+40)&65535)==1 && u32(cmd+44)==0x10001);
            CHECK((u32(cmd+40)>>16)==c->depth-1); c->isq=prp;
        } else CHECK(!"unexpected admin command");
    } else {
        CHECK(nsid==c->nsid);
        if (op==1 || op==2) {
            uint64_t lba=u64(cmd+40);
            unsigned count=(u32(cmd+48)&65535)+1, size=count*(1u<<c->block_shift);
            uint8_t *data=dma_at(c,prp,size);
            CHECK(lba<c->blocks && count<=c->blocks-lba && size<=4096);
            c->last_lba=lba;
            if (op==1) {
                memcpy(c->stored,data,size);c->stored_lba=lba;c->stored_size=size;++c->writes;
            } else {
                ++c->reads;
                if (c->stored_size==size && c->stored_lba==lba) memcpy(data,c->stored,size);
                else for (unsigned i=0;i<size;++i) data[i]=(uint8_t)(i^(lba&255));
            }
        } else { CHECK(op==0 && prp==0);++c->flushes; }
    }
    c->sq_head[qid]=tail;
    {
        uint8_t *entry=dma_at(c,cq+c->cq_tail[qid]*16,16);
        memset(entry,0,16);
        p16(entry+8,c->corrupt_head?(tail+1)%c->depth:tail);
        p16(entry+10,c->corrupt_qid?qid+1:qid);
        p16(entry+12,c->corrupt_cid?cid+1:cid);
        p16(entry+14,(status<<1)|c->cq_phase[qid]);
        if (++c->cq_tail[qid]==c->depth) { c->cq_tail[qid]=0;c->cq_phase[qid]^=1; }
    }
}
static uint64_t read_reg(void *context,uint32_t region,uint32_t offset,uint32_t width)
{
    model *c=context;CHECK(region==7);
    if (offset==0) { CHECK(width==8);return c->cap; }
    CHECK(width==4);
    if (offset==0x14) return c->enabled;
    if (offset==0x1c) return c->enabled;
    CHECK(!"unexpected register read");return 0;
}
static void write_reg(void *context,uint32_t region,uint32_t offset,uint32_t width,uint64_t value)
{
    model *c=context;CHECK(region==7);
    if (offset==0x28 || offset==0x30) {
        CHECK(width==8 && !c->enabled && !(value&4095));
        if (offset==0x28)c->asq=value;else c->acq=value;return;
    }
    CHECK(width==4);
    if (offset==0x14) {
        if (value&1) { CHECK(value==0x460001 && c->asq && c->acq); if (!c->stuck_enable)c->enabled=1; }
        else if (!c->stuck_reset) {
            c->enabled=0;c->isq=c->icq=0;
            for (unsigned i=0;i<2;++i) { c->sq_head[i]=c->cq_tail[i]=0;c->cq_phase[i]=1; }
        }
    } else if (offset==0x0c) CHECK(value==UINT32_MAX);
    else if (offset==0x24) { CHECK(!c->enabled);c->aqa=(unsigned)value;c->depth=((unsigned)value&4095)+1;CHECK(value==((c->depth-1)<<16|(c->depth-1))); }
    else if (offset>=0x1000 && (offset-0x1000)%c->doorbell_stride==0) {
        unsigned index=(offset-0x1000)/c->doorbell_stride;CHECK(index<4);
        if (!(index&1))execute(c,index/2,(unsigned)value);
        else CHECK(value<c->depth);
    } else CHECK(!"unexpected register write");
}
static int32_t alloc_dma(void *context,uint32_t size,uint32_t align,drift_dma *out)
{
    model *c=context;unsigned i=c->allocated;
    CHECK(!c->enabled && size==4096 && align==4096);
    if (c->fail_alloc==i+1)return -1;
    CHECK(i<5);c->addresses[i]=UINT64_C(0x100000000)+i*4096;
    memset(c->cpu[i],0xa5,4096);memset(c->device[i],0x5a,4096);
    *out=(drift_dma){c->cpu[i],c->addresses[i],4096,i};
    if (c->bad_alloc==i+1)out->address++;
    ++c->allocated;c->live[i]=1;return 0;
}
static void free_dma(void *context,drift_dma *b)
{
    model *c=context;CHECK(!c->enabled && b->token<c->allocated && c->live[b->token]);
    c->live[b->token]=0;++c->released;
}
static void sync_dma(void *context,drift_dma *b,uint32_t off,uint32_t size,uint32_t direction)
{
    model *c=context;CHECK(b->token<c->allocated && c->live[b->token] && off<=4096 && size<=4096-off);
    CHECK(direction>=1 && direction<=3);
    if (direction==1 || direction==3) { memcpy(c->device[b->token]+off,b->data+off,size);++c->sync_to; }
    else { memcpy(b->data+off,c->device[b->token]+off,size);++c->sync_from; }
}
static uint64_t now(void *context) { return ((model *)context)->now; }
static void wait_us(void *context,uint32_t us) { ((model *)context)->now+=us; }
static drift_hw setup(void)
{
    memset(&m,0,sizeof(m));m.cap=UINT64_C(1)<<37|7;m.blocks=UINT64_C(1)<<40;
    m.block_shift=9;m.nsid=1;m.doorbell_stride=4;
    return (drift_hw){&m,read_reg,write_reg,alloc_dma,free_dma,sync_dma,now,wait_us};
}
static void init(drift_nvme *d,drift_hw *h)
{
    CHECK(drift_nvme_init(d,h,7,1,100)==0);CHECK(d->state==1 && d->blocks==m.blocks && d->block_size==512);
    CHECK(m.allocated==5 && !m.released && m.commands==4 && m.sync_from);
}
int main(void)
{
    drift_nvme d={0};drift_hw h=setup();uint8_t input[4096],output[4096];
    CHECK(drift_nvme_init(NULL,&h,7,1,100)==-1);
    CHECK(drift_nvme_init(&d,NULL,7,1,100)==-1);
    CHECK(drift_nvme_init(&d,&h,7,0,100)==-1);
    CHECK(drift_nvme_init(&d,&h,7,1,0)==-1);
    m.enabled=1;init(&d,&h);
    CHECK(drift_nvme_init(&d,&h,7,1,100)==-7);
    for (unsigned n=0;n<40;++n) {
        uint64_t lba=(UINT64_C(1)<<33)+n;
        for (unsigned i=0;i<4096;++i)input[i]=(uint8_t)(i+n*3);
        CHECK(drift_nvme_write(&d,lba,8,input)==0);
        memset(output,0,sizeof(output));CHECK(drift_nvme_read(&d,lba,8,output)==0);
        CHECK(!memcmp(input,output,4096) && m.last_lba==lba);
    }
    CHECK(drift_nvme_flush(&d)==0 && m.flushes==1 && m.reads==40 && m.writes==40);
    CHECK(drift_nvme_read(&d,d.blocks,1,output)==-1);
    CHECK(drift_nvme_read(&d,d.blocks-1,2,output)==-1);
    CHECK(drift_nvme_read(&d,0,9,output)==-1);
    CHECK(drift_nvme_read(&d,0,0,output)==-1);
    CHECK(drift_nvme_write(&d,0,1,NULL)==-1);
    CHECK(drift_nvme_close(&d)==0 && m.released==5 && !m.enabled);
    CHECK(drift_nvme_close(&d)==0 && m.released==5);
    CHECK(drift_nvme_read(&d,0,1,output)==-7);
    for (unsigned failure=1;failure<=5;++failure) {
        h=setup();memset(&d,0,sizeof(d));m.fail_alloc=failure;
        CHECK(drift_nvme_init(&d,&h,7,1,100)==-3);
        CHECK(m.released==failure-1 && !m.enabled);
        CHECK(drift_nvme_close(&d)==0);
    }
    h=setup();memset(&d,0,sizeof(d));m.bad_alloc=3;
    CHECK(drift_nvme_init(&d,&h,7,1,100)==-3 && m.released==3);
    for (unsigned kind=0;kind<3;++kind) {
        h=setup();memset(&d,0,sizeof(d));
        if (kind==0)m.cap&=~(UINT64_C(1)<<37);
        if (kind==1)m.cap|=UINT64_C(1)<<48;
        if (kind==2)m.cap&=~UINT64_C(65535);
        CHECK(drift_nvme_init(&d,&h,7,1,100)==-2 && !m.allocated);
    }
    for (unsigned kind=0;kind<4;++kind) {
        h=setup();memset(&d,0,sizeof(d));
        if (kind==0)m.identify_bad=1;
        if (kind==1)m.metadata=1;
        if (kind==2)m.protection=1;
        if (kind==3)m.block_shift=13;
        CHECK(drift_nvme_init(&d,&h,7,1,100)==-2 && m.released==5 && !m.enabled);
    }
    for (unsigned kind=0;kind<3;++kind) {
        h=setup();memset(&d,0,sizeof(d));init(&d,&h);
        m.corrupt_cid=kind==0;m.corrupt_qid=kind==1;m.corrupt_head=kind==2;
        CHECK(drift_nvme_read(&d,0,1,output)==-6 && d.state==2 && m.released==5);
        CHECK(drift_nvme_flush(&d)==-7);
    }
    h=setup();memset(&d,0,sizeof(d));init(&d,&h);
    m.error_once=1;m.error_opcode=2;
    CHECK(drift_nvme_read(&d,0,1,output)==-5 && d.last_status==2 && d.state==1);
    CHECK(drift_nvme_read(&d,0,1,output)==0);
    CHECK(drift_nvme_close(&d)==0);
    h=setup();memset(&d,0,sizeof(d));init(&d,&h);m.drop=1;
    CHECK(drift_nvme_read(&d,0,1,output)==-4 && d.state==2 && m.released==5);
    h=setup();memset(&d,0,sizeof(d));init(&d,&h);m.drop=1;m.stuck_reset=1;
    CHECK(drift_nvme_read(&d,0,1,output)==-4 && d.state==3 && !m.released);
    CHECK(drift_nvme_flush(&d)==-7);
    CHECK(drift_nvme_close(&d)==-4 && !m.released);
    m.stuck_reset=0;CHECK(drift_nvme_close(&d)==0 && m.released==5);
    h=setup();memset(&d,0,sizeof(d));m.stuck_enable=1;
    CHECK(drift_nvme_init(&d,&h,7,1,100)==-4 && m.released==5);
    h=setup();memset(&d,0,sizeof(d));m.cap|=UINT64_C(2)<<32;m.doorbell_stride=16;
    CHECK(drift_nvme_init(&d,&h,7,1,100)==0);
    CHECK(drift_nvme_flush(&d)==0);CHECK(drift_nvme_close(&d)==0);
    h=setup();memset(&d,0,sizeof(d));m.block_shift=12;
    CHECK(drift_nvme_init(&d,&h,7,1,100)==0 && d.block_size==4096);
    CHECK(drift_nvme_read(&d,1,1,output)==0);
    CHECK(drift_nvme_read(&d,1,2,output)==-1);
    CHECK(drift_nvme_close(&d)==0);
    printf("nvme: %u independent register/DMA checks passed\n",checks);return 0;
}
