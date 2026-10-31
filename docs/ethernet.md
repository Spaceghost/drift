# Native Ethernet

Drift includes polling drivers for **Intel 82540EM (e1000)** and **Realtek
RTL8139 classic mode**. Both implementations are Nelua; the generated C exports
one small frame API. RTL8139 is the selected Realtek family for this release;
RTL8169/RTL8125 and other Intel generations are not covered by these bindings.

## Bind and run

The OS discovers the PCI function, enables the device's bus mastering and
address decoding, and supplies `drift_hw`. Bind an e1000 BAR0 MMIO register
window or an RTL8139 BAR0 port-I/O / BAR1 MMIO window to one region number.
The driver owns all register sequences and DMA descriptor construction.

```c
#include "drift_ethernet.h"

static drift_ethernet nic; /* zero before the first initialization */

int attach(drift_hw *hardware, uint32_t region)
{
    return drift_eth_init(&nic, hardware, region, DRIFT_ETH_E1000, 1000000);
}
```

`nic.mac` contains the hardware address. `drift_eth_tx(&nic, frame, length)`
sends one Ethernet frame synchronously. `drift_eth_rx(&nic, frame, capacity,
&length)` polls for one received frame. Frames exclude the Ethernet FCS;
transmit frames shorter than 60 bytes are zero-padded. The maximum frame is
1518 bytes, including any VLAN header and excluding the FCS. No checksum,
segmentation, VLAN stripping, jumbo-frame or promiscuous mode is enabled.
Multicast frames are accepted on RTL8139; e1000 accepts its own unicast address
and broadcasts in this initial configuration.

| Result | Meaning |
| --- | --- |
| `0` | Operation completed; RX returned `length` bytes |
| `1` | RX has no available frame |
| `-1` | Invalid argument or TX length |
| `-2` | Instance is not running or is already initialized |
| `-3` | DMA allocation cannot satisfy the device |
| `-4` | A reset or transmit operation timed out |
| `-5` | Malformed RX descriptor or device error |
| `-6` | RX destination is too small; the frame remains available |
| `-7` | Unsupported device binding |

These drivers provide raw Ethernet frames. ARP, DHCP, IP, TCP and socket APIs
belong to the OS's network stack. A successful TX completion indicates device
completion, not remote delivery. The drivers poll and mask device interrupts;
they do not install an interrupt handler or manage PHY configuration for other
chip families.

## Memory ownership and failure

Calls on one instance must be serialized. e1000 uses sixteen legacy receive
and transmit descriptors, 32 KiB of receive buffers and one 2 KiB transmit
buffer. RTL8139 uses an 8 KiB receive ring with a guarded overflow tail and
four 2 KiB transmit slots. RTL8139 DMA addresses must fit below 4 GiB; e1000
uses 64-bit DMA addresses. CPU and device addresses may differ.

DMA control memory must be coherent or uncached: adjacent hardware descriptor
entries can be owned by the CPU and NIC simultaneously. The resource layer's
cache synchronization and ordering rules apply to every handoff. Initial
receive buffers use bidirectional preparation before enabling reception;
completed descriptors and packet bytes are acquired before inspection.

An invalid e1000 packet descriptor is consumed and reported as an error. A
transmit failure, timeout, RTL8139 receive-ring corruption or overrun stops the
instance. Further data calls return `ESTATE`. `drift_eth_close` resets the
controller before releasing any DMA allocation. If reset itself times out,
allocations stay in the object and `close` may be retried. The OS must retain
the object, resource callbacks and quarantined buffers until reset succeeds
or an external platform action has conclusively stopped DMA. Do not discard
a timed-out object and reinitialize it over its live buffers.

## Verification

`tests/test_ethernet.c` uses independent register models and separate CPU/device
DMA mirrors with nonidentity bus addresses. It checks both families, transmit
padding and ring wrap, RX overflow-tail wrap, small-buffer retry, malformed
completions, hardware error paths, reset and TX timeouts, allocation rollback,
64-bit e1000 addresses and rejected high RTL8139 addresses. Sanitizers check
both the driver and the models. The shared QEMU 8.2.2 integration also passed
real e1000 and RTL8139 transmit/receive operations against a TCP network peer.
Neither model nor QEMU testing qualifies a physical NIC.

Protocol references used to check the register-level implementation:

- [QEMU e1000 model](https://github.com/qemu/qemu/blob/master/hw/net/e1000.c)
- [QEMU RTL8139 model](https://github.com/qemu/qemu/blob/master/hw/net/rtl8139.c)
- Intel 8254x Gigabit Ethernet Controllers Software Developer's Manual

Hardware quirks and PHYs outside the named device bindings require additional
qualification before expanding the compatibility list.
