# Use Drift

The shipped tools do useful work without changes to a driver or host. Linux
provides live SPI and USB HID endpoints; every platform can run protocol replay.
The release checks run on Linux. A C11 compiler, Make, binutils and standard
Unix tools are sufficient; Nelua is needed only when changing the runtime or
assembling a driver.

## Start without hardware

```sh
make
make demo
```

The flash demo runs `modules/flash.drv` against an explicit synthetic 64 KiB
SPI endpoint, creates `build/demo.XXXXXX/flash.bin`, and reports its exact path.
The byte at address `a` is `(a ^ (a >> 8)) & 255`. Every block is read twice.
The keyboard demo runs the fixture through the actual module and prints usages:

```text
press usage=0x04 modifiers=0x00
release usage=0x04 modifiers=0x00
press usage=0xe1 modifiers=0x02
press usage=0x05 modifiers=0x02
release usage=0xe1 modifiers=0x00
release usage=0x05 modifiers=0x00
```

Run `make check` for the complete checks, including corrupted flash replies,
failed outputs, malformed keyboard descriptors and recorded-trace roundtrips.

## Flash backups

Use an accessible, already configured Linux spidev endpoint connected to a
compatible idle chip. Replace the endpoint below with your actual endpoint:

```sh
./build/drift flash /dev/spidev0.0 backup.bin
sha256sum backup.bin
```

An optional final argument selects SPI speed in Hz; the default is 1 MHz:

```sh
./build/drift flash /dev/spidev0.0 backup.bin 500000
```

The adapter accepts SPI mode 0 or 3, eight-bit MSB-first words and active-low
chip select. It inspects configuration and preserves chip select across each
command/response. It does not change mode, driver bindings or permissions.
The chip and endpoint must be exclusively owned and remain unchanged during
acquisition. Choose correct wiring and voltage for your part.

The portable driver uses `9f` for JEDEC identification and `03` plus a three-byte
address for reading. It accepts conventional capacity codes 16–24: 64 KiB
through 16 MiB. These are the supported **protocol requirements**, not a tested
part-number database. Quad/QPI-only devices, four-byte address mode, larger
parts, bank switching and special address schemes are unsupported. The tool
does not change chip mode or issue write-enable, erase or program commands.

Each 256-byte block is read twice into separate guest buffers. The module
compares every byte before emitting a chunk. Matching reads establish repeatable
acquisition; they do not authenticate firmware or create a snapshot of a chip
that another writer is changing.

The host enforces sequential chunks and the identified length. It writes a
private sibling `*.part-XXXXXX` file, flushes it, then publishes the complete
image without replacing any existing path. Protocol/transport failures and
handled SIGINT/SIGTERM remove the partial file. A process kill or machine crash
can leave a part file; the destination filename still indicates a completed
acquisition. Publication requires filesystem hard-link support.

An unsupported size, short reply or read mismatch makes the command fail. The
diagnostic includes the driver's result:

| Result | Meaning |
| --- | --- |
| `0` | Complete acquisition |
| `1` | Short identification or read reply |
| `2` | Invalid manufacturer byte (`00` or `ff`) |
| `3` | Unsupported capacity code |
| `4` | The two reads differed |

Negative transport and sink statuses are retained as the driver's 32-bit result
word. The CLI returns a nonzero process status for any failure. No physical chip
has been qualified in this environment.

## Keyboard inspection and capture

```sh
./build/drift-keyboard --list
./build/drift-keyboard --device /dev/hidraw0
```

Discovery prints `ready`, `unsupported` or `unavailable`, with the device name
or reason. Select a `ready` device. Access permissions are supplied by your OS;
the command does not change them.

The host requires USB HID and proves the descriptor describes an unnumbered,
eight-byte boot-format input report: eight modifier bits, one reserved byte and
six keyboard usages. Standard LED output fields are accepted. Report IDs, NKRO,
composite layouts and unfamiliar equivalent descriptors are rejected. It does
not switch USB protocol modes; a descriptor proving the current report format
is required. The keyboard module performs the state transitions.

To keep a reproducer, choose a new recording path:

```sh
./build/drift-keyboard --device /dev/hidraw0 --record session.trace
./build/drift-keyboard --replay session.trace
```

Ctrl-C ends an idle or active capture. Recordings contain raw key reports,
are created with mode `0600`, and never replace existing files. The host flushes
each row before handing it to the module. It appends an explicit `stop` on clean
termination. Transport failures are recorded as failed transfers and replay as
failures. Incomplete recordings end in an error instead of silently succeeding.

Events contain HID usage IDs, press/release state and the current modifier byte;
they are not layout-translated characters. Duplicate reports do not duplicate
presses. On stop or detach, the host releases previously delivered held usages
with `cleanup=stop`. Those are host cleanup events, not extra captured reports.
The tool does not inject keystrokes into the OS or take exclusive input control.

You can also use the generic host with the same recording:

```sh
./build/drift replay modules/keyboard.drv session.trace
```

An explicit `--module PATH` selects another compatible module. Live HID I/O is
Linux-specific; replay does not require hidraw or a connected device. Physical
keyboard attachment/detachment still needs hardware qualification.

## Install

For a private installation:

```sh
make install PREFIX="$HOME/.local"
"$HOME/.local/bin/drift-keyboard" --list
```

The commands find modules under the configured prefix, so the installed tools
work outside the source directory. Add that prefix's `bin` directory to your
PATH if desired. For a system installation on Alpine:

```sh
doas make install
```

`PREFIX`, `BINDIR`, `DATADIR` and packaging `DESTDIR` are supported. The library
embedding interface is documented in [porting](porting.md); the command install
target installs the tools and their three modules.
