# Drift

**A small machine for portable device drivers.**

Write the device protocol once. Each operating system supplies the transport.
The same `.drv` module handles the bytes, state and events.

[Walkthrough](docs/walkthrough.md) · [Porting](docs/porting.md) ·
[ABI](docs/abi.md) · [MIT](LICENSE)

## Run it

You need a C11 compiler, Make and standard Unix tools. Generated C and driver
modules are included; Nelua is only needed to change or build drivers.

```sh
git clone https://github.com/Spaceghost/drift.git
cd drift
make check
./build/drift replay modules/jedec.drv fixtures/jedec.trace
```

```text
event 2 239 64 24
done 0 steps=22
```

That's a flash chip's three-byte ID, decoded by a **233-byte driver**.
The replay checks the outgoing command and supplies a synthetic device reply.
No hardware required.

**[Follow the example →](docs/walkthrough.md)** Run it, inspect the protocol,
break a reply, change the driver, then embed it in a small host.

## Inside

The runtime and assembler are written in **Nelua**. The core has 23 instructions,
16 registers and three effects: **transfer, event, wait**. No heap, GC, libc or
hidden threads. The host owns the device and the memory.

| Included | Support |
| --- | --- |
| SPI flash | Read-only JEDEC identification · 233-byte module |
| Boot keyboard | Key and modifier events, rollover handling · 576-byte module |
| Hosts | Transaction replay, Linux SPI identification, packet input |
| Embedding | Three-call C interface; freestanding example |

Drift handles device protocols. Your OS still provides bus controllers and
device discovery. The keyboard expects eight-byte boot reports. Physical
hardware and a second OS port are the next milestones.

## Build on it

```sh
make example       # run the small host from the walkthrough
make sanitize      # address and undefined-behavior checks
make freestanding  # run without libc on x86-64; link i386
make size          # measure this build
```

Start with [the JEDEC driver](drivers/jedec.nelua) or
[the embedding example](examples/identify.c).
[Verification](docs/verification.md) records the tested limits and measurements;
[the roadmap](docs/roadmap.md) lists the next work.
