/* Class facade tests deliberately stub native protocols rather than repeat
 * controller models: dispatch, bounds, chunking and result normalization.
 * SPDX-License-Identifier: MIT */
#include "drift_io.h"
#include "drift_virtio.h"
#include "drift_nvme.h"
#include "drift_ahci.h"
#include "drift_usb.h"
#include "drift_ethernet.h"
#include "drift_usb_classes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"io line %d: %s\n",__LINE__,#c); abort(); } } while (0)
static uint32_t kind,sector,limit,done,calls,write_mode,fail_at,info_calls,net_calls,net_length,net_capacity;
static int32_t failure,flush_result,info_result,net_result;
static uint64_t base_lba;
static uint8_t *base_buffer;
static void *expected_driver;
static unsigned byte(uint64_t lba,unsigned off) { return (unsigned)((lba*sector+off)^((lba*sector+off)>>8))&255; }
static int32_t rw(void*d,unsigned expected,uint64_t lba,uint32_t n,uint8_t*p,unsigned writing) {
    CHECK(d==expected_driver&&kind==expected&&writing==write_mode);
    CHECK(n&&n<=limit&&lba==base_lba+done&&p==base_buffer+done*sector);
    calls++;
    if(calls==fail_at)return failure;
    for(uint32_t i=0;i<n*sector;i++) {
        if(writing)CHECK(p[i]==byte(lba,i));else p[i]=(uint8_t)byte(lba,i);
    }
    done+=n;return 0;
}
int32_t drift_virtio_block(drift_virtio*d,uint64_t l,uint32_t n,uint8_t*p,int32_t w) { return rw(d,1,l,n,p,(unsigned)w); }
int32_t drift_nvme_read(drift_nvme*d,uint64_t l,uint32_t n,uint8_t*p) { return rw(d,2,l,n,p,0); }
int32_t drift_nvme_write(drift_nvme*d,uint64_t l,uint32_t n,uint8_t*p) { return rw(d,2,l,n,p,1); }
int32_t drift_ahci_read(drift_ahci*d,uint64_t l,uint32_t n,uint8_t*p) { return rw(d,3,l,n,p,0); }
int32_t drift_ahci_write(drift_ahci*d,uint64_t l,uint32_t n,uint8_t*p) { return rw(d,3,l,n,p,1); }
int32_t drift_usb_storage_read(drift_usb*d,uint64_t l,uint32_t n,uint8_t*p) { return rw(d,4,l,n,p,0); }
int32_t drift_usb_storage_write(drift_usb*d,uint64_t l,uint32_t n,uint8_t*p) { return rw(d,4,l,n,p,1); }
static int32_t flush(void*d,unsigned k) { CHECK(d==expected_driver&&kind==k);calls++;return flush_result; }
int32_t drift_virtio_flush(drift_virtio*d) { return flush(d,1); }
int32_t drift_nvme_flush(drift_nvme*d) { return flush(d,2); }
int32_t drift_ahci_flush(drift_ahci*d) { return flush(d,3); }
int32_t drift_usb_storage_flush(drift_usb*d) { return flush(d,4); }
int32_t drift_usb_storage_info(drift_usb*d,uint64_t*blocks,uint32_t*size) {
    info_calls++; if(info_result)return info_result;
    *blocks=d->blocks;*size=d->sector_size;return 0;
}
static int32_t send(void*d,unsigned k,uint8_t*p,uint32_t n) {
    CHECK(d==expected_driver&&kind==k&&p==base_buffer&&n==net_length);net_calls++;return net_result;
}
static int32_t receive(void*d,unsigned k,uint8_t*p,uint32_t n) {
    CHECK(d==expected_driver&&kind==k&&p==base_buffer&&n==net_capacity);net_calls++;return net_result;
}
int32_t drift_virtio_send(drift_virtio*d,uint8_t*p,uint32_t n) { return send(d,1,p,n); }
int32_t drift_virtio_receive(drift_virtio*d,uint8_t*p,uint32_t n) { return receive(d,1,p,n); }
int32_t drift_eth_tx(drift_ethernet*d,uint8_t*p,uint32_t n) { return send(d,2,p,n); }
int32_t drift_eth_rx(drift_ethernet*d,uint8_t*p,uint32_t n,uint32_t*got) { int32_t s=receive(d,2,p,n);*got=net_length;return s; }
int32_t drift_usb_cdc_write(drift_usb_cdc*d,uint8_t*p,uint32_t n) { return send(d,3,p,n); }
int32_t drift_usb_cdc_read(drift_usb_cdc*d,uint8_t*p,uint32_t n) { return receive(d,3,p,n); }

static void block(unsigned k,unsigned size) {
    drift_virtio v={0};drift_nvme n={0};drift_ahci a={0};drift_usb u={0};drift_block b={0};
    uint32_t max_bytes=k==1||k==3?65536:k==2?4096:16384;
    uint64_t blocks=k==4?UINT64_C(0x100000):UINT64_C(0x100010000);
    v.state=2;v.device_id=2;v.capacity=blocks;n.state=1;n.blocks=blocks;n.block_size=size;
    a.state=1;a.sectors=blocks;a.sector_size=size;u.state=1;u.slot=1;u.kind=3;u.blocks=blocks;u.sector_size=size;
    void*d=k==1?(void*)&v:k==2?(void*)&n:k==3?(void*)&a:(void*)&u;
    kind=k;sector=size;limit=max_bytes/size;expected_driver=d;info_result=0;info_calls=0;
    CHECK(drift_block_bind(&b,k,d)==0);CHECK(b.blocks==blocks&&b.sector_size==size&&b.max_sectors==limit&&b.completed==0);
    CHECK(info_calls==(k==4));
    uint32_t count=limit*2+(limit>2?3:1),bytes=count*size;
    uint8_t*p=malloc(bytes+32);CHECK(p);base_buffer=p;base_lba=k==4?200:UINT64_C(0x100000000);fail_at=0;
    for(unsigned mode=0;mode<2;mode++) {
        calls=done=0;write_mode=mode;memset(p,0xee,bytes+32);
        if(mode)for(uint32_t i=0;i<bytes;i++)p[i]=(uint8_t)byte(base_lba,i);
        int32_t r=mode?drift_block_write(&b,base_lba,count,p,bytes):drift_block_read(&b,base_lba,count,p,bytes);
        if(r||b.completed!=count||done!=count||calls!=3)fprintf(stderr,"kind%u size%u mode%u r%d complete%u count%u done%u calls%u limit%u\n",k,size,mode,r,b.completed,count,done,calls,limit);
        CHECK(r==0&&b.completed==count&&done==count&&calls==3);
        for(uint32_t i=0;i<bytes;i++)CHECK(p[i]==byte(base_lba,i));
        for(unsigned i=0;i<32;i++)CHECK(p[bytes+i]==0xee);
    }
    calls=done=0;write_mode=0;fail_at=2;failure=-91;
    CHECK(drift_block_read(&b,base_lba,count,p,bytes)==-91&&b.completed==limit&&calls==2);
    calls=done=0;failure=7;
    CHECK(drift_block_read(&b,base_lba,count,p,bytes)==DRIFT_IO_EPROTO&&b.completed==limit);
    fail_at=0;calls=done=0;
    CHECK(drift_block_read(&b,blocks-1,2,p,bytes)==DRIFT_IO_ERANGE);
    CHECK(drift_block_read(&b,UINT64_MAX,1,p,bytes)==DRIFT_IO_ERANGE);
    CHECK(drift_block_read(&b,0,count,p,bytes-1)==DRIFT_IO_ERANGE);
    CHECK(drift_block_read(&b,0,UINT32_MAX,p,UINT32_MAX)==DRIFT_IO_ERANGE);
    CHECK(drift_block_read(&b,0,0,p,bytes)==DRIFT_IO_EARG);
    CHECK(drift_block_read(&b,0,1,NULL,bytes)==DRIFT_IO_EARG&&b.completed==0&&calls==0);
    b.max_sectors++;CHECK(drift_block_read(&b,0,1,p,bytes)==DRIFT_IO_ESTATE);b.max_sectors--;
    flush_result=-92;CHECK(drift_block_flush(&b)==-92&&b.completed==0);
    flush_result=2;CHECK(drift_block_flush(&b)==DRIFT_IO_EPROTO);
    flush_result=0;CHECK(drift_block_flush(&b)==0);
    if(k==1)v.state=0;else if(k==2)n.state=0;else if(k==3)a.state=0;else u.state=0;
    calls=0;CHECK(drift_block_read(&b,0,1,p,bytes)==DRIFT_IO_ESTATE&&calls==0);
    CHECK(drift_block_flush(&b)==DRIFT_IO_ESTATE);
    CHECK(drift_block_bind(&b,k,d)==DRIFT_IO_ESTATE&&b.driver==NULL&&b.kind==0);
    free(p);
}
static void bad_geometry(void) {
    drift_nvme n={0};drift_block b={0};n.state=1;n.blocks=1;
    unsigned sizes[]={0,1,256,513,768,8192,UINT32_MAX};
    for(unsigned i=0;i<sizeof(sizes)/sizeof(*sizes);i++) { n.block_size=sizes[i];CHECK(drift_block_bind(&b,2,&n)==DRIFT_IO_EPROTO&&b.driver==NULL); }
    n.block_size=512;n.blocks=0;CHECK(drift_block_bind(&b,2,&n)==DRIFT_IO_EPROTO);
    CHECK(drift_block_bind(&b,99,&n)==DRIFT_IO_ENOTSUP);
    CHECK(drift_block_bind(&b,2,NULL)==DRIFT_IO_EARG);
    CHECK(drift_block_bind(NULL,2,&n)==DRIFT_IO_EARG);
    CHECK(drift_block_read(NULL,0,1,NULL,0)==DRIFT_IO_EARG);
    CHECK(drift_block_flush(NULL)==DRIFT_IO_EARG);
    drift_usb u={0};u.state=1;u.slot=1;u.kind=3;u.blocks=1;u.sector_size=512;
    info_result=-95;CHECK(drift_block_bind(&b,4,&u)==-95&&b.driver==NULL);
    info_result=3;CHECK(drift_block_bind(&b,4,&u)==DRIFT_IO_EPROTO&&b.driver==NULL);info_result=0;
    n.blocks=UINT64_MAX;n.block_size=512;
    CHECK(drift_block_bind(&b,2,&n)==0);
    uint8_t p[512];kind=2;sector=512;limit=8;done=calls=write_mode=fail_at=0;expected_driver=&n;base_buffer=p;base_lba=UINT64_MAX-1;
    CHECK(drift_block_read(&b,base_lba,1,p,sizeof(p))==0&&b.completed==1);
}
static void network(unsigned k) {
    drift_virtio v={0};drift_ethernet e={0};drift_usb u={0};drift_usb_cdc c={0};drift_net n={0};
    const uint8_t mac[]={0x52,0x54,0,0x12,0x34,0x56};
    v.state=2;v.device_id=1;memcpy(v.mac,mac,6);e.state=1;memcpy(e.mac,mac,6);
    u.state=1;c.state=1;c.device=&u;c.kind=2;c.maximum_segment=1514;memcpy(c.mac,mac,6);
    void*d=k==1?(void*)&v:k==2?(void*)&e:(void*)&c;
    kind=k;expected_driver=d;net_calls=0;
    CHECK(drift_net_bind(&n,k,d)==0&&n.max_frame==(k==1?2036:k==2?1518:1514));CHECK(!memcmp(n.mac,mac,6));
    uint8_t p[32768];base_buffer=p;net_capacity=n.max_frame;net_length=60;
    net_result=k==3?60:0;CHECK(drift_net_send(&n,p,60)==0);
    net_result=-82;CHECK(drift_net_send(&n,p,60)==-82);
    net_result=k==3?59:4;CHECK(drift_net_send(&n,p,60)==DRIFT_IO_EPROTO);
    net_calls=0;CHECK(drift_net_send(&n,p,13)==DRIFT_IO_ERANGE);CHECK(drift_net_send(&n,p,n.max_frame+1)==DRIFT_IO_ERANGE&&net_calls==0);
    net_result=k==2?0:60;net_length=60;CHECK(drift_net_receive(&n,p,sizeof(p))==60);
    net_length=0;net_result=k==2?1:0;CHECK(drift_net_receive(&n,p,sizeof(p))==0);
    net_result=-83;CHECK(drift_net_receive(&n,p,sizeof(p))==-83);
    net_result=k==2?0:13;net_length=13;CHECK(drift_net_receive(&n,p,sizeof(p))==DRIFT_IO_EPROTO);
    net_result=k==2?0:(int32_t)n.max_frame+1;net_length=n.max_frame+1;CHECK(drift_net_receive(&n,p,sizeof(p))==DRIFT_IO_EPROTO);
    if(k==2) { net_result=1;net_length=60;CHECK(drift_net_receive(&n,p,sizeof(p))==DRIFT_IO_EPROTO); }
    net_calls=0;CHECK(drift_net_receive(&n,p,n.max_frame-1)==DRIFT_IO_ERANGE&&net_calls==0);
    CHECK(drift_net_receive(&n,NULL,sizeof(p))==DRIFT_IO_EARG);
    if(k==1)v.state=0;else if(k==2)e.state=0;else u.state=0;
    CHECK(drift_net_receive(&n,p,sizeof(p))==DRIFT_IO_ESTATE);
    CHECK(drift_net_bind(&n,k,d)==DRIFT_IO_ESTATE&&n.driver==NULL);
}
static void bad_network(void) {
    drift_net n={0};drift_ethernet e={0};e.state=1;
    CHECK(drift_net_bind(&n,2,&e)==DRIFT_IO_EPROTO);
    e.mac[0]=1;CHECK(drift_net_bind(&n,2,&e)==DRIFT_IO_EPROTO);
    CHECK(drift_net_bind(&n,99,&e)==DRIFT_IO_ENOTSUP);
    CHECK(drift_net_bind(&n,2,NULL)==DRIFT_IO_EARG);
    CHECK(drift_net_bind(NULL,2,&e)==DRIFT_IO_EARG);
    CHECK(drift_net_send(NULL,NULL,0)==DRIFT_IO_EARG);
    CHECK(drift_net_receive(NULL,NULL,0)==DRIFT_IO_EARG);
}
int main(void) {
    block(1,512);
    for(unsigned k=2;k<=4;k++) { block(k,512);block(k,4096); }
    block(2,1024);block(2,2048);
    bad_geometry();network(1);network(2);network(3);bad_network();
    printf("io: %u checks passed (4 block + 3 network bindings)\n",checks);return 0;
}
