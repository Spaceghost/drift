# Shipped protocol modules

These event contracts describe device classes above VM ABI 1. They add no
instructions or effects to the machine. A host still binds one endpoint and
grants transfer/event capabilities.

| Module | Bus | Events |
| --- | --- | --- |
| `jedec.drv` | SPI | Tag 2: manufacturer, memory type, capacity code |
| `flash.drv` | SPI | Tag 2: identification; tag 3: verified block |
| `keyboard.drv` | Record stream | Tag 1: HID usage, pressed flag, modifier byte |

## Verified flash blocks

After a valid tag-2 identification, capacity is `1 << z` for codes 16–24.
The module issues two identical four-byte `03` read commands per 256-byte block,
then compares the buffers. Tag 3 contains:

| Field | Meaning |
| --- | --- |
| `x` | Byte offset in the flash image, starting at zero |
| `y` | Byte count, currently always 256 |
| `z` | Offset of the verified data in guest RAM, currently 16 |

The VM remains suspended during this event. A host may consume exactly those
bytes through its own `vm.memory`, after checking the range, expected sequence
and identified size. It must finish consuming them before acknowledging the
event; the module can reuse the buffer after completion. No native pointer is
stored in the module or passed as an event value.

This defines a portable guest offset, not a serialization of the native VM
structure. A host acknowledges the event with no response bytes. If file/event
delivery fails, return a negative completion status or stop the instance.

The module halts after all bytes are delivered, propagates transport and sink
errors, rejects short transfers and stops at the first mismatched read. See
[using Drift](using.md#flash-backups) for compatibility and result codes.
