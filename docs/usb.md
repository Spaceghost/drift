# Portable USB on xHCI

`native/usb.nelua` implements the controller, USB enumeration and the HID/Bulk-
Only Transport device protocols. The OS supplies register windows, DMA buffers,
cache synchronization and a monotonic clock through `drift_hw`; it does not
construct TRBs, USB setup packets, endpoint contexts or SCSI commands.

## Start a device

```c
#include "drift_usb.h"

drift_usb usb = {0};
/* pci_resource maps the xHCI BAR. Its bus-mastering and firmware ownership
 * must already be established by the platform's PCI/boot code. */
int status = drift_usb_open(&usb, &pci_resource, 0);
if (!status) status = drift_usb_attach(&usb, 0); /* first connected root port */
if (!status && usb.kind == DRIFT_USB_STORAGE) {
    uint64_t sectors;
    uint32_t sector_size;
    status = drift_usb_storage_info(&usb, &sectors, &sector_size);
    /* Provide a buffer large enough for the reported sector size. */
    uint8_t sector[4096];
    if (!status) status = drift_usb_storage_read(&usb, 0, 1, sector);
}
/* Always check close: memory remains quarantined if the controller won't halt. */
int closed = drift_usb_close(&usb);
```

Read/write requests support 512- and 4096-byte logical sectors, a maximum of
16 KiB per request and the SCSI READ/WRITE(10) address range. Flush uses
SYNCHRONIZE CACHE(10). The BOT layer verifies CBW/CSW signatures, tags, residue
and status, and performs reset/clear-halt recovery on transport faults. A CSW
command failure remains visible; capacity discovery retries after REQUEST SENSE.

The controller uses one command ring, one event ring and polled endpoint rings.
Its normal arena is 256 KiB, aligned to 64 KiB. Scratchpads add a pointer table
and the controller-requested 4 KiB pages. CPU virtual and DMA bus addresses may
differ. Each endpoint has a separate DMA buffer; transfer data never crosses a
64 KiB physical boundary. No allocator or libc is linked into the driver.

## Input

After attachment, `DRIFT_USB_HID_KEYBOARD` means an eight-byte boot keyboard;
`DRIFT_USB_HID_MOUSE` means a three-byte boot mouse. The driver issues HID
SET_PROTOCOL to select boot reports. `drift_usb_hid` returns report bytes;
keyboard state decoding can use Drift's existing portable keyboard module.
The OS chooses its keyboard layout and input delivery policy.

An idle HID/CDC receive returns `-11` while keeping its submitted transfer alive.
Poll again with the same endpoint and requested length. A different endpoint,
including a bulk OUT or EP0 control transfer, can run in the meantime. Completion
routing retains earlier receive completions until their caller consumes them.
A held request keeps DMA ownership until completion or controller halt.

## Interfaces and classes

`drift_usb_attach_config` selects a configuration descriptor index before the
first SET_CONFIGURATION. Default attachment uses index zero; a device exposing
both RNDIS and CDC-ECM can select its standard ECM configuration explicitly.
`drift_usb_control` implements bounded USB control requests.
`drift_usb_transfer` handles configured bulk/interrupt endpoints, short packets
and zero-length OUT packets. `drift_usb_set_interface` validates the requested
alternate in the stored configuration, constructs controller endpoint contexts,
and issues SET_INTERFACE. It refuses interface changes while a transfer is
pending. Configuration descriptors are exposed as immutable caller-inspectable
bytes in `config[0..total_config)`.

The [CDC layer](usb-classes.md) supplies ACM serial and ECM Ethernet. The
[hub layer](usb_hub.md) powers, debounces, resets and selects one child through
one USB 1.x/2.0 hub. Child selection keeps the hub's slot and EP0 alive, updates
routing/transaction-translator context and enumerates the child in another slot.
Selection is one-way until controller close.

## Limits and validation

The current implementation supports xHCI register versions 0.96–1.20, 32/64-byte
contexts, 4 KiB controller pages, USB low/full/high-speed devices, up to eight
bulk/interrupt endpoints, and configurations up to 1024 bytes. A controller
instance selects one direct device or one child of one hub. It does not claim
USB 3 SuperSpeed, isochronous/audio, arbitrary hub trees, multi-TT hubs or live
hub interrupt monitoring. Firmware ownership must be handed to the OS first.

Register/event/controller model tests cover initialization, complete descriptor
and endpoint enumeration, 600 HID reports across ring wraps, BOT disk
read/write/flush, pending receives with intervening control completion, malformed
trailing descriptors, DMA width checks, scratchpads, timeout and failed-halt
quarantine. The reference kernel and qtest paths additionally run the same
compiled Nelua core against QEMU's xHCI and USB device implementations. These
are emulated-controller tests; physical-controller qualification is separate.

A timeout or malformed controller completion stops the controller before reuse.
If it cannot halt, `state == 3` retains every DMA allocation and close fails.
The host must keep those allocations and mappings alive until a platform bus
reset establishes that the device cannot DMA. Serialized access is required;
there are no hidden threads or interrupt handlers.

Primary specifications: [Intel xHCI 1.22 requirements](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf),
[USB Mass Storage Bulk-Only Transport](https://www.usb.org/sites/default/files/usbmassbulk_10.pdf).
