# Drift

**Device drivers that travel between operating systems. Built with Nelua.**

Give Drift register access, DMA memory and a clock. Its native drivers handle
the controller. Link the generated C into your kernel; no Nelua installation,
GC, libc or background threads are needed on the target.

[Bring up your OS](docs/native.md) · [Driver guides](#drivers) ·
[Protocol tools](docs/using.md) · [Write a module](docs/walkthrough.md) · [MIT](LICENSE)

## Start

```sh
git clone https://github.com/Spaceghost/drift.git
cd drift
make check
```

A C11 compiler, Make and standard Unix tools build the library and run the
independent register/DMA models. Generated C is included alongside its Nelua
source. Install QEMU to exercise actual emulated controllers:

```sh
make qemu-check
make kernel-check
```

The kernel example boots without libc and uses the same native driver source.
Tests operate on temporary disk images and local network peers.

## Drivers

| Family | Included |
| --- | --- |
| [Virtio](docs/virtio.md) | Modern PCI/MMIO; block, Ethernet, console, entropy, input |
| [USB/xHCI](docs/usb.md) | Enumeration, one hub/selected child, boot keyboard/mouse, storage, CDC ACM/ECM |
| [NVMe](docs/nvme.md) | Identify, namespace geometry, read/write/flush |
| [Ethernet](docs/ethernet.md) | Intel 82540EM and Realtek RTL8139; frame TX/RX |
| [AHCI/SATA](docs/ahci.md) | Port setup, ATA identification, read/write/flush |

These are deliberately bounded, polling implementations. Read each guide for
supported devices, transfer limits and recovery requirements. USB currently
selects one device per controller, directly or through one hub. QEMU tests and
model tests are evidence for those paths; physical hardware remains unqualified.

## Put them in an OS

Want to reuse Linux or Windows driver source? See the
[unchanged-source import boundary](docs/linux-imports.md), its first compatibility
profile, and the [Linux/Windows target and qualification plan](docs/compatibility.md).
This is separate from the native
Nelua drivers below; it is not a loader for Linux kernel modules.

The first opt-in profiles compile an unchanged
[Linux PC-speaker driver](imports/linux/pcspkr) and
[Windows KMDF mouse filter](imports/windows/moufiltr). Their compatibility
runtimes are Nelua. Source/profile tests establish their documented API subsets;
they do not establish arbitrary `.ko`/`.sys` loading or hardware support.

The common [resource interface](include/drift_hw.h) has callbacks for register
access, coherent DMA allocation, synchronization and time. Drivers own their
commands, queues and resets. [Portable PCI helpers](include/drift_pci.h) provide
enumeration, capability traversal and BAR decoding. The [common I/O API](docs/io.md)
gives your OS one block interface and one network interface across backends.

```sh
make                         # build/libdrift-native.a and build/libdrift.a
make install PREFIX="$HOME/.local"  # libraries, headers, tools and modules
make native-sanitize         # native drivers under ASan + UBSan
```

Start with the [reference kernel](examples/kernel/main.c) and
[porting contract](docs/native.md). Serialize calls to each device and retain
quarantined DMA after a reset failure.

## Small protocol machines, too

The original bounded VM runs portable `.drv` modules for verified SPI flash
backups and boot-keyboard decoding. The supplied Linux tools are ready to use:

```sh
make demo
./build/drift flash /dev/spidev0.0 backup.bin
./build/drift-keyboard --list
```

See [using Drift](docs/using.md), the [guided example](docs/walkthrough.md) and
[verification](docs/verification.md). The VM and native drivers share explicit
ownership and small interfaces; native controllers do not execute in the VM.
