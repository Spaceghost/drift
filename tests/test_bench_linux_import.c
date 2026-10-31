/* Unit tests for the benchmark's verification/error contract.
 * Synthetic clocks below are NEVER performance data. SPDX-License-Identifier: MIT */
#include "bench_linux_import.h"
#include <stdint.h>
#include <stdio.h>

static unsigned checks;
#define CHECK(condition) do { \
    ++checks; \
    if(!(condition)){fprintf(stderr,"benchmark unit line %u: %s\n", \
        (unsigned)__LINE__,#condition);return 1;} \
} while(0)

enum clock_mode { ADVANCING, FAILED, FROZEN, BACKWARD };
typedef struct fixture {
    uint32_t mode,clock_calls,fail_clock_at,rows,fail_emit_at;
    drift_linux_import_bench_sample samples[DRIFT_LINUX_IMPORT_BENCH_SAMPLES];
} fixture;

static int32_t synthetic_clock(void *context,uint64_t *out)
{
    fixture *f=context;
    ++f->clock_calls;
    if(f->mode==FAILED&&f->clock_calls==f->fail_clock_at)return -123;
    if(f->mode==FROZEN)*out=1000;
    else if(f->mode==BACKWARD)*out=1000-f->clock_calls;
    else *out=f->clock_calls;
    return 0;
}
static int32_t collect(void *context,const drift_linux_import_bench_sample *sample)
{
    fixture *f=context;
    if(f->rows>=DRIFT_LINUX_IMPORT_BENCH_SAMPLES)return -91;
    f->samples[f->rows++]=*sample;
    return f->fail_emit_at==f->rows?-77:0;
}
static int32_t run(fixture *f)
{
    return drift_bench_linux_import_run(synthetic_clock,f,collect,f,16);
}
static int verify_success(void)
{
    fixture f={0};
    CHECK(run(&f)==0);
    CHECK(f.rows==DRIFT_LINUX_IMPORT_BENCH_SAMPLES);
    CHECK(f.clock_calls==DRIFT_LINUX_IMPORT_BENCH_SAMPLES*2);
    for(uint32_t i=0;i<DRIFT_LINUX_IMPORT_BENCH_SAMPLES;++i){
        const drift_linux_import_bench_sample *s=&f.samples[i];
        CHECK(s->sample==i+1);
        CHECK(s->round==i/2+1);
        CHECK(s->position==i%2+1);
        CHECK(s->variant==((i%2)^((i/2)&1U)));
        CHECK(s->events==16);
        CHECK(s->operations==112);
        CHECK(s->modeled_delay_us==35);
        CHECK(s->control==0xa8);
        CHECK(s->checksum!=0&&s->checksum==f.samples[0].checksum);
        /* Elapsed values from this synthetic clock are deliberately not
         * asserted or reported as benchmark measurements. */
    }
    return 0;
}
static int verify_arguments(void)
{
    fixture f={0};
    CHECK(drift_bench_linux_import_run(NULL,&f,collect,&f,16)==-22);
    CHECK(drift_bench_linux_import_run(synthetic_clock,&f,NULL,&f,16)==-22);
    const uint32_t invalid[]={0,1,15,17,4000016U,UINT32_MAX};
    for(uint32_t i=0;i<sizeof(invalid)/sizeof(*invalid);++i){
        CHECK(drift_bench_linux_import_run(synthetic_clock,&f,collect,&f,invalid[i])==-22);
        CHECK(f.clock_calls==0&&f.rows==0);
    }
    return 0;
}
static int verify_clock_failures(void)
{
    /* First timestamp, first ending timestamp, and a later round all fail.
     * Rows must remain unreported even after earlier trials were measured. */
    const uint32_t failure_at[]={1,2,7,16};
    for(uint32_t i=0;i<sizeof(failure_at)/sizeof(*failure_at);++i){
        fixture f={.mode=FAILED,.fail_clock_at=failure_at[i]};
        CHECK(run(&f)==-3);
        CHECK(f.rows==0);
        CHECK(verify_success()==0); /* A failed trial released imported state. */
    }
    for(uint32_t mode=FROZEN;mode<=BACKWARD;++mode){
        fixture f={.mode=mode};
        CHECK(run(&f)==-3);
        CHECK(f.rows==0);
        CHECK(verify_success()==0);
    }
    return 0;
}
static int verify_emit_failures(void)
{
    const uint32_t failure_at[]={1,4,8};
    for(uint32_t i=0;i<sizeof(failure_at)/sizeof(*failure_at);++i){
        fixture f={.fail_emit_at=failure_at[i]};
        CHECK(run(&f)==-77);
        CHECK(f.rows==failure_at[i]);
        CHECK(f.clock_calls==16); /* All pairs were verified before any emit. */
        CHECK(verify_success()==0);
    }
    return 0;
}
int main(void)
{
    if(verify_arguments()||verify_clock_failures()||verify_emit_failures()||verify_success())return 1;
    printf("benchmark API unit: %u checks passed (synthetic clocks; no performance data)\n",checks);
    return 0;
}
