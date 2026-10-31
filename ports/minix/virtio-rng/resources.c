/* Generic resources only: no virtqueue, entropy, or device protocol. */
/* SPDX-License-Identifier: MIT */
#include <minix/drivers.h>
#include <minix/vm.h>
#include <minix/endpoint.h>
#include <sys/mman.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "resources.h"

static void order(void) { __sync_synchronize(); }
static int valid_window(minix_resources *h,unsigned r,unsigned o,unsigned w) {
    if(r>=6 || !h->window[r] || (w!=1&&w!=2&&w!=4&&w!=8) ||
       (o&(w-1)) || o>h->window_size[r] || w>h->window_size[r]-o) {
        h->fault=1;return 0;
    }
    return 1;
}
static uint64_t read_register(void *p,uint32_t r,uint32_t o,uint32_t w) {
    minix_resources *h=p;uint64_t v;
    if(!valid_window(h,r,o,w))return UINT64_MAX;
    volatile unsigned char *a=h->window[r]+o;
    order();
    if(w==1)v=*(volatile uint8_t*)a;
    else if(w==2)v=*(volatile uint16_t*)a;
    else if(w==4)v=*(volatile uint32_t*)a;
    /* i386: low then high, not a claim of an atomic 64-bit register read.
     * The tested modern Virtio transport uses only widths 1/2/4. */
    else {
        uint32_t low=*(volatile uint32_t*)a;
        uint32_t high=*(volatile uint32_t*)(a+4);
        v=(uint64_t)low|((uint64_t)high<<32);
    }
    order();return v;
}
static void write_register(void *p,uint32_t r,uint32_t o,uint32_t w,uint64_t v) {
    minix_resources *h=p;if(!valid_window(h,r,o,w))return;
    volatile unsigned char *a=h->window[r]+o;
    order();
    if(w==1)*(volatile uint8_t*)a=(uint8_t)v;
    else if(w==2)*(volatile uint16_t*)a=(uint16_t)v;
    else if(w==4)*(volatile uint32_t*)a=(uint32_t)v;
    else {*(volatile uint32_t*)a=(uint32_t)v;*(volatile uint32_t*)(a+4)=(uint32_t)(v>>32);}
    order();
}
static uint32_t read_config(void *p,uint32_t bdf,uint32_t off) {
    minix_resources *h=p;
    if(bdf!=h->bdf || off>252 || (off&3)){h->fault=1;return UINT32_MAX;}
    return pci_attr_r32(h->devind,(int)off);
}
static void write_config(void *p,uint32_t bdf,uint32_t off,uint32_t value) {
    minix_resources *h=p;
    /* The consumer only enables command bits. BAR remapping is not delegated.
     * A 16-bit command write preserves the W1C status halfword. */
    if(bdf!=h->bdf || off!=4 || (value&~0xffffu) ||
       ((value^pci_attr_r16(h->devind,4))&~7u)){h->fault=1;return;}
    pci_attr_w16(h->devind,4,(u16_t)value);
}
static struct minix_dma_slot *find_dma(minix_resources *h,drift_dma *b) {
    if(!b || !b->data || !b->token)return NULL;
    for(unsigned i=0;i<DRIFT_MINIX_DMA_SLOTS;i++) {
        drift_dma *a=&h->dma_slots[i].allocation;
        if(a->token==b->token && a->data==b->data && a->address==b->address && a->size==b->size)return &h->dma_slots[i];
    }
    return NULL;
}
static int32_t allocate_dma(void *p,uint32_t n,uint32_t a,drift_dma *out) {
    minix_resources *h=p;struct minix_dma_slot *s=NULL;phys_bytes physical;
    if(!out||!n||n>4096||!a||a>4096||(a&(a-1)))return -22;
    for(unsigned i=0;i<DRIFT_MINIX_DMA_SLOTS;i++)if(!h->dma_slots[i].allocation.data){s=&h->dma_slots[i];break;}
    if(!s)return -12;
    void *cpu=alloc_contig(4096,AC_ALIGN4K,&physical);
    if(!cpu)return -12;
    if(((uintptr_t)cpu&(a-1)) || (physical&(a-1)) || physical>UINT32_MAX-4095) {free_contig(cpu,4096);return -5;}
    memset(cpu,0,4096);order();
    s->actual_size=4096;s->allocation=(drift_dma){cpu,(uint64_t)physical,n,++h->next_token};
    *out=s->allocation;++h->allocations;return 0;
}
static void release_dma(void *p,drift_dma *b) {
    minix_resources *h=p;struct minix_dma_slot *s=find_dma(h,b);
    if(!s){h->fault=1;return;}
    order();if(free_contig(s->allocation.data,s->actual_size)){h->fault=1;return;}
    memset(s,0,sizeof(*s));memset(b,0,sizeof(*b));++h->releases;
}
static void sync_dma(void *p,drift_dma *b,uint32_t o,uint32_t n,uint32_t direction) {
    minix_resources *h=p;
    if(!find_dma(h,b)||o>b->size||n>b->size-o||direction<1||direction>3){h->fault=1;return;}
    /* QEMU x86 coherent RAM; no streaming bounce shadow. A real locked CPU
     * barrier supplies acquire/release ordering even on coherent machines. */
    order();++h->syncs;
}
static uint64_t monotonic_us(void *p) {
    minix_resources *h=p;struct timespec t;
    if(clock_gettime(CLOCK_MONOTONIC,&t)){h->fault=1;return 0;}
    return (uint64_t)t.tv_sec*1000000u+t.tv_nsec/1000u;
}
static void wait_us(void *p,uint32_t n) {
    minix_resources *h=p;
    /* Actual delay/yield, preserving the coarse 60Hz clock rather than
     * inventing a high-resolution software clock. */
    if(micro_delay(n?n:1))h->fault=1;
}
int minix_resources_open(minix_resources *h) {
    int d,more;u16_t vendor,device;unsigned bus,slot,function;char *label;
    memset(h,0,sizeof(*h));h->devind=UINT32_MAX;
    for(unsigned i=0;i<6;i++)h->bars[i].region=UINT32_MAX;
    pci_init();
    for(more=pci_first_dev(&d,&vendor,&device);more;more=pci_next_dev(&d,&vendor,&device)) {
        if(vendor==0x1af4&&device==0x1044){h->devind=d;break;}
    }
    if(h->devind==UINT32_MAX)return -19;
    label=pci_slot_name(h->devind);
    if(!label)return -5;
    char *end;bus=strtoul(label,&end,10);if(*end!='.')return -5;
    slot=strtoul(end+1,&end,10);if(*end!='.')return -5;
    function=strtoul(end+1,&end,10);if(*end||bus>255||slot>31||function>7)return -5;
    h->bdf=(bus<<8)|(slot<<3)|function;
    int r=pci_reserve_ok(h->devind);if(r)return r;
    for(unsigned i=0;i<6;i++) {
        u32_t base,size;int io;uint32_t raw=pci_attr_r32(h->devind,16+4*i);
        if(!raw||raw==UINT32_MAX)continue;
        if(pci_get_bar(h->devind,16+4*i,&base,&size,&io))return -5;
        if(io||!size||(base&4095)||(size&4095)||size>16u*1024*1024||base>UINT32_MAX-size)return -5;
        if((raw&6)==4 && (i==5||pci_attr_r32(h->devind,20+4*i)))return -5;
        void *v=vm_map_phys(SELF,(void*)(uintptr_t)base,size);
        if(v==MAP_FAILED)return -5;
        h->window[i]=v;h->window_size[i]=size;h->bars[i]=(drift_virtio_bar){i,size};++h->maps;
        if((raw&6)==4)i++;
    }
    if(!h->maps)return -5;
    h->hw=(drift_hw){h,read_register,write_register,allocate_dma,release_dma,sync_dma,monotonic_us,wait_us};
    h->pci=(drift_pci){h,read_config,write_config};
    return drift_pci_enable(&h->pci,h->bdf,6);
}
int minix_resources_close(minix_resources *h) {
    for(unsigned i=0;i<DRIFT_MINIX_DMA_SLOTS;i++)if(h->dma_slots[i].allocation.data)return -16;
    for(unsigned i=0;i<6;i++)if(h->window[i]) {
        if(vm_unmap_phys(SELF,(void*)h->window[i],h->window_size[i]))return -5;
        h->window[i]=NULL;h->window_size[i]=0;
    }
    return h->fault?-5:0;
}
