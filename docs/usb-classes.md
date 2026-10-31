# USB serial and Ethernet classes

`native/usb_classes.nelua` implements USB-standard CDC ACM serial and CDC ECM
Ethernet over the portable USB core. The code discovers interfaces and bulk
endpoints from descriptors; it does not ask the OS to implement serial or
Ethernet device protocols. Its C interface is
[drift_usb_classes.h](../include/drift_usb_classes.h).

First open the controller and attach/configure a device using `drift_usb`.
Then bind a zero-initialized class instance:

```c
drift_usb_cdc serial = {0};
int rc = drift_usb_cdc_open(&serial, &usb, DRIFT_USB_CDC_ACM, UINT32_MAX);
if (rc == 0)
    rc = drift_usb_acm_configure(&serial, 115200, 0, 0, 8, 3);
```

`UINT32_MAX` selects the first matching control interface; pass an interface
number to select a particular function in a composite device. The Union
functional descriptor binds control to data, rather than assuming consecutive
interface numbers. A single data interface with one bulk IN and one bulk OUT
endpoint is required. An available data alternate is selected through the USB
core. The core's controller, attachment and speed limits still apply.

## CDC ACM serial

The standard ACM functional descriptor must advertise line-coding and
control-line support. `drift_usb_acm_configure` sends the exact seven-byte line
coding followed by DTR/RTS control state. Stop format is 0 for one bit, 1 for
one-and-a-half and 2 for two. Parity values are 0–4; data size is 5–8 or 16.
If the second request fails, the first may already have changed device state;
the caller can retry or close.

`drift_usb_cdc_read` and `drift_usb_cdc_write` return byte counts or a negative
error. Read returns zero while the core has no completed input. An idle read remains
armed in the controller; repeat with the same capacity until completion or
close the core. The caller buffer itself is not retained between calls. Modem-status
notifications and break signalling are not implemented. Vendor UART protocols
such as FTDI, CH34x and CP210x are separate device drivers, not CDC ACM.

## CDC ECM Ethernet

Bind `DRIFT_USB_CDC_ECM` instead of ACM. The class parses the Ethernet functional
descriptor, fetches its UTF-16 hexadecimal MAC string with an advertised language
ID, selects the data alternate, and requests directed plus broadcast reception.
The six-byte address is available as `class.mac`; `maximum_segment` contains the
maximum complete frame size advertised by the device.

Read and write exchange raw Ethernet frames. The receive buffer must be at least
`maximum_segment` bytes; short or oversized frames are rejected. A transmit that
ends on a full USB packet boundary is followed by a zero-length packet so ECM
receivers can identify the frame boundary. There is no Ethernet/IP stack inside
the class. RNDIS and CDC NCM use different protocols and are not implemented by
this interface.

## Lifetimes and errors

The class borrows the already attached `drift_usb` instance and owns no DMA
allocations. Serialize calls sharing that core. `drift_usb_cdc_close` clears the
serial control lines or Ethernet filter; it does not stop or free the USB
controller. Close the core separately, following its DMA quarantine rules.

Invalid arguments return `-22`; incompatible descriptors return `-95`; malformed
protocol data returns `-71`. Underlying USB transfer errors propagate. Descriptors,
class state and buffers are caller-owned; retain the core and its resources for
as long as a class is in use.

`tests/test_usb_classes.c` independently checks descriptor association,
alternate selection, control requests, line coding, MAC parsing, packet filters,
serial transfer semantics, Ethernet bounds, OUT zero-length termination,
truncation and malformed descriptors. These class tests stub the documented USB
transfer API; they are not controller or physical-device qualification.

References: USB CDC 1.2, PSTN subclass ACM requests and Ethernet networking
subclass descriptors/requests. Field and request definitions are also available
in the Linux UAPI [usb/cdc.h](https://github.com/torvalds/linux/blob/master/include/uapi/linux/usb/cdc.h).
