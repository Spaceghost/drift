# Your first portable driver

Identify an SPI flash chip, change the driver, and run it in a tiny host.
Everything through step 5 runs without hardware. The device replies are
synthetic; the driver and machine are the real implementations.

## 1. Run the included driver

You need Git, a C11 compiler, Make and standard Unix tools. On Alpine these
come from `doas apk add git build-base`; on Debian/Ubuntu, install `git` and
`build-essential`. The reference build uses GCC on Linux.

```sh
git clone https://github.com/Spaceghost/drift.git
cd drift
make check
./build/drift replay modules/jedec.drv fixtures/jedec.trace
```

Run all following commands from this directory. Nelua is not needed yet.

```text
event 2 239 64 24
done 0 steps=22
```

The driver requested a chip ID, checked the response and emitted an event.
`2` is this driver's event tag. `239 64 24` is `ef 40 18` in decimal:
manufacturer, memory type and capacity code. `done 0` means success; `steps`
counts executed VM instructions.

The whole fixture is one transaction:

```text
xfer 9f 3 0 ef4018
```

| Field | Meaning |
| --- | --- |
| `xfer` | Expect a transfer request |
| `9f` | The driver must send exactly this byte |
| `3` | The driver must request space for three response bytes |
| `0` | Complete the transfer successfully; negative values are transport errors |
| `ef4018` | Bytes supplied by the simulated device |

For SPI, writing the command and reading the response happen under one
chip-select interval. Replay checks the bytes and lengths; it cannot check
electrical timing. A mismatched request fails instead of inventing a reply.

## 2. Follow the protocol

Open [drivers/jedec.nelua](../drivers/jedec.nelua). This is a **build-time
Nelua program** that assembles the portable module. Its instructions execute
later inside Drift; the target needs neither Nelua nor the assembler.

First, reserve 16 bytes of guest memory and request transfer/event capabilities:

```lua
local asm = require 'drift.asm'
local b = asm.new(16, asm.XFER | asm.EMIT, asm.SPI)
b:set(0, 0x9f)
```

Memory byte 0 holds the command. The reply will go at byte 4. The module's
declared capabilities must also be granted by its host.

Next, put buffer offsets and lengths in registers, then issue the transfer:

```lua
b:lit(2, 0)                         -- r2: command offset; constant zero
b:lit(3, 1)                         -- r3: command length
b:lit(4, 4)                         -- r4: response offset
b:lit(5, 3)                         -- r5: response capacity
b:ins(asm.IO, 2, 3, 4, 5)
```

`IO` takes **register numbers**. Here its last operand is register 5, which
contains the response capacity of 3. Guest offsets are not native pointers.
The machine checks both buffers before yielding a transfer effect to the host.

After the host completes it, `r0` holds the status and `r1` the received length:

```lua
b:branch(asm.JNE, 0, 2, 'done')      -- transport error: preserve status
b:branch(asm.JNE, 1, 5, 'short')     -- success still requires three bytes
b:ins(asm.LD8, 6, 4, 0, 0)          -- r6 = memory[r4 + 0]
b:ins(asm.LD8, 7, 4, 0, 1)          -- r7 = memory[r4 + 1]
b:ins(asm.LD8, 8, 4, 0, 2)          -- r8 = memory[r4 + 2]
```

The remaining code rejects all-zero and all-`ff` replies, then emits:

```lua
b:ins(asm.EVENT, 6, 7, 8, 2)
```

Unlike `IO`'s final operand, `EVENT`'s final operand is a literal tag. This
produces tag `2` with the values in registers 6, 7 and 8. The host delivers the
event and acknowledges it. The driver checks that acknowledgment before halting.

The full source includes all branches and the final `b:write(...)`. Keep those
checks when adapting it; a successful transport can still return bad data.

## 3. Make it fail

Supply only two response bytes:

```sh
printf 'xfer 9f 3 0 ef40\n' > build/short.trace
./build/drift replay modules/jedec.drv build/short.trace
```

```text
done 1 steps=10
```

The command exits with status 1. That is expected: the **driver** rejected the
short reply, emitted no identification event and halted with result 1.

Now simulate an all-`ff` reply:

```sh
printf 'xfer 9f 3 0 ffffff\n' > build/absent.trace
./build/drift replay modules/jedec.drv build/absent.trace
```

```text
done 2 steps=19
```

This also exits with status 1; the driver's result is 2. Driver results and
process exit status are different: the CLI maps any nonzero driver result to
a failed process. An all-`ff` response is rejected, but does not prove why the
device failed to respond.

These two commands intentionally fail. Run them individually if your shell
uses `set -e`.

## 4. Change and assemble the driver

Fetch and build the pinned Nelua compiler. This needs internet access; it
installs under `.toolchain/`, without changing your system compiler:

```sh
sh tools/bootstrap.sh
```

If you already have a compatible Nelua, use its path instead of
`.toolchain/nelua/nelua` in the following commands.

First, rebuild the unchanged driver and compare it with the shipped module:

```sh
.toolchain/nelua/nelua -r -b -L lib -o build/jedec-asm drivers/jedec.nelua
./build/jedec-asm build/jedec.drv
cmp modules/jedec.drv build/jedec.drv
```

`cmp` is silent on success. The Nelua program built an assembler executable;
that executable wrote the 233-byte `.drv` file. Only the `.drv` travels to the
target machine.

Make a working copy that changes the event tag from `2` to `42`:

```sh
sed 's/asm.EVENT, 6, 7, 8, 2/asm.EVENT, 6, 7, 8, 42/' \
    drivers/jedec.nelua > build/my-jedec.nelua
.toolchain/nelua/nelua -r -b -L lib -o build/my-jedec-asm build/my-jedec.nelua
./build/my-jedec-asm build/my-jedec.drv
./build/drift replay build/my-jedec.drv fixtures/jedec.trace
```

```text
event 42 239 64 24
done 0 steps=22
```

You changed the driver's behavior without rebuilding the machine or host.
The shipped source and module are untouched. For changes to the runtime or
shipped drivers, use `make regen NELUA="$PWD/.toolchain/nelua/nelua"`, then
`make check`.

## 5. Supply a host

[examples/identify.c](../examples/identify.c) is a complete, small host with a
simulated SPI endpoint. Build and run it:

```sh
make example
```

After Make's build commands, the program prints:

```text
transfer 9f -> ef 40 18
event 2: ef 40 18
```

The host uses three calls:

| Call | Responsibility |
| --- | --- |
| `drift_load` | Validate and copy the module; grant effects; assign a fresh instance ID |
| `drift_run` | Execute a bounded number of instructions until an effect, quota, halt or trap |
| `drift_complete` | Complete the outstanding instance/ticket, returning a status and response bytes |

The example runs eight instructions per turn and enforces a total budget. On
a transfer, it checks that the request is the permitted `9f` identification
transaction and supplies three simulated bytes. On an event, it prints the
values and checks delivery before acknowledging. `drift_complete` copies any
response into guest memory before the driver resumes.

Run your modified module in this second host too:

```sh
./build/identify build/my-jedec.drv
```

```text
transfer 9f -> ef 40 18
event 42: ef 40 18
```

For an OS port, replace the simulated reply with your SPI controller operation
and replace printing with your event sink. Preserve one chip-select interval
across command and response. Check the actual transfer result and return the
actual byte count; use a negative completion status on transport failure.

The demo uses libc for loading a file and printing. The VM core does not.
A kernel can supply a module from its boot image and caller-owned storage.
For scheduler integration, start with [examples/embed.c](../examples/embed.c)
and [the porting guide](porting.md), which covers asynchronous completion,
instance lifetimes and detach handling.

Linux already has a narrow SPI identification adapter; its setup and command
are in [the hardware section](porting.md#linux-hardware-adapters). Hardware
qualification remains separate from this walkthrough.
