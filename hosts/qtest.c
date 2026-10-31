/* Generic resources implemented using QEMU's test protocol. No device logic.
 * SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L
#include "drift_qtest.h"
#include <errno.h>
#include <arpa/inet.h>
#include <time.h>
#include <inttypes.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static int send_all(int fd, const char *data, size_t size)
{
    while (size) {
        ssize_t sent = send(fd, data, size, MSG_NOSIGNAL);
        if (sent < 0 && errno == EINTR) continue;
        if (sent <= 0) return -1;
        data += sent; size -= (size_t)sent;
    }
    return 0;
}

static int line(int fd, char *data, size_t capacity)
{
    size_t count = 0;
    while (count + 1 < capacity) {
        struct pollfd input = {fd, POLLIN, 0};
        int ready;
        ssize_t received;
        do ready = poll(&input, 1, 10000); while (ready < 0 && errno == EINTR);
        if (ready <= 0) return -1;
        received = recv(fd, data + count, 1, 0);
        if (received < 0 && errno == EINTR) continue;
        if (received <= 0) return -1;
        if (data[count++] == '\n') { data[count] = 0; return 0; }
    }
    return -1;
}

static int command(drift_qtest *q, char *response, size_t capacity, const char *format, ...)
{
    char request[8192];
    va_list arguments;
    int size;
    if (q->failed) return -1;
    va_start(arguments, format);
    size = vsnprintf(request, sizeof(request), format, arguments);
    va_end(arguments);
    if (size < 0 || (size_t)size >= sizeof(request) || send_all(q->fd, request, (size_t)size)) goto failed;
    do {
        if (line(q->fd, response, capacity)) goto failed;
    } while (!strncmp(response, "IRQ ", 4));
    if (!strncmp(response, "OK", 2)) return 0;
    fprintf(stderr, "qtest command failed: %s%s", request, response);
failed:
    q->failed = 1;
    return -1;
}

static uint64_t reg_read(void *context, uint32_t region, uint32_t offset, uint32_t width)
{
    drift_qtest *q = context;
    drift_qtest_region *r;
    char response[128];
    const char *operation;
    if (region >= q->region_count) goto failed;
    r = &q->regions[region];
    if (offset > r->size || width > r->size-offset) goto failed;
    operation = width == 1 ? "b" : width == 2 ? "w" : width == 4 ? "l" : width == 8 ? "q" : NULL;
    if (!operation || (r->io && width == 8)) goto failed;
    if (command(q, response, sizeof(response), "%s%s 0x%" PRIx64 "\n", r->io ? "in" : "read", operation, r->base+offset)) return UINT64_MAX;
    return strtoull(response+2, NULL, 0);
failed:
    fprintf(stderr, "qtest: invalid register read region=%u off=%x width=%u\n", region, offset, width);
    q->failed = 1;
    return UINT64_MAX;
}

static void reg_write(void *context, uint32_t region, uint32_t offset, uint32_t width, uint64_t value)
{
    drift_qtest *q = context;
    drift_qtest_region *r;
    char response[128];
    const char *operation;
    if (region >= q->region_count) goto failed;
    r = &q->regions[region];
    if (offset > r->size || width > r->size-offset) goto failed;
    operation = width == 1 ? "b" : width == 2 ? "w" : width == 4 ? "l" : width == 8 ? "q" : NULL;
    if (!operation || (r->io && width == 8)) goto failed;
    (void)command(q, response, sizeof(response), "%s%s 0x%" PRIx64 " 0x%" PRIx64 "\n", r->io ? "out" : "write", operation, r->base+offset, value);
    return;
failed:
    fprintf(stderr, "qtest: invalid register write region=%u off=%x width=%u\n", region, offset, width);
    q->failed = 1;
}

static unsigned digit(char c)
{
    if (c >= '0' && c <= '9') return (unsigned)(c-'0');
    if (c >= 'a' && c <= 'f') return (unsigned)(c-'a'+10);
    if (c >= 'A' && c <= 'F') return (unsigned)(c-'A'+10);
    return 256;
}

int drift_qtest_memory(drift_qtest *q, uint64_t address, uint8_t *bytes, uint32_t length, int write)
{
    static const char hex[] = "0123456789abcdef";
    char data[2049], response[4096];
    while (length) {
        uint32_t count = length > 1024 ? 1024 : length;
        if (write) {
            for (uint32_t i=0; i<count; ++i) { data[2*i]=hex[bytes[i]>>4]; data[2*i+1]=hex[bytes[i]&15]; }
            data[2*count]=0;
            if (command(q,response,sizeof(response),"write 0x%" PRIx64 " %u 0x%s\n",address,count,data)) return -1;
        } else {
            char *p;
            if (command(q,response,sizeof(response),"read 0x%" PRIx64 " %u\n",address,count)) return -1;
            p = response+2;
            while (*p == ' ') ++p;
            if (p[0]=='0' && p[1]=='x') p+=2;
            if (strlen(p) < 2*count) goto failed;
            for (uint32_t i=0; i<count; ++i) {
                unsigned a=digit(p[2*i]), b=digit(p[2*i+1]);
                if (a>15 || b>15) goto failed;
                bytes[i]=(uint8_t)((a<<4)|b);
            }
        }
        address+=count; bytes+=count; length-=count;
    }
    return 0;
failed:
    q->failed=1;
    return -1;
}

static int32_t dma_allocate(void *context, uint32_t size, uint32_t alignment, drift_dma *out)
{
    drift_qtest *q=context;
    uint64_t address;
    if (!size || !alignment || (alignment & (alignment-1)) || q->allocation_count==DRIFT_QTEST_ALLOCS) return -1;
    address=(q->next_dma+alignment-1)&~(uint64_t)(alignment-1);
    if (address+size > UINT64_C(0x7000000)) return -1;
    memset(out,0,sizeof(*out));
    if (posix_memalign((void **)&out->data, alignment < sizeof(void *) ? sizeof(void *) : alignment, size)) return -1;
    memset(out->data,0,size);
    out->address=address; out->size=size; out->token=q->allocation_count+1;
    q->allocations[q->allocation_count++]=*out;
    q->next_dma=address+size;
    return drift_qtest_memory(q,address,out->data,size,1);
}

static void dma_release(void *context, drift_dma *buffer)
{
    drift_qtest *q=context;
    if (!buffer->token || buffer->token>q->allocation_count || q->allocations[buffer->token-1].data != buffer->data) { q->failed=1; return; }
    free(buffer->data);
    q->allocations[buffer->token-1].data=NULL;
    memset(buffer,0,sizeof(*buffer));
}

static void dma_sync(void *context, drift_dma *buffer, uint32_t offset, uint32_t length, uint32_t direction)
{
    drift_qtest *q=context;
    if (offset>buffer->size || length>buffer->size-offset || direction<1 || direction>3) { q->failed=1; return; }
    if (direction != DRIFT_DMA_FROM_DEVICE) (void)drift_qtest_memory(q,buffer->address+offset,buffer->data+offset,length,1);
    if (direction == DRIFT_DMA_FROM_DEVICE) (void)drift_qtest_memory(q,buffer->address+offset,buffer->data+offset,length,0);
}

static uint64_t time_us(void *context)
{
    drift_qtest *q=context;
    struct timespec now;
    if (q->clock) return q->now;
    if (clock_gettime(CLOCK_MONOTONIC,&now)) { q->failed=1; return UINT64_MAX; }
    return (uint64_t)now.tv_sec*1000000+(uint64_t)now.tv_nsec/1000;
}
static void wait_us(void *context, uint32_t duration)
{
    drift_qtest *q=context;
    char response[128];
    q->now+=duration;
    if (q->clock) (void)command(q,response,sizeof(response),"clock_step %" PRIu64 "\n",(uint64_t)duration*1000);
    else { struct timespec pause={duration/1000000,(long)(duration%1000000)*1000}; (void)nanosleep(&pause,NULL); }
}

int drift_qtest_connect(drift_qtest *q, const char *path)
{
    struct sockaddr_in address;
    char *end;
    unsigned long port=strtoul(path,&end,10);
    memset(q,0,sizeof(*q)); q->fd=-1;
    if (!*path || *end || !port || port>65535) return -1;
    q->fd=socket(AF_INET,SOCK_STREAM,0);
    if (q->fd<0) return -1;
    memset(&address,0,sizeof(address)); address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK); address.sin_port=htons((uint16_t)port);
    if (connect(q->fd,(struct sockaddr *)&address,sizeof(address))) { close(q->fd); q->fd=-1; return -1; }
    q->clock=0; q->next_dma=UINT64_C(0x1000000);
    q->hardware=(drift_hw){q,reg_read,reg_write,dma_allocate,dma_release,dma_sync,time_us,wait_us};
    return 0;
}

void drift_qtest_disconnect(drift_qtest *q)
{
    for (uint32_t i=0;i<q->allocation_count;++i) free(q->allocations[i].data);
    if (q->fd>=0) close(q->fd);
    q->fd=-1;
}

uint32_t drift_qtest_pci_read(drift_qtest *q,uint32_t bdf,uint32_t offset)
{
    char response[128];
    if (command(q,response,sizeof(response),"outl 0xcf8 0x%x\n",0x80000000u|bdf|(offset&252))) return UINT32_MAX;
    if (command(q,response,sizeof(response),"inl 0xcfc\n")) return UINT32_MAX;
    return (uint32_t)strtoul(response+2,NULL,0);
}
void drift_qtest_pci_write(drift_qtest *q,uint32_t bdf,uint32_t offset,uint32_t value)
{
    char response[128];
    if (command(q,response,sizeof(response),"outl 0xcf8 0x%x\n",0x80000000u|bdf|(offset&252))) return;
    (void)command(q,response,sizeof(response),"outl 0xcfc 0x%x\n",value);
}
int drift_qtest_pci_find(drift_qtest *q,uint16_t vendor,uint16_t device,unsigned instance,uint32_t *bdf)
{
    for (unsigned slot=0;slot<32;++slot) for (unsigned function=0;function<8;++function) {
        uint32_t at=(slot<<11)|(function<<8), id=drift_qtest_pci_read(q,at,0);
        if ((uint16_t)id==vendor && (uint16_t)(id>>16)==device && !instance--) { *bdf=at; return 0; }
    }
    return -1;
}
int drift_qtest_pci_bar(drift_qtest *q,uint32_t bdf,unsigned bar,uint32_t *region)
{
    uint32_t offset=16+4*bar, original, mask, high=0, size, assigned;
    int io, wide;
    if (bar>5 || q->region_count==DRIFT_QTEST_REGIONS) return -1;
    original=drift_qtest_pci_read(q,bdf,offset);
    io=(original&1)!=0; wide=!io && (original&6)==4;
    if (wide && bar==5) return -1;
    if (wide) high=drift_qtest_pci_read(q,bdf,offset+4);
    drift_qtest_pci_write(q,bdf,offset,UINT32_MAX);
    mask=drift_qtest_pci_read(q,bdf,offset);
    drift_qtest_pci_write(q,bdf,offset,original);
    size=~(mask & (io ? ~3u : ~15u))+1;
    if (!size || size>0x100000) return -1;
    assigned=io ? 0x2000+q->region_count*0x1000 : 0xc0000000+q->region_count*0x100000;
    if (assigned&(size-1)) return -1;
    drift_qtest_pci_write(q,bdf,offset,assigned|(original&(io?3:15)));
    if (wide) drift_qtest_pci_write(q,bdf,offset+4,0);
    (void)high;
    drift_qtest_pci_write(q,bdf,4,(drift_qtest_pci_read(q,bdf,4)&0xffffu)|7u);
    *region=q->region_count;
    q->regions[q->region_count++]=(drift_qtest_region){assigned,size,io};
    return q->failed ? -1 : 0;
}

int drift_qtest_pci_regions(drift_qtest *q,uint32_t bdf,uint32_t regions[6],uint32_t sizes[6])
{
    for (unsigned i=0;i<6;++i) { regions[i]=UINT32_MAX; sizes[i]=0; }
    for (unsigned bar=0;bar<6;++bar) {
        uint32_t original=drift_qtest_pci_read(q,bdf,16+4*bar);
        int wide=!(original&1) && (original&6)==4;
        if (!drift_qtest_pci_bar(q,bdf,bar,&regions[bar]))
            sizes[bar]=q->regions[regions[bar]].size;
        if (q->failed) return -1;
        if (wide) ++bar;
    }
    return 0;
}
