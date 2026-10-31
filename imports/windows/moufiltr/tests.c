/* Independent request/lower-stack model of the actual imported source.
 * SPDX-License-Identifier: MIT */
#include "compat/wdf_compat.h"
#include "upstream/moufiltr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

static unsigned checks;
int32_t drift_mou_begin(drift_win_moufiltr *,drift_win_mou_request *,uint32_t,void *,size_t);
int32_t drift_mou_end(drift_win_mou_request *);
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %u: %s\n",__LINE__,#x);exit(1);} } while(0)
#ifndef DRIFT_MOU_NO_WRAP
static int allocation_fault;
NTSTATUS __real_WdfDriverCreate(PDRIVER_OBJECT,PVOID,PVOID,PVOID,WDFDRIVER *);
NTSTATUS __real_WdfDeviceCreate(PWDFDEVICE_INIT *,PVOID,WDFDEVICE *);
NTSTATUS __real_WdfIoQueueCreate(WDFDEVICE,PVOID,PVOID,WDFQUEUE *);
NTSTATUS __wrap_WdfDriverCreate(PDRIVER_OBJECT a,PVOID b,PVOID c,PVOID d,WDFDRIVER *e)
{return allocation_fault==1?DRIFT_WIN_INSUFFICIENT_RESOURCES:__real_WdfDriverCreate(a,b,c,d,e);}
NTSTATUS __wrap_WdfDeviceCreate(PWDFDEVICE_INIT *a,PVOID b,WDFDEVICE *c)
{return allocation_fault==2?DRIFT_WIN_INSUFFICIENT_RESOURCES:__real_WdfDeviceCreate(a,b,c);}
NTSTATUS __wrap_WdfIoQueueCreate(WDFDEVICE a,PVOID b,PVOID c,WDFQUEUE *d)
{return allocation_fault==3?DRIFT_WIN_INSUFFICIENT_RESOURCES:__real_WdfIoQueueCreate(a,b,c,d);}
#endif

enum { INLINE, PENDING, FAILED, CONTRADICTORY };
typedef struct model {
    drift_win_moufiltr state;
    drift_win_mou_host host;
    drift_win_mou_object upper;
    unsigned mode,sends,completes,packet_calls,isr_calls,nested,inside;
    drift_win_mou_request nested_request;
    INTERNAL_I8042_HOOK_MOUSE hook;
    BOOLEAN isr_return,isr_continue;
} model;

static BOOLEAN upper_isr(PVOID context,PMOUSE_INPUT_DATA input,POUTPUT_PACKET output,
    UCHAR status,PUCHAR data,PBOOLEAN proceed,PMOUSE_STATE mouse,PMOUSE_RESET_SUBSTATE reset)
{
    model *m=context; CHECK(input&&output&&data&&proceed&&mouse&&reset); CHECK(status==0x41);
    ++m->isr_calls;*data=0x5a;*proceed=m->isr_continue;return m->isr_return;
}
static void write_port(PVOID c,UCHAR b){(void)c;(void)b;}
static void queue_packet(PVOID c){(void)c;}
static void upper_packets(drift_win_mou_object *upper,drift_win_mouse_packet *begin,
    drift_win_mouse_packet *end,uint32_t *consumed)
{
    model *m=upper->context;
    CHECK(begin<=end);CHECK(end-begin==3);CHECK(begin[0].LastX==123);CHECK(begin[2].LastY==-999);
    ++m->packet_calls; *consumed=2;
    /* A connected callback cannot close state while still executing. */
    CHECK(drift_windows_moufiltr_close(&m->state)==DRIFT_WIN_DEVICE_BUSY);
}
static void completed(void *context,drift_win_mou_request *r)
{
    model *m=context;++m->completes;
    CHECK(r->phase==3&&r->completions==1);
    CHECK(drift_windows_moufiltr_request_reset(r)==DRIFT_WIN_DEVICE_BUSY);
    CHECK(drift_windows_moufiltr_close(&m->state)==DRIFT_WIN_DEVICE_BUSY);
}
static int32_t lower_send(void *context,drift_win_mou_request *r,uint32_t code)
{
    model *m=context;++m->sends;
    CHECK(r->owner==&m->state&&r->phase==2&&r->completions==0);
    if(m->nested&&!m->inside&&code==DRIFT_WIN_MOUSE_CONNECT){
        m->inside=1;
        m->hook.Context=m;m->hook.IsrRoutine=upper_isr;m->hook.IsrWritePort=write_port;
        m->hook.QueueMousePacket=queue_packet;m->hook.CallContext=m;
        CHECK(drift_windows_moufiltr_control(&m->state,&m->nested_request,
            DRIFT_WIN_MOUSE_HOOK,&m->hook,sizeof(m->hook))==0);
        CHECK(m->hook.Context!=m&&m->hook.IsrRoutine==MouFilter_IsrHook);
        CHECK(m->hook.IsrWritePort==write_port&&m->hook.QueueMousePacket==queue_packet);
        m->inside=0;
    }
    if(m->mode==FAILED)return DRIFT_WIN_INSUFFICIENT_RESOURCES;
    if(m->mode==PENDING)return DRIFT_WIN_SUCCESS;
    CHECK(drift_windows_moufiltr_complete(r,DRIFT_WIN_SUCCESS,r->input_length)==0);
    if(m->mode==CONTRADICTORY)return DRIFT_WIN_INSUFFICIENT_RESOURCES;
    return DRIFT_WIN_SUCCESS;
}
static void init(model *m)
{
    memset(m,0,sizeof(*m));
    m->host=(drift_win_mou_host){m,lower_send,completed};m->upper.context=m;
    CHECK(drift_windows_moufiltr_open(&m->state,&m->host)==0);
    CHECK(m->state.phase==4&&m->state.filter==1&&m->state.device_type==15);
}
static void test_creation(void)
{
    CHECK(drift_windows_moufiltr_open(NULL,NULL)==DRIFT_WIN_INVALID_PARAMETER);
    model m;memset(&m,0,sizeof(m));
    CHECK(drift_windows_moufiltr_open(&m.state,&m.host)==DRIFT_WIN_INVALID_PARAMETER);
#ifndef DRIFT_MOU_NO_WRAP
    for(int fault=1;fault<=3;++fault){
        memset(&m,0,sizeof(m));m.host=(drift_win_mou_host){&m,lower_send,completed};
        allocation_fault=fault;
        CHECK(drift_windows_moufiltr_open(&m.state,&m.host)==DRIFT_WIN_INSUFFICIENT_RESOURCES);
        CHECK(m.state.phase==0&&m.state.host==NULL&&m.state.outstanding==0);
        allocation_fault=0;
        CHECK(drift_windows_moufiltr_open(&m.state,&m.host)==0);
        CHECK(drift_windows_moufiltr_close(&m.state)==0);
    }
#endif
    init(&m);CHECK(drift_windows_moufiltr_open(&m.state,&m.host)==DRIFT_WIN_DEVICE_BUSY);
    CHECK(drift_windows_moufiltr_close(&m.state)==0);
    CHECK(drift_windows_moufiltr_close(&m.state)==DRIFT_WIN_INVALID_DEVICE_STATE);
    CHECK(drift_windows_moufiltr_open(&m.state,&m.host)==0);
    CHECK(drift_windows_moufiltr_close(&m.state)==0);
}
static void test_connect_and_callbacks(void)
{
    model m;init(&m);m.nested=1;
    drift_win_mouse_connect connect={&m.upper,upper_packets};
    drift_win_mou_request r={0};
    CHECK(drift_windows_moufiltr_control(&m.state,&r,DRIFT_WIN_MOUSE_CONNECT,&connect,sizeof(connect))==0);
    CHECK(m.sends==2&&m.completes==2&&m.state.outstanding==0&&m.state.depth==0);
    CHECK(connect.ClassService==MouFilter_ServiceCallback&&connect.ClassDeviceObject==&m.state.device_object);
    drift_win_mouse_packet packets[3]={{0}};packets[0].LastX=123;packets[2].LastY=-999;uint32_t consumed=0;
    CHECK(drift_windows_moufiltr_packets(&m.state,packets,3,&consumed)==0);
    CHECK(consumed==2&&m.packet_calls==1);
    CHECK(drift_windows_moufiltr_request_reset(&r)==0);
    drift_win_mouse_connect second={&m.upper,upper_packets};
    CHECK(drift_windows_moufiltr_control(&m.state,&r,DRIFT_WIN_MOUSE_CONNECT,&second,sizeof(second))==DRIFT_WIN_SHARING_VIOLATION);
    CHECK(m.sends==2&&r.completions==1&&second.ClassService==upper_packets);
    CHECK(drift_windows_moufiltr_request_reset(&r)==0);
    CHECK(drift_windows_moufiltr_control(&m.state,&r,DRIFT_WIN_MOUSE_DISCONNECT,NULL,0)==DRIFT_WIN_NOT_IMPLEMENTED);
    CHECK(m.sends==2&&r.completions==1);
    MOUSE_INPUT_DATA input={0};OUTPUT_PACKET output={{0}};UCHAR data=0;BOOLEAN proceed=FALSE;
    MOUSE_STATE mouse=DriftMouseIdle;MOUSE_RESET_SUBSTATE reset=DriftMouseResetIdle;
    m.isr_return=TRUE;m.isr_continue=TRUE;
    CHECK(m.hook.IsrRoutine(m.hook.Context,&input,&output,0x41,&data,&proceed,&mouse,&reset)==TRUE);
    CHECK(proceed==TRUE&&data==0x5a&&m.isr_calls==1);
    m.isr_return=FALSE;proceed=TRUE;
    CHECK(m.hook.IsrRoutine(m.hook.Context,&input,&output,0x41,&data,&proceed,&mouse,&reset)==FALSE);
    m.isr_return=TRUE;m.isr_continue=FALSE;proceed=TRUE;
    CHECK(m.hook.IsrRoutine(m.hook.Context,&input,&output,0x41,&data,&proceed,&mouse,&reset)==TRUE);
    CHECK(proceed==FALSE&&m.isr_calls==3);
    CHECK(drift_windows_moufiltr_close(&m.state)==0);
    CHECK(drift_windows_moufiltr_packets(&m.state,packets,3,&consumed)==DRIFT_WIN_INVALID_PARAMETER);
}
static void test_requests(void)
{
    model m;init(&m);drift_win_mou_request a={0},b={0};
    m.mode=PENDING;
    CHECK(drift_windows_moufiltr_control(&m.state,&a,DRIFT_WIN_MOUSE_QUERY_ATTRIBUTES,NULL,0)==DRIFT_WIN_PENDING);
    CHECK(drift_windows_moufiltr_control(&m.state,&b,0x12345678,NULL,0)==DRIFT_WIN_PENDING);
    CHECK(m.state.outstanding==2&&m.completes==0);
    CHECK(drift_windows_moufiltr_complete(&a,DRIFT_WIN_PENDING,0)==DRIFT_WIN_INVALID_PARAMETER);
    CHECK(a.phase==2&&a.completions==0&&m.completes==0&&m.state.outstanding==2);
    CHECK(drift_windows_moufiltr_close(&m.state)==DRIFT_WIN_DEVICE_BUSY);
    CHECK(drift_windows_moufiltr_request_reset(&a)==DRIFT_WIN_DEVICE_BUSY);
    CHECK(drift_windows_moufiltr_control(&m.state,&a,0,NULL,0)==DRIFT_WIN_DEVICE_BUSY);
    CHECK(drift_windows_moufiltr_complete(&b,0,77)==0);
    CHECK(b.information==77&&m.state.outstanding==1&&m.completes==1);
    CHECK(drift_windows_moufiltr_complete(&b,DRIFT_WIN_INVALID_PARAMETER,0)==DRIFT_WIN_INVALID_DEVICE_STATE);
    CHECK(b.status==0&&b.information==77&&m.completes==1);
    CHECK(drift_windows_moufiltr_complete(&a,DRIFT_WIN_INSUFFICIENT_RESOURCES,0)==0);
    CHECK(a.status==DRIFT_WIN_INSUFFICIENT_RESOURCES&&m.state.outstanding==0&&m.completes==2);
    CHECK(drift_windows_moufiltr_request_reset(&a)==0);
    m.mode=FAILED;
    CHECK(drift_windows_moufiltr_control(&m.state,&a,DRIFT_WIN_MOUSE_QUERY_ATTRIBUTES,NULL,0)==DRIFT_WIN_INSUFFICIENT_RESOURCES);
    CHECK(a.phase==3&&a.completions==1&&m.state.outstanding==0);
    CHECK(drift_windows_moufiltr_request_reset(&a)==0);
    m.mode=CONTRADICTORY;
    CHECK(drift_windows_moufiltr_control(&m.state,&a,DRIFT_WIN_MOUSE_QUERY_ATTRIBUTES,NULL,0)==DRIFT_WIN_INVALID_DEVICE_STATE);
    CHECK(a.phase==3&&a.completions==1&&m.state.outstanding==0);
    CHECK(drift_windows_moufiltr_close(&m.state)==0);
}
static void test_buffers_and_bounds(void)
{
    model m;init(&m);drift_win_mou_request r={0};
    _Alignas(void *) unsigned char data[sizeof(drift_win_mouse_connect)+8]={0};
    CHECK(drift_windows_moufiltr_control(&m.state,&r,DRIFT_WIN_MOUSE_CONNECT,data,1)==DRIFT_WIN_BUFFER_TOO_SMALL);
    CHECK(r.completions==1&&m.sends==0);CHECK(drift_windows_moufiltr_request_reset(&r)==0);
    CHECK(drift_windows_moufiltr_control(&m.state,&r,DRIFT_WIN_MOUSE_CONNECT,NULL,sizeof(drift_win_mouse_connect))==DRIFT_WIN_INVALID_PARAMETER);
    CHECK(drift_windows_moufiltr_request_reset(&r)==0);
    CHECK(drift_windows_moufiltr_control(&m.state,&r,DRIFT_WIN_MOUSE_CONNECT,data+1,sizeof(drift_win_mouse_connect))==DRIFT_WIN_INVALID_PARAMETER);
    CHECK(m.sends==0&&m.state.outstanding==0);
    drift_win_mou_object forged=m.state.queue;
    CHECK(WdfIoQueueGetDevice(&forged)==NULL);
    m.mode=PENDING;drift_win_mou_request requests[65]={{0}};
    for(unsigned i=0;i<64;++i)CHECK(drift_windows_moufiltr_control(&m.state,&requests[i],0,NULL,0)==DRIFT_WIN_PENDING);
    CHECK(drift_windows_moufiltr_control(&m.state,&requests[64],0,NULL,0)==DRIFT_WIN_DEVICE_BUSY);
    CHECK(m.state.outstanding==64&&requests[64].phase==0);
    for(unsigned i=0;i<64;++i)CHECK(drift_windows_moufiltr_complete(&requests[i],0,0)==0);
    CHECK(m.state.outstanding==0);CHECK(drift_windows_moufiltr_close(&m.state)==0);
}
static void test_instance_isolation(void)
{
    model a,b;init(&a);init(&b);
    drift_win_mou_request ar={0},br={0};
    drift_win_mouse_connect ac={&a.upper,upper_packets},bc={&b.upper,upper_packets};
    CHECK(drift_windows_moufiltr_control(&a.state,&ar,DRIFT_WIN_MOUSE_CONNECT,&ac,sizeof(ac))==0);
    CHECK(drift_windows_moufiltr_control(&b.state,&br,DRIFT_WIN_MOUSE_CONNECT,&bc,sizeof(bc))==0);
    CHECK(ac.ClassDeviceObject!=bc.ClassDeviceObject&&ac.ClassService==bc.ClassService);
    drift_win_mouse_packet packets[3]={{0}};packets[0].LastX=123;packets[2].LastY=-999;uint32_t consumed=0;
    CHECK(drift_windows_moufiltr_packets(&a.state,packets,3,&consumed)==0);
    CHECK(a.packet_calls==1&&b.packet_calls==0);
    CHECK(drift_windows_moufiltr_packets(&b.state,packets,3,&consumed)==0);
    CHECK(a.packet_calls==1&&b.packet_calls==1);
    CHECK(drift_windows_moufiltr_request_reset(&ar)==0&&drift_windows_moufiltr_request_reset(&br)==0);
    a.mode=PENDING;b.mode=PENDING;
    CHECK(drift_windows_moufiltr_control(&a.state,&ar,0,NULL,0)==DRIFT_WIN_PENDING);
    CHECK(drift_windows_moufiltr_control(&b.state,&br,0,NULL,0)==DRIFT_WIN_PENDING);
    WDF_REQUEST_SEND_OPTIONS options;WDF_REQUEST_SEND_OPTIONS_INIT(&options,WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);
    drift_win_mou_request local={0};
    CHECK(drift_mou_begin(&a.state,&local,0,NULL,0)==0);
    CHECK(WdfRequestSend(&local,&b.state.target,&options)==FALSE);
    CHECK(a.sends==2&&b.sends==2&&local.phase==1&&local.status==DRIFT_WIN_INVALID_PARAMETER);
    WdfRequestComplete(&local,local.status);
    CHECK(drift_mou_end(&local)==DRIFT_WIN_INVALID_PARAMETER);
    CHECK(drift_windows_moufiltr_complete(&ar,0,1)==0);
    CHECK(a.state.outstanding==0&&b.state.outstanding==1&&br.phase==2);
    CHECK(drift_windows_moufiltr_close(&a.state)==0);
    CHECK(drift_windows_moufiltr_complete(&br,0,2)==0);
    CHECK(drift_windows_moufiltr_close(&b.state)==0);
    /* Completed records can reject a duplicate without dereferencing a freed owner. */
    model *heap=malloc(sizeof(*heap));CHECK(heap!=NULL);init(heap);
    drift_win_mou_request done={0};CHECK(drift_windows_moufiltr_control(&heap->state,&done,0,NULL,0)==0);
    CHECK(drift_windows_moufiltr_close(&heap->state)==0);free(heap);
    CHECK(drift_windows_moufiltr_complete(&done,0,0)==DRIFT_WIN_INVALID_DEVICE_STATE);
    CHECK(drift_windows_moufiltr_request_reset(&done)==0);
}
int main(void)
{
    test_creation();test_connect_and_callbacks();test_requests();test_buffers_and_bounds();test_instance_isolation();
    printf("Windows Moufiltr: %u checks passed (actual source, nested/pending requests, callbacks, ownership and failures)\n",checks);
    return 0;
}
