# Source-import component model: Linux and Windows hosts

[Run 41227386461](https://github.com/Spaceghost/drift/actions/runs/41227386461)
completed successfully at source commit
`0452d561e5356d390b9f6bd74a73262b410c5be7`. Both the Ubuntu machine job and
the Windows native-host job are green. This record compares the unchanged
Linux PC speaker driver through Drift with an independent direct reference
for the same transaction contract. The reference is **not a native Linux
kernel-driver measurement**.

## Within-host results

Each median has four samples, with 500,000 events per sample. The times include
the software port model, its atomic lock, ordered digest and status checks.

| Host | Variant | Median wall microseconds | Nanoseconds / modeled event | Ratio to that host's reference |
| --- | --- | ---: | ---: | ---: |
| Linux | Direct reference | 12281.0 | 24.562 | 1.000 |
| Linux | Imported Linux driver | 26298.0 | 52.596 | 2.141 |
| Windows | Direct reference | 16745.0 | 33.490 | 1.000 |
| Windows | Imported Linux driver | 44607.5 | 89.215 | 2.664 |

The additional modeled event cost is 28.034 ns on the Linux runner and
55.725 ns on the Windows runner. These are separate within-host comparisons,
not an operating-system speed ranking. The runners, compilers and clocks
differ. Windows imported samples span 36.493–54.691 ms; Linux imported samples
span 25.709–27.002 ms. Four repeats do not establish a stable performance ceiling.

## What was verified

Both hosts ran four rounds in AB, BA, AB, BA order (A = direct reference,
B = imported driver). All eight raw rows per host have exactly:

- 500,000 events and 3,500,000 modeled callback operations;
- checksum `64f59f6df89549d1` and final control byte `a8`;
- 1,093,750 modeled delay microseconds.

The fixed 16-request corpus covers valid tones, bell conversion, off/boundary
values and rejected events. Exact preflight transcripts and every timed-pair
digest/count must match before any rows are emitted. Registration and teardown
are outside timing. Required one-microsecond delays update a model counter;
they **do not sleep**. No real port I/O, audio, interrupt, DMA or device
throughput was timed. These are software compatibility-path measurements.

The fixture, generated Nelua compatibility code and unchanged Linux source
were compiled together with the same `-Os` settings. The recorded benchmark
commands use no LTO or sanitizers. Separate benchmark API unit runs passed
794 checks on both hosts; the Linux unit run also passed ASan/UBSan with
synthetic clocks. Those synthetic-clock runs supply no performance data.
Selected original job-log lines are labeled as build-evidence excerpts.

Linux used GCC 13.3.0 and `CLOCK_MONOTONIC` wall microseconds. Windows used
MSYS2 UCRT64 GCC 16.2.0 with `QueryPerformanceCounter`/frequency conversion;
the compiler target is `x86_64-w64-mingw32` and the recorded binary format is
`pei-x86-64`. MSYS2 supplies the shell; this is not a Windows kernel/WDK device run. No Genode or
MINIX timing is included in this host-only snapshot.

## Preserved evidence

Each host directory contains the two original artifact members, byte for byte:
`linux-import-benchmark.tsv` and `compatibility-environment.txt`.
`ARTIFACTS.tsv` records the original run, artifact IDs, source head and ZIP
hashes. Both downloaded ZIP hashes match GitHub's artifact digests. ZIP files
are not committed. `SHA256SUMS` covers the archived text, including this report;
build-evidence excerpts were retrieved separately from their corresponding job
logs and are not presented as original ZIP members.

Reproduce the model with `make linux-bench` (or `make linux-bench CC=gcc` in
MSYS2 UCRT64). The portable fixture is
`tests/bench_linux_import.c` at the pinned source commit above. Its imported
driver remains Linux v6.18 commit
`7d0a66e4bb9081d75c82ec4957c50034cb0ea449`, SHA256
`a73cf08b1543da44b905957ee35b92ddf5d55a4c283da98d24556636e121d492`.
