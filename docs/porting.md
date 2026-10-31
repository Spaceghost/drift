# Bringing Drift to an operating system

Port the transport once. Share protocol modules across ports.

For controller drivers (Virtio, xHCI, NVMe, Ethernet and AHCI), start with
[the native resource interface](native.md). This page describes the bounded
protocol VM and its SPI/record-stream transports.

For a complete runnable introduction, follow [the walkthrough](walkthrough.md).

## Build the core

Compile `generated/drift.c` with `include/` on your header path. It needs
`stdint.h`, `stddef.h` and `stdbool.h`, but no linked C library. The code uses
fixed-width guest integers and `uintptr_t` only to handle overlap of host
response buffers. No platform-sized guest value or C structure is read from
the wire. Use the same ABI header and generated source together.

For GCC, the reference freestanding flags are:

```sh
cc -std=c11 -Os -ffreestanding -fno-builtin -fno-stack-protector \
   -Iinclude -c generated/drift.c -o drift.o
nm -u drift.o
```

Check your own target's undefined symbols. A compiler may introduce helpers,
especially on small architectures. Zero undefined symbols were observed for
the x86-64 reference object, not promised for every compiler/target.

Allocate one `drift_vm` per device in host-owned storage. The current fixed
capacity reserves 1,024 decoded instructions and 4,096 bytes of guest memory
per machine even if a module uses less. Ask C's `sizeof(drift_vm)` on your
target; do not hardcode a host structure layout. Three API functions have no
global mutable state, so independent machines may run independently.

## Bind authority

The OS enumerates devices and selects a compatible module. Bind that instance
to exactly one endpoint and an operation policy. Keep endpoint identity,
address, chip select, transfer speed and OS handles in the host. Only grant
the effect types that the port supports. The C API's input byte pointers are
read-only by contract even though the Nelua export currently uses unqualified
pointer types.

The loader grants no native memory, DMA mapping, file access or path lookup.
A transfer capability is not permission to send every command to a device.
The reference Linux SPI adapter illustrates a narrow read-only command policy.

## Schedule and complete

Call `drift_run` with a finite budget. On QUOTA, reschedule. On EFFECT, copy or
retain the effect and dispatch its ticket once. On DONE or a trap, stop it.

For synchronous ports see `examples/embed.c`. For asynchronous ports, retain
the instance/ticket in the I/O completion object. Keep the VM and its TX bytes
alive until the operation is finished or safely cancelled. Post completion
back through the VM's serialized scheduler context. Completion copies RX
bytes, so the host can then release the response buffer.

An interrupt handler should enqueue a completion rather than race execution
or mutate VM state. One outstanding effect keeps ordering explicit. After
detach, stop scheduling and dispatching that instance, cancel/drain host I/O,
and release class state such as held keyboard keys. Reinitializing a VM needs
a fresh instance identity. Never free it while a completion can still reach it.

## Preserve the bus transaction

For SPI, command and response share a chip-select interval. Do not implement
IO as separate `write` and `read` syscalls that deassert chip select between
them. Linux's `SPI_IOC_MESSAGE` preserves that composite operation. For I2C,
preserve the repeated-start boundary. For packet input, preserve complete
records; an arbitrary fragmented byte stream is not the ABI's record stream.

The boot-keyboard module expects a host-negotiated or otherwise verified
eight-byte unnumbered boot report. USB enumeration, `SET_PROTOCOL`, descriptor
parsing, layout translation, LED output and OS input injection are separate
work. This module emits HID usages, not characters.

## Linux hardware adapters

With an exclusively owned, already configured spidev endpoint:

```sh
./build/drift spi modules/jedec.drv /dev/spidev0.0 1000000
```

The identification adapter checks SPI mode 0/3, holds chip select across command
and response, and permits only the read-only JEDEC identification transaction.
The complete `drift flash` command additionally permits bounded `03` reads; see
[using Drift](using.md) and [its event contract](modules.md). It
does not change driver bindings, device configuration or permissions. Choose
the actual device path and a speed supported by your board and chip.

For packet-producing input endpoints with verified, unnumbered, eight-byte
boot-keyboard reports:

```sh
./build/drift stream modules/keyboard.drv PATH
```

For descriptor validation, discovery and captures, use the complete
`drift-keyboard` tool in [using Drift](using.md#keyboard-inspection-and-capture).

An arbitrary hidraw endpoint need not have that format. This adapter does not
discover descriptors or negotiate boot mode. The OS port must release held
input state on detach. Neither hardware adapter has been qualified against a
physical device yet.

## Grow from evidence

The first additional port should run the unchanged JEDEC module through a real
SPI controller, compare a logic-analyzer trace with the contract, and publish
the exact board, chip and bus settings. Then use the unchanged keyboard module
through a second OS's USB input transport.

Supporting a new operating system is a port, not a branch of each driver.
Supporting a new bus controller still requires a controller driver. ABI 1 is
a protocol-driver foundation; it does not replace xHCI, interrupt controllers,
IOMMU setup, PCI discovery or a kernel's memory manager.
