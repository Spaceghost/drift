/* Independent AHCI register/DMA model; no generated-driver internals.
 * SPDX-License-Identifier: MIT */
#include "drift_ahci.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEM_SIZE 69632u
#define PORT 0x100u
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"AHCI check failed: %s:%d: %s\n",__FILE__,__LINE__,#x); abort(); } } while (0)
static unsigned checks, scenarios;
typedef struct model {
    uint32_t registers[0x1100/4];
    uint8_t device[MEM_SIZE], saved[65536];
    uint8_t *cpu;
    uint64_t address, now, sectors, saved_lba;
    uint32_t sector_size, saved_count, commands, allocated, released, sync_to, sync_from;
    int alloc_fail, bad_alignment, stop_stuck, busy, timeout, task_error, host_error, short_dma;
    int bad_lba48, bad_sector_size, bad_capacity;
    uint32_t word83;
} model;

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static uint64_t le64(const uint8_t *p) { return le32(p)|(uint64_t)le32(p+4)<<32; }
static void put16(uint8_t *p,uint16_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
static void put32(uint8_t *p,uint32_t v) { put16(p,(uint16_t)v); put16(p+2,(uint16_t)(v>>16)); }
static uint8_t pattern(uint64_t position) { return (uint8_t)(position^(position>>17)); }
static uint32_t *reg(model *m,unsigned offset) { CHECK(offset%4==0 && offset<sizeof(m->registers)); return &m->registers[offset/4]; }
static uint8_t *dma_address(model *m,uint64_t address,uint32_t size) {
    CHECK(m->allocated && address>=m->address && address-m->address<=MEM_SIZE);
    CHECK(size<=MEM_SIZE-(address-m->address));
    return m->device+(address-m->address);
}
static void execute(model *m) {
    uint64_t clb=*reg(m,PORT)|(uint64_t)*reg(m,PORT+4)<<32;
    uint8_t *header=dma_address(m,clb,32), *table, *data=NULL;
    uint32_t flags=le32(header), bytes=0, count;
    uint64_t lba;
    CHECK((flags&31)==5 && !(flags&0xfe00));
    table=dma_address(m,le64(header+8),144);
    CHECK(table[0]==0x27 && table[1]==0x80 && table[7]==0x40);
    for (unsigned i=14;i<64;i++) CHECK(table[i]==0);
    if (flags>>16) {
        CHECK((flags>>16)==1);
        CHECK((le32(table+140)&0x7fc00000u)==0);
        bytes=(le32(table+140)&0x3fffff)+1;
        data=dma_address(m,le64(table+128),bytes);
    }
    lba=table[4]|(uint64_t)table[5]<<8|(uint64_t)table[6]<<16|
        (uint64_t)table[8]<<24|(uint64_t)table[9]<<32|(uint64_t)table[10]<<40;
    count=table[12]|(uint32_t)table[13]<<8;
    ++m->commands;
    if (m->timeout) return;
    if (m->host_error) { *reg(m,PORT+0x10)=1u<<30; return; }
    if (table[2]==0xec) {
        CHECK(bytes==512 && !(flags&0x40));
        memset(data,0,bytes);
        put16(data+166,m->bad_lba48 ? 0x4000 : m->word83);
        for (unsigned i=0;i<4;i++) put16(data+200+i*2,(uint16_t)((m->bad_capacity ? 0 : m->sectors)>>(i*16)));
        if (m->sector_size!=512 || m->bad_sector_size) {
            put16(data+212,0x5000);
            put32(data+234,m->bad_sector_size ? 1024 : m->sector_size/2);
        }
    } else if (table[2]==0x25 || table[2]==0x35) {
        CHECK(count && bytes==count*m->sector_size && lba<m->sectors && count<=m->sectors-lba);
        CHECK(((flags&0x40)!=0)==(table[2]==0x35));
        if (table[2]==0x35) {
            memcpy(m->saved,data,bytes); m->saved_lba=lba; m->saved_count=count;
        } else if (m->saved_count==count && m->saved_lba==lba) memcpy(data,m->saved,bytes);
        else for (unsigned i=0;i<bytes;i++) data[i]=pattern(lba*m->sector_size+i);
    } else { CHECK((table[2]==0xea || table[2]==0xe7) && !bytes && !(flags&0x40)); }
    put32(header+4,m->short_dma && bytes ? bytes-1 : bytes);
    *reg(m,PORT+0x38)=0;
    *reg(m,PORT+0x20)=m->task_error ? 1 : 0;
    *reg(m,PORT+0x10)=1;
}
static uint64_t read_register(void *context,uint32_t region,uint32_t offset,uint32_t width) {
    model *m=context; CHECK(region==7 && width==4);
    if (m->busy && offset==PORT+0x20) return 0x80;
    return *reg(m,offset);
}
static void write_register(void *context,uint32_t region,uint32_t offset,uint32_t width,uint64_t value) {
    model *m=context; uint32_t *r=reg(m,offset); CHECK(region==7 && width==4 && value<=UINT32_MAX);
    if (offset==PORT+0x10 || offset==PORT+0x30) *r&=~(uint32_t)value;
    else if (offset==PORT+0x18) {
        *r=(uint32_t)value&~0xc000u;
        if (value&1) *r|=0x8000;
        if (value&0x10) *r|=0x4000;
        if (m->stop_stuck) *r|=0xc000;
    } else if (offset==PORT+0x38) {
        CHECK(value==1 && (*reg(m,PORT+0x18)&0x11)==0x11);
        *r|=(uint32_t)value; execute(m);
    } else *r=(uint32_t)value;
}
static int32_t allocate(void *context,uint32_t size,uint32_t alignment,drift_dma *out) {
    model *m=context; CHECK(size==MEM_SIZE && alignment==4096 && !m->allocated);
    if (m->alloc_fail) return -1;
    m->cpu=calloc(1,MEM_SIZE); CHECK(m->cpu); m->allocated=1;
    out->data=m->cpu; out->size=size; out->address=m->address+(m->bad_alignment ? 1 : 0); out->token=77;
    return 0;
}
static void release(void *context,drift_dma *buffer) {
    model *m=context;
    CHECK(m->allocated && buffer->data==m->cpu && buffer->token==77);
    CHECK((*reg(m,PORT+0x18)&0xc011)==0); /* A lease cannot outlive stopped DMA. */
    free(m->cpu); m->cpu=NULL; m->allocated=0; ++m->released;
}
static void sync_dma(void *context,drift_dma *buffer,uint32_t offset,uint32_t size,uint32_t direction) {
    model *m=context; CHECK(buffer->data==m->cpu && offset<=MEM_SIZE && size<=MEM_SIZE-offset);
    CHECK(direction>=1 && direction<=3);
    if (direction&1) { memcpy(m->device+offset,m->cpu+offset,size); ++m->sync_to; }
    else { memcpy(m->cpu+offset,m->device+offset,size); ++m->sync_from; }
}
static uint64_t time_us(void *context) { return ((model*)context)->now; }
static void wait_us(void *context,uint32_t duration) { ((model*)context)->now+=duration; }
static drift_hw prepare(model *m) {
    memset(m,0,sizeof(*m));
    m->address=UINT64_C(0x123400000000); m->sector_size=512; m->word83=0x7400; m->sectors=UINT64_C(1)<<40;
    *reg(m,0)=0x80000000; *reg(m,0x0c)=1;
    *reg(m,PORT+0x28)=0x103; *reg(m,PORT+0x24)=0x101;
    return (drift_hw){m,read_register,write_register,allocate,release,sync_dma,time_us,wait_us};
}
static void success(unsigned sector_size) {
    model m; drift_hw hw=prepare(&m); drift_ahci disk={0}; uint8_t buffer[65536], copy[65536];
    uint64_t lba=(UINT64_C(1)<<32)+7;
    m.sector_size=sector_size;
    CHECK(drift_ahci_init(&disk,&hw,7,0,1000)==0);
    CHECK(disk.sectors==m.sectors && disk.sector_size==sector_size && m.commands==1);
    CHECK(drift_ahci_init(&disk,&hw,7,0,1000)==DRIFT_AHCI_ESTATE);
    CHECK(drift_ahci_read(&disk,lba,65536/sector_size,buffer)==0);
    for (unsigned i=0;i<sizeof(buffer);i++) CHECK(buffer[i]==pattern(lba*sector_size+i));
    for (unsigned i=0;i<sizeof(buffer);i++) copy[i]=buffer[i]=(uint8_t)(i^0xa5);
    CHECK(drift_ahci_write(&disk,lba,65536/sector_size,buffer)==0);
    CHECK(!memcmp(buffer,copy,sizeof(buffer)));
    memset(buffer,0,sizeof(buffer));
    CHECK(drift_ahci_read(&disk,lba,65536/sector_size,buffer)==0);
    CHECK(!memcmp(buffer,copy,sizeof(buffer)));
    CHECK(drift_ahci_read(&disk,m.sectors-1,1,buffer)==0);
    CHECK(drift_ahci_read(&disk,m.sectors,1,buffer)==DRIFT_AHCI_EARG);
    CHECK(drift_ahci_read(&disk,m.sectors-1,2,buffer)==DRIFT_AHCI_EARG);
    CHECK(drift_ahci_read(&disk,0,65536/sector_size+1,buffer)==DRIFT_AHCI_EARG);
    CHECK(drift_ahci_read(&disk,0,0,buffer)==DRIFT_AHCI_EARG);
    CHECK(drift_ahci_read(&disk,0,1,NULL)==DRIFT_AHCI_EARG);
    CHECK(drift_ahci_flush(&disk)==0);
    CHECK(m.sync_to && m.sync_from);
    CHECK(drift_ahci_close(&disk)==0 && m.released==1 && disk.state==0);
    CHECK(drift_ahci_close(&disk)==0 && m.released==1);
    CHECK(drift_ahci_read(&disk,0,1,buffer)==DRIFT_AHCI_ESTATE);
    ++scenarios;
}
int main(void) {
    model m; drift_hw hw; drift_ahci d; uint8_t bytes[512];
    CHECK(drift_ahci_init(NULL,NULL,0,0,0)==DRIFT_AHCI_EARG);
    CHECK(drift_ahci_close(NULL)==DRIFT_AHCI_EARG);
    CHECK(drift_ahci_flush(NULL)==DRIFT_AHCI_EARG);
    success(512); success(4096);
    for (unsigned mode=0;mode<11;mode++) {
        hw=prepare(&m); memset(&d,0,sizeof(d));
        int expected;
        switch(mode) {
        case 0: *reg(&m,0x0c)=0; expected=DRIFT_AHCI_ENODEV; break;
        case 1: *reg(&m,PORT+0x28)=0; expected=DRIFT_AHCI_ENODEV; break;
        case 2: *reg(&m,PORT+0x24)=0xeb140101; expected=DRIFT_AHCI_ENOTSUP; break;
        case 3: m.alloc_fail=1; expected=DRIFT_AHCI_EDMA; break;
        case 4: m.bad_alignment=1; expected=DRIFT_AHCI_EDMA; break;
        case 5: *reg(&m,0)=0; expected=DRIFT_AHCI_EDMA; break;
        case 6: m.bad_lba48=1; expected=DRIFT_AHCI_ENOTSUP; break;
        case 7: m.bad_capacity=1; expected=DRIFT_AHCI_ENOTSUP; break;
        case 8: m.bad_sector_size=1; expected=DRIFT_AHCI_ENOTSUP; break;
        case 9: *reg(&m,0x24)=1; *reg(&m,0x28)=1; expected=DRIFT_AHCI_ETIMEOUT; break;
        default: m.stop_stuck=1; expected=DRIFT_AHCI_ETIMEOUT; break;
        }
        CHECK(drift_ahci_init(&d,&hw,7,0,100)==expected);
        m.stop_stuck=0; CHECK(drift_ahci_close(&d)==0); CHECK(!m.allocated); ++scenarios;
    }
    /* A 32-bit HBA accepts a low DMA lease, but never silently truncates it. */
    hw=prepare(&m); memset(&d,0,sizeof(d)); *reg(&m,0)=0; m.address=0x200000;
    CHECK(drift_ahci_init(&d,&hw,7,0,100)==0); CHECK(drift_ahci_close(&d)==0); ++scenarios;
    for (unsigned mode=0;mode<2;mode++) {
        hw=prepare(&m); memset(&d,0,sizeof(d)); m.word83=mode ? 0x4400 : 0x5400;
        CHECK(drift_ahci_init(&d,&hw,7,0,100)==0);
        CHECK(drift_ahci_flush(&d)==(mode ? DRIFT_AHCI_ENOTSUP : 0));
        CHECK(d.state==1); CHECK(drift_ahci_close(&d)==0); ++scenarios;
    }
    for (unsigned mode=0;mode<6;mode++) {
        hw=prepare(&m); memset(&d,0,sizeof(d)); CHECK(drift_ahci_init(&d,&hw,7,0,100)==0);
        int expected=DRIFT_AHCI_EIO;
        switch(mode) {
        case 0: m.short_dma=1; break;
        case 1: m.task_error=1; break;
        case 2: m.host_error=1; break;
        case 3: m.timeout=1; expected=DRIFT_AHCI_ETIMEOUT; break;
        case 4: m.busy=1; expected=DRIFT_AHCI_ETIMEOUT; break;
        default: m.timeout=1; m.stop_stuck=1; expected=DRIFT_AHCI_ETIMEOUT; break;
        }
        memset(bytes,0x5a,sizeof(bytes));
        CHECK(drift_ahci_read(&d,1,1,bytes)==expected);
        for (unsigned i=0;i<sizeof(bytes);i++) CHECK(bytes[i]==0x5a);
        CHECK(drift_ahci_read(&d,1,1,bytes)==DRIFT_AHCI_ESTATE);
        CHECK(m.allocated && !m.released);
        if (m.stop_stuck) {
            CHECK(drift_ahci_close(&d)==DRIFT_AHCI_ETIMEOUT);
            CHECK(m.allocated && !m.released); m.stop_stuck=0;
        }
        CHECK(drift_ahci_close(&d)==0 && !m.allocated && m.released==1); ++scenarios;
    }
    printf("ahci: %u scenarios, %u checks passed\n",scenarios,checks);
    return 0;
}
