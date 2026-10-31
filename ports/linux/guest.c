/* Linux userspace resource adapter and guest-only qualification.
 * SPDX-License-Identifier: MIT
 * Links the separately licensed GPL-2.0-only PC speaker import. */
#define _GNU_SOURCE
#include "drift_linux_pcspkr.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/capability.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

typedef struct host {
    pthread_mutex_t mutex;
    pthread_t owner;
    unsigned enabled,held,denied,writes,reads,delays,locks;
    uint64_t hash;
    uint8_t last_control;
} host;
static void record(host *h,unsigned kind,unsigned port,unsigned value)
{
    h->hash=(h->hash^kind)*UINT64_C(1099511628211);
    h->hash=(h->hash^port)*UINT64_C(1099511628211);
    h->hash=(h->hash^value)*UINT64_C(1099511628211);
}
static int32_t read8(void *context,uint16_t port)
{
    host *h=context;
    if (!h->enabled) { ++h->denied; return -EACCES; }
    if (!h->held || port!=0x61) return -EINVAL;
    unsigned value=inb(port); h->last_control=(uint8_t)value; ++h->reads;
    /* PIT output/refresh bits vary with time; check preservation at write,
     * normalize only those hardware inputs in the cross-trial digest. */
    record(h,'R',port,value&3);
    return (int32_t)value;
}
static int32_t write8(void *context,uint16_t port,uint8_t value)
{
    host *h=context;
    if (!h->enabled) { ++h->denied; return -EACCES; }
    if (!h->held || (port!=0x42 && port!=0x43 && port!=0x61)) return -EINVAL;
    if (port==0x61) assert((value&0xfc)==(h->last_control&0xfc));
    outb(value,port); ++h->writes;
    record(h,'W',port,port==0x61 ? value&3 : value);
    return 0;
}
static int32_t take(void *context,unsigned long *flags)
{
    host *h=context;
    int error=pthread_mutex_lock(&h->mutex); if (error) return -error;
    assert(!h->held); h->held=1; h->owner=pthread_self(); *flags=0x42;
    ++h->locks; record(h,'L',0,0); return 0;
}
static int32_t release(void *context,unsigned long flags)
{
    host *h=context;
    if (!h->held || flags!=0x42 || !pthread_equal(h->owner,pthread_self())) return -EPERM;
    record(h,'U',0,0); h->held=0;
    int error=pthread_mutex_unlock(&h->mutex); return error ? -error : 0;
}
static int32_t pause_us(void *context,uint32_t duration)
{
    host *h=context;
    if (!h->held || duration!=1) return -EINVAL;
    struct timespec pause={0,1000};
    while (nanosleep(&pause,&pause)) if (errno!=EINTR) return -errno;
    ++h->delays; record(h,'D',0,1); return 0;
}
static uint64_t now_us(void)
{
    struct timespec t; assert(!clock_gettime(CLOCK_MONOTONIC,&t));
    return (uint64_t)t.tv_sec*1000000+(uint64_t)t.tv_nsec/1000;
}
/* Independent reference fixture, not another OS's native driver. Both paths
 * use the exact same authorized callbacks, locking and real delays. */
static int reference(drift_linux_pcspkr_host *h,int value)
{
    unsigned divisor=value>20 && value<32767 ? 1193182u/(unsigned)value : 0;
    unsigned long flags=0; int result=h->lock(h->context,&flags);
    if (result) return result;
#define STEP(x) do { result=(x); if (result) goto done; } while (0)
    if (divisor) {
        STEP(h->write8(h->context,0x43,0xb6)); STEP(h->wait_us(h->context,1));
        STEP(h->write8(h->context,0x42,divisor&255)); STEP(h->wait_us(h->context,1));
        STEP(h->write8(h->context,0x42,divisor>>8));
    }
    result=h->read8(h->context,0x61); if (result<0) goto done;
    unsigned control=(unsigned)result; STEP(h->wait_us(h->context,1));
    STEP(h->write8(h->context,0x61,divisor ? control|3 : control&0xfc));
    if (divisor) STEP(h->wait_us(h->context,1));
done:
    { int unlock=h->unlock(h->context,flags); if (!result) result=unlock; }
#undef STEP
    return result;
}
static void kernel_denial(void)
{
    pid_t child=fork(); assert(child>=0);
    if (!child) {
        struct __user_cap_header_struct header={_LINUX_CAPABILITY_VERSION_3,0};
        struct __user_cap_data_struct caps[2]={{0},{0}};
        if (syscall(SYS_capset,&header,caps)) _exit(2);
        errno=0;
        _exit(ioperm(0x61,1,1)==-1 && errno==EPERM ? 0 : 3);
    }
    int status; assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
    puts("DRIFT_LINUX_KERNEL_IOPERM_DENIAL_PASS");
}
static void read_line(const char *path,char *buffer,size_t capacity)
{
    FILE *f=fopen(path,"r"); assert(f && fgets(buffer,(int)capacity,f)); fclose(f);
    buffer[strcspn(buffer,"\r\n")]=0;
}
static void models(void)
{
    const char *names[]={"/linux-model","/windows-model"};
    for (unsigned i=0;i<2;++i) {
        pid_t child=fork(); assert(child>=0);
        if (!child) { execl(names[i],names[i],NULL); _exit(127); }
        int status; assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
    }
    puts("DRIFT_LINUX_GUEST_BOTH_SOURCE_MODELS_PASS");
}
int main(void)
{
    if (getpid()!=1) { fputs("guest init only; refusing host execution\n",stderr); return 2; }
    setvbuf(stdout,NULL,_IONBF,0); setvbuf(stderr,NULL,_IONBF,0);
    assert(!mount("proc","/proc","proc",0,NULL));
    assert(!mount("sysfs","/sys","sysfs",0,NULL));
    struct utsname info; assert(!uname(&info));
    printf("DRIFT_LINUX_GUEST %s %s %s\n",info.sysname,info.release,info.machine);
    assert(access("/sys/module/pcspkr",F_OK)==-1 && errno==ENOENT);
    char clock[128]; read_line("/sys/devices/system/clocksource/clocksource0/current_clocksource",clock,sizeof(clock));
    assert(strcmp(clock,"pit")); printf("clocksource=%s; pcspkr module absent; no module loader or other userspace service\n",clock);
    models(); kernel_denial();
    host h={.mutex=PTHREAD_MUTEX_INITIALIZER};
    drift_linux_pcspkr_host ops={&h,read8,write8,take,release,pause_us};
    drift_linux_pcspkr state={0}; assert(!drift_linux_pcspkr_open(&state,&ops));
    assert(drift_linux_pcspkr_event(&state,18,2,440)==-EACCES);
    assert(h.denied==1 && !h.writes && !h.reads && !h.held);
    assert(drift_linux_pcspkr_close(&state)==-EACCES && state.phase==3);
    assert(!ioperm(0x42,2,1) && !ioperm(0x61,1,1)); h.enabled=1;
    assert(!drift_linux_pcspkr_close(&state));
    assert(!drift_linux_pcspkr_open(&state,&ops));
    assert(!drift_linux_pcspkr_event(&state,18,2,440)); assert((inb(0x61)&3)==3);
    outb(0xe8,0x43); assert((inb(0x42)&0x3f)==0x36);
    assert(!drift_linux_pcspkr_suspend(&state)); assert((inb(0x61)&3)==0);
    assert(!drift_linux_pcspkr_shutdown(&state));
    assert(drift_linux_pcspkr_event(&state,18,2,1000)==-ENODEV);
    assert(!drift_linux_pcspkr_close(&state));
    assert(!drift_linux_pcspkr_open(&state,&ops));
    uint64_t expected_hash=0;
    for (unsigned round=0;round<4;++round) for (unsigned slot=0;slot<2;++slot) {
        unsigned imported=(round&1) ? !slot : slot;
        /* Off, then on/off cycles: every trial begins with identical gate bits. */
        assert(!reference(&ops,0));
        h.writes=h.reads=h.delays=h.locks=0; h.hash=UINT64_C(14695981039346656037);
        uint64_t start=now_us();
        for (unsigned i=0;i<100;++i) {
            unsigned hz=(i&1) ? 1000 : 440;
            assert(!(imported ? drift_linux_pcspkr_event(&state,18,2,(int32_t)hz) : reference(&ops,(int)hz)));
            assert((inb(0x61)&3)==3);
            assert(!(imported ? drift_linux_pcspkr_event(&state,18,2,0) : reference(&ops,0)));
            assert((inb(0x61)&3)==0);
        }
        uint64_t elapsed=now_us()-start;
        assert(elapsed && h.writes==500 && h.reads==200 && h.delays==500 && h.locks==200 && !h.held);
        if (!round && !slot) expected_hash=h.hash; else assert(h.hash==expected_hash);
        printf("PIO_SAMPLE\t%u\t%s\t%llu\t200\t%u\t%u\t%u\t%016llx\n",round+1,imported?"import":"reference",(unsigned long long)elapsed,h.writes,h.reads,h.delays,(unsigned long long)h.hash);
    }
    assert(!drift_linux_pcspkr_close(&state)); assert((inb(0x61)&3)==0);
    assert(!ioperm(0x42,2,0) && !ioperm(0x61,1,0)); h.enabled=0;
    assert(!pthread_mutex_destroy(&h.mutex));
    puts("DRIFT_LINUX_REAL_PIO_PASS");
    puts("DRIFT_LINUX_GUEST_EXIT=0");
    sync(); reboot(RB_POWER_OFF);
    return 1;
}
