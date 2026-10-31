# Drift

**Portable device drivers, with useful tools attached.**

Back up an SPI flash chip. Inspect a USB keyboard. Record reports, replay bugs,
and carry the same protocol modules into another operating system.

[Use it](docs/using.md) · [Write a driver](docs/walkthrough.md) ·
[Port it](docs/porting.md) · [ABI](docs/abi.md) · [MIT](LICENSE)

## Try it

A C11 compiler, Make and standard Unix tools are enough. Generated C and
assembled drivers are included.

```sh
git clone https://github.com/Spaceghost/drift.git
cd drift
make
make demo
```

The demo creates a **64 KiB flash image**, checking every block against a second
read, then prints keyboard press/release events. Both inputs are synthetic;
the portable drivers and file/event sinks are real.

## Back up a flash chip

On Linux, with a configured SPI endpoint:

```sh
./build/drift flash /dev/spidev0.0 backup.bin
```

The driver identifies the chip, reads each 256-byte block twice, compares every
byte, and saves the complete image. A mismatch fails the backup. Existing files
are never overwritten. Only identification and read commands are permitted.

Supports standard three-byte-address SPI NOR, **64 KiB–16 MiB**, already in
SPI mode 0/3 and three-byte address mode. [Compatibility and details](docs/using.md#flash-backups).

## Inspect a keyboard

```sh
./build/drift-keyboard --list
./build/drift-keyboard --device /dev/hidraw0 --record session.trace
./build/drift-keyboard --replay session.trace
```

Choose a device listed as `ready`. The host validates its report descriptor;
the Nelua driver handles key state, modifiers, duplicates and rollover.
Ctrl-C stops cleanly. Captures are private and replayable without hardware.
Events are HID usages; this is a diagnostic tool with no OS key injection.

## Carry the drivers

The VM and protocol modules are written in **Nelua**. Hosts supply transfers,
event delivery and waits. The core has no heap, GC, libc or hidden threads.
The verified flash driver is **569 bytes**; the keyboard driver is **576 bytes**.

```sh
make install PREFIX="$HOME/.local"  # binaries and modules
make check                        # VM, protocols, descriptors and CLI failures
make sanitize                     # ASan + UBSan, including the hosts
```

[Start with the guided example](docs/walkthrough.md), then use the
[porting guide](docs/porting.md) to connect your OS's transport. Physical hardware
qualification and a second OS port remain unverified;
[verification](docs/verification.md) records what has actually been tested.
