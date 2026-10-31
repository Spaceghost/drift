/* Actual QEMU controllers exercised through the generic resource boundary.
 * This is emulator integration, not physical hardware qualification.
 * SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L
#include "drift_qtest.h"
#include "drift_nvme.h"
#include "drift_ahci.h"
#include "drift_virtio.h"
#include "drift_ethernet.h"
#include "drift_usb.h"
#include "drift_usb_hub.h"
#include "drift_usb_classes.h"
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr,"qemu: %s:%d: %s\n",__FILE__,__LINE__,#c); exit(1); } } while (0)
static drift_qtest q;

static uint32_t bind_device(uint16_t vendor,uint16_t device,unsigned bar,unsigned instance)
{
    uint32_t bdf,region;
    CHECK(!drift_qtest_pci_find(&q,vendor,device,instance,&bdf));
    CHECK(!drift_qtest_pci_bar(&q,bdf,bar,&region));
    return region;
}
static uint32_t config_read(void *context,uint32_t bdf,uint32_t offset)
{
    return drift_qtest_pci_read(context,bdf<<8,offset);
}
static void config_write(void *context,uint32_t bdf,uint32_t offset,uint32_t value)
{
    drift_qtest_pci_write(context,bdf<<8,offset,value);
}
static void virtio(drift_virtio *device,uint32_t id)
{
    uint32_t bdf,regions[6],sizes[6];
    drift_virtio_bar bindings[6];
    drift_pci pci={&q,config_read,config_write};
    CHECK(!drift_qtest_pci_find(&q,0x1af4,(uint16_t)(0x1040+id),0,&bdf));
    CHECK(!drift_qtest_pci_regions(&q,bdf,regions,sizes));
    for (unsigned i=0;i<6;++i) bindings[i]=(drift_virtio_bar){regions[i],sizes[i]};
    CHECK(!drift_virtio_open_function(device,&q.hardware,&pci,bdf>>8,bindings));
}
static void pattern(uint8_t *bytes,size_t size,unsigned seed)
{
    for (size_t i=0;i<size;++i) bytes[i]=(uint8_t)((i*29+seed)^(i>>7));
}
static void disks(void)
{
    uint8_t original[4096],data[4096],copy[4096];
    drift_nvme nvme={0};
    drift_ahci ahci={0};
    drift_virtio block={0};
    uint32_t region;
    region=bind_device(0x1b36,0x0010,0,0);
    CHECK(!drift_nvme_init(&nvme,&q.hardware,region,1,1000000));
    CHECK(nvme.blocks>=64 && nvme.block_size==512);
    CHECK(!drift_nvme_read(&nvme,8,8,original)); pattern(data,sizeof(data),73);
    CHECK(!drift_nvme_write(&nvme,8,8,data)); CHECK(!drift_nvme_flush(&nvme));
    CHECK(!drift_nvme_read(&nvme,8,8,copy)); CHECK(!memcmp(data,copy,sizeof(data)));
    CHECK(!drift_nvme_write(&nvme,8,8,original)); CHECK(!drift_nvme_close(&nvme));
    puts("qemu: NVMe identify/read/write/flush passed"); fflush(stdout);

    region=bind_device(0x8086,0x2922,5,0);
    /* The explicit controller is slot4; q35's unused built-in is slot31. */
    CHECK(!drift_ahci_init(&ahci,&q.hardware,region,0,1000000));
    CHECK(ahci.sectors>=64 && ahci.sector_size==512);
    CHECK(!drift_ahci_read(&ahci,16,8,original)); pattern(data,sizeof(data),91);
    CHECK(!drift_ahci_write(&ahci,16,8,data)); CHECK(!drift_ahci_flush(&ahci));
    CHECK(!drift_ahci_read(&ahci,16,8,copy)); CHECK(!memcmp(data,copy,sizeof(data)));
    CHECK(!drift_ahci_write(&ahci,16,8,original)); CHECK(!drift_ahci_close(&ahci));
    puts("qemu: AHCI identify/read/write/flush passed"); fflush(stdout);

    virtio(&block,2);
    CHECK(block.capacity>=64);
    CHECK(!drift_virtio_block(&block,24,8,original,0)); pattern(data,sizeof(data),123);
    CHECK(!drift_virtio_block(&block,24,8,data,1)); CHECK(!drift_virtio_flush(&block));
    CHECK(!drift_virtio_block(&block,24,8,copy,0)); CHECK(!memcmp(data,copy,sizeof(data)));
    CHECK(!drift_virtio_block(&block,24,8,original,1)); CHECK(!drift_virtio_close(&block));
    puts("qemu: Virtio block read/write/flush passed"); fflush(stdout);
}

static int peer(unsigned port)
{
    struct sockaddr_in address;
    int fd=socket(AF_INET,SOCK_STREAM,0);
    CHECK(fd>=0);
    memset(&address,0,sizeof(address)); address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK); address.sin_port=htons((uint16_t)port);
    CHECK(!connect(fd,(struct sockaddr *)&address,sizeof(address)));
    return fd;
}
static void transfer(int fd,uint8_t *bytes,size_t size,int write)
{
    while (size) {
        struct pollfd p={fd,write?POLLOUT:POLLIN,0};
        ssize_t n;
        CHECK(poll(&p,1,5000)>0);
        n=write?send(fd,bytes,size,MSG_NOSIGNAL):recv(fd,bytes,size,0);
        if (n<0 && errno==EINTR) continue;
        CHECK(n>0); bytes+=n; size-=(size_t)n;
    }
}
static void frame_send(int fd,uint8_t *bytes,uint32_t size)
{
    uint32_t wire=htonl(size); transfer(fd,(uint8_t *)&wire,4,1); transfer(fd,bytes,size,1);
}
static void frame_receive(int fd,uint8_t *bytes,uint32_t expected)
{
    uint32_t size; transfer(fd,(uint8_t *)&size,4,0); CHECK(ntohl(size)==expected); transfer(fd,bytes,expected,0);
}
static void ethernet(unsigned port)
{
    uint8_t frame[60],copy[1518];
    uint32_t got;
    int fd;
    drift_ethernet nic={0};
    drift_virtio vnet={0};
    for (unsigned kind=1;kind<=2;++kind) {
        uint32_t region=kind==1?bind_device(0x8086,0x100e,0,0):bind_device(0x10ec,0x8139,0,0);
        fd=peer(port+kind-1);
        CHECK(!drift_eth_init(&nic,&q.hardware,region,kind,1000000));
        pattern(frame,sizeof(frame),kind); memset(frame,255,6); memcpy(frame+6,nic.mac,6); frame[12]=0x88;frame[13]=0xb5;
        CHECK(!drift_eth_tx(&nic,frame,sizeof(frame))); frame_receive(fd,copy,sizeof(frame)); CHECK(!memcmp(frame,copy,sizeof(frame)));
        memcpy(frame,nic.mac,6); frame_send(fd,frame,sizeof(frame));
        int status=1;
        for (unsigned i=0;i<10000 && status==1;++i) { status=drift_eth_rx(&nic,copy,sizeof(copy),&got); q.hardware.wait_us(&q,100); }
        CHECK(!status && got==sizeof(frame) && !memcmp(frame,copy,sizeof(frame)));
        CHECK(!drift_eth_close(&nic)); close(fd); memset(&nic,0,sizeof(nic));
        printf("qemu: %s TX/RX passed\n",kind==1?"e1000":"RTL8139"); fflush(stdout);
    }
    fd=peer(port+2); virtio(&vnet,1);
    pattern(frame,sizeof(frame),19); memset(frame,255,6); memcpy(frame+6,vnet.mac,6); frame[12]=0x88;frame[13]=0xb5;
    CHECK(!drift_virtio_send(&vnet,frame,sizeof(frame))); frame_receive(fd,copy,sizeof(frame)); CHECK(!memcmp(frame,copy,sizeof(frame)));
    memcpy(frame,vnet.mac,6); frame_send(fd,frame,sizeof(frame));
    int received=0;
    for (unsigned i=0;i<10000 && !received;++i) { received=drift_virtio_receive(&vnet,copy,sizeof(copy)); q.hardware.wait_us(&q,100); }
    CHECK(received==(int)sizeof(frame) && !memcmp(frame,copy,sizeof(frame)));
    CHECK(!drift_virtio_close(&vnet)); close(fd);
    puts("qemu: Virtio network TX/RX passed"); fflush(stdout);
}
static void qmp_line(int fd,char *text,size_t capacity)
{
    size_t n=0;
    while (n+1<capacity) {
        transfer(fd,(uint8_t *)text+n,1,0);
        if (text[n++]=='\n') { text[n]=0; return; }
    }
    CHECK(0);
}
static int qmp_command(int fd,const char *command)
{
    char response[4096];
    transfer(fd,(uint8_t *)(uintptr_t)command,strlen(command),1);
    do { qmp_line(fd,response,sizeof(response)); } while (!strstr(response,"\"return\"") && !strstr(response,"\"error\""));
    if (strstr(response,"\"error\"")) fprintf(stderr,"QMP: %s",response);
    CHECK(!strstr(response,"\"error\""));
    if (strstr(command,"query-mice")) {
        char *name=strstr(response,"QEMU HID Mouse"),*begin,*index;
        CHECK(name);begin=name;
        while (begin>response && *begin!='{') --begin;
        index=strstr(begin,"\"index\"");CHECK(index && index<name);
        index=strchr(index,':');CHECK(index);
        return (int)strtol(index+1,NULL,10);
    }
    return -1;
}
static int qmp_connect(unsigned port)
{
    char response[4096];
    int fd=peer(port);
    qmp_line(fd,response,sizeof(response)); CHECK(strstr(response,"QMP"));
    qmp_command(fd,"{\"execute\":\"qmp_capabilities\"}\n");
    return fd;
}
static void key(int fd,int down)
{
    qmp_command(fd,down ?
        "{\"execute\":\"input-send-event\",\"arguments\":{\"events\":[{\"type\":\"key\",\"data\":{\"down\":true,\"key\":{\"type\":\"qcode\",\"data\":\"a\"}}}]}}\n" :
        "{\"execute\":\"input-send-event\",\"arguments\":{\"events\":[{\"type\":\"key\",\"data\":{\"down\":false,\"key\":{\"type\":\"qcode\",\"data\":\"a\"}}}]}}\n");
}
static void auxiliary(unsigned base,int qmp)
{
    drift_virtio rng={0},console={0},input={0}; uint8_t bytes[64],zero[64]={0};
    drift_virtio_input_event event;
    virtio(&rng,4); CHECK(drift_virtio_entropy(&rng,bytes,sizeof(bytes))==(int)sizeof(bytes));
    CHECK(memcmp(bytes,zero,sizeof(bytes))); CHECK(!drift_virtio_close(&rng));
    puts("qemu: Virtio entropy passed"); fflush(stdout);
    int fd=peer(base+5); virtio(&console,3);
    memcpy(bytes,"drift console",13);
    CHECK(drift_virtio_console_write(&console,bytes,13)==13);
    transfer(fd,zero,13,0); CHECK(!memcmp(bytes,zero,13));
    transfer(fd,bytes,13,1);
    int received=0;
    for (unsigned i=0;i<1000 && !received;++i) { received=drift_virtio_console_read(&console,zero,sizeof(zero)); q.hardware.wait_us(&q,100); }
    CHECK(received==13 && !memcmp(bytes,zero,13)); CHECK(!drift_virtio_close(&console));close(fd);
    puts("qemu: Virtio console TX/RX passed"); fflush(stdout);
    virtio(&input,18); key(qmp,1);
    int found=0;
    for (unsigned i=0;i<1000 && !found;++i) {
        int status=drift_virtio_input(&input,&event); CHECK(status>=0);
        found=status==1 && event.type==1 && event.code==30 && event.value==1;
        q.hardware.wait_us(&q,100);
    }
    CHECK(found); key(qmp,0); CHECK(!drift_virtio_close(&input));
    puts("qemu: Virtio input key event passed"); fflush(stdout);
}
static void usb(unsigned base,int qmp)
{
    drift_usb controller={0};
    uint32_t region=bind_device(0x1b36,0x000d,0,0),size;
    uint64_t blocks;
    uint8_t original[512],data[512],copy[512];
    int result=drift_usb_open(&controller,&q.hardware,region);
    if (result) fprintf(stderr,"USB open returned %d state=%u error=%u\n",result,controller.state,controller.error);
    CHECK(!result);
    result=drift_usb_attach(&controller,0);
    if (result) fprintf(stderr,"USB attach returned %d state=%u error=%u slot=%u port=%u\n",result,controller.state,controller.error,controller.slot,controller.port);
    CHECK(!result); CHECK(controller.kind==DRIFT_USB_STORAGE);
    CHECK(!drift_usb_storage_info(&controller,&blocks,&size)); CHECK(blocks>=64 && size==512);
    CHECK(!drift_usb_storage_read(&controller,32,1,original)); pattern(data,sizeof(data),41);
    CHECK(!drift_usb_storage_write(&controller,32,1,data)); CHECK(!drift_usb_storage_flush(&controller));
    CHECK(!drift_usb_storage_read(&controller,32,1,copy)); CHECK(!memcmp(data,copy,sizeof(data)));
    CHECK(!drift_usb_storage_write(&controller,32,1,original)); CHECK(!drift_usb_close(&controller));
    puts("qemu: xHCI/USB enumeration/BOT storage read/write/flush passed"); fflush(stdout);
    qmp_command(qmp,"{\"execute\":\"device_add\",\"arguments\":{\"driver\":\"usb-kbd\",\"id\":\"keyboard\",\"bus\":\"xkbd.0\"}}\n");
    memset(&controller,0,sizeof(controller)); region=bind_device(0x1b36,0x000d,0,1);
    CHECK(!drift_usb_open(&controller,&q.hardware,region)); CHECK(!drift_usb_attach(&controller,0));
    CHECK(controller.kind==DRIFT_USB_HID_KEYBOARD); key(qmp,1);
    int received=0;
    for (unsigned i=0;i<4 && !received;++i) {
        int status=drift_usb_hid(&controller,copy,sizeof(copy),&size); CHECK(!status || status==-11);
        if (!status && size==8 && copy[2]==4) received=1;
        q.hardware.wait_us(&q,100);
    }
    CHECK(received);key(qmp,0);CHECK(!drift_usb_close(&controller));
    puts("qemu: xHCI/USB HID keyboard report passed"); fflush(stdout);
    qmp_command(qmp,"{\"execute\":\"device_add\",\"arguments\":{\"driver\":\"usb-mouse\",\"id\":\"mouse\",\"bus\":\"xmouse.0\"}}\n");
    memset(&controller,0,sizeof(controller));region=bind_device(0x1b36,0x000d,0,2);
    CHECK(!drift_usb_open(&controller,&q.hardware,region));CHECK(!drift_usb_attach(&controller,0));
    CHECK(controller.kind==DRIFT_USB_HID_MOUSE);
    int index=qmp_command(qmp,"{\"execute\":\"query-mice\"}\n");
    char command[160];
    snprintf(command,sizeof(command),"{\"execute\":\"human-monitor-command\",\"arguments\":{\"command-line\":\"mouse_set %d\"}}\n",index);
    qmp_command(qmp,command);
    qmp_command(qmp,"{\"execute\":\"input-send-event\",\"arguments\":{\"events\":[{\"type\":\"rel\",\"data\":{\"axis\":\"x\",\"value\":7}}]}}\n");
    received=0;
    for (unsigned i=0;i<4 && !received;++i) {
        int status=drift_usb_hid(&controller,copy,sizeof(copy),&size);CHECK(!status || status==-11);
        if (!status && size>=3 && copy[1]==7) received=1;
        q.hardware.wait_us(&q,100);
    }
    CHECK(received);CHECK(!drift_usb_close(&controller));
    puts("qemu: xHCI/USB HID mouse report passed"); fflush(stdout);
    drift_usb_cdc cdc={0};
    memset(&controller,0,sizeof(controller));region=bind_device(0x1b36,0x000d,0,4);
    CHECK(!drift_usb_open(&controller,&q.hardware,region));
    CHECK(!drift_usb_attach_config(&controller,0,1));
    CHECK(!drift_usb_cdc_open(&cdc,&controller,DRIFT_USB_CDC_ECM,UINT32_MAX));
    int fd=peer(base+6);uint8_t frame[64],packet[2048];
    pattern(frame,sizeof(frame),99);memset(frame,255,6);memcpy(frame+6,cdc.mac,6);frame[12]=0x88;frame[13]=0xb5;
    CHECK(drift_usb_cdc_write(&cdc,frame,sizeof(frame))==(int)sizeof(frame));
    frame_receive(fd,packet,sizeof(frame));CHECK(!memcmp(frame,packet,sizeof(frame)));
    memcpy(frame,cdc.mac,6);frame_send(fd,frame,sizeof(frame));
    int length=0;
    for (unsigned i=0;i<4 && !length;++i) length=drift_usb_cdc_read(&cdc,packet,sizeof(packet));
    CHECK(length==(int)sizeof(frame) && !memcmp(frame,packet,sizeof(frame)));
    CHECK(!drift_usb_cdc_close(&cdc));CHECK(!drift_usb_close(&controller));close(fd);
    puts("qemu: xHCI/USB CDC-ECM Ethernet TX/RX passed");fflush(stdout);    memset(&controller,0,sizeof(controller));region=bind_device(0x1b36,0x000d,0,3);
    CHECK(!drift_usb_open(&controller,&q.hardware,region));CHECK(!drift_usb_attach(&controller,0));
    int hub_result=drift_usb_attach_hub(&controller,0);
    if (hub_result) fprintf(stderr,"hub attach returned %d kind=%u state=%u slot=%u\n",hub_result,controller.kind,controller.state,controller.slot);
    CHECK(!hub_result);CHECK(controller.kind==DRIFT_USB_STORAGE);
    CHECK(!drift_usb_storage_info(&controller,&blocks,&size));CHECK(blocks>=64 && size==512);
    CHECK(!drift_usb_storage_read(&controller,0,1,copy));
    CHECK(!drift_usb_close(&controller));
    puts("qemu: xHCI/USB hub child storage passed"); fflush(stdout);

}
int main(int argc,char **argv)
{
    int connected=-1;
    CHECK(argc==3);
    alarm(90);
    for (unsigned i=0;i<100 && connected;++i) {
        connected=drift_qtest_connect(&q,argv[1]);
        if (connected) { struct timespec delay={0,10000000}; nanosleep(&delay,NULL); }
    }
    CHECK(!connected);
    unsigned base=(unsigned)strtoul(argv[1],NULL,10);
    int qmp=qmp_connect(base+4);
    disks(); ethernet((unsigned)strtoul(argv[2],NULL,10)); auxiliary(base,qmp);
    usb(base,qmp);
    close(qmp);CHECK(!q.failed);
    for (uint32_t i=0;i<q.allocation_count;++i) CHECK(q.allocations[i].data==NULL);
    drift_qtest_disconnect(&q);
    puts("qemu: all native controller integration checks passed");
    return 0;
}
