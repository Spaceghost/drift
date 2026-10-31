# Latest published-source host validation

[Check run 41228402217](https://github.com/Spaceghost/drift/actions/runs/41228402217)
completed successfully at exact source commit
`d9cbb4411c3378b96acc33fbf27cabb625419149`. Both Ubuntu and Windows host jobs,
including every benchmark/upload step, are green. These measurements are a
separate validation repeat; the earlier `ci-41227386461` record is unchanged.
They are not pooled with that record or with guest-OS measurements.

## Within-host component measurements

Each median has four samples of 500,000 modeled events. The imported path
executes the unchanged Linux PC speaker driver through Drift. The independent
direct reference implements the same transaction contract and shares the
identical port model, atomic resource lock, digest and status checks. It is
**not a measurement of a native Linux kernel driver**.

| Host | Variant | Median wall microseconds | Nanoseconds / modeled event | Ratio to that host's reference |
| --- | --- | ---: | ---: | ---: |
| Linux | Direct reference | 15690.5 | 31.381 | 1.000 |
| Linux | Imported Linux driver | 35353.5 | 70.707 | 2.253 |
| Windows | Direct reference | 9892.0 | 19.784 | 1.000 |
| Windows | Imported Linux driver | 22864.5 | 45.729 | 2.311 |

The additional modeled event cost is 39.326 ns within the Linux runner and
25.945 ns within the Windows runner. Imported samples span 34.917–35.773 ms
on Linux and 22.787–23.467 ms on Windows. These hosted wall-clock values do
not establish a cross-OS ranking or an I/O performance result. Different runs
also vary; four samples per path do not establish a performance ceiling.

## Golden model verification

All eight rows per host use AB, BA, AB, BA order (A = direct reference,
B = imported driver). Every one of the 16 rows has exactly 500,000 events,
3,500,000 modeled callback operations, checksum `64f59f6df89549d1`, final
control byte `a8` and 1,093,750 modeled delay microseconds. Sample IDs,
rounds, positions, variants, counts and digests were independently checked.
Both source ZIP hashes match the digests returned by GitHub's artifact API.

The fixture checks exact transcripts before timing and validates each pair's
digest/counts before emitting any data. Registration and teardown are outside
the timed interval. Required I/O delays only update the model counter; they
do not sleep. This benchmark performs no real port I/O, IRQ, DMA or audio
operation and reports no device rate or hardware latency.

Each timed build compiles the fixture, generated Nelua compatibility code and
unchanged upstream driver together at `-Os`, without LTO or sanitizers.
Benchmark API unit tests pass **794 checks** on both hosts; Ubuntu also passes
the synthetic-clock ASan/UBSan unit run. Synthetic-clock unit tests provide
no performance data. Original job-log excerpts retain the build commands and
unit-test results separately from the artifact members.

Linux uses GCC 13.3.0 and `CLOCK_MONOTONIC` wall microseconds. Windows uses
MSYS2 UCRT64 GCC 16.2.0, target `x86_64-w64-mingw32`, with
`QueryPerformanceCounter`/frequency conversion; the environment records
`pei-x86-64` output. MSYS2 supplies the build shell. This is hosted Windows
execution, not a Windows kernel/WDK device test.

The same Check run also passes the Linux import lifecycle/failure/reentry/
pthread checks, both strict zero-undefined-symbol audits, the Microsoft
profile's 584 normal and 572 optimized/LTO checks, native-controller
QEMU/kernel regressions and generated-code verification. Those regression
checks are not presented as device qualification for this timed model.

## Preserved source and provenance

Each host directory preserves both original artifact members byte for byte:
`linux-import-benchmark.tsv` and `compatibility-environment.txt`.
`ARTIFACTS.tsv` records run/source identity, artifact IDs, original ZIP sizes
and SHA256 hashes; ZIP binaries are not committed. `JOBS.tsv` records exact
successful job identities and clock scope. `SHA256SUMS` covers this text
archive, including the report and labeled build-log excerpts.

The shared fixture, its header/unit tests, generated compatibility code and
import wrapper are unchanged between the earlier host record's source
`0452d561e5356d390b9f6bd74a73262b410c5be7` and this run. Reproduce with
`make linux-bench` or `make linux-bench CC=gcc` in MSYS2 UCRT64. The pinned
upstream driver remains Linux v6.18 commit
`7d0a66e4bb9081d75c82ec4957c50034cb0ea449`, SHA256
`a73cf08b1543da44b905957ee35b92ddf5d55a4c283da98d24556636e121d492`.

Guest boot/real-PIO records belong to their separately labeled port evidence;
this snapshot contains only the two hosted model measurements.
