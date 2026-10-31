/* Separate source-profile executables; never links the two import licenses. */
/* SPDX-License-Identifier: MIT */
#include <minix/drivers.h>
#include <minix/sef.h>
#include <minix/endpoint.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "resources.h"
#include "fixture.h"
#if defined(DRIFT_PROFILE_LINUX)
#include "drift_linux_virtio_rng.h"
static drift_virtio transport;
static drift_linux_virtio_rng profile;
#define PERSONALITY "linux"
#elif defined(DRIFT_PROFILE_WINDOWS)
#include "drift_windows_viorng.h"
static drift_win_rng profile;
static drift_win_rng_request request;
static unsigned notifications;
static void completed(void *context,drift_win_rng_request *r) {
    (void)context;if(r==&request)++notifications;
}
static drift_win_rng_host client={NULL,completed};
#define PERSONALITY "windows"
#else
#error choose exactly one source personality
#endif
static minix_resources host;
static int serial_fd=-1;
static void log_line(const char *format,...) {
    char b[512];va_list ap;va_start(ap,format);int n=vsnprintf(b,sizeof(b),format,ap);va_end(ap);
    if(n<=0||n>=(int)sizeof(b))return;
    for(int i=0;i<n;){ssize_t r=write(serial_fd,b+i,n-i);if(r<=0)return;i+=(int)r;}
}
static int run(void) {
    int r=0,closed=0;uint8_t guarded[544];
    uint32_t expected=DRIFT_FIXTURE_SEED;
    uint64_t digest=UINT64_C(14695981039346656037);
    uint8_t *output=guarded+16;memset(guarded,0xa5,sizeof(guarded));
#define TRY(call) do{r=(call);if(r){log_line("DRIFT_MINIX_RNG_PROFILE_FAIL kind=%s line=%u code=%d fault=%u\n",PERSONALITY,__LINE__,r,host.fault);goto done;}}while(0)
    TRY(minix_resources_open(&host));
    log_line("DRIFT_MINIX_RNG_PROFILE_PCI kind=%s bdf=%u vendor_device=%08x maps=%u\n",PERSONALITY,host.bdf,host.pci.read(&host,host.bdf,0),host.maps);
#if defined(DRIFT_PROFILE_LINUX)
    TRY(drift_virtio_raw_open_function(&transport,&host.hw,&host.pci,host.bdf,host.bars,4,1));
    TRY(drift_linux_virtio_rng_open(&profile,&transport));
    r=drift_linux_virtio_rng_read(&profile,output,512,1);
    if(r!=(int)512){log_line("DRIFT_MINIX_RNG_PROFILE_READ_FAIL kind=linux count=%d\n",r);r=-5;goto done;}
#else
    r=drift_windows_viorng_open_function(&profile,&host.hw,&host.pci,host.bdf,host.bars,&client);
    log_line("DRIFT_MINIX_RNG_PROFILE_STATE phase=%u configured=%u prepared=%u powered=%u error=%d last_error=%d transport=%u callbacks=%u\n",profile.phase,profile.configured,profile.prepared,profile.powered,profile.error,profile.last_error,profile.transport.state,profile.callbacks);
    if(r)goto done;
    r=drift_windows_viorng_read(&profile,&request,output,512,512);
    if(r!=DRIFT_WIN_RNG_PENDING&&r!=DRIFT_WIN_RNG_SUCCESS)goto done;
    uint64_t start=host.hw.time_us(&host);
    while(request.phase!=DRIFT_WIN_RNG_COMPLETE) {
        r=drift_windows_viorng_pump(&profile);if(r<0)goto done;
        if(host.hw.time_us(&host)-start>3000000){r=-5;goto done;}
        if(request.phase!=DRIFT_WIN_RNG_COMPLETE)host.hw.wait_us(&host,100);
    }
    if(request.status||request.information!=512||request.completions!=1||notifications!=1){r=-5;goto done;}
    log_line("DRIFT_MINIX_RNG_PROFILE_WDF completions=%u notifications=%u information=%lu\n",request.completions,notifications,(unsigned long)request.information);
#endif
    if(fixture_compare(&expected,output,512)){r=-5;goto done;}
    digest=fixture_hash(digest,output,512);
    for(unsigned i=0;i<16;i++)if(guarded[i]!=0xa5||guarded[528+i]!=0xa5){r=-5;goto done;}
    log_line("DRIFT_MINIX_RNG_PROFILE_DATA kind=%s bytes=512 exact_fixture=1 hardware_irq=0 canaries=1\n",PERSONALITY);
#if defined(DRIFT_PROFILE_LINUX)
    uint64_t prefetch_start=host.hw.time_us(&host);
    do {
        r=drift_linux_virtio_rng_service(&profile);if(r<0)goto done;
        if(host.hw.time_us(&host)-prefetch_start>3000000){r=-5;goto done;}
        if(!r)host.hw.wait_us(&host,100);
    }while(!r);
    if(r!=1){r=-5;goto done;}
    for(unsigned i=0;i<64;i++)fixture_next(&expected);
    log_line("DRIFT_MINIX_RNG_PROFILE_PREFETCH kind=linux delivered=1 discarded_bytes=64 restored_absolute_offset=576\n");
    TRY(drift_linux_virtio_rng_freeze(&profile));
    if(host.allocations!=host.releases||transport.state!=0){r=-5;goto done;}
    TRY(drift_virtio_raw_open_function(&transport,&host.hw,&host.pci,host.bdf,host.bars,4,1));
    TRY(drift_linux_virtio_rng_restore(&profile,&transport));
    memset(output,0xa5,512);
    r=drift_linux_virtio_rng_read(&profile,output,64,1);
    if(r!=64){r=-5;goto done;}
    if(fixture_compare(&expected,output,64)){r=-5;goto done;}
    digest=fixture_hash(digest,output,64);
    for(unsigned i=64;i<512;i++)if(output[i]!=0xa5){r=-5;goto done;}
    TRY(drift_linux_virtio_rng_cancel(&profile));
    r=drift_linux_virtio_rng_read(&profile,output,64,1);
    if(r!=-4){r=-5;goto done;}
    log_line("DRIFT_MINIX_RNG_PROFILE_LIFECYCLE kind=linux freeze_restore=1 restored_bytes=64 restored_absolute_offset=576 exact_fixture=1 cancel_next=-4\n");
#else
    TRY(drift_windows_viorng_request_reset(&request));notifications=0;memset(output,0xa5,512);
    r=drift_windows_viorng_read(&profile,&request,output,512,64);
    if(r!=DRIFT_WIN_RNG_PENDING){r=-5;goto done;}
    TRY(drift_windows_viorng_cancel(&profile,&request));
    if(request.phase!=DRIFT_WIN_RNG_COMPLETE||request.status!=DRIFT_WIN_RNG_CANCELLED||request.information||request.completions!=1||notifications!=1||!profile.transfer.active||host.releases){r=-5;goto done;}
    for(unsigned i=0;i<512;i++)if(output[i]!=0xa5){r=-5;goto done;}
    log_line("DRIFT_MINIX_RNG_PROFILE_LIFECYCLE kind=windows cancel_before_dispatch=1 cancelled_once=1 output_unchanged=1 dma_retained=1\n");
#endif
    for(unsigned i=0;i<16;i++)if(guarded[i]!=0xa5||guarded[528+i]!=0xa5){r=-5;goto done;}
    log_line("DRIFT_MINIX_RNG_PROFILE_HASH kind=%s fnv64=%016llx\n",PERSONALITY,(unsigned long long)digest);
    r=0;
done:
    if(r)log_line("DRIFT_MINIX_RNG_PROFILE_FAIL kind=%s code=%d fault=%u\n",PERSONALITY,r,host.fault);
#if defined(DRIFT_PROFILE_LINUX)
    closed=profile.phase?drift_linux_virtio_rng_close(&profile):(transport.hw?drift_virtio_close(&transport):0);
#else
    closed=profile.phase?drift_windows_viorng_close(&profile):0;
#endif
    if(closed){log_line("DRIFT_MINIX_RNG_PROFILE_QUARANTINE kind=%s reset=%d allocations=%u releases=%u\n",PERSONALITY,closed,host.allocations,host.releases);return closed;}
    closed=minix_resources_close(&host);if(!r)r=closed;
    log_line("DRIFT_MINIX_RNG_PROFILE_CLEANUP kind=%s allocations=%u releases=%u syncs=%u fault=%u\n",PERSONALITY,host.allocations,host.releases,host.syncs,host.fault);
    if(!r)log_line("DRIFT_MINIX_RNG_PROFILE_PASS kind=%s unchanged_source=1 actual_device_dma=1 polled_completion=1\n",PERSONALITY);
    return r;
}
int main(int argc,char **argv) {
    message m;env_setargs(argc,argv);sef_startup();serial_fd=open("/dev/tty00",O_WRONLY);
    int r=serial_fd<0?-5:run();int fd=open("/tmp/drift-rng.status",O_WRONLY|O_CREAT|O_TRUNC,0600);
    if(fd>=0){const char *s=r?"FAIL\n":"PASS\n";write(fd,s,5);close(fd);}
    for(;;)sef_receive(ANY,&m);
}
