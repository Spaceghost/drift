/* Generic owned-VM resources; no controller/descriptor/entropy logic.
 * SPDX-License-Identifier: MIT */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "virtio_qtest.h"
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <malloc.h>
typedef SOCKET socket_type;
#define BAD_SOCKET INVALID_SOCKET
#define CLOSE_SOCKET closesocket
#else
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
typedef int socket_type;
#define BAD_SOCKET (-1)
#define CLOSE_SOCKET close
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#endif

static void fail(drift_vqt *q,const char *message)
{
    if (!q->failed) snprintf(q->error,sizeof(q->error),"%s",message);
    q->failed=1;
}
static uint64_t monotonic(void *context)
{
    drift_vqt *q=context;
#ifdef _WIN32
    LARGE_INTEGER counter;
    if (!QueryPerformanceCounter(&counter) || !q->timer_frequency) { fail(q,"monotonic clock failed");return UINT64_MAX; }
    return (uint64_t)counter.QuadPart/q->timer_frequency*1000000+
        ((uint64_t)counter.QuadPart%q->timer_frequency)*1000000/q->timer_frequency;
#else
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC,&t)) { fail(q,"monotonic clock failed");return UINT64_MAX; }
    return (uint64_t)t.tv_sec*1000000+(uint64_t)t.tv_nsec/1000;
#endif
}
static int interrupted(void)
{
#ifdef _WIN32
    return WSAGetLastError()==WSAEINTR;
#else
    return errno==EINTR;
#endif
}
static int send_all(drift_vqt *q,const char *data,size_t size)
{
    while (size) {
#ifdef _WIN32
        int count=send((socket_type)q->socket,data,(int)size,0);
#else
        ssize_t count=send((socket_type)q->socket,data,size,MSG_NOSIGNAL);
#endif
        if (count<0 && interrupted()) continue;
        if (count<=0) return -1;
        data+=count;size-=(size_t)count;
    }
    return 0;
}
static int read_line(drift_vqt *q,char *text,size_t capacity)
{
    uint64_t start=monotonic(q);
    for (size_t used=0;used+1<capacity;) {
        uint64_t elapsed=monotonic(q)-start;
        if (elapsed>=10000000) return -1;
        uint64_t remaining=10000000-elapsed;
        struct timeval timeout={(long)(remaining/1000000),(long)(remaining%1000000)};
        fd_set input;FD_ZERO(&input);FD_SET((socket_type)q->socket,&input);
#ifdef _WIN32
        int ready=select(0,&input,NULL,NULL,&timeout);
#else
        int ready=select((socket_type)q->socket+1,&input,NULL,NULL,&timeout);
#endif
        if (ready<0 && interrupted()) continue;
        if (ready<=0) return -1;
        int count=(int)recv((socket_type)q->socket,text+used,1,0);
        if (count<0 && interrupted()) continue;
        if (count<=0) return -1;
        if (text[used++]=='\n') { text[used]=0;return 0; }
    }
    return -1;
}
static int command(drift_vqt *q,char *reply,size_t capacity,const char *format,...)
{
    char request[4096];va_list args;
    if (q->failed || q->socket==UINTPTR_MAX) return -1;
    va_start(args,format);int length=vsnprintf(request,sizeof(request),format,args);va_end(args);
    if (length<0 || (size_t)length>=sizeof(request)) { fail(q,"qtest request overflow");return -1; }
    if (send_all(q,request,(size_t)length)) { fail(q,"qtest send failed");return -1; }
    ++q->commands;
    do {
        if (read_line(q,reply,capacity)) { fail(q,"qtest receive failed/timed out");return -1; }
    } while (!strncmp(reply,"IRQ ",4));
    if (strncmp(reply,"OK",2) || (reply[2]!=' ' && reply[2]!='\n' && reply[2]!='\r')) {
        fail(q,"qtest rejected command");return -1;
    }
    return 0;
}
static uint64_t number(drift_vqt *q,const char *text)
{
    char *end;errno=0;uint64_t value=strtoull(text+2,&end,0);
    if (errno || end==text+2) { fail(q,"invalid qtest numeric reply");return UINT64_MAX; }
    while (*end==' ' || *end=='\r' || *end=='\n') ++end;
    if (*end) { fail(q,"trailing qtest numeric data");return UINT64_MAX; }
    return value;
}
static uint32_t config_read(void *context,uint32_t bdf,uint32_t offset)
{
    drift_vqt *q=context;char reply[128];
    if (bdf>65535 || offset>252 || (offset&3) || (q->bound_bdf!=UINT32_MAX && bdf!=q->bound_bdf)) {
        fail(q,"PCI config outside owned domain");return UINT32_MAX;
    }
    ++q->pci_reads;
    if (command(q,reply,sizeof(reply),"outl 0xcf8 0x%x\n",0x80000000u|(bdf<<8)|offset) ||
        command(q,reply,sizeof(reply),"inl 0xcfc\n")) return UINT32_MAX;
    uint64_t value=number(q,reply);
    if (value>UINT32_MAX) fail(q,"PCI config width overflow");
    return (uint32_t)value;
}
static void config_write(void *context,uint32_t bdf,uint32_t offset,uint32_t value)
{
    drift_vqt *q=context;char reply[128];
    if (bdf>65535 || offset>252 || (offset&3) || (q->bound_bdf!=UINT32_MAX && bdf!=q->bound_bdf)) {
        fail(q,"PCI config outside owned domain");return;
    }
    ++q->pci_writes;
    if (command(q,reply,sizeof(reply),"outl 0xcf8 0x%x\n",0x80000000u|(bdf<<8)|offset)) return;
    (void)command(q,reply,sizeof(reply),"outl 0xcfc 0x%x\n",value);
}
static const char *width_name(uint32_t width)
{
    return width==1?"b":width==2?"w":width==4?"l":width==8?"q":NULL;
}
static uint64_t register_read(void *context,uint32_t region,uint32_t offset,uint32_t width)
{
    drift_vqt *q=context;char reply[128];const char *name=width_name(width);
    if (region>=q->region_count || !name || offset>q->regions[region].size ||
        width>q->regions[region].size-offset || (q->regions[region].io && width==8)) {
        fail(q,"register read outside bound BAR");return UINT64_MAX;
    }
    ++q->register_reads;
    if (command(q,reply,sizeof(reply),"%s%s 0x%" PRIx64 "\n",q->regions[region].io?"in":"read",name,q->regions[region].base+offset)) return UINT64_MAX;
    uint64_t value=number(q,reply);
    if (width<8 && value>>(width*8)) fail(q,"register read width overflow");
    return value;
}
static void register_write(void *context,uint32_t region,uint32_t offset,uint32_t width,uint64_t value)
{
    drift_vqt *q=context;char reply[128];const char *name=width_name(width);
    if (region>=q->region_count || !name || offset>q->regions[region].size ||
        width>q->regions[region].size-offset || (q->regions[region].io && width==8) ||
        (width<8 && value>>(width*8))) { fail(q,"register write outside bound BAR");return; }
    ++q->register_writes;
    (void)command(q,reply,sizeof(reply),"%s%s 0x%" PRIx64 " 0x%" PRIx64 "\n",q->regions[region].io?"out":"write",name,q->regions[region].base+offset,value);
}
static unsigned nibble(char c)
{
    return c>='0'&&c<='9'?(unsigned)(c-'0'):c>='a'&&c<='f'?(unsigned)(c-'a'+10):c>='A'&&c<='F'?(unsigned)(c-'A'+10):256;
}
static int memory(drift_vqt *q,uint64_t address,uint8_t *bytes,uint32_t size,int write)
{
    static const char digits[]="0123456789abcdef";char payload[2049],reply[4096];
    while (size) {
        uint32_t count=size>1024?1024:size;
        if (write) {
            for (uint32_t i=0;i<count;++i) {payload[2*i]=digits[bytes[i]>>4];payload[2*i+1]=digits[bytes[i]&15];}
            payload[count*2]=0;
            if (command(q,reply,sizeof(reply),"write 0x%" PRIx64 " %u 0x%s\n",address,count,payload)) return -1;
        } else {
            if (command(q,reply,sizeof(reply),"read 0x%" PRIx64 " %u\n",address,count)) return -1;
            const char *p=reply+2;while (*p==' ') ++p;if (p[0]=='0'&&p[1]=='x')p+=2;
            if (strlen(p)<count*2) {fail(q,"short guest RAM reply");return -1;}
            for (uint32_t i=0;i<count;++i) {
                unsigned a=nibble(p[2*i]),b=nibble(p[2*i+1]);
                if (a>15||b>15) {fail(q,"invalid guest RAM hex");return -1;}
                bytes[i]=(uint8_t)((a<<4)|b);
            }
            p+=count*2;while (*p=='\r'||*p=='\n'||*p==' ')++p;
            if (*p) {fail(q,"trailing guest RAM data");return -1;}
        }
        address+=count;bytes+=count;size-=count;
    }
    return 0;
}
static void aligned_release(void *data)
{
#ifdef _WIN32
    _aligned_free(data);
#else
    free(data);
#endif
}
static int32_t allocate(void *context,uint32_t size,uint32_t alignment,drift_dma *out)
{
    drift_vqt *q=context;
    if (!out || !size || size>1048576 || !alignment || (alignment&(alignment-1)) ||
        alignment>1048576 || q->allocation_count==DRIFT_VQT_ALLOCS || q->failed) return -1;
    uint64_t address=(q->next_dma+alignment-1)&~(uint64_t)(alignment-1);
    if (address+size>UINT64_C(0x3000000)) return -1;
    size_t cpu_alignment=alignment<sizeof(void *)?sizeof(void *):alignment;void *data=NULL;
#ifdef _WIN32
    data=_aligned_malloc(size,cpu_alignment);
#else
    if (posix_memalign(&data,cpu_alignment,size)) data=NULL;
#endif
    if (!data) return -1;
    memset(data,0,size);
    if (memory(q,address,data,size,1)) {aligned_release(data);return -1;}
    *out=(drift_dma){data,address,size,q->allocation_count+1};
    q->allocations[q->allocation_count++]=*out;q->next_dma=address+size;
    ++q->allocated;++q->live;return 0;
}
static int mapping(drift_vqt *q,const drift_dma *buffer)
{
    if (!buffer || !buffer->token || buffer->token>q->allocation_count) return 0;
    const drift_dma *b=&q->allocations[buffer->token-1];
    return b->data && b->data==buffer->data && b->address==buffer->address && b->size==buffer->size;
}
static void release(void *context,drift_dma *buffer)
{
    drift_vqt *q=context;
    if (!mapping(q,buffer)) {fail(q,"invalid/double DMA release");return;}
    aligned_release(buffer->data);q->allocations[buffer->token-1].data=NULL;
    memset(buffer,0,sizeof(*buffer));++q->released;--q->live;
}
static void sync_dma(void *context,drift_dma *buffer,uint32_t offset,uint32_t size,uint32_t direction)
{
    drift_vqt *q=context;
    if (!mapping(q,buffer) || offset>buffer->size || size>buffer->size-offset || direction<1 || direction>3) {
        fail(q,"invalid DMA synchronization");return;
    }
    if (!size) return;
    if (direction==DRIFT_DMA_FROM_DEVICE) {
        if (!memory(q,buffer->address+offset,buffer->data+offset,size,0)) {
            atomic_thread_fence(memory_order_acquire);q->acquired_bytes+=size;
        }
    } else {
        atomic_thread_fence(memory_order_release);
        if (!memory(q,buffer->address+offset,buffer->data+offset,size,1)) q->published_bytes+=size;
    }
}
static void wait_us(void *context,uint32_t duration)
{
    drift_vqt *q=context;
#ifdef _WIN32
    Sleep((DWORD)(duration/1000+(duration%1000!=0)));
#else
    struct timespec pause={duration/1000000,(long)(duration%1000000)*1000};
    while (nanosleep(&pause,&pause)) if (errno!=EINTR) {fail(q,"monotonic wait failed");break;}
#endif
    (void)q;
}
int drift_vqt_connect(drift_vqt *q,uint16_t port)
{
    if (!q || !port) return -1;
    if (q->hardware.context) return -16;
    memset(q,0,sizeof(*q));q->socket=UINTPTR_MAX;q->bound_bdf=UINT32_MAX;
#ifdef _WIN32
    WSADATA data;LARGE_INTEGER frequency;
    if (WSAStartup(MAKEWORD(2,2),&data)) return -1;
    q->winsock=1;
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart<=0) {WSACleanup();q->winsock=0;return -1;}
    q->timer_frequency=(uint64_t)frequency.QuadPart;
    q->timer_resolution_ns=UINT64_C(1000000000)/q->timer_frequency+
        (UINT64_C(1000000000)%q->timer_frequency!=0);
#else
    struct timespec resolution;
    if (clock_getres(CLOCK_MONOTONIC,&resolution) || resolution.tv_sec<0 || resolution.tv_nsec<0) return -1;
    q->timer_resolution_ns=(uint64_t)resolution.tv_sec*UINT64_C(1000000000)+(uint64_t)resolution.tv_nsec;
    if (!q->timer_resolution_ns) return -1;
#endif
    struct sockaddr_in address;memset(&address,0,sizeof(address));address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=htons(port);
    uint64_t started=monotonic(q);socket_type sock;
    for (;;) {
        sock=socket(AF_INET,SOCK_STREAM,0);
        if (sock==BAD_SOCKET) goto failed;
#ifndef _WIN32
        if (sock>=FD_SETSIZE) {CLOSE_SOCKET(sock);goto failed;}
#endif
        if (!connect(sock,(struct sockaddr *)&address,sizeof(address))) break;
        CLOSE_SOCKET(sock);
        if (monotonic(q)-started>=10000000) goto failed;
        wait_us(q,10000);
    }
    int yes=1;
    if (setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,(const char *)&yes,sizeof(yes))) {CLOSE_SOCKET(sock);goto failed;}
    q->socket=(uintptr_t)sock;q->next_dma=UINT64_C(0x1000000);q->next_mmio=UINT64_C(0xc0000000);
    q->hardware=(drift_hw){q,register_read,register_write,allocate,release,sync_dma,monotonic,wait_us};
    q->pci=(drift_pci){q,config_read,config_write};return 0;
failed:
#ifdef _WIN32
    if (q->winsock) WSACleanup();
    q->winsock=0;
#endif
    fail(q,"qtest TCP connection failed");return -1;
}
int drift_vqt_bind(drift_vqt *q,uint16_t vendor,uint16_t device,unsigned instance,uint32_t *bdf,drift_virtio_bar bars[6])
{
    if (!q || !bdf || !bars || q->failed || q->bound_bdf!=UINT32_MAX) return -1;
    drift_pci_device found;
    if (drift_pci_find(&q->pci,0,0,vendor,device,instance,&found)) return -1;
    q->bound_bdf=found.bdf;
    uint32_t original_command=config_read(q,found.bdf,4)&65535;
    config_write(q,found.bdf,4,original_command&~7u);
    unsigned used_io=0,used_mmio=0;
    for (unsigned i=0;i<6;++i) bars[i]=(drift_virtio_bar){UINT32_MAX,0};
    for (unsigned index=0;index<6;++index) {
        uint32_t offset=16+4*index,original=config_read(q,found.bdf,offset),high=0;
        unsigned io=original&1,wide=!io && (original&6)==4;
        if (wide && index==5) {fail(q,"truncated 64-bit BAR");return -1;}
        if (!io && (original&6)!=0 && !wide) {fail(q,"unsupported memory BAR type");return -1;}
        if (wide) high=config_read(q,found.bdf,offset+4);
        config_write(q,found.bdf,offset,UINT32_MAX);
        if (wide) config_write(q,found.bdf,offset+4,UINT32_MAX);
        uint32_t low_mask=config_read(q,found.bdf,offset),high_mask=wide?config_read(q,found.bdf,offset+4):0;
        config_write(q,found.bdf,offset,original);if(wide)config_write(q,found.bdf,offset+4,high);
        uint64_t masked=low_mask&(io?~3u:~15u),size;
        if (!masked && !high_mask) {if(wide)++index;continue;}
        if (wide) {masked|=(uint64_t)high_mask<<32;size=~masked+1;}
        else size=(uint32_t)(~(uint32_t)masked+1);
        if (!size || (size&(size-1)) || size>0x1000000 || q->region_count==DRIFT_VQT_REGIONS) {
            fail(q,"unsupported/invalid BAR size");return -1;
        }
        uint64_t base;
        if (io) {base=0x2000+q->region_count*0x1000;if(size>0x1000 || base+size>65536) {fail(q,"IO BAR allocation exhausted");return -1;}}
        else {base=(q->next_mmio+size-1)&~(size-1);if(base+size>UINT64_C(0xd0000000)) {fail(q,"MMIO allocation exhausted");return -1;}q->next_mmio=base+size;}
        config_write(q,found.bdf,offset,(uint32_t)base|(original&(io?3:15)));
        if (wide) config_write(q,found.bdf,offset+4,(uint32_t)(base>>32));
        if (config_read(q,found.bdf,offset)!=((uint32_t)base|(original&(io?3:15)))) {fail(q,"BAR assignment readback mismatch");return -1;}
        bars[index]=(drift_virtio_bar){q->region_count,size};
        q->regions[q->region_count++]=(drift_vqt_region){base,size,io};
        used_io|=io;used_mmio|=!io;if(wide)++index;
    }
    config_write(q,found.bdf,4,original_command|4|(used_io?1:0)|(used_mmio?2:0));
    *bdf=found.bdf;return q->failed?-1:0;
}
int drift_vqt_dispose(drift_vqt *q,int vm_stopped)
{
    if (!q) return -1;
    if (q->live && !vm_stopped) return -16;
    if (vm_stopped) for (unsigned i=0;i<q->allocation_count;++i) if(q->allocations[i].data) {
        aligned_release(q->allocations[i].data);q->allocations[i].data=NULL;--q->live;++q->released;
    }
    if (q->socket!=UINTPTR_MAX) {CLOSE_SOCKET((socket_type)q->socket);q->socket=UINTPTR_MAX;}
#ifdef _WIN32
    if (q->winsock) {WSACleanup();q->winsock=0;}
#endif
    return 0;
}
int drift_vqt_failed(const drift_vqt *q) {return !q || q->failed;}
