# One USB2 hub and one selected child

The native hub helper owns hub-class requests and port sequencing in Nelua.
The xHCI core owns the hub/child slots, transaction translator fields, routes,
contexts and rings. The OS continues supplying only generic controller
resources; it does not issue hub or xHCI commands.

```c
#include "drift_usb_hub.h"

/* Controller open and root attach have already completed. */
if (controller.kind == DRIFT_USB_HUB)
    result = drift_usb_attach_hub(&controller, 0);
/* The same controller now selects the first connected child. */
```

Port zero selects the first connected downstream port. A positive number
selects that particular port. Supported USB2 hubs expose 1–15 ports. Hub
selection is one-way until controller close; this is not a background device
manager or an arbitrary-depth hub tree.

The helper validates a USB2 hub descriptor, including its complete removable
bitmap. It accepts omission of unused bytes from the obsolete power-mask tail
(as seen in QEMU8.2.2), while rejecting truncated mandatory fields and declared
prefixes. It powers the ports when needed,
waits the advertised power-good interval, requires a stable connected state
for 100ms, resets the selected port, polls for reset completion/enable, clears
observed change bits and derives the child speed. A chattering connection has
a bounded debounce deadline, and a noncompleting reset has its own deadline.
Disconnects, overcurrent, malformed descriptors and short status replies fail.

The core preserves the hub's slot and default endpoint. It retires the hub's
unused interrupt endpoints before recycling their transfer rings, configures
the hub slot, then enumerates one child using its own slot/context/default
endpoint. Full-/low-speed children behind a supported high-speed hub get the
transaction translator hub-slot and port fields. Multi-TT hub protocol2,
SuperSpeed hubs and a second hub level are unsupported.

After selection, the normal keyboard/mouse, mass-storage or generic endpoint
APIs operate on the child. There is no automatic hub interrupt monitoring,
child replacement, arbitrary hotplug topology traversal or second simultaneously
selected child in this implementation. Close halts the controller before any
of its shared DMA arena is released.

`tests/test_usb_hub.c` independently models class requests, port status and
virtual time. It covers power modes, multi-byte descriptor bitmaps, explicit
and automatic selection, full/low/high speeds, connection debounce, chatter,
reset completion/timeout, malformed replies, detach and overcurrent. It links
`generated/usb_hub.c` with stubbed core transfer/child-selection APIs, so its
passing assertions establish hub-class behavior rather than xHCI operation.
The shared QEMU8.2.2 test also enumerates an eight-port hub and its selected
mass-storage child, then exercises that child through the same controller
engine. This provides separate emulated-controller/route evidence. Physical
hubs and transaction translators still require hardware qualification.
