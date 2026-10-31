/* Actual virtual-PCI integration. Device logic remains in the selected
 * unchanged source profile or native Nelua driver. SPDX-License-Identifier: MIT */
#include "virtio_qtest.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(DRIFT_RNG_LINUX)
#include "drift_linux_virtio_rng.h"
#elif defined(DRIFT_RNG_WINDOWS)
#include "drift_windows_viorng.h"
#elif !defined(DRIFT_RNG_NATIVE)
#error select a real source profile or native driver
#endif

#define CHECK(c) do { if (!(c)) { fprintf(stderr,"controller:%d: %s; broker=%s\n",__LINE__,#c,q.error);return 1; } } while (0)
static drift_vqt q;
#ifdef DRIFT_RNG_WINDOWS
static unsigned notifications;
static void completed(void *context,drift_win_rng_request *request)
{
    unsigned *count=context;
    if (request->phase!=DRIFT_WIN_RNG_COMPLETE || request->completions!=1) abort();
    ++*count;
}
#endif
int main(int argc,char **argv)
{
    if (argc!=3) {fprintf(stderr,"usage: controller QTEST_TCP_PORT REQUESTS\n");return 2;}
    char *end;unsigned long port=strtoul(argv[1],&end,10);
    if (*end || !port || port>65535) return 2;
    unsigned long requests=strtoul(argv[2],&end,10);
    if (*end || !requests || requests>4096) return 2;
    CHECK(!drift_vqt_connect(&q,(uint16_t)port));
    uint32_t bdf;drift_virtio_bar bars[6];
    CHECK(!drift_vqt_bind(&q,0x1af4,0x1044,0,&bdf,bars));
    /* Resource-boundary qualification before any RNG queue exists. This
     * unrelated allocation is never given to the device or returned as RNG. */
    drift_dma check_mapping={0};
    CHECK(q.hardware.dma(&q,0,64,&check_mapping)<0);
    CHECK(q.hardware.dma(&q,96,3,&check_mapping)<0);
    CHECK(!q.hardware.dma(&q,96,64,&check_mapping));
    CHECK((uintptr_t)check_mapping.data!=(uintptr_t)check_mapping.address);
    for (unsigned i=0;i<96;++i) check_mapping.data[i]=(uint8_t)(i*7+3);
    q.hardware.sync(&q,&check_mapping,0,96,DRIFT_DMA_TO_DEVICE);
    memset(check_mapping.data,0,96);
    q.hardware.sync(&q,&check_mapping,0,96,DRIFT_DMA_FROM_DEVICE);
    for (unsigned i=0;i<96;++i) CHECK(check_mapping.data[i]==(uint8_t)(i*7+3));
    q.hardware.release(&q,&check_mapping);CHECK(!check_mapping.data && !check_mapping.token);
    CHECK(!drift_vqt_failed(&q) && !q.live);
    uint64_t start=q.hardware.time_us(&q),digest=UINT64_C(14695981039346656037),offset=0;
    uint32_t expected_state=UINT32_C(0x6d2b79f5);
    enum { DATA_BYTES=32, WARMUP_REQUESTS=8 };
    uint8_t storage[DATA_BYTES+2],*bytes=storage+1;
    uint64_t warmup_digest=UINT64_C(14695981039346656037);
#if defined(DRIFT_RNG_LINUX)
    drift_virtio transport={0};drift_linux_virtio_rng source={0};
    CHECK(!drift_virtio_raw_open_function(&transport,&q.hardware,&q.pci,bdf,bars,4,1));
    CHECK(!drift_linux_virtio_rng_open(&source,&transport));
    const char *variant="linux-source";
#elif defined(DRIFT_RNG_WINDOWS)
    drift_win_rng source={0};drift_win_rng_host host={&notifications,completed};
    CHECK(!drift_windows_viorng_open_function(&source,&q.hardware,&q.pci,bdf,bars,&host));
    const char *variant="windows-source";
#else
    drift_virtio source={0};
    CHECK(!drift_virtio_open_function(&source,&q.hardware,&q.pci,bdf,bars));
    const char *variant="native-nelua";
#endif
    uint64_t opened=q.hardware.time_us(&q),open_elapsed=opened-start;
    uint64_t measured_started=0,warmup_elapsed=0;
    for (unsigned long request=0;request<requests+WARMUP_REQUESTS;++request) {
        if (request==WARMUP_REQUESTS) {measured_started=q.hardware.time_us(&q);warmup_elapsed=measured_started-opened;}
        memset(storage,0xcc,sizeof(storage));storage[0]=0xa7;storage[DATA_BYTES+1]=0x3d;
#if defined(DRIFT_RNG_LINUX)
        int32_t count=drift_linux_virtio_rng_read(&source,bytes,DATA_BYTES,1);
        CHECK(count==DATA_BYTES);
#elif defined(DRIFT_RNG_WINDOWS)
        drift_win_rng_request pending={0};unsigned before=notifications;
        int32_t status=drift_windows_viorng_read(&source,&pending,bytes,DATA_BYTES,DATA_BYTES);
        CHECK(status==DRIFT_WIN_RNG_PENDING || status==DRIFT_WIN_RNG_SUCCESS);
        uint64_t deadline=q.hardware.time_us(&q)+2000000;
        while (pending.phase!=DRIFT_WIN_RNG_COMPLETE) {
            CHECK(drift_windows_viorng_pump(&source)>=0);
            CHECK(q.hardware.time_us(&q)<deadline);
            q.hardware.wait_us(&q,10);
        }
        CHECK(pending.status==DRIFT_WIN_RNG_SUCCESS && pending.information==DATA_BYTES);
        CHECK(pending.completions==1 && notifications==before+1);
#else
        CHECK(drift_virtio_entropy(&source,bytes,DATA_BYTES)==DATA_BYTES);
#endif
        CHECK(!drift_vqt_failed(&q));
        CHECK(storage[0]==0xa7 && storage[DATA_BYTES+1]==0x3d);
        for (unsigned i=0;i<DATA_BYTES;++i,++offset) {
            /* The independent EGD hardware fixture feeds this stream to QEMU.
             * The provider never knows or manufactures the expected data. */
            expected_state^=expected_state<<13;expected_state^=expected_state>>17;expected_state^=expected_state<<5;
            CHECK(bytes[i]==(uint8_t)(expected_state>>24));
            if (request<WARMUP_REQUESTS) warmup_digest=(warmup_digest^bytes[i])*UINT64_C(1099511628211);
            else digest=(digest^bytes[i])*UINT64_C(1099511628211);
        }
    }
    uint64_t elapsed=q.hardware.time_us(&q)-measured_started,close_started=q.hardware.time_us(&q);
#if defined(DRIFT_RNG_LINUX)
    CHECK(!drift_linux_virtio_rng_close(&source));
    CHECK(transport.state==0 && source.phase==0);
#elif defined(DRIFT_RNG_WINDOWS)
    CHECK(!drift_windows_viorng_close(&source));
    CHECK(source.phase==0 && source.transport.state==0);
#else
    CHECK(!drift_virtio_close(&source));CHECK(source.state==0);
#endif
    uint64_t close_elapsed=q.hardware.time_us(&q)-close_started;
    CHECK(!drift_vqt_failed(&q) && open_elapsed && warmup_elapsed && elapsed && close_elapsed);
    CHECK(!q.live && q.allocated==q.released && q.allocated>=2);
    CHECK(q.published_bytes && q.acquired_bytes && q.register_reads && q.register_writes);
    printf("RNG_RESULT\t%s\t%lu\t%lu\t%" PRIu64 "\t%016" PRIx64 "\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%u\t%u\t%" PRIu64 "\t%016" PRIx64 "\t%" PRIu64 "\t1\t%" PRIu64 "\n",
           variant,requests,requests*DATA_BYTES,elapsed,digest,q.commands,q.allocated,q.released,q.published_bytes+q.acquired_bytes,open_elapsed,close_elapsed,
           WARMUP_REQUESTS,WARMUP_REQUESTS*DATA_BYTES,warmup_elapsed,warmup_digest,offset,q.timer_resolution_ns);
    CHECK(!drift_vqt_dispose(&q,0));
    puts("DRIFT_ACTUAL_VIRTIO_RNG_PASS");return 0;
}
