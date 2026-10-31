# Shared Linux import component model on Genode

This is the **same** `tests/bench_linux_import.c` fixture used on other hosts,
compiled with `DRIFT_LINUX_IMPORT_BENCH_NO_MAIN` and a real Genode monotonic
Timer callback. It is a separate Linux/GPL-linked ELF from the real-I/O speaker
service. No Windows/MS-PL driver is linked into it.

The Genode image grants this test **no `IO_PORT` authority**. Driver accesses
and required delays are modeled in memory; the modeled delay counter does not
sleep. Booting the component is real; its device interactions are a model.
These results are not pooled with the real-PIO/RPC measurements one directory
above.

## Run

Use the SDK pinned in `../SDK` and the dependencies described in `../README.md`:

```sh
python3 ports/genode/component-model/run.py \
  --genode-dir /path/to/genode-26.08 \
  --toolchain-prefix /path/to/toolchain/bin/genode-x86-
```

Private tools can use `--expect`, `--qemu`, `--qemu-data`, `--bios` and
`--module-dir`; supply their shared-library environment when needed. The
runner checks the complete SDK source-tree pin before building and retains
its logs under `build/genode-component-model/`.

Genode GCC **14.2.0** compiled the fixture, imported driver wrapper and Nelua
compatibility C with **`-Os`**, `-fno-strict-aliasing`, `-ffreestanding`,
`-fno-builtin`, x86-64-v2, large code model and PIC. The actual three compiler
commands are retained in [records/fixture-compiler-commands.log](records/fixture-compiler-commands.log).
Genode's supporting base libraries retain their normal build settings.

## Verified run

[Eight raw samples](records/samples.tsv) cover four reversed-order rounds with
**500,000 events per sample**. Every sample matches:

- 3,500,000 modeled operations;
- 1,093,750 modeled delay microseconds;
- checksum `64f59f6df89549d1` and final control byte `a8`.

The fixture verifies exact preflight transcripts and timed-pair results before
emitting a sample. The guest adapter additionally checks the expected counts
and checksum. The [guest log](records/guest.log) ends with an explicit model
PASS; [metadata](records/result.json) records source hashes, image hash, SDK
identity and commands. No guest image or SDK is committed.

Median elapsed time per 500,000-event sample:

| Imported Linux | Direct reference |
| ---: | ---: |
| 441,941.5 us | 198,226.5 us |

The import takes about **2.23 times** the reference's elapsed time in this
TCG component-model run. These measurements include modeled callback,
validation and digest work. They are not native-kernel or hardware timings.
The direct reference is the independent fixture implementation, not a native
Genode device driver. Different compilers, architectures and execution modes
on other hosts prevent treating a four-OS table as a capacity ranking.
