/* Actual MINIX RS resource/queue qualification; not a host model. */
/* SPDX-License-Identifier: MIT */
#include <minix/drivers.h>
#include <minix/sef.h>
#include <minix/endpoint.h>
#include <fcntl.h>
#include <minix/vm.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdarg.h>
#include "resources.h"
#include "fixture.h"
static int serial_fd=-1;
static void log_line(const char *format,...) {
    char b[512];va_list ap;va_start(ap,format);int n=vsnprintf(b,sizeof(b),format,ap);va_end(ap);
    if(n<=0||n>=(int)sizeof(b))return;
    for(int i=0;i<n;){ssize_t r=write(serial_fd,b+i,n-i);if(r<=0)return;i+=(int)r;}
}
static minix_resources host;
static drift_virtio transport;
static drift_dma payload;
static drift_virtio_raw_transfer request;
static void (*real_write)(void *,uint32_t,uint32_t,uint32_t,uint64_t);
static unsigned drop_reset,reset_dropped;
/* Deliberate test injection, never part of the generic provider. */
static void test_write(void *p,uint32_t region,uint32_t offset,uint32_t width,uint64_t value) {
    if(drop_reset&&region==transport.region&&offset==transport.common_offset+20&&width==1&&value==0){++reset_dropped;return;}
    real_write(p,region,offset,width,value);
}

static int run(void) {
    int r,close_result;unsigned length=0;void *cookie=NULL;
#define TRY(call) do{r=(call);if(r){log_line("DRIFT_MINIX_RNG_FAIL line=%u code=%d state=%u fault=%u\n",__LINE__,r,transport.state,host.fault);goto done;}}while(0)
    void *forbidden=vm_map_phys(SELF,(void*)(uintptr_t)0xfee00000u,4096);
    if(forbidden!=MAP_FAILED){vm_unmap_phys(SELF,forbidden,4096);return -5;}
    log_line("DRIFT_MINIX_RNG_DENIAL physical=fee00000 map_failed=1 errno_unavailable=1\n");
    TRY(minix_resources_open(&host));
    log_line("DRIFT_MINIX_RNG_PCI bdf=%u vendor_device=%08x maps=%u command=%04x\n",host.bdf,host.pci.read(&host,host.bdf,0),host.maps,pci_attr_r16(host.devind,4));
    for(unsigned i=0;i<6;i++)if(host.window[i])log_line("DRIFT_MINIX_RNG_BAR index=%u bytes=%u cpu=%08lx\n",i,host.window_size[i],(unsigned long)host.window[i]);
    TRY(drift_virtio_raw_open_function(&transport,&host.hw,&host.pci,host.bdf,host.bars,4,1));
    log_line("DRIFT_MINIX_RNG_PROBE state=%u features=%08x common_region=%u common_offset=%u notify_region=%u\n",transport.state,transport.features,transport.region,transport.common_offset,transport.notify_region);
    TRY(host.hw.dma(&host,128,16,&payload));
    log_line("DRIFT_MINIX_RNG_DMA bytes=%u cpu=%08lx physical=%08lx token=%lu\n",payload.size,(unsigned long)payload.data,(unsigned long)payload.address,(unsigned long)payload.token);
    real_write=host.hw.write;host.hw.write=test_write;
    TRY(drift_virtio_raw_device_ready(&transport));
    TRY(drift_virtio_raw_add(&transport,0,&request,&payload,0,128,DRIFT_DMA_FROM_DEVICE,&request));
    TRY(drift_virtio_raw_kick(&transport,0));
    do {r=drift_virtio_raw_get(&transport,0,&request,&cookie,&length);if(!r)host.hw.wait_us(&host,100);}while(!r);
    if(r!=1||cookie!=&request||length!=128){r=-5;goto done;}
    uint32_t expected=DRIFT_FIXTURE_SEED;
    if(fixture_compare(&expected,payload.data,128)){r=-5;goto done;}
    log_line("DRIFT_MINIX_RNG_DATA bytes=128 exact_fixture=1 used_ring_callback=1 hardware_irq=0\n");
    if(minix_resources_close(&host)!=-16){r=-5;goto done;}
    unsigned released=host.releases;drop_reset=1;
    r=drift_virtio_close(&transport);drop_reset=0;
    if(r!=-2||reset_dropped!=1||host.releases!=released||!transport.queue[0].ring.data||!payload.data){r=-5;goto done;}
    log_line("DRIFT_MINIX_RNG_RESET_FAULT injected=write-drop code=%d allocations=%u releases=%u retained=1 cleanup_refused=1\n",r,host.allocations,host.releases);
    r=0;
done:
    if(r)log_line("DRIFT_MINIX_RNG_FAIL code=%d state=%u fault=%u\n",r,transport.state,host.fault);
    /* A failed reset leaves both mappings and all DMA alive. The service
     * remains resident; the runner may stop the entire emulated machine. */
    drop_reset=0;close_result=transport.hw?drift_virtio_close(&transport):0;
    if(close_result){log_line("DRIFT_MINIX_RNG_QUARANTINE reset=%d allocations=%u releases=%u\n",close_result,host.allocations,host.releases);return close_result;}
    if(payload.data)host.hw.release(&host,&payload);
    close_result=minix_resources_close(&host);if(!r)r=close_result;
    log_line("DRIFT_MINIX_RNG_CLEANUP allocations=%u releases=%u syncs=%u fault=%u\n",host.allocations,host.releases,host.syncs,host.fault);
    if(!r)log_line("DRIFT_MINIX_RNG_PASS: actual MINIX PCI reservation/MMIO/coherent DMA modern Virtio raw queue; polling completion; exact 128-byte device fixture\n");
    return r;
}
int main(int argc,char **argv) {
    message m;env_setargs(argc,argv);sef_startup();serial_fd=open("/dev/tty00",O_WRONLY);
    int r=serial_fd<0?-5:run();int fd=open("/tmp/drift-rng.status",O_WRONLY|O_CREAT|O_TRUNC,0600);
    if(fd>=0){const char *s=r?"FAIL\n":"PASS\n";write(fd,s,5);close(fd);}
    for(;;)sef_receive(ANY,&m);
}
