# A real kernel using Drift

This example boots a small Multiboot x86 kernel under QEMU TCG. It links the
same native Nelua drivers used by the hosted tests, without a C library, kernel
framework or driver-specific host callbacks.

From the repository root:

```sh
sh tools/check-kernel.sh
```

You need an x86 GCC/binutils toolchain with freestanding `-m32` support,
`qemu-system-x86_64`, and standard Unix tools (`dd`, `timeout`, `grep`). No
32-bit libc is needed. Set `CC`, `LD` or `QEMU` to override executables. The
script also recognizes Drift's private `.toolchain/qemu/` installation.

The test creates disposable disk images and an isolated virtual Ethernet hub.
It never attaches a host disk or connects the hub to the host's network.
Expected serial output:

```text
Drift reference kernel
AHCI read/write/flush/restore PASS
NVMe read/write/flush/restore PASS
Virtio block read/write/flush/restore PASS
Virtio/Intel/Realtek Ethernet frame exchange PASS
xHCI/USB storage read/write/flush/restore PASS
DRIFT KERNEL PASS
```

The SATA, NVMe, Virtio and USB tests all call the **same block-class interface**.
One shared test reads 8 KiB, writes a pattern, flushes, checks the returned bytes
and restores the original data. Requests are split to each controller's limits
by the Nelua facade; the kernel checks its completed-sector count. The network
test uses the common frame interface to exchange actual Ethernet frames between the Virtio, Intel e1000 and
Realtek RTL8139 virtual devices. The USB path includes xHCI initialization,
root-port enumeration and bulk-only mass storage.

`main.c` supplies the generic OS work: PCI configuration ports, bound register
regions, an identity-mapped DMA arena, x86 cache-ordering barriers, serial
logging and a TSC clock calibrated against the PIT. PCI discovery and
capability traversal use Drift's portable PCI helper. Queue setup, command
encoding, device enumeration and recovery remain in the Nelua drivers.

The OS adapter is intentionally for this controlled x86/QEMU machine. It does
not implement a scheduler, IOMMU, physical memory allocator, paging, filesystem
or network stack. Its bump DMA arena is reused only after the previous device
has successfully closed. A real OS must replace those platform choices while
preserving the resource and lifetime contracts.

Emulated-controller execution is distinct from physical-hardware qualification.
It demonstrates a genuine kernel consumer of the driver kit, not support for
arbitrary PCs or a second independent operating-system port.
