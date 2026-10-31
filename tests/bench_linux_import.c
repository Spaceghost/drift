/* Component-only benchmark: modeled ports/delays, no hardware qualification.
 * SPDX-License-Identifier: MIT */
#if !defined(_WIN32) && !defined(DRIFT_LINUX_IMPORT_BENCH_NO_MAIN)
#define _POSIX_C_SOURCE 200809L
#endif
#include "bench_linux_import.h"
#include "drift_linux_pcspkr.h"
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

typedef struct transaction { uint32_t type, code; int32_t value; } transaction;
/* Valid tones, bell conversion, off/boundary values and rejected requests. */
static const transaction requests[16]={
    {18,2,440}, {18,2,0}, {18,1,1}, {18,1,0},
    {18,2,21}, {18,2,32766}, {18,2,20}, {18,2,32767},
    {18,2,-1}, {18,1,-1}, {18,2,16000}, {18,2,880},
    {1,2,440}, {18,99,440}, {18,2,2147483647}, {18,2,0}
};
typedef struct trace_event { uint32_t kind, port, value; } trace_event;
typedef struct port_model {
    atomic_flag mutex;
    uint32_t control, held, failed, tracing, trace_used;
    uint64_t operations, modeled_delay_us, checksum;
    trace_event trace[16];
} port_model;

static void mix(port_model *m,uint32_t kind,uint32_t port,uint32_t value)
{
    /* Fixed-width, order-sensitive digest; no native struct padding is read. */
    uint64_t word=(uint64_t)kind|((uint64_t)port<<8)|((uint64_t)value<<24);
    m->checksum=(m->checksum^word)*UINT64_C(1099511628211);
}
static void record(port_model *m,uint32_t kind,uint32_t port,uint32_t value)
{
    ++m->operations;
    mix(m,kind,port,value);
    if(m->tracing){
        if(m->trace_used>=16){m->failed=1;return;}
        m->trace[m->trace_used++]=(trace_event){kind,port,value};
    }
}
static int32_t take(void *context,unsigned long *flags)
{
    port_model *m=context;
    if(!flags||atomic_flag_test_and_set_explicit(&m->mutex,memory_order_acquire)){
        m->failed=1;return -5;
    }
    if(m->held){m->failed=1;return -5;}
    m->held=1;*flags=0x5a;record(m,'L',0,0);return 0;
}
static int32_t release(void *context,unsigned long flags)
{
    port_model *m=context;
    if(!m->held||flags!=0x5a){m->failed=1;return -5;}
    record(m,'U',0,0);m->held=0;
    atomic_flag_clear_explicit(&m->mutex,memory_order_release);return 0;
}
static int32_t read8(void *context,uint16_t port)
{
    port_model *m=context;
    if(!m->held||port!=0x61){m->failed=1;return -5;}
    record(m,'R',port,m->control);return (int32_t)m->control;
}
static int32_t write8(void *context,uint16_t port,uint8_t value)
{
    port_model *m=context;
    if(!m->held||(port!=0x42&&port!=0x43&&port!=0x61)){
        m->failed=1;return -5;
    }
    record(m,'W',port,value);if(port==0x61)m->control=value;return 0;
}
static int32_t wait_us(void *context,uint32_t duration)
{
    port_model *m=context;
    if(!m->held||duration!=1){m->failed=1;return -5;}
    record(m,'D',0,duration);m->modeled_delay_us+=duration;return 0;
}
static drift_linux_pcspkr_host host(port_model *m)
{
    return (drift_linux_pcspkr_host){m,read8,write8,take,release,wait_us};
}

/* Independent reference for the transaction contract, not a kernel driver.
 * Both paths use the same host callbacks, atomics, digest and modeled waits. */
static int32_t reference_event(drift_linux_pcspkr_host *h,const transaction *t)
{
    if(t->type!=18||(t->code!=1&&t->code!=2))return -22;
    uint32_t hz=t->code==1?(t->value?1000U:0U):(uint32_t)t->value;
    uint32_t divisor=(hz>=21U&&hz<=32766U)?1193182U/hz:0U;
    unsigned long flags;
    int32_t result=h->lock(h->context,&flags);
    if(result)return result;
    if(divisor){
        result=h->write8(h->context,0x43,0xb6);
        if(!result)result=h->wait_us(h->context,1);
        if(!result)result=h->write8(h->context,0x42,(uint8_t)divisor);
        if(!result)result=h->wait_us(h->context,1);
        if(!result)result=h->write8(h->context,0x42,(uint8_t)(divisor>>8));
    }
    if(!result){
        int32_t control=h->read8(h->context,0x61);
        if(control<0||control>255)result=-5;
        else{
            result=h->wait_us(h->context,1);
            if(!result)result=h->write8(h->context,0x61,
                (uint8_t)(divisor?((uint32_t)control|3U):((uint32_t)control&252U)));
            if(!result&&divisor)result=h->wait_us(h->context,1);
        }
    }
    int32_t unlocked=h->unlock(h->context,flags);
    return result?result:unlocked;
}
static int same_trace(const port_model *a,const port_model *b)
{
    if(a->failed||b->failed||a->held||b->held||a->control!=b->control||
       a->trace_used!=b->trace_used||a->operations!=b->operations||
       a->modeled_delay_us!=b->modeled_delay_us||a->checksum!=b->checksum)return 0;
    for(uint32_t i=0;i<a->trace_used;++i){
        if(a->trace[i].kind!=b->trace[i].kind||a->trace[i].port!=b->trace[i].port||
           a->trace[i].value!=b->trace[i].value)return 0;
    }
    return 1;
}
static int32_t preflight(void)
{
    port_model imported={.mutex=ATOMIC_FLAG_INIT,.control=0xa8,.tracing=1,
        .checksum=UINT64_C(14695981039346656037)};
    port_model reference={.mutex=ATOMIC_FLAG_INIT,.control=0xa8,.tracing=1,
        .checksum=UINT64_C(14695981039346656037)};
    drift_linux_pcspkr state={0};
    drift_linux_pcspkr_host ih=host(&imported),rh=host(&reference);
    if(drift_linux_pcspkr_open(&state,&ih)||imported.operations)return -5;
    int32_t result=0;
    for(uint32_t i=0;i<16;++i){
        imported.trace_used=reference.trace_used=0;
        const transaction *t=&requests[i];
        int32_t a=drift_linux_pcspkr_event(&state,t->type,t->code,t->value);
        int32_t b=reference_event(&rh,t);
        mix(&imported,'S',0,(uint32_t)a);mix(&reference,'S',0,(uint32_t)b);
        if(a!=b||!same_trace(&imported,&reference)){result=-6;break;}
    }
    imported.trace_used=reference.trace_used=0;
    int32_t closed=drift_linux_pcspkr_close(&state);
    transaction off={18,1,0};
    int32_t stopped=reference_event(&rh,&off);
    if(closed||stopped||state.phase||state.allocated||state.registered||state.host)
        return -5;
    if(result)return result;
    return same_trace(&imported,&reference)?0:-6;
}
static int32_t trial(uint32_t variant,uint32_t events,
    drift_linux_import_bench_clock clock,void *clock_context,
    drift_linux_import_bench_sample *sample)
{
    port_model model={.mutex=ATOMIC_FLAG_INIT,.control=0xa8,
        .checksum=UINT64_C(14695981039346656037)};
    drift_linux_pcspkr state={0};drift_linux_pcspkr_host h=host(&model);
    if(variant==DRIFT_LINUX_IMPORT_BENCH_IMPORTED&&drift_linux_pcspkr_open(&state,&h))return -5;
    uint64_t start=0,end=0;
    atomic_signal_fence(memory_order_seq_cst);
    int32_t result=clock(clock_context,&start)?-3:0;
    atomic_signal_fence(memory_order_seq_cst);
    if(!result){
        for(uint32_t i=0;i<events;++i){
            const transaction *t=&requests[i&15U];
            int32_t status=variant==DRIFT_LINUX_IMPORT_BENCH_IMPORTED?
                drift_linux_pcspkr_event(&state,t->type,t->code,t->value):reference_event(&h,t);
            mix(&model,'S',0,(uint32_t)status);
            if(status!=(t->type==18&&(t->code==1||t->code==2)?0:-22)){
                result=-5;break;
            }
        }
        atomic_signal_fence(memory_order_seq_cst);
        if(clock(clock_context,&end)||end<=start)result=-3;
        atomic_signal_fence(memory_order_seq_cst);
    }
    if(model.failed||model.held)result=-5;
    sample->variant=variant;sample->events=events;sample->control=model.control;
    sample->elapsed_us=end>=start?end-start:0;sample->operations=model.operations;
    sample->modeled_delay_us=model.modeled_delay_us;sample->checksum=model.checksum;
    if(variant==DRIFT_LINUX_IMPORT_BENCH_IMPORTED){
        if(drift_linux_pcspkr_close(&state)||state.phase||state.host||model.held||model.failed)result=-5;
    }
    return result;
}
static int same_result(const drift_linux_import_bench_sample *a,
    const drift_linux_import_bench_sample *b)
{
    return a->events==b->events&&a->operations==b->operations&&
        a->modeled_delay_us==b->modeled_delay_us&&a->control==b->control&&a->checksum==b->checksum;
}
int32_t drift_bench_linux_import_run(drift_linux_import_bench_clock clock,void *clock_context,
    drift_linux_import_bench_emit emit,void *emit_context,uint32_t events)
{
    if(!clock||!emit||events<16||events>4000000U||(events&15U))return -22;
    int32_t result=preflight();if(result)return result;
    drift_linux_import_bench_sample samples[DRIFT_LINUX_IMPORT_BENCH_SAMPLES]={{0}};
    for(uint32_t round=0;round<DRIFT_LINUX_IMPORT_BENCH_ROUNDS;++round){
        for(uint32_t position=0;position<2;++position){
            uint32_t i=round*2+position;
            samples[i].sample=i+1;samples[i].round=round+1;samples[i].position=position+1;
            result=trial(position^(round&1U),events,clock,clock_context,&samples[i]);
            if(result)return result;
        }
        if(!same_result(&samples[round*2],&samples[round*2+1]))return -6;
    }
    for(uint32_t i=0;i<DRIFT_LINUX_IMPORT_BENCH_SAMPLES;++i){
        result=emit(emit_context,&samples[i]);if(result)return result;
    }
    return 0;
}

#ifndef DRIFT_LINUX_IMPORT_BENCH_NO_MAIN
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
typedef struct hosted_clock { uint64_t frequency; } hosted_clock;
static int32_t now_us(void *context,uint64_t *out)
{
    hosted_clock *clock=context;LARGE_INTEGER counter;
    if(!QueryPerformanceCounter(&counter)||counter.QuadPart<0||!clock->frequency)return -1;
    uint64_t value=(uint64_t)counter.QuadPart,quotient=value/clock->frequency;
    if(quotient>(UINT64_MAX-999999U)/1000000U)return -1;
    *out=quotient*1000000U+(value%clock->frequency)*1000000U/clock->frequency;
    return 0;
}
#else
#include <time.h>
typedef struct hosted_clock { unsigned unused; } hosted_clock;
static int32_t now_us(void *context,uint64_t *out)
{
    struct timespec value;(void)context;
    if(clock_gettime(CLOCK_MONOTONIC,&value)||value.tv_sec<0||value.tv_nsec<0||value.tv_nsec>=1000000000L)
        return -1;
    if((uint64_t)value.tv_sec>(UINT64_MAX-999999U)/1000000U)return -1;
    *out=(uint64_t)value.tv_sec*1000000U+(uint64_t)value.tv_nsec/1000U;return 0;
}
#endif
static int32_t print_sample(void *context,const drift_linux_import_bench_sample *s)
{
    (void)context;
    return printf("%u\t%u\t%u\t%s\t%" PRIu64 "\t%u\t%" PRIu64 "\t%016" PRIx64 "\t%02x\t%" PRIu64 "\n",
        s->sample,s->round,s->position,
        s->variant==DRIFT_LINUX_IMPORT_BENCH_IMPORTED?"imported-linux":"direct-reference",
        s->elapsed_us,s->events,s->operations,s->checksum,s->control,s->modeled_delay_us)<0?-1:0;
}
int main(int argc,char **argv)
{
    uint32_t events=DRIFT_LINUX_IMPORT_BENCH_DEFAULT_EVENTS;
    if(argc>2){fprintf(stderr,"usage: %s [events: multiple of 16, 16..4000000]\n",argv[0]);return 2;}
    if(argc==2){
        char *end=NULL;errno=0;unsigned long parsed=strtoul(argv[1],&end,10);
        if(argv[1][0]<'0'||argv[1][0]>'9'||errno||!end||end==argv[1]||*end||
           parsed<16||parsed>4000000U||(parsed&15U)){
            fprintf(stderr,"invalid event count\n");return 2;
        }
        events=(uint32_t)parsed;
    }
    hosted_clock clock={0};
#ifdef _WIN32
    LARGE_INTEGER frequency;
    if(!QueryPerformanceFrequency(&frequency)||frequency.QuadPart<=0||
       (uint64_t)frequency.QuadPart>UINT64_MAX/1000000U){
        fprintf(stderr,"QueryPerformanceFrequency failed\n");return 1;
    }
    clock.frequency=(uint64_t)frequency.QuadPart;
#endif
    puts("# scope=" DRIFT_LINUX_IMPORT_BENCH_SCOPE);
    puts("# monotonic wall microseconds; modeled waits do not sleep; no native-kernel or hardware timing");
    puts("sample\tround\tposition\tvariant\telapsed_us\tevents\toperations\tchecksum\tcontrol\tmodeled_delay_us");
    int32_t result=drift_bench_linux_import_run(now_us,&clock,print_sample,NULL,events);
    if(result){fprintf(stderr,"Linux source component benchmark failed: %" PRId32 "\n",result);return 1;}
    return fflush(stdout)?1:0;
}
#endif
