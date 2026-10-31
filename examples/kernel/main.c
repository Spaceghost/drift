/* A tiny x86 OS adapter. Device commands remain in the native Nelua drivers.
 * SPDX-License-Identifier: MIT */
#include "drift_ahci.h"
#include "drift_nvme.h"
#include "drift_pci.h"
#include "drift_virtio.h"
#include "drift_ethernet.h"
#include "drift_usb.h"
#include "drift_io.h"
#include <stdint.h>

static inline void out8(uint16_t p,uint8_t v) { __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t in8(uint16_t p) { uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v; }
static inline void out32(uint16_t p,uint32_t v) { __asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p)); }
static inline uint32_t in32(uint16_t p) { uint32_t v; __asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p)); return v; }
static inline uint64_t tsc(void) { uint32_t a,d; __asm__ volatile("rdtsc":"=a"(a),"=d"(d)); return a|(uint64_t)d<<32; }

/* Freestanding compiler helpers: shift/subtract division needs no runtime. */
uint64_t __udivdi3(uint64_t numerator,uint64_t denominator) {
    uint64_t quotient=0,remainder=0;
    if (!denominator) return UINT64_MAX;
    for (unsigned i=64;i--;) {
        uint64_t carry=remainder>>63;
        remainder=(remainder<<1)|((numerator>>i)&1);
        if (carry || remainder>=denominator) { remainder-=denominator; quotient|=UINT64_C(1)<<i; }
    }
    return quotient;
}
uint64_t __umoddi3(uint64_t n,uint64_t d) { return n-__udivdi3(n,d)*d; }

static void serial_init(void) {
    out8(0x3f9,0); out8(0x3fb,0x80); out8(0x3f8,1); out8(0x3f9,0);
    out8(0x3fb,3); out8(0x3fa,0xc7); out8(0x3fc,0x0b);
}
static void text(const char *s) { while (*s) { while (!(in8(0x3fd)&0x20)) {} out8(0x3f8,(uint8_t)*s++); } }
static void hex(uint64_t v) { static const char digits[]="0123456789abcdef"; for (unsigned i=16;i--;) { char c[2]={digits[(v>>(i*4))&15],0}; text(c); } }
static void exit_kernel(int success) {
    text(success ? "DRIFT KERNEL PASS\n" : "DRIFT KERNEL FAIL\n");
    out32(0xf4,success ? 0x10 : 0x11);
    for (;;) __asm__ volatile("cli; hlt");
}
static void require(int ok,const char *message) { if (!ok) { text(message); text("\n"); exit_kernel(0); } }

static uint64_t epoch,ticks_per_us;
static void clock_init(void) {
    /* Calibrate the QEMU x86 TSC against a 10 ms PIT channel-2 one-shot. */
    uint8_t speaker=in8(0x61);
    out8(0x61,(uint8_t)((speaker&~2u)|1u));
    out8(0x43,0xb0); out8(0x42,0x9c); out8(0x42,0x2e); /* 11932 ticks */
    uint64_t first=tsc();
    while (!(in8(0x61)&0x20)) {}
    ticks_per_us=(tsc()-first)/10000;
    require(ticks_per_us!=0,"PIT/TSC calibration failed");
    out8(0x61,speaker); epoch=tsc();
}

static uint32_t config_read(void *ctx,uint32_t bdf,uint32_t offset) {
    (void)ctx; out32(0xcf8,0x80000000u|(bdf<<8)|(offset&0xfc)); return in32(0xcfc);
}
static void config_write(void *ctx,uint32_t bdf,uint32_t offset,uint32_t value) {
    (void)ctx; out32(0xcf8,0x80000000u|(bdf<<8)|(offset&0xfc)); out32(0xcfc,value);
}
static drift_pci pci={0,config_read,config_write};
typedef struct region { uint32_t base, io, size; } region;
static region regions[32];
static uint32_t region_count;
static uint8_t arena[1024*1024] __attribute__((aligned(4096)));
static uint32_t used;
static uint64_t register_read(void *ctx,uint32_t region,uint32_t offset,uint32_t width) {
    (void)ctx;
    require(region<region_count && offset<=regions[region].size && width<=regions[region].size-offset &&
            (width==1 || width==2 || width==4 || width==8),"invalid register region");
    if (regions[region].io) {
        uint16_t port=(uint16_t)(regions[region].base+offset);
        if (width==1) return in8(port);
        if (width==2) { uint16_t v; __asm__ volatile("inw %1,%0":"=a"(v):"Nd"(port)); return v; }
        require(width==4,"unsupported port width"); return in32(port);
    }
    volatile uint8_t *p=(volatile uint8_t *)(uintptr_t)(regions[region].base+offset);
    if (width==1) return *p;
    if (width==2) return *(volatile uint16_t *)p;
    if (width==4) return *(volatile uint32_t *)p;
    return *(volatile uint32_t *)p|(uint64_t)*(volatile uint32_t *)(p+4)<<32;
}
static void register_write(void *ctx,uint32_t region,uint32_t offset,uint32_t width,uint64_t value) {
    (void)ctx;
    require(region<region_count && offset<=regions[region].size && width<=regions[region].size-offset &&
            (width==1 || width==2 || width==4 || width==8),"invalid register region");
    if (regions[region].io) {
        uint16_t port=(uint16_t)(regions[region].base+offset);
        if (width==1) out8(port,(uint8_t)value);
        else if (width==2) { uint16_t v=(uint16_t)value; __asm__ volatile("outw %0,%1"::"a"(v),"Nd"(port)); }
        else { require(width==4,"unsupported port width"); out32(port,(uint32_t)value); }
        return;
    }
    volatile uint8_t *p=(volatile uint8_t *)(uintptr_t)(regions[region].base+offset);
    if (width==1) *p=(uint8_t)value;
    else if (width==2) *(volatile uint16_t *)p=(uint16_t)value;
    else if (width==4) *(volatile uint32_t *)p=(uint32_t)value;
    else { *(volatile uint32_t *)p=(uint32_t)value; *(volatile uint32_t *)(p+4)=(uint32_t)(value>>32); }
}
static int32_t dma_allocate(void *ctx,uint32_t size,uint32_t alignment,drift_dma *out) {
    (void)ctx;
    if (!alignment || (alignment&(alignment-1))) return -1;
    uint32_t at=(((uint32_t)(uintptr_t)arena+used+alignment-1)&~(alignment-1))-(uint32_t)(uintptr_t)arena;
    if (at>sizeof(arena) || size>sizeof(arena)-at) return -1;
    out->data=arena+at; out->address=(uintptr_t)out->data; out->size=size; out->token=at;
    for (uint32_t i=0;i<size;i++) out->data[i]=0;
    used=at+size; return 0;
}
static void dma_release(void *ctx,drift_dma *buffer) { (void)ctx; (void)buffer; /* Bump arena reclaimed after close. */ }
static void dma_sync(void *ctx,drift_dma *buffer,uint32_t offset,uint32_t size,uint32_t direction) {
    (void)ctx; (void)buffer; (void)offset; (void)size; (void)direction;
    /* x86 DMA is coherent. A locked instruction supplies a full memory barrier. */
    __asm__ volatile("lock; addl $0,0(%%esp)":::"memory","cc");
}
static uint64_t clock_us(void *ctx) { (void)ctx; return (tsc()-epoch)/ticks_per_us; }
static void wait_us(void *ctx,uint32_t duration) {
    uint64_t start=clock_us(ctx);
    while (clock_us(ctx)-start<duration) __asm__ volatile("pause");
}
static drift_hw resources={0,register_read,register_write,dma_allocate,dma_release,dma_sync,clock_us,wait_us};
static uint32_t bind_bar(uint32_t bdf,uint32_t index) {
    drift_pci_bar bar;
    require(region_count<32,"register region table full");
    require(drift_pci_get_bar(&pci,bdf,index,&bar)==0 && bar.address && bar.address<=UINT32_MAX,"invalid controller BAR");
    /* Generic BAR sizing, only before handing an idle device to its driver. */
    uint32_t command=config_read(0,bdf,4)&65535;
    uint32_t offset=16+index*4,low=config_read(0,bdf,offset),high=0;
    config_write(0,bdf,4,command&~7u);
    if (bar.width==64) { high=config_read(0,bdf,offset+4); config_write(0,bdf,offset+4,UINT32_MAX); }
    config_write(0,bdf,offset,UINT32_MAX);
    uint32_t mask=config_read(0,bdf,offset)&(bar.io ? ~3u : ~15u);
    config_write(0,bdf,offset,low);
    if (bar.width==64) config_write(0,bdf,offset+4,high);
    config_write(0,bdf,4,command);
    uint32_t size=~mask+1;
    require(size && !(size&(size-1)),"invalid BAR size");
    require(drift_pci_enable(&pci,bdf,7)==0,"PCI enable failed");
    uint32_t region=region_count++;
    regions[region]=(struct region){(uint32_t)bar.address,bar.io,size}; return region;
}
static uint32_t find_device(uint32_t class_code,uint32_t vendor,uint32_t product) {
    drift_pci_device device;
    for (uint32_t index=0;index<64;index++) {
        if (drift_pci_find(&pci,0,0,vendor,product,index,&device)) break;
        if (class_code==UINT32_MAX || device.class_code==class_code) return device.bdf;
    }
    text("missing PCI class "); hex(class_code); text("\n"); exit_kernel(0); return 0;
}
static void virtio_open(drift_virtio *d,uint32_t product,uint32_t type) {
    uint32_t bdf=find_device(UINT32_MAX,0x1af4,product);
    drift_virtio_bar bindings[6];
    for (uint32_t index=0;index<6;index++) { bindings[index].region=UINT32_MAX; bindings[index].size=0; }
    for (uint32_t index=0;index<6;index++) {
        drift_pci_bar bar;
        if (drift_pci_get_bar(&pci,bdf,index,&bar)) continue;
        uint32_t region=bind_bar(bdf,index);
        bindings[index].region=region; bindings[index].size=regions[region].size;
        if (bar.width==64) ++index;
    }
    int32_t result=drift_virtio_open_function(d,&resources,&pci,bdf,bindings);
    if (result) { text("Virtio open status "); hex((uint32_t)result); text("\n"); }
    require(result==0 && d->device_id==type,"Virtio PCI initialization failed");
}
static uint8_t saved[8192],written[8192],readback[8192];
static void fill(uint32_t size) { for (uint32_t i=0;i<size;i++) written[i]=(uint8_t)(0xa5^i); }
static int equal(const uint8_t *a,const uint8_t *b,uint32_t size) { for (uint32_t i=0;i<size;i++) if (a[i]!=b[i]) return 0; return 1; }

/* The filesystem-facing path is identical for every disk controller. The
 * 8 KiB request also exercises splitting across NVMe's smaller command limit. */
static void block_test(uint32_t kind,void *native,const char *name) {
    drift_block disk={0};
    require(drift_block_bind(&disk,kind,native)==0,"Block class bind failed");
    require(disk.sector_size && disk.sector_size<=4096 && disk.blocks>32,"Invalid block geometry");
    uint32_t count=sizeof(saved)/disk.sector_size,bytes=count*disk.sector_size;
    require(drift_block_read(&disk,16,count,saved,sizeof(saved))==0 && disk.completed==count,"Block read failed"); fill(bytes);
    require(drift_block_write(&disk,16,count,written,sizeof(written))==0 && disk.completed==count,"Block write failed");
    require(drift_block_flush(&disk)==0,"Block flush failed");
    require(drift_block_read(&disk,16,count,readback,sizeof(readback))==0 && disk.completed==count && equal(written,readback,bytes),"Block readback failed");
    require(drift_block_write(&disk,16,count,saved,sizeof(saved))==0 && disk.completed==count && drift_block_flush(&disk)==0,"Block restore failed");
    text(name); text(" read/write/flush/restore PASS\n");
}
static void ahci_test(void) {
    drift_ahci disk={0}; uint32_t region=bind_bar(find_device(0x010601,0xffff,0xffff),5); used=0;
    int32_t status=drift_ahci_init(&disk,&resources,region,0,1000000);
    if (status) { text("AHCI init status "); hex((uint32_t)status); text("\n"); }
    require(status==0,"AHCI initialization failed");
    block_test(DRIFT_BLOCK_AHCI,&disk,"AHCI");
    require(drift_ahci_close(&disk)==0,"AHCI close failed");
}
static void nvme_test(void) {
    drift_nvme disk={0}; uint32_t region=bind_bar(find_device(0x010802,0xffff,0xffff),0); used=0;
    int32_t status=drift_nvme_init(&disk,&resources,region,1,1000000);
    if (status) { text("NVMe init status "); hex((uint32_t)status); text("\n"); }
    require(status==0,"NVMe initialization failed");
    block_test(DRIFT_BLOCK_NVME,&disk,"NVMe");
    require(drift_nvme_close(&disk)==0,"NVMe close failed");
}
static void virtio_disk_test(void) {
    drift_virtio disk={0}; used=0; virtio_open(&disk,0x1042,2);
    block_test(DRIFT_BLOCK_VIRTIO,&disk,"Virtio block");
    require(drift_virtio_close(&disk)==0,"Virtio block close failed");
}
static void receive_frame(drift_net *net,uint8_t *expected) {
    uint8_t packet[4096];
    uint64_t start=clock_us(0); int32_t result;
    do { result=drift_net_receive(net,packet,sizeof(packet)); if (!result) wait_us(0,100); }
    while (!result && clock_us(0)-start<1000000);
    require(result==60 && equal(expected,packet,60),"Network class receive failed");
}
static void network_test(void) {
    drift_virtio virtio={0}; drift_ethernet intel={0},realtek={0};
    drift_net net[3]={{0}};
    uint8_t frame[60];
    used=0; virtio_open(&virtio,0x1041,1);
    uint32_t intel_region=bind_bar(find_device(UINT32_MAX,0x8086,0x100e),0);
    uint32_t realtek_region=bind_bar(find_device(UINT32_MAX,0x10ec,0x8139),0);
    require(drift_eth_init(&intel,&resources,intel_region,DRIFT_ETH_E1000,1000000)==0,"Intel initialization failed");
    require(drift_eth_init(&realtek,&resources,realtek_region,DRIFT_ETH_RTL8139,1000000)==0,"Realtek initialization failed");
    require(drift_net_bind(&net[0],DRIFT_NET_VIRTIO,&virtio)==0 &&
            drift_net_bind(&net[1],DRIFT_NET_ETHERNET,&intel)==0 &&
            drift_net_bind(&net[2],DRIFT_NET_ETHERNET,&realtek)==0,"Network class bind failed");
    for (unsigned i=0;i<60;i++) frame[i]=(uint8_t)i;
    for (unsigned i=0;i<6;i++) { frame[i]=0xff; frame[i+6]=net[0].mac[i]; }
    frame[12]=0x88; frame[13]=0xb5; /* Experimental EtherType; an isolated QEMU hub. */
    require(drift_net_send(&net[0],frame,60)==0,"Virtio frame TX failed");
    receive_frame(&net[1],frame); receive_frame(&net[2],frame);
    frame[20]=0xc1; require(drift_net_send(&net[1],frame,60)==0,"Intel frame TX failed");
    receive_frame(&net[0],frame);
    frame[20]=0xc2; require(drift_net_send(&net[2],frame,60)==0,"Realtek frame TX failed");
    receive_frame(&net[0],frame);
    require(drift_eth_close(&realtek)==0 && drift_eth_close(&intel)==0 && drift_virtio_close(&virtio)==0,"Network close failed");
    text("Virtio/Intel/Realtek Ethernet frame exchange PASS\n");
}
static void usb_test(void) {
    drift_usb disk={0};
    uint32_t region=bind_bar(find_device(0x0c0330,0xffff,0xffff),0); used=0;
    int32_t status=drift_usb_open(&disk,&resources,region);
    if (status) { text("xHCI open status "); hex((uint32_t)status); text("\n"); }
    require(status==0,"xHCI open failed");
    status=drift_usb_attach(&disk,0);
    if (status) { text("USB attach status "); hex((uint32_t)status); text("\n"); }
    require(status==0 && disk.kind==DRIFT_USB_STORAGE,"USB mass storage attach failed");
    block_test(DRIFT_BLOCK_USB,&disk,"xHCI/USB storage");
    require(drift_usb_close(&disk)==0,"xHCI close failed");
}
void kernel_main(uint32_t magic,uint32_t information) {
    (void)information; serial_init(); text("Drift reference kernel\n");
    require(magic==0x2badb002,"not a Multiboot boot"); clock_init();
    ahci_test(); nvme_test(); virtio_disk_test(); network_test(); usb_test(); exit_kernel(1);
}
