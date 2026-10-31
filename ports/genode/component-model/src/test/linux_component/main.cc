/* Shared fixture in an actual Genode component; no I/O-port authority.
 * SPDX-License-Identifier: MIT */
#include <base/component.h>
#include <base/log.h>
#include <timer_session/connection.h>
#include "bench_linux_import.h"
static int32_t now_us(void *context,uint64_t *out)
{
 if(!context || !out) return -22;
 *out=static_cast<Timer::Connection *>(context)->elapsed_us();return 0;
}
static int32_t emit(void *context,const drift_linux_import_bench_sample *s)
{
 if(!context || !s || s->events!=500000 || s->operations!=3500000 ||
    s->modeled_delay_us!=1093750 || s->control!=0xa8 || s->checksum!=UINT64_C(0x64f59f6df89549d1)) return -5;
 unsigned &count=*static_cast<unsigned *>(context);++count;
 Genode::log("DRIFT_GENODE_COMPONENT sample=",s->sample," round=",s->round," position=",s->position,
  " variant=",s->variant==DRIFT_LINUX_IMPORT_BENCH_IMPORTED?"imported-linux":"direct-reference",
  " elapsed_us=",s->elapsed_us," events=",s->events," operations=",s->operations,
  " checksum=",Genode::Hex(s->checksum)," control=",Genode::Hex(s->control)," modeled_delay_us=",s->modeled_delay_us);
 return 0;
}
void Component::construct(Genode::Env &env)
{
 Timer::Connection clock(env);unsigned emitted=0;
 Genode::log("scope=" DRIFT_LINUX_IMPORT_BENCH_SCOPE " hardware_access=none modeled_waits=no-sleep optimization=-Os");
 int32_t result=drift_bench_linux_import_run(now_us,&clock,emit,&emitted,DRIFT_LINUX_IMPORT_BENCH_DEFAULT_EVENTS);
 if(result || emitted!=8) { Genode::error("component model failed status=",result," emitted=",emitted);env.parent().exit(1);return; }
 Genode::log("Drift Genode shared Linux component MODEL PASS");env.parent().exit(0);
}
