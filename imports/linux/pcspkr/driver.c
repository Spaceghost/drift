/* Host-controlled registration for an unchanged Linux source import.
 * SPDX-License-Identifier: MIT
 * The included upstream driver is GPL-2.0-only; this combined object is not
 * offered as an MIT-only artifact. See upstream/COPYING.GPL-2.0. */
#include "pcspkr_compat.h"
#include <stdatomic.h>

raw_spinlock_t i8253_lock;
static atomic_flag call_gate=ATOMIC_FLAG_INIT;
static drift_linux_pcspkr *active;
drift_linux_pcspkr *drift_lx_current(void) { return active; }

/* The file is byte-identical to the pinned Linux source. */
#include "upstream/pcspkr.c"

static int32_t enter(drift_linux_pcspkr *state, bool recover)
{
    if (!state) return -22;
    if (atomic_flag_test_and_set_explicit(&call_gate,memory_order_acquire)) return -16;
    if (state->locked && !recover) {
        atomic_flag_clear_explicit(&call_gate,memory_order_release);
        return -16;
    }
    active=state;
    state->error=0;
    return 0;
}
static int32_t leave(drift_linux_pcspkr *state, int32_t result)
{
    result=drift_lx_finish(state,result);
    active=NULL;
    atomic_flag_clear_explicit(&call_gate,memory_order_release);
    return result;
}
static struct platform_device *platform(drift_linux_pcspkr *state)
{
    return (void *)state->platform_slot;
}
int32_t drift_linux_pcspkr_open(drift_linux_pcspkr *state,drift_linux_pcspkr_host *host)
{
    int32_t result=enter(state,false);
    if (result) return result;
    result=drift_lx_prepare(state,host);
    if (result) return leave(state,result);
    platform(state)->dev.owner=state;
    result=drift_lx_driver->probe(platform(state));
    if (!result && (!state->allocated || !state->registered)) result=-5;
    if (!result) state->phase=2;
    else if (!state->allocated && !state->registered) { state->phase=0; state->host=NULL; }
    return leave(state,result);
}
int32_t drift_linux_pcspkr_event(drift_linux_pcspkr *state,uint32_t type,uint32_t code,int32_t value)
{
    int32_t result=enter(state,false);
    if (result) return result;
    if (state->phase!=2) return leave(state,-19);
    return leave(state,drift_lx_slot(state)->event(drift_lx_slot(state),type,code,value));
}
int32_t drift_linux_pcspkr_suspend(drift_linux_pcspkr *state)
{
    int32_t result=enter(state,false);
    if (result) return result;
    if (state->phase!=2) return leave(state,-19);
    return leave(state,drift_lx_driver->driver.pm->suspend(&platform(state)->dev));
}
int32_t drift_linux_pcspkr_shutdown(drift_linux_pcspkr *state)
{
    int32_t result=enter(state,false);
    if (result) return result;
    if (state->phase!=2 && state->phase!=3) return leave(state,-19);
    drift_lx_driver->shutdown(platform(state));
    state->phase=3;
    return leave(state,0);
}
int32_t drift_linux_pcspkr_close(drift_linux_pcspkr *state)
{
    int32_t result=enter(state,true);
    if (result) return result;
    if (!state->phase) return leave(state,0);
    if (state->locked) {
        drift_lx_unlock(state->irq_flags);
        if (state->error) return leave(state,0);
    }
    state->phase=3;
    if (state->registered) drift_lx_driver->remove(platform(state));
    else drift_lx_driver->shutdown(platform(state));
    if (!state->error) {
        state->allocated=0; state->registered=0; state->phase=0; state->host=NULL;
    }
    return leave(state,0);
}
