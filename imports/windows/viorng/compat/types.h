/* Closed source-API definitions for the pinned Windows Virtio RNG driver.
 * These are profile representations, not Windows kernel binary ABI layouts.
 * SPDX-License-Identifier: MIT */
#ifndef DRIFT_WINDOWS_VIORNG_TYPES_H
#define DRIFT_WINDOWS_VIORNG_TYPES_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include "drift_windows_viorng.h"

#define IN
#define OUT
#define _In_
#define _Out_
#define _Inout_
#define _IRQL_requires_(level)
#define UNREFERENCED_PARAMETER(x) ((void)(x))
#define PAGED_CODE() ((void)0) /* no pageable code; owner-domain contract is external */
#define VOID void
#define TRUE 1
#define FALSE 0
#define PASSIVE_LEVEL 0
#define PAGE_SIZE 4096U
#define NonPagedPool 0
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

typedef uint8_t UCHAR, BOOLEAN, *PUCHAR;
typedef uint16_t USHORT;
typedef uint32_t ULONG, *PULONG;
typedef int32_t LONG, NTSTATUS;
typedef uint64_t ULONGLONG;
typedef uintptr_t ULONG_PTR;
typedef void *PVOID;
typedef struct { int64_t QuadPart; } PHYSICAL_ADDRESS;
typedef struct { uint16_t Length, MaximumLength; uint16_t *Buffer; } UNICODE_STRING, *PUNICODE_STRING;
typedef struct drift_rng_guid { uint32_t Data1; uint16_t Data2,Data3; uint8_t Data4[8]; } GUID;
#if defined(__GNUC__)
#define DRIFT_RNG_HEADER_UNUSED __attribute__((unused))
#else
#define DRIFT_RNG_HEADER_UNUSED
#endif
/* Value metadata local to each translation unit; no linked Windows GUID ABI. */
#define DEFINE_GUID(n,a,b,c,d,e,f,g,h,i,j,k) \
    static GUID n DRIFT_RNG_HEADER_UNUSED = {a,b,c,{d,e,f,g,h,i,j,k}}

#define STATUS_SUCCESS ((NTSTATUS)0)
#define STATUS_PENDING ((NTSTATUS)0x103)
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xc0000001)
#define STATUS_INVALID_HANDLE ((NTSTATUS)0xc0000008)
#define STATUS_INVALID_PARAMETER ((NTSTATUS)0xc000000d)
#define STATUS_INVALID_DEVICE_REQUEST ((NTSTATUS)0xc0000010)
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS)0xc0000023)
#define STATUS_INSUFFICIENT_RESOURCES ((NTSTATUS)0xc000009a)
#define STATUS_NOT_SUPPORTED ((NTSTATUS)0xc00000bb)
#define STATUS_CANCELLED ((NTSTATUS)0xc0000120)
#define STATUS_INVALID_DEVICE_STATE ((NTSTATUS)0xc0000184)
#define NT_SUCCESS(s) ((NTSTATUS)(s)>=0)
void drift_win_rng_assert_fail(void);
#define NT_ASSERT(x) do { if (!(x)) drift_win_rng_assert_fail(); } while(0)

typedef struct _SINGLE_LIST_ENTRY { struct _SINGLE_LIST_ENTRY *Next; } SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;
static inline PSINGLE_LIST_ENTRY PushEntryList(PSINGLE_LIST_ENTRY head, PSINGLE_LIST_ENTRY entry)
{
    PSINGLE_LIST_ENTRY old=head->Next; entry->Next=old;head->Next=entry;return old;
}
static inline PSINGLE_LIST_ENTRY PopEntryList(PSINGLE_LIST_ENTRY head)
{
    PSINGLE_LIST_ENTRY entry=head->Next;if(entry)head->Next=entry->Next;return entry;
}
#define CONTAINING_RECORD(address,type,field) ((type *)((unsigned char *)(address)-offsetof(type,field)))

typedef drift_win_rng_object DRIVER_OBJECT, *PDRIVER_OBJECT, WDFDEVICE_INIT, *PWDFDEVICE_INIT;
typedef drift_win_rng_object *WDFOBJECT, *WDFDRIVER, *WDFDEVICE, *WDFQUEUE;
typedef drift_win_rng_object *WDFINTERRUPT, *WDFSPINLOCK, *WDFCMRESLIST;
typedef drift_win_rng_request *WDFREQUEST;
typedef int32_t WDF_POWER_DEVICE_STATE;
enum { WdfPowerDeviceD0=1,WdfPowerDeviceD3=4,WdfPowerDeviceD3Final=5 };
enum { WdfDeviceIoDirect=3,WdfRequestTypeRead=3,WdfIoQueueDispatchSequential=1 };
enum { WdfRequestStopActionSuspend=1,WdfRequestStopActionPurge=2 };

typedef NTSTATUS DRIVER_INITIALIZE(PDRIVER_OBJECT,PUNICODE_STRING);
typedef NTSTATUS EVT_WDF_DRIVER_DEVICE_ADD(WDFDRIVER,PWDFDEVICE_INIT);
typedef VOID EVT_WDF_OBJECT_CONTEXT_CLEANUP(WDFOBJECT);
typedef NTSTATUS EVT_WDF_DEVICE_PREPARE_HARDWARE(WDFDEVICE,WDFCMRESLIST,WDFCMRESLIST);
typedef NTSTATUS EVT_WDF_DEVICE_RELEASE_HARDWARE(WDFDEVICE,WDFCMRESLIST);
typedef NTSTATUS EVT_WDF_DEVICE_D0_ENTRY(WDFDEVICE,WDF_POWER_DEVICE_STATE);
typedef NTSTATUS EVT_WDF_DEVICE_D0_EXIT(WDFDEVICE,WDF_POWER_DEVICE_STATE);
typedef BOOLEAN EVT_WDF_INTERRUPT_ISR(WDFINTERRUPT,ULONG);
typedef VOID EVT_WDF_INTERRUPT_DPC(WDFINTERRUPT,WDFOBJECT);
typedef NTSTATUS EVT_WDF_INTERRUPT_ENABLE(WDFINTERRUPT,WDFDEVICE);
typedef NTSTATUS EVT_WDF_INTERRUPT_DISABLE(WDFINTERRUPT,WDFDEVICE);
typedef VOID EVT_WDF_IO_QUEUE_IO_READ(WDFQUEUE,WDFREQUEST,size_t);
typedef VOID EVT_WDF_IO_QUEUE_IO_STOP(WDFQUEUE,WDFREQUEST,ULONG);
typedef VOID EVT_WDF_REQUEST_CANCEL(WDFREQUEST);

typedef struct {
    ULONG Size;
    EVT_WDF_DRIVER_DEVICE_ADD *EvtDriverDeviceAdd;
} WDF_DRIVER_CONFIG;
typedef struct {
    ULONG Size;
    size_t ContextSize;
    WDFOBJECT ParentObject;
    EVT_WDF_OBJECT_CONTEXT_CLEANUP *EvtCleanupCallback;
} WDF_OBJECT_ATTRIBUTES;
typedef struct {
    ULONG Size;
    EVT_WDF_DEVICE_PREPARE_HARDWARE *EvtDevicePrepareHardware;
    EVT_WDF_DEVICE_RELEASE_HARDWARE *EvtDeviceReleaseHardware;
    EVT_WDF_DEVICE_D0_ENTRY *EvtDeviceD0Entry;
    EVT_WDF_DEVICE_D0_EXIT *EvtDeviceD0Exit;
} WDF_PNPPOWER_EVENT_CALLBACKS;
typedef struct {
    ULONG Size;
    EVT_WDF_INTERRUPT_ISR *EvtInterruptIsr;
    EVT_WDF_INTERRUPT_DPC *EvtInterruptDpc;
    EVT_WDF_INTERRUPT_ENABLE *EvtInterruptEnable;
    EVT_WDF_INTERRUPT_DISABLE *EvtInterruptDisable;
} WDF_INTERRUPT_CONFIG;
typedef struct { ULONG Size; BOOLEAN MessageSignaled; } WDF_INTERRUPT_INFO;
typedef struct {
    ULONG Size,DispatchType;
    BOOLEAN AllowZeroLengthRequests;
    EVT_WDF_IO_QUEUE_IO_READ *EvtIoRead;
    EVT_WDF_IO_QUEUE_IO_STOP *EvtIoStop;
} WDF_IO_QUEUE_CONFIG;

#define WDF_NO_HANDLE NULL
#define WDF_NO_OBJECT_ATTRIBUTES NULL
#define WDF_DRIVER_CONFIG_INIT(c,f) do { *(c)=(WDF_DRIVER_CONFIG){0};(c)->Size=sizeof(*(c));(c)->EvtDriverDeviceAdd=(f); } while(0)
#define WDF_OBJECT_ATTRIBUTES_INIT(a) do { *(a)=(WDF_OBJECT_ATTRIBUTES){0};(a)->Size=sizeof(*(a)); } while(0)
#define WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(a,t) do { WDF_OBJECT_ATTRIBUTES_INIT(a);(a)->ContextSize=sizeof(t); } while(0)
#define WDF_PNPPOWER_EVENT_CALLBACKS_INIT(c) do { *(c)=(WDF_PNPPOWER_EVENT_CALLBACKS){0};(c)->Size=sizeof(*(c)); } while(0)
#define WDF_INTERRUPT_CONFIG_INIT(c,i,d) do { *(c)=(WDF_INTERRUPT_CONFIG){0};(c)->Size=sizeof(*(c));(c)->EvtInterruptIsr=(i);(c)->EvtInterruptDpc=(d); } while(0)
#define WDF_INTERRUPT_INFO_INIT(i) do { *(i)=(WDF_INTERRUPT_INFO){0};(i)->Size=sizeof(*(i)); } while(0)
#define WDF_IO_QUEUE_CONFIG_INIT(c,t) do { *(c)=(WDF_IO_QUEUE_CONFIG){0};(c)->Size=sizeof(*(c));(c)->DispatchType=(t); } while(0)
PVOID drift_win_rng_get_context(WDFOBJECT);
#define WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(t,n) static inline t *n(WDFOBJECT o) { return (t *)drift_win_rng_get_context(o); }

/* Typed handles, generic configuration pointers. All functional implementations
 * live in the Nelua personality; there are no general fallback WDF calls. */
NTSTATUS WdfDriverCreate(PDRIVER_OBJECT,PVOID,PVOID,PVOID,WDFDRIVER *);
VOID WdfDeviceInitSetPnpPowerEventCallbacks(PWDFDEVICE_INIT,PVOID);
VOID WdfDeviceInitSetIoType(PWDFDEVICE_INIT,ULONG);
NTSTATUS WdfDeviceCreate(PWDFDEVICE_INIT *,PVOID,WDFDEVICE *);
NTSTATUS WdfInterruptCreate(WDFDEVICE,PVOID,PVOID,WDFINTERRUPT *);
NTSTATUS WdfSpinLockCreate(PVOID,WDFSPINLOCK *);
NTSTATUS WdfDeviceCreateDeviceInterface(WDFDEVICE,PVOID,PVOID);
NTSTATUS WdfIoQueueCreate(WDFDEVICE,PVOID,PVOID,WDFQUEUE *);
NTSTATUS WdfDeviceConfigureRequestDispatching(WDFDEVICE,WDFQUEUE,ULONG);
PDRIVER_OBJECT WdfDriverWdmGetDriverObject(WDFDRIVER);
WDFDEVICE WdfIoQueueGetDevice(WDFQUEUE);
WDFDEVICE WdfInterruptGetDevice(WDFINTERRUPT);
WDFQUEUE WdfRequestGetIoQueue(WDFREQUEST);
VOID WdfSpinLockAcquire(WDFSPINLOCK);
VOID WdfSpinLockRelease(WDFSPINLOCK);
VOID WdfInterruptGetInfo(WDFINTERRUPT,PVOID);
BOOLEAN WdfInterruptQueueDpcForIsr(WDFINTERRUPT);
NTSTATUS WdfRequestRetrieveOutputBuffer(WDFREQUEST,size_t,PVOID,size_t *);
NTSTATUS WdfRequestMarkCancelableEx(WDFREQUEST,EVT_WDF_REQUEST_CANCEL *);
NTSTATUS WdfRequestUnmarkCancelable(WDFREQUEST);
VOID WdfRequestComplete(WDFREQUEST,NTSTATUS);
VOID WdfRequestCompleteWithInformation(WDFREQUEST,NTSTATUS,ULONG_PTR);
VOID WdfRequestStopAcknowledge(WDFREQUEST,BOOLEAN);
PVOID ExAllocatePoolUninitialized(int32_t,size_t,ULONG);
VOID ExFreePoolWithTag(PVOID,ULONG);
VOID RtlCopyMemory(PVOID,PVOID,size_t);

typedef struct VirtIODevice { drift_win_rng *owner; } VirtIODevice;
typedef struct virtio_wdf_driver {
    VirtIODevice VIODevice;
    drift_win_rng *owner;
} VIRTIO_WDF_DRIVER, *PVIRTIO_WDF_DRIVER;
struct virtqueue { drift_win_rng *owner; ULONG index; };
typedef struct virtio_wdf_queue_param { WDFINTERRUPT Interrupt; } VIRTIO_WDF_QUEUE_PARAM, *PVIRTIO_WDF_QUEUE_PARAM;
struct VirtIOBufferDescriptor { PHYSICAL_ADDRESS physAddr; ULONG length; };
NTSTATUS VirtIOWdfInitialize(PVIRTIO_WDF_DRIVER,WDFDEVICE,WDFCMRESLIST,WDFINTERRUPT,ULONG);
NTSTATUS VirtIOWdfInitQueues(PVIRTIO_WDF_DRIVER,ULONG,struct virtqueue **,PVOID);
VOID VirtIOWdfSetDriverOK(PVIRTIO_WDF_DRIVER);
VOID VirtIOWdfSetDriverFailed(PVIRTIO_WDF_DRIVER);
NTSTATUS VirtIOWdfDestroyQueues(PVIRTIO_WDF_DRIVER);
NTSTATUS VirtIOWdfShutdown(PVIRTIO_WDF_DRIVER);
UCHAR VirtIOWdfGetISRStatus(PVIRTIO_WDF_DRIVER);
PVOID VirtIOWdfDeviceAllocDmaMemory(VirtIODevice *,size_t,ULONG);
PHYSICAL_ADDRESS VirtIOWdfDeviceGetPhysicalAddress(VirtIODevice *,PVOID);
VOID VirtIOWdfDeviceFreeDmaMemory(VirtIODevice *,PVOID);
int virtqueue_add_buf(struct virtqueue *,struct VirtIOBufferDescriptor *,unsigned,unsigned,PVOID,PVOID,ULONGLONG);
VOID virtqueue_kick(struct virtqueue *);
PVOID virtqueue_get_buf(struct virtqueue *,unsigned *);
bool virtqueue_enable_cb(struct virtqueue *);
VOID virtqueue_disable_cb(struct virtqueue *);

/* The WPP-off build consumes argument types without evaluating them. It does
 * not emit ETW events or claim a Windows tracing provider. */
enum { DBG_ALL=1,DBG_INIT=2,DBG_POWER=4,DBG_INTERRUPT=8,DBG_DPC=16,DBG_READ=32 };
enum { TRACE_LEVEL_ERROR=2,TRACE_LEVEL_WARNING=3,TRACE_LEVEL_INFORMATION=4,TRACE_LEVEL_VERBOSE=5 };
static inline void drift_rng_trace_sink(unsigned level,unsigned flags,const char *format,...)
{ (void)level;(void)flags;(void)format; }
#define TraceEvents(...) do { if(0) drift_rng_trace_sink(__VA_ARGS__); } while(0)
#define WPP_INIT_TRACING(a,b) do { if(0) { (void)(a);(void)(b); } } while(0)
#define WPP_CLEANUP(a) do { if(0) { (void)(a); } } while(0)
#endif
