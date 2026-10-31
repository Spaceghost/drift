/* Typed entry/lifecycle bridge; the imported source below is unchanged.
 * SPDX-License-Identifier: MIT */
#include "compat/wdf_compat.h"
#include "upstream/moufiltr.c"

_Static_assert(sizeof(DEVICE_EXTENSION)<=sizeof(((drift_win_moufiltr *)0)->extension),"context storage");
_Static_assert(_Alignof(DEVICE_EXTENSION)<=_Alignof(drift_win_moufiltr),"context alignment");
_Static_assert(sizeof(MOUSE_INPUT_DATA)==24,"packet layout");
_Static_assert(sizeof(ULONG)==4&&sizeof(LONG)==4&&sizeof(NTSTATUS)==4,"Windows fixed-width types");

int32_t drift_mou_prepare(drift_win_moufiltr *,drift_win_mou_host *);
int32_t drift_mou_open_finish(drift_win_moufiltr *,int32_t);
int32_t drift_mou_begin(drift_win_moufiltr *,drift_win_mou_request *,uint32_t,void *,size_t);
int32_t drift_mou_end(drift_win_mou_request *);

int32_t drift_windows_moufiltr_open(drift_win_moufiltr *s,drift_win_mou_host *host)
{
    int32_t status=drift_mou_prepare(s,host);
    if(status<0)return status;
    UNICODE_STRING registry={0,0,NULL};
    status=DriverEntry(&s->driver_object,&registry);
    if(status>=0){
        if(!s->add)status=DRIFT_WIN_INVALID_DEVICE_STATE;
        else status=s->add(&s->driver,&s->init);
    }
    return drift_mou_open_finish(s,status);
}

int32_t drift_windows_moufiltr_control(drift_win_moufiltr *s,drift_win_mou_request *r,
    uint32_t control,void *input,size_t length)
{
    int32_t status=drift_mou_begin(s,r,control,input,length);
    if(status<0)return status;
    s->control(&s->queue,r,0,length,control);
    return drift_mou_end(r);
}

int32_t drift_windows_moufiltr_packets(drift_win_moufiltr *s,drift_win_mouse_packet *packets,
    uint32_t count,uint32_t *consumed)
{
    if(!s||s->phase!=4||!packets||!consumed||count>65536)return DRIFT_WIN_INVALID_PARAMETER;
    DEVICE_EXTENSION *ext=(DEVICE_EXTENSION *)s->extension;
    if(!ext->UpperConnectData.ClassService||!ext->UpperConnectData.ClassDeviceObject)
        return DRIFT_WIN_INVALID_DEVICE_STATE;
    if(s->depth>=64)return DRIFT_WIN_DEVICE_BUSY;
    ++s->depth;
    MouFilter_ServiceCallback(&s->device_object,packets,packets+count,consumed);
    --s->depth;
    return DRIFT_WIN_SUCCESS;
}
