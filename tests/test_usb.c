/* Independent small xHCI endpoint model. SPDX-License-Identifier: MIT */
#include "drift_usb.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct model {
    uint32_t registers[0x3000/4], command, producer, producer_cycle, index[32];
    uint64_t ring[32], time;
    uint8_t *memory;
    unsigned releases, commands, transfers, failure, no_event, cannot_halt, class_kind;
    uint32_t cbw_tag, data_length, lba;
    unsigned bot_phase, opcode, defer_hid, bad_config, high_dma, allocation_size;
    uint8_t disk[8192];
} model;
static uint32_t le(const uint8_t *p) { return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void put(uint8_t *p,uint32_t n) { p[0]=n;p[1]=n>>8;p[2]=n>>16;p[3]=n>>24; }
static uint64_t le64(const uint8_t*p) { return le(p)|(uint64_t)le(p+4)<<32; }
static void put64(uint8_t*p,uint64_t n) { put(p,(uint32_t)n);put(p+4,(uint32_t)(n>>32)); }
static uint8_t *memory(model*m,uint64_t address,uint32_t size) { assert(address>=0x100000 && address-0x100000<=m->allocation_size && size<=m->allocation_size-(address-0x100000));return m->memory+(address-0x100000); }
static uint64_t read_register(void *context,uint32_t region,uint32_t off,uint32_t width) {
    model*m=context; assert(region==0 && off+width<=sizeof(m->registers));
    uint64_t value=m->registers[off/4];value>>=(off%4)*8;
    return width==1?value&255:width==2?value&65535:value;
}
static void push_event(model*m,uint64_t pointer,uint32_t length,unsigned code,unsigned type,unsigned ep) {
    if(m->no_event)return;
    uint8_t*p=m->memory+8192+m->producer*16;
    put64(p,m->failure==1?pointer+16:pointer); put(p+8,length|(code<<24));
    put(p+12,(1u<<24)|(ep<<16)|(type<<10)|m->producer_cycle);
    if(++m->producer==256){m->producer=0;m->producer_cycle^=1;}
}
static uint8_t* next_trb(model*m,unsigned ep,uint64_t*address) {
    uint64_t base=m->ring[ep];
    assert(base);
    if(m->index[ep]==255)m->index[ep]=0;
    *address=base+m->index[ep]++*16;
    return memory(m,*address,16);
}
static void run_command(model*m) {
    uint8_t*p=m->memory+4096+m->command*16;
    if(m->command==255){m->command=0;p=m->memory+4096;}
    uint64_t pointer=0x101000+m->command*16;
    uint32_t control=le(p+12),kind=(control>>10)&63;
    ++m->command;++m->commands;
    if(kind==11 || kind==12 || kind==13) {
        uint8_t*input=memory(m,le64(p),4096);
        uint32_t mask=le(input+4);
        for(unsigned ep=0;ep<32;++ep)if(mask&(1u<<ep)){
            memcpy(m->memory+20480+ep*32,input+(ep+1)*32,32);
            if(ep && kind!=13)m->ring[ep]=le64(input+(ep+1)*32+8)&~15ull;
            if(ep && kind!=13)m->index[ep]=0;
        }
    }else assert(kind==9 || kind==14 || kind==16);
    push_event(m,pointer,0,1,33,0);
}
static const uint8_t keyboard_config[]={9,2,34,0,1,1,0,0x80,50,9,4,0,0,1,3,1,1,0,9,0x21,0x11,1,0,1,0x22,63,0,7,5,0x81,3,8,0,10};
static const uint8_t storage_config[]={9,2,32,0,1,1,0,0x80,50,9,4,0,0,2,8,6,0x50,0,7,5,0x81,2,64,0,0,7,5,2,2,64,0,0};
static void run_control(model*m) {
    uint64_t setup_at,data_at=0,last;
    uint8_t*setup=next_trb(m,1,&setup_at);
    uint64_t packet=le64(setup);
    uint8_t request=(uint8_t)(packet>>8); uint16_t value=(uint16_t)(packet>>16),want=(uint16_t)(packet>>48);
    uint8_t*data=NULL;
    if(want){uint8_t*t=next_trb(m,1,&data_at);data=memory(m,le64(t),want);}
    next_trb(m,1,&last);
    unsigned received=0;
    if(request==6 && (value>>8)==1){
        uint8_t desc[18]={18,1,0,2,0,0,0,8,0x34,0x12,0x78,0x56,0,1,0,0,0,1};
        received=want<18?want:18;memcpy(data,desc,received);
    }else if(request==6 && (value>>8)==2 && m->bad_config){
        uint8_t desc[1024]={9,2,0,4,1,1,0,0x80,50};
        unsigned at=9;
        while(at<1023){unsigned n=1023-at;if(n>254)n=254;desc[at]=(uint8_t)n;desc[at+1]=0x99;at+=n;}
        received=want<1024?want:1024;memcpy(data,desc,received);
    }else if(request==6 && (value>>8)==2){
        const uint8_t*desc=m->class_kind==3?storage_config:keyboard_config;
        unsigned size=m->class_kind==3?sizeof(storage_config):sizeof(keyboard_config);
        received=want<size?want:size;memcpy(data,desc,received);
    }else assert(request==9 || request==11 || request==1 || request==0xff);
    if(want && received<want)push_event(m,data_at,want-received,13,32,1);
    if(m->defer_hid){push_event(m,m->ring[3]+(m->index[3]-1)*16,0,1,32,3);m->defer_hid=0;}
    push_event(m,last,0,1,32,1);
}
static void run_transfer(model*m,unsigned ep) {
    uint64_t pointer;uint8_t*t=next_trb(m,ep,&pointer);
    uint32_t length=le(t+8)&0x1ffff;uint8_t*data=memory(m,le64(t),length);
    ++m->transfers;
    if(m->class_kind!=3){assert(ep==3 && length==8);memset(data,0,length);data[2]=4;}
    else if(m->bot_phase==0){
        assert(ep==4 && length==31 && le(data)==0x43425355);
        m->cbw_tag=le(data+4);m->data_length=le(data+8);m->opcode=data[15];
        m->lba=(uint32_t)data[17]<<24|(uint32_t)data[18]<<16|(uint32_t)data[19]<<8|data[20];
        m->bot_phase=m->data_length?1:2;
    }else if(m->bot_phase==1){
        assert(length==m->data_length);
        if(m->opcode==0x25){assert(ep==3 && length==8);memset(data,0,8);data[3]=15;data[6]=2;}
        else if(m->opcode==0x28){assert(ep==3 && m->lba<16);memcpy(data,m->disk+m->lba*512,length);}
        else if(m->opcode==0x2a){assert(ep==4 && m->lba<16);memcpy(m->disk+m->lba*512,data,length);}
        else assert(0);
        m->bot_phase=2;
    }else{
        assert(ep==3 && length==13);memset(data,0,13);put(data,0x53425355);put(data+4,m->cbw_tag);m->bot_phase=0;
    }
    push_event(m,pointer,0,1,32,ep);
}
static void write_register(void *context,uint32_t region,uint32_t off,uint32_t width,uint64_t value) {
    model*m=context;assert(!region && width==4 && off+4<=sizeof(m->registers));
    m->registers[off/4]=(uint32_t)value;
    if(off==0x40){
        if(value&2){m->registers[off/4]=0;m->registers[0x44/4]=1;}
        else m->registers[0x44/4]=(value&1)?0:m->cannot_halt?0:1;
    }else if(off==0x440 && (value&16)){m->registers[off/4]=3|(3<<10);}
    else if(off==0x1000)run_command(m);
    else if(off==0x1004){if(value==1)run_control(m);else run_transfer(m,(unsigned)value);}
}
static int32_t allocate(void*ctx,uint32_t size,uint32_t alignment,drift_dma*out){model*m=ctx;assert(size>=262144 && alignment==65536);m->allocation_size=size;m->memory=aligned_alloc(alignment,size);assert(m->memory);memset(m->memory,0,size);*out=(drift_dma){m->memory,m->high_dma?0x100000000ull:0x100000,size,1};return 0;}
static void release(void*ctx,drift_dma*b){model*m=ctx;assert(b->data==m->memory);free(b->data);m->memory=NULL;b->data=NULL;++m->releases;}
static void synchronize(void*ctx,drift_dma*b,uint32_t off,uint32_t n,uint32_t direction){model*m=ctx;assert(b->data==m->memory && off<=b->size && n<=b->size-off && direction>=1 && direction<=3);}
static uint64_t now(void*ctx){return ((model*)ctx)->time;}
static void wait_us(void*ctx,uint32_t us){((model*)ctx)->time+=us;}
static void init(model*m,drift_hw*h,unsigned kind){memset(m,0,sizeof(*m));m->class_kind=kind;m->producer_cycle=1;m->registers[0]=0x1000040;m->registers[1]=1|(1<<24);m->registers[4]=1;m->registers[5]=0x1000;m->registers[6]=0x2000;m->registers[0x44/4]=1;m->registers[0x48/4]=1;m->registers[0x440/4]=3|(3<<10);*h=(drift_hw){m,read_register,write_register,allocate,release,synchronize,now,wait_us};}
int main(void){
    model m;drift_hw h;drift_usb d={0};uint8_t report[8];uint32_t got;
    init(&m,&h,1);assert(!drift_usb_open(&d,&h,0));assert(!drift_usb_attach(&d,0));assert(d.kind==DRIFT_USB_HID_KEYBOARD);
    assert(!drift_usb_hid(&d,report,8,&got) && got==8 && report[2]==4);
    for(unsigned i=0;i<600;++i)assert(!drift_usb_hid(&d,report,8,&got));
    assert(drift_usb_hid(&d,report,7,&got)==-22);
    m.no_event=1;assert(drift_usb_hid(&d,report,8,&got)==-11 && d.state==1);
    m.no_event=0;m.defer_hid=1;assert(!drift_usb_control(&d,0,9,1,0,NULL,0,&got));
    assert(!drift_usb_hid(&d,report,8,&got) && got==8 && report[2]==4);
    assert(!drift_usb_close(&d) && m.releases==1);
    memset(&d,0,sizeof(d));init(&m,&h,3);assert(!drift_usb_open(&d,&h,0));assert(!drift_usb_attach(&d,1));
    uint64_t blocks;uint32_t sector;assert(!drift_usb_storage_info(&d,&blocks,&sector) && blocks==16 && sector==512);
    uint8_t data[1024],copy[1024];for(unsigned i=0;i<sizeof(data);++i)data[i]=(uint8_t)(i^0x53);
    assert(!drift_usb_storage_write(&d,3,2,data));assert(!drift_usb_storage_read(&d,3,2,copy));assert(!memcmp(data,copy,sizeof(data)));assert(!drift_usb_storage_flush(&d));
    assert(drift_usb_storage_read(&d,16,1,copy)==-22);assert(drift_usb_storage_read(&d,15,2,copy)==-22);assert(!drift_usb_close(&d));
    memset(&d,0,sizeof(d));init(&m,&h,1);assert(!drift_usb_open(&d,&h,0));m.failure=1;assert(drift_usb_attach(&d,0)==-5 && d.state==2);assert(!drift_usb_close(&d));
    memset(&d,0,sizeof(d));init(&m,&h,1);assert(!drift_usb_open(&d,&h,0));m.no_event=1;assert(drift_usb_attach(&d,0)==-110 && d.state==2);assert(!drift_usb_close(&d));
    memset(&d,0,sizeof(d));init(&m,&h,1);assert(!drift_usb_open(&d,&h,0));m.no_event=1;m.cannot_halt=1;assert(drift_usb_attach(&d,0)==-110 && d.state==3);assert(drift_usb_close(&d)==-110 && !m.releases);m.cannot_halt=0;assert(!drift_usb_close(&d) && m.releases==1);
    memset(&d,0,sizeof(d));init(&m,&h,1);m.registers[8/4]=1u<<27;assert(!drift_usb_open(&d,&h,0));assert(le64(m.memory)==0x100000+262144);assert(le64(m.memory+262144)==0x100000+270336);assert(!drift_usb_close(&d));
    memset(&d,0,sizeof(d));init(&m,&h,1);m.registers[4]=0;m.high_dma=1;assert(drift_usb_open(&d,&h,0)==-22 && m.releases==1 && !m.memory);
    memset(&d,0,sizeof(d));init(&m,&h,1);m.bad_config=1;assert(!drift_usb_open(&d,&h,0));assert(drift_usb_attach(&d,0)==-5);assert(!drift_usb_close(&d));
    memset(&d,0,sizeof(d));init(&m,&h,1);assert(!drift_usb_open(&d,&h,0));assert(!drift_usb_attach(&d,0));m.registers[0x440/4]=0;assert(drift_usb_hid(&d,report,8,&got)==-19 && d.state==2);assert(!drift_usb_close(&d));
    puts("usb: xHCI enumeration, HID ring wraps, BOT disk roundtrip, bounds, timeout and quarantine passed");
    return 0;
}
