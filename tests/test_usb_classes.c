/* Class request/descriptor tests; transport is independently stubbed here.
 * SPDX-License-Identifier: MIT */
#include "drift_usb_classes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(c) do { ++checks; if(!(c)) { fprintf(stderr,"usb classes:%d: %s\n",__LINE__,#c);abort(); } } while(0)
static unsigned checks;
static struct {
    unsigned controls,transfers,alternates,last_request,last_value,last_index,last_endpoint,last_length,zlp;
    unsigned bad_mac,short_mac,mac_multicast,mac_zero,wide_mac,idle,rx_length,short_write,wrong_count;
    int control_error,transfer_error,alternate_error;
    uint8_t line[7],sent[2048];
} model;
int32_t drift_usb_control(drift_usb *d,uint8_t type,uint8_t request,uint16_t value,uint16_t index,uint8_t *data,uint32_t length,uint32_t *got)
{
    CHECK(d && d->state==1 && got);++model.controls;
    model.last_request=request;model.last_value=value;model.last_index=index;
    if(model.control_error)return model.control_error;
    *got=length;
    if(type==0x80 && request==6) {
        CHECK(length==255 && data);
        if(value==0x300) {CHECK(index==0);data[0]=4;data[1]=3;data[2]=9;data[3]=4;*got=4;}
        else {
            CHECK(value==0x305 && index==0x409);
            const char *mac=model.mac_zero?"000000000000":model.mac_multicast?"030304050607":"020304050607";
            data[0]=26;data[1]=3;
            for(unsigned i=0;i<12;i++){data[2+i*2]=(uint8_t)mac[i];data[3+i*2]=0;}
            if(model.bad_mac) data[4]='x';
            if(model.wide_mac) data[3]=1;
            *got=model.short_mac?24:26;
        }
    } else if(type==0x21 && request==0x20) {CHECK(index==1 && value==0 && length==7);memcpy(model.line,data,7);}
    else if(type==0x21 && request==0x22) {CHECK(index==1 && value<=3 && !length);}
    else if(type==0x21 && request==0x43) {CHECK(index==1 && (value==12||value==0) && !length);}
    else CHECK(!"unexpected USB class request");
    if(model.wrong_count)++*got;
    return 0;
}
int32_t drift_usb_set_interface(drift_usb *d,uint8_t interface,uint8_t alternate)
{
    CHECK(d && interface==2 && alternate==1);++model.alternates;
    return model.alternate_error;
}
int32_t drift_usb_transfer(drift_usb *d,uint8_t endpoint,uint8_t *data,uint32_t length,uint32_t *got)
{
    CHECK(d && got);++model.transfers;
    model.last_endpoint=endpoint;model.last_length=length;
    if(model.transfer_error)return model.transfer_error;
    if(endpoint&0x80) {
        CHECK(endpoint==0x82 && data);
        if(model.idle)return -11;
        *got=model.rx_length;
        if(*got<=length)for(unsigned i=0;i<*got;i++)data[i]=(uint8_t)i;
    } else {
        CHECK(endpoint==3 && length<=sizeof(model.sent));
        if(!length){CHECK(!data);++model.zlp;*got=0;}
        else {CHECK(data);memcpy(model.sent,data,length);*got=model.short_write?length-1:length;}
    }
    return 0;
}
static void append(drift_usb *d,const uint8_t *bytes,unsigned size)
{
    CHECK(d->total_config+size<=sizeof(d->config));
    memcpy(d->config+d->total_config,bytes,size);d->total_config+=size;
}
#define ADD(...) do { const uint8_t b[]={__VA_ARGS__};append(d,b,sizeof(b)); } while(0)
static void fixture(drift_usb *d,unsigned kind)
{
    memset(&model,0,sizeof(model));memset(d,0,sizeof(*d));d->state=1;d->slot=1;
    ADD(9,2,0,0,3,1,0,0x80,50);
    ADD(9,4,0,0,0,0xff,0,0,0); /* unrelated interface must not supply endpoints */
    ADD(9,4,1,0,1,2,kind==1?2:6,kind==1?1:0,0);
    ADD(5,0x24,0,0x10,1);
    if(kind==1){ADD(4,0x24,2,2);}
    else {ADD(13,0x24,15,5,0,0,0,0,0xea,5,0,0,0);}
    ADD(5,0x24,6,1,2);
    ADD(7,5,0x81,3,8,0,16);
    if(kind==2){ADD(9,4,2,0,0,10,0,0,0);}
    ADD(9,4,2,kind==2?1:0,2,10,0,0,0);
    ADD(7,5,0x82,2,64,0,0);
    ADD(7,5,3,2,64,0,0);
    d->config[2]=(uint8_t)d->total_config;d->config[3]=(uint8_t)(d->total_config>>8);
}
static unsigned find(drift_usb *d,unsigned type,unsigned subtype)
{
    for(unsigned at=0;at<d->total_config;at+=d->config[at])
        if(d->config[at+1]==type && (subtype==UINT32_MAX||d->config[at+2]==subtype))return at;
    CHECK(!"descriptor missing from test fixture");return 0;
}
int main(void)
{
    drift_usb d;drift_usb_cdc c={0};uint8_t data[2048];
    fixture(&d,1);CHECK(drift_usb_cdc_open(&c,&d,1,UINT32_MAX)==0);
    CHECK(c.control_interface==1 && c.data_interface==2 && c.endpoint_in==0x82 && c.endpoint_out==3);
    CHECK(!model.controls && !model.alternates);
    CHECK(drift_usb_acm_configure(&c,115200,0,0,8,3)==0);
    const uint8_t expected[]={0,0xc2,1,0,0,0,8};CHECK(!memcmp(model.line,expected,7));
    CHECK(model.last_request==0x22 && model.last_value==3 && model.last_index==1);
    CHECK(drift_usb_acm_configure(&c,0,0,0,8,3)==-22);CHECK(drift_usb_acm_configure(&c,9600,3,0,8,3)==-22);
    CHECK(drift_usb_acm_configure(&c,9600,0,5,8,3)==-22);CHECK(drift_usb_acm_configure(&c,9600,0,0,4,3)==-22);
    CHECK(drift_usb_acm_configure(&c,9600,0,0,8,4)==-22);
    model.rx_length=7;CHECK(drift_usb_cdc_read(&c,data,sizeof(data))==7);for(unsigned i=0;i<7;i++)CHECK(data[i]==i);
    model.idle=1;CHECK(drift_usb_cdc_read(&c,data,sizeof(data))==0);model.idle=0;
    memset(data,0xab,64);CHECK(drift_usb_cdc_write(&c,data,64)==64);CHECK(!memcmp(model.sent,data,64));CHECK(!model.zlp);
    model.short_write=1;CHECK(drift_usb_cdc_write(&c,data,64)==-71);model.short_write=0;
    model.transfer_error=-32;CHECK(drift_usb_cdc_read(&c,data,sizeof(data))==-32);model.transfer_error=0;
    CHECK(drift_usb_cdc_close(&c)==0);CHECK(!c.state && model.last_request==0x22 && !model.last_value);
    fixture(&d,2);CHECK(drift_usb_cdc_open(&c,&d,2,1)==0);
    CHECK(c.maximum_segment==1514 && c.out_packet_size==64 && model.alternates==1);
    CHECK(c.mac[0]==2 && c.mac[5]==7 && model.controls==3 && model.last_request==0x43 && model.last_value==12);
    CHECK(drift_usb_cdc_read(&c,data,64)==-22);model.rx_length=64;CHECK(drift_usb_cdc_read(&c,data,1514)==64);
    CHECK(drift_usb_cdc_write(&c,data,64)==64);CHECK(model.zlp==1 && model.last_length==0);
    CHECK(drift_usb_cdc_write(&c,data,63)==63);CHECK(model.zlp==1 && model.last_length==63);
    CHECK(drift_usb_cdc_write(&c,data,13)==-22);CHECK(drift_usb_cdc_write(&c,data,1515)==-22);
    model.rx_length=13;CHECK(drift_usb_cdc_read(&c,data,1514)==-71);
    model.rx_length=1600;CHECK(drift_usb_cdc_read(&c,data,1514)==-71);
    CHECK(drift_usb_cdc_close(&c)==0);CHECK(model.last_request==0x43 && !model.last_value);
    for(unsigned fault=0;fault<7;fault++) {
        fixture(&d,2);
        model.bad_mac=fault==0;model.short_mac=fault==1;model.mac_multicast=fault==2;model.mac_zero=fault==3;model.wide_mac=fault==4;
        model.alternate_error=fault==5?-5:0;model.control_error=fault==6?-32:0;
        CHECK(drift_usb_cdc_open(&c,&d,2,UINT32_MAX)<0);CHECK(!c.state);
    }
    fixture(&d,1);CHECK(drift_usb_cdc_open(&c,&d,1,0)==-95);
    for(unsigned fault=0;fault<8;fault++) {
        fixture(&d,1);
        unsigned union_at=find(&d,0x24,6), acm_at=find(&d,0x24,2), ep_at=find(&d,5,0x82);
        if(fault==0)d.config[union_at+4]=1; /* master cannot be slave */
        if(fault==1)d.config[acm_at+3]=0; /* no standard line-coding support */
        if(fault==2)d.config[ep_at+2]=3; /* two output endpoints */
        if(fault==3)d.config[ep_at+3]=3; /* interrupt is not bulk data */
        if(fault==4)d.config[ep_at+4]=0; /* zero packet size */
        if(fault==5)d.config[find(&d,4,1)+7]=255; /* vendor RNDIS protocol */
        if(fault==6)d.config[union_at]=2; /* invalid functional descriptor */
        if(fault==7)d.config[2]--; /* total length mismatch */
        CHECK(drift_usb_cdc_open(&c,&d,1,UINT32_MAX)<0);CHECK(!c.state);
    }
    fixture(&d,1);
    for(unsigned cut=0;cut<d.total_config;cut++) {
        uint32_t saved=d.total_config;d.total_config=cut;
        CHECK(drift_usb_cdc_open(&c,&d,1,UINT32_MAX)<0);d.total_config=saved;
    }
    CHECK(drift_usb_cdc_open(NULL,&d,1,UINT32_MAX)==-22);CHECK(drift_usb_cdc_close(NULL)==-22);
    printf("usb classes: %u checks passed (CDC ACM/ECM descriptors, requests, frames, ZLP)\n",checks);return 0;
}
