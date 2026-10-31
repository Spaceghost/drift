/* Deliberately closed source API, not a Windows binary ABI. SPDX-License-Identifier: MIT */
#ifndef DRIFT_MOU_WDF_COMPAT_H
#define DRIFT_MOU_WDF_COMPAT_H
#include "drift_windows_moufiltr.h"
#include <stdint.h>
#include <stddef.h>
#define IN
#define OUT
#define _In_
#define _Out_
#define _Inout_
#define DBG 0
#define PAGED_CODE() ((void)0) /* checked owner-domain entry; no pageable storage */
#define UNREFERENCED_PARAMETER(x) ((void)(x))
#define TRUE 1
#define FALSE 0
#define VOID void
typedef void *PVOID;
typedef uint8_t UCHAR, BOOLEAN, *PUCHAR, *PBOOLEAN;
typedef uint16_t USHORT;
typedef uint32_t ULONG, *PULONG;
typedef int32_t LONG, NTSTATUS;
typedef drift_win_mou_object DEVICE_OBJECT, *PDEVICE_OBJECT, DRIVER_OBJECT, *PDRIVER_OBJECT;
typedef struct { uint16_t Length, MaximumLength; uint16_t *Buffer; } UNICODE_STRING, *PUNICODE_STRING;
typedef drift_win_mou_object *WDFDRIVER, *WDFDEVICE, *WDFQUEUE, *WDFIOTARGET;
typedef drift_win_mou_object WDFDEVICE_INIT, *PWDFDEVICE_INIT;
typedef drift_win_mou_request *WDFREQUEST;
typedef drift_win_mouse_packet MOUSE_INPUT_DATA, *PMOUSE_INPUT_DATA;
typedef drift_win_mouse_connect CONNECT_DATA, *PCONNECT_DATA;
typedef drift_win_mouse_service PSERVICE_CALLBACK_ROUTINE;
typedef struct { uint8_t Bytes[4]; } OUTPUT_PACKET, *POUTPUT_PACKET;
typedef enum { DriftMouseIdle } MOUSE_STATE, *PMOUSE_STATE;
typedef enum { DriftMouseResetIdle } MOUSE_RESET_SUBSTATE, *PMOUSE_RESET_SUBSTATE;
typedef BOOLEAN (*PI8042_MOUSE_ISR)(PVOID,PMOUSE_INPUT_DATA,POUTPUT_PACKET,UCHAR,PUCHAR,PBOOLEAN,PMOUSE_STATE,PMOUSE_RESET_SUBSTATE);
typedef VOID (*PI8042_ISR_WRITE_PORT)(PVOID,UCHAR);
typedef VOID (*PI8042_QUEUE_PACKET)(PVOID);
typedef struct {
    PVOID Context;
    PI8042_MOUSE_ISR IsrRoutine;
    PI8042_ISR_WRITE_PORT IsrWritePort;
    PI8042_QUEUE_PACKET QueueMousePacket;
    PVOID CallContext;
} INTERNAL_I8042_HOOK_MOUSE, *PINTERNAL_I8042_HOOK_MOUSE;
typedef NTSTATUS DRIVER_INITIALIZE(PDRIVER_OBJECT,PUNICODE_STRING);
typedef NTSTATUS EVT_WDF_DRIVER_DEVICE_ADD(WDFDRIVER,PWDFDEVICE_INIT);
typedef VOID EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL(WDFQUEUE,WDFREQUEST,size_t,size_t,ULONG);
typedef struct { uint32_t Size; drift_win_mou_add_fn EvtDriverDeviceAdd; } WDF_DRIVER_CONFIG;
typedef struct { uint32_t Size; size_t ContextSize; } WDF_OBJECT_ATTRIBUTES;
typedef struct { uint32_t Size, DispatchType, DefaultQueue; drift_win_mou_control_fn EvtIoInternalDeviceControl; } WDF_IO_QUEUE_CONFIG;
typedef struct { uint32_t Size, Flags; } WDF_REQUEST_SEND_OPTIONS;
#define WDF_NO_OBJECT_ATTRIBUTES NULL
#define WDF_NO_HANDLE NULL
#define WdfIoQueueDispatchParallel 2
#define WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET 8
#define FILE_DEVICE_MOUSE 0x0f
#define STATUS_SUCCESS DRIFT_WIN_SUCCESS
#define STATUS_SHARING_VIOLATION DRIFT_WIN_SHARING_VIOLATION
#define STATUS_NOT_IMPLEMENTED DRIFT_WIN_NOT_IMPLEMENTED
#define NT_SUCCESS(x) ((int32_t)(x)>=0)
#define IOCTL_INTERNAL_MOUSE_CONNECT DRIFT_WIN_MOUSE_CONNECT
#define IOCTL_INTERNAL_MOUSE_DISCONNECT DRIFT_WIN_MOUSE_DISCONNECT
#define IOCTL_INTERNAL_I8042_HOOK_MOUSE DRIFT_WIN_MOUSE_HOOK
#define IOCTL_MOUSE_QUERY_ATTRIBUTES DRIFT_WIN_MOUSE_QUERY_ATTRIBUTES
#define WDF_DRIVER_CONFIG_INIT(c,f) do { (c)->Size=sizeof(*(c));(c)->EvtDriverDeviceAdd=(f); } while(0)
#define WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(a,t) do { (a)->Size=sizeof(*(a));(a)->ContextSize=sizeof(t); } while(0)
#define WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(c,t) do { (c)->Size=sizeof(*(c));(c)->DispatchType=(t);(c)->DefaultQueue=1;(c)->EvtIoInternalDeviceControl=NULL; } while(0)
#define WDF_REQUEST_SEND_OPTIONS_INIT(o,f) do { (o)->Size=sizeof(*(o));(o)->Flags=(f); } while(0)
#define WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(t,n) static inline t *n(WDFDEVICE d) { return (t *)drift_wdf_context(d); }

NTSTATUS WdfDriverCreate(PDRIVER_OBJECT,PVOID,PVOID,PVOID,WDFDRIVER *);
VOID WdfFdoInitSetFilter(PWDFDEVICE_INIT);
VOID WdfDeviceInitSetDeviceType(PWDFDEVICE_INIT,ULONG);
NTSTATUS WdfDeviceCreate(PWDFDEVICE_INIT *,PVOID,WDFDEVICE *);
NTSTATUS WdfIoQueueCreate(WDFDEVICE,PVOID,PVOID,WDFQUEUE *);
BOOLEAN WdfRequestSend(WDFREQUEST,WDFIOTARGET,PVOID);
NTSTATUS WdfRequestGetStatus(WDFREQUEST);
VOID WdfRequestComplete(WDFREQUEST,NTSTATUS);
WDFDEVICE WdfIoQueueGetDevice(WDFQUEUE);
NTSTATUS WdfRequestRetrieveInputBuffer(WDFREQUEST,size_t,PVOID,size_t *);
PDEVICE_OBJECT WdfDeviceWdmGetDeviceObject(WDFDEVICE);
WDFIOTARGET WdfDeviceGetIoTarget(WDFDEVICE);
WDFDEVICE WdfWdmDeviceGetWdfDeviceHandle(PDEVICE_OBJECT);
PVOID drift_wdf_context(WDFDEVICE);

/* C bindings only: typed callback and object-storage access. Runtime state
 * transitions and request semantics live in compat.nelua. */
static inline int32_t drift_mou_host_valid(drift_win_mou_host *h) { return h&&h->send&&h->completed; }
static inline void drift_mou_object_init(drift_win_moufiltr *s) {
    drift_win_mou_object *o[]={&s->driver,&s->device,&s->queue,&s->target,&s->init,&s->driver_object,&s->device_object};
    for(unsigned i=0;i<7;++i) { o[i]->owner=s;o[i]->kind=i+1;o[i]->context=NULL; }
    for(unsigned i=0;i<16;++i)s->extension[i]=0;
    s->add=NULL;s->control=NULL;
}
static inline int32_t drift_mou_config_valid(WDF_DRIVER_CONFIG *c) { return c&&c->Size==sizeof(*c)&&c->EvtDriverDeviceAdd; }
static inline void drift_mou_save_config(drift_win_moufiltr *s,WDF_DRIVER_CONFIG *c) { s->add=c->EvtDriverDeviceAdd; }
static inline drift_win_mou_object *drift_mou_object(drift_win_moufiltr *s,uint32_t n) {
    switch(n) { case 1:return &s->driver;case 2:return &s->device;case 3:return &s->queue;case 4:return &s->target;case 5:return &s->init;case 6:return &s->driver_object;case 7:return &s->device_object;default:return NULL; }
}
static inline int32_t drift_mou_attributes_valid(WDF_OBJECT_ATTRIBUTES *a) { return a&&a->Size==sizeof(*a)&&a->ContextSize>0&&a->ContextSize<=128; }
static inline int32_t drift_mou_queue_valid(WDF_IO_QUEUE_CONFIG *c) { return c&&c->Size==sizeof(*c)&&c->DispatchType==WdfIoQueueDispatchParallel&&c->DefaultQueue==1&&c->EvtIoInternalDeviceControl; }
static inline void drift_mou_save_queue(drift_win_moufiltr *s,WDF_IO_QUEUE_CONFIG *c) { s->control=c->EvtIoInternalDeviceControl; }
static inline int32_t drift_mou_send_valid(WDF_REQUEST_SEND_OPTIONS *o) { return o&&o->Size==sizeof(*o)&&o->Flags==WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET; }
static inline void drift_mou_device_result(PWDFDEVICE_INIT *i,WDFDEVICE *d,drift_win_moufiltr *s) { *i=NULL;*d=&s->device; }
static inline int32_t drift_mou_lower_send(drift_win_moufiltr *s,drift_win_mou_request *r) { return s->host->send(s->host->context,r,r->control); }
static inline void drift_mou_notify(drift_win_moufiltr *s,drift_win_mou_request *r) { s->host->completed(s->host->context,r); }
static inline void *drift_mou_extension(drift_win_moufiltr *s) { return s->extension; }
static inline int32_t drift_mou_buffer_valid(drift_win_mou_request *r) {
    return r->input&&((uintptr_t)r->input%_Alignof(void*)==0);
}
static inline void drift_mou_buffer_result(drift_win_mou_request *r,PVOID *p,size_t *n) { *p=r->input;if(n)*n=r->input_length; }
#endif
