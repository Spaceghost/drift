/* Actual MINIX device/read-loop comparison; each personality is a separate ELF. */
/* SPDX-License-Identifier: MIT */
#include <minix/drivers.h>
#include <minix/sef.h>
#include <minix/endpoint.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "resources.h"
#include "fixture.h"
#ifndef DRIFT_BENCH_EVENTS
#define DRIFT_BENCH_EVENTS 4096
#endif
#if defined(DRIFT_BENCH_NATIVE)
static drift_virtio transport;
#define PERSONALITY "native-nelua"
#elif defined(DRIFT_BENCH_LINUX)
#include "drift_linux_virtio_rng.h"
static drift_virtio transport;
static drift_linux_virtio_rng profile;
#define PERSONALITY "imported-linux"
#elif defined(DRIFT_BENCH_WINDOWS)
#include "drift_windows_viorng.h"
static drift_win_rng profile;
static drift_win_rng_request request;
static unsigned notifications;
static void completed(void *p,drift_win_rng_request *r){(void)p;if(r==&request)++notifications;}
static drift_win_rng_host client={NULL,completed};
#define PERSONALITY "imported-windows"
#else
#error choose one benchmark personality
#endif
static minix_resources host;
static int serial_fd=-1;
static void log_line(const char *format,...) {
    char b[512];va_list ap;va_start(ap,format);int n=vsnprintf(b,sizeof(b),format,ap);va_end(ap);
    if(n<=0||n>=(int)sizeof(b))return;
    for(int i=0;i<n;){ssize_t r=write(serial_fd,b+i,n-i);if(r<=0)return;i+=(int)r;}
}
static int read64(uint8_t *out) {
#if defined(DRIFT_BENCH_NATIVE)
    int n=drift_virtio_entropy(&transport,out,64);return n==64?0:(n<0?n:-5);
#elif defined(DRIFT_BENCH_LINUX)
    int n=drift_linux_virtio_rng_read(&profile,out,64,1);return n==64?0:(n<0?n:-5);
#else
    int r;if(request.phase==DRIFT_WIN_RNG_COMPLETE){r=drift_windows_viorng_request_reset(&request);if(r)return r;}
    r=drift_windows_viorng_read(&profile,&request,out,64,64);
    if(r!=DRIFT_WIN_RNG_PENDING&&r!=DRIFT_WIN_RNG_SUCCESS)return r;
    uint64_t start=host.hw.time_us(&host);
    while(request.phase!=DRIFT_WIN_RNG_COMPLETE) {
        r=drift_windows_viorng_pump(&profile);if(r<0)return r;
        if(host.hw.time_us(&host)-start>3000000)return -110;
        if(request.phase!=DRIFT_WIN_RNG_COMPLETE)host.hw.wait_us(&host,10);
    }
    return request.status?request.status:((request.information==64&&request.completions==1)?0:-5);
#endif
}
static int run(void) {
    int r=0,closed=0;struct timespec resolution;
    uint8_t guarded[96];uint8_t *output=guarded+16;memset(guarded,0xa5,sizeof(guarded));
    uint32_t expected=DRIFT_FIXTURE_SEED;uint64_t digest=UINT64_C(14695981039346656037);
#define TRY(call) do{r=(call);if(r){log_line("DRIFT_MINIX_DMA_BENCH_FAIL kind=%s line=%u code=%d\n",PERSONALITY,__LINE__,r);goto done;}}while(0)
    TRY(minix_resources_open(&host));
    TRY(clock_getres(CLOCK_MONOTONIC,&resolution));
    uint64_t resolution_ns=(uint64_t)resolution.tv_sec*1000000000u+resolution.tv_nsec;
    if(!resolution_ns){r=-5;goto done;}
#if defined(DRIFT_BENCH_NATIVE)
    TRY(drift_virtio_open_function(&transport,&host.hw,&host.pci,host.bdf,host.bars));
#elif defined(DRIFT_BENCH_LINUX)
    TRY(drift_virtio_raw_open_function(&transport,&host.hw,&host.pci,host.bdf,host.bars,4,1));
    TRY(drift_linux_virtio_rng_open(&profile,&transport));
#else
    TRY(drift_windows_viorng_open_function(&profile,&host.hw,&host.pci,host.bdf,host.bars,&client));
#endif
    TRY(read64(output));
    if(fixture_compare(&expected,output,64)){r=-5;goto done;}
    uint64_t start=host.hw.time_us(&host);
    for(unsigned i=0;i<DRIFT_BENCH_EVENTS;i++) {
        TRY(read64(output));
        if(fixture_compare(&expected,output,64)){r=-5;goto done;}
        digest=fixture_hash(digest,output,64);
        for(unsigned j=0;j<16;j++)if(guarded[j]!=0xa5||guarded[80+j]!=0xa5){r=-5;goto done;}
    }
    uint64_t elapsed=host.hw.time_us(&host)-start;
    if(!elapsed||elapsed<=(resolution_ns+999)/1000){log_line("DRIFT_MINIX_DMA_BENCH_REJECT kind=%s elapsed_us=%llu resolution_ns=%llu\n",PERSONALITY,(unsigned long long)elapsed,(unsigned long long)resolution_ns);r=-5;goto done;}
#if defined(DRIFT_BENCH_WINDOWS)
    if(notifications!=DRIFT_BENCH_EVENTS+1){r=-5;goto done;}
    drift_virtio *device=&profile.transport;
#else
    drift_virtio *device=&transport;
#endif
    unsigned status=(unsigned)host.hw.read(&host,device->region,device->common_offset+20,1);
    if(status!=15||device->features){r=-5;goto done;}
    log_line("DRIFT_MINIX_DMA_BENCH kind=%s events=%u bytes=%u elapsed_us=%llu resolution_ns=%llu fnv64=%016llx posted=%u features_low=%u status=%u poll_wait_us=10\n",PERSONALITY,DRIFT_BENCH_EVENTS,DRIFT_BENCH_EVENTS*64,(unsigned long long)elapsed,(unsigned long long)resolution_ns,(unsigned long long)digest,device->queue[0].available,device->features,status);
    r=0;
done:
#if defined(DRIFT_BENCH_NATIVE)
    closed=transport.hw?drift_virtio_close(&transport):0;
#elif defined(DRIFT_BENCH_LINUX)
    closed=profile.phase?drift_linux_virtio_rng_close(&profile):(transport.hw?drift_virtio_close(&transport):0);
#else
    closed=profile.phase?drift_windows_viorng_close(&profile):0;
#endif
    if(closed){log_line("DRIFT_MINIX_DMA_BENCH_QUARANTINE kind=%s code=%d\n",PERSONALITY,closed);return closed;}
    closed=minix_resources_close(&host);if(!r)r=closed;
    log_line("DRIFT_MINIX_DMA_BENCH_CLEANUP kind=%s allocations=%u releases=%u fault=%u\n",PERSONALITY,host.allocations,host.releases,host.fault);
    if(!r)log_line("DRIFT_MINIX_DMA_BENCH_PASS kind=%s\n",PERSONALITY);
    return r;
}
int main(int argc,char **argv) {
    message m;env_setargs(argc,argv);sef_startup();serial_fd=open("/dev/tty00",O_WRONLY);
    int r=serial_fd<0?-5:run();int fd=open("/tmp/drift-rng.status",O_WRONLY|O_CREAT|O_TRUNC,0600);
    if(fd>=0){const char *s=r?"FAIL\n":"PASS\n";write(fd,s,5);close(fd);}
    for(;;)sef_receive(ANY,&m);
}
