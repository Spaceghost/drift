/* Drives the actual imported driver; the host is an independent port model.
 * SPDX-License-Identifier: MIT */
#include "pcspkr_compat.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

static int fail_allocate, fail_register;
struct input_dev *__real_drift_lx_input_allocate_device(void);
int32_t __real_drift_lx_input_register_device(struct input_dev *);
struct input_dev *__wrap_drift_lx_input_allocate_device(void)
{
    return fail_allocate ? NULL : __real_drift_lx_input_allocate_device();
}
int32_t __wrap_drift_lx_input_register_device(struct input_dev *device)
{
    return fail_register ? -5 : __real_drift_lx_input_register_device(device);
}

typedef struct event { char kind; uint16_t port; uint32_t value; } event;
typedef struct model {
    drift_linux_pcspkr *state;
    event trace[128]; unsigned used;
    uint8_t control;
    unsigned held, reenter, concurrent;
    int fail_lock, fail_read, fail_write, fail_wait, fail_unlock;
    int32_t nested_result, concurrent_result;
} model;
static void record(model *m,char kind,uint16_t port,uint32_t value)
{
    assert(m->used<128); m->trace[m->used++]=(event){kind,port,value};
}
static void *contender(void *context)
{
    model *m=context;
    m->concurrent_result=drift_linux_pcspkr_event(m->state,EV_SND,SND_TONE,500);
    return NULL;
}
static int32_t take(void *context,unsigned long *flags)
{
    model *m=context; assert(!m->held); record(m,'L',0,0);
    if (m->fail_lock) return -5;
    m->held=1; *flags=0x5a;
    if (m->reenter) m->nested_result=drift_linux_pcspkr_event(m->state,EV_SND,SND_TONE,500);
    if (m->concurrent) {
        pthread_t thread;
        assert(!pthread_create(&thread,NULL,contender,m));
        assert(!pthread_join(thread,NULL));
    }
    return 0;
}
static int32_t release(void *context,unsigned long flags)
{
    model *m=context; assert(m->held && flags==0x5a); record(m,'U',0,0);
    if (m->fail_unlock) return -5;
    m->held=0; return 0;
}
static int32_t read8(void *context,uint16_t port)
{
    model *m=context; assert(m->held && port==0x61);
    record(m,'R',port,m->control);
    return m->fail_read==2 ? 256 : m->fail_read ? -5 : m->control;
}
static int32_t write8(void *context,uint16_t port,uint8_t value)
{
    model *m=context; assert(m->held && (port==0x42 || port==0x43 || port==0x61));
    record(m,'W',port,value);
    if (m->fail_write) return m->fail_write==2 ? 7 : -5;
    if (port==0x61) m->control=value;
    return 0;
}
static int32_t wait_us(void *context,uint32_t duration)
{
    model *m=context; assert(m->held && duration==1); record(m,'D',0,duration);
    return m->fail_wait ? -5 : 0;
}
static drift_linux_pcspkr_host host(model *m)
{
    return (drift_linux_pcspkr_host){m,read8,write8,take,release,wait_us};
}
static void clear_trace(model *m) { m->used=0; }
static void tone_trace(model *m,unsigned hz,uint8_t before)
{
    unsigned divider=1193182/hz;
    const event expected[]={
        {'L',0,0},{'W',0x43,0xb6},{'D',0,1},
        {'W',0x42,divider&255},{'D',0,1},{'W',0x42,divider>>8},
        {'R',0x61,before},{'D',0,1},{'W',0x61,before|3},{'D',0,1},{'U',0,0}
    };
    assert(m->used==sizeof(expected)/sizeof(*expected));
    for (unsigned i=0;i<m->used;++i) {
        assert(m->trace[i].kind==expected[i].kind);
        assert(m->trace[i].port==expected[i].port);
        assert(m->trace[i].value==expected[i].value);
    }
    assert(!m->held && m->control==(before|3));
}
static void off_trace(model *m,uint8_t before)
{
    const event expected[]={{'L',0,0},{'R',0x61,before},{'D',0,1},{'W',0x61,before&252},{'U',0,0}};
    assert(m->used==sizeof(expected)/sizeof(*expected));
    for (unsigned i=0;i<m->used;++i) {
        assert(m->trace[i].kind==expected[i].kind);
        assert(m->trace[i].port==expected[i].port);
        assert(m->trace[i].value==expected[i].value);
    }
    assert(!m->held && m->control==(before&252));
}
static void normal(void)
{
    drift_linux_pcspkr state={0}; model m={.state=&state,.control=0xa8};
    drift_linux_pcspkr_host h=host(&m);
    assert(drift_linux_pcspkr_open(NULL,&h)==-22);
    assert(drift_linux_pcspkr_open(&state,NULL)==-22);
    assert(drift_linux_pcspkr_event(&state,EV_SND,SND_BELL,1)==-19);
    assert(!drift_linux_pcspkr_open(&state,&h));
    assert(state.phase==2 && state.allocated && state.registered && !m.used);
    struct input_dev *device=drift_lx_slot(&state);
    assert(!strcmp(device->name,"PC Speaker") && !strcmp(device->phys,"isa0061/input0"));
    assert(device->id.bustype==BUS_ISA && device->id.vendor==0x1f && device->id.product==1 && device->id.version==0x100);
    assert(drift_linux_pcspkr_open(&state,&h)==-16);
    assert(drift_linux_pcspkr_event(&state,0,SND_BELL,1)==-22 && !m.used);
    assert(drift_linux_pcspkr_event(&state,EV_SND,99,1)==-22 && !m.used);
    assert(!drift_linux_pcspkr_event(&state,EV_SND,SND_BELL,1)); tone_trace(&m,1000,0xa8);
    clear_trace(&m);
    assert(!drift_linux_pcspkr_event(&state,EV_SND,SND_TONE,440)); tone_trace(&m,440,0xab);
    clear_trace(&m);
    assert(!drift_linux_pcspkr_suspend(&state)); off_trace(&m,0xab);
    clear_trace(&m);
    m.reenter=1; m.concurrent=1;
    assert(!drift_linux_pcspkr_event(&state,EV_SND,SND_TONE,21)); tone_trace(&m,21,0xa8);
    assert(m.nested_result==-16 && m.concurrent_result==-16);
    m.reenter=0; m.concurrent=0; clear_trace(&m);
    assert(!drift_linux_pcspkr_event(&state,EV_SND,SND_TONE,32766)); tone_trace(&m,32766,0xab);
    const int off_values[]={-1,0,20,32767};
    for (unsigned i=0;i<sizeof(off_values)/sizeof(*off_values);++i) {
        uint8_t before=m.control; clear_trace(&m);
        assert(!drift_linux_pcspkr_event(&state,EV_SND,SND_TONE,off_values[i])); off_trace(&m,before);
    }
    clear_trace(&m);
    assert(!drift_linux_pcspkr_event(&state,EV_SND,SND_BELL,-1)); tone_trace(&m,1000,0xa8);
    clear_trace(&m);
    assert(!drift_linux_pcspkr_shutdown(&state)); off_trace(&m,0xab);
    assert(drift_linux_pcspkr_event(&state,EV_SND,SND_BELL,1)==-19);
    clear_trace(&m); assert(!drift_linux_pcspkr_close(&state)); off_trace(&m,0xa8);
    assert(!state.phase && !state.allocated && !state.registered && !state.host);
    clear_trace(&m); assert(!drift_linux_pcspkr_close(&state) && !m.used);
    assert(!drift_linux_pcspkr_open(&state,&h)); assert(!drift_linux_pcspkr_close(&state));
}
static void probe_errors(void)
{
    drift_linux_pcspkr state={0}; model m={.state=&state}; drift_linux_pcspkr_host h=host(&m);
    fail_allocate=1; assert(drift_linux_pcspkr_open(&state,&h)==-12);
    assert(!state.phase && !state.allocated && !state.registered && !m.used);
    fail_allocate=0; fail_register=1; assert(drift_linux_pcspkr_open(&state,&h)==-5);
    assert(!state.phase && !state.allocated && !state.registered && !m.used);
    fail_register=0; assert(!drift_linux_pcspkr_open(&state,&h)); assert(!drift_linux_pcspkr_close(&state));
}
static void resource_errors(void)
{
    drift_linux_pcspkr state={0}; model m={.state=&state,.control=0xa8}; drift_linux_pcspkr_host h=host(&m);
    assert(!drift_linux_pcspkr_open(&state,&h));
    m.fail_lock=1; assert(drift_linux_pcspkr_event(&state,EV_SND,SND_BELL,1)==-5);
    assert(m.used==1 && m.trace[0].kind=='L' && !state.locked && !m.held);
    m.fail_lock=0; clear_trace(&m);
    m.fail_write=1; assert(drift_linux_pcspkr_event(&state,EV_SND,SND_BELL,1)==-5);
    assert(m.used==3 && m.trace[2].kind=='U' && !m.held);
    m.fail_write=0; clear_trace(&m);
    m.fail_wait=1; assert(drift_linux_pcspkr_event(&state,EV_SND,SND_BELL,1)==-5);
    assert(m.used==4 && m.trace[3].kind=='U' && !m.held);
    m.fail_wait=0; clear_trace(&m);
    m.fail_read=1; assert(drift_linux_pcspkr_event(&state,EV_SND,SND_BELL,0)==-5);
    assert(m.used==3 && m.trace[2].kind=='U' && !m.held);
    m.fail_read=2; clear_trace(&m);
    assert(drift_linux_pcspkr_event(&state,EV_SND,SND_BELL,0)==-5);
    assert(m.used==3 && m.trace[2].kind=='U' && !m.held);
    m.fail_read=0; m.fail_write=2; clear_trace(&m);
    assert(drift_linux_pcspkr_event(&state,EV_SND,SND_BELL,1)==-5);
    assert(m.used==3 && m.trace[2].kind=='U' && !m.held);
    m.fail_write=0; clear_trace(&m);
    m.fail_unlock=1; assert(drift_linux_pcspkr_event(&state,EV_SND,SND_BELL,1)==-5);
    assert(state.locked && m.held);
    unsigned used=m.used;
    assert(drift_linux_pcspkr_event(&state,EV_SND,SND_BELL,0)==-16 && m.used==used);
    m.fail_unlock=0; assert(!drift_linux_pcspkr_close(&state));
    assert(!state.locked && !m.held && !state.phase);
    assert(!drift_linux_pcspkr_open(&state,&h)); clear_trace(&m);
    m.fail_write=1; assert(drift_linux_pcspkr_close(&state)==-5);
    assert(state.phase==3 && !state.allocated && !state.registered && state.host==&h);
    m.fail_write=0; assert(!drift_linux_pcspkr_close(&state));
    assert(!state.phase && !state.host && !m.held);
}
int main(void)
{
    normal(); probe_errors(); resource_errors();
    puts("Linux pcspkr import: lifecycle, PIT transcript, errors, reentry and concurrency passed");
    return 0;
}
