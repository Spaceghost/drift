/* Source-entry context and typed host boundary. SPDX-License-Identifier: MIT */
#include "profile.h"
static drift_win_rng *current;
drift_win_rng *drift_win_rng_current(void){return current;}
int32_t drift_rng_open(drift_win_rng *);
int32_t drift_rng_submit(drift_win_rng *,drift_win_rng_request *,uint8_t *,size_t,size_t);
int32_t drift_rng_pump(drift_win_rng *);
int32_t drift_rng_cancel(drift_win_rng *,drift_win_rng_request *);
int32_t drift_rng_close(drift_win_rng *);
int32_t drift_rng_drain(drift_win_rng *);
static int32_t enter(drift_win_rng *s)
{
    if(!s)return DRIFT_WIN_RNG_INVALID_PARAMETER;
    if(current||s->depth>=32)return DRIFT_WIN_RNG_BUSY;
    current=s;++s->depth;return 0;
}
static int32_t leave(drift_win_rng *s)
{
    current=NULL;
    int32_t n=s->host?drift_rng_drain(s):0;
    --s->depth;
    if(s->phase==0&&s->depth==0)s->host=NULL;
    return n;
}
static int32_t open_source(drift_win_rng *s)
{
    int32_t result=drift_rng_open(s);
    if(result<0&&s->phase){int32_t cleanup=drift_rng_close(s);if(cleanup<0)result=cleanup;}
    leave(s);return result;
}
int32_t drift_windows_viorng_open_mmio(drift_win_rng *s,drift_hw *h,uint32_t region,drift_win_rng_host *host)
{
    if(!s)return DRIFT_WIN_RNG_INVALID_PARAMETER;
    if(s->phase||s->depth)return DRIFT_WIN_RNG_BUSY;
    int32_t n=enter(s);if(n<0)return n;
    s->host=host;s->hw=h;s->pci=NULL;s->bars=NULL;s->mode=1;s->region=region;s->bdf=0;
    return open_source(s);
}
int32_t drift_windows_viorng_open_function(drift_win_rng *s,drift_hw *h,drift_pci *pci,uint32_t bdf,drift_virtio_bar *bars,drift_win_rng_host *host)
{
    if(!s)return DRIFT_WIN_RNG_INVALID_PARAMETER;
    if(s->phase||s->depth)return DRIFT_WIN_RNG_BUSY;
    int32_t n=enter(s);if(n<0)return n;
    s->host=host;s->hw=h;s->pci=pci;s->bars=bars;s->mode=2;s->bdf=bdf;s->region=0;
    return open_source(s);
}
int32_t drift_windows_viorng_read(drift_win_rng *s,drift_win_rng_request *r,uint8_t *p,size_t capacity,size_t n)
{
    int32_t status=enter(s);if(status<0)return status;
    status=drift_rng_submit(s,r,p,capacity,n);leave(s);return status;
}
int32_t drift_windows_viorng_pump(drift_win_rng *s)
{
    int32_t status=enter(s);if(status<0)return status;
    status=drift_rng_pump(s);int32_t count=leave(s);return status<0?status:count;
}
int32_t drift_windows_viorng_cancel(drift_win_rng *s,drift_win_rng_request *r)
{
    int32_t status=enter(s);if(status<0)return status;
    status=drift_rng_cancel(s,r);leave(s);return status;
}
int32_t drift_windows_viorng_close(drift_win_rng *s)
{
    int32_t status=enter(s);if(status<0)return status;
    status=drift_rng_close(s);leave(s);return status;
}
