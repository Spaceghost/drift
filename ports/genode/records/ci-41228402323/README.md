# Exact-head Genode CI qualification

[Guest workflow run 41228402323](https://github.com/Spaceghost/drift/actions/runs/41228402323),
Genode job `124617281564`, tested exact source
`d9cbb4411c3378b96acc33fbf27cabb625419149`. Every Genode step completed
successfully: host setup, SDK archive verification, source-tree checks and
real-I/O service, denied authority, separate no-I/O model, and artifact upload.

The original `guest-genode` artifact is `13052179320`: ZIP SHA-256
`455dccac4bb97c8a8d08cf8cd00f89599334bd693c1fcf122c67a3c61682d056`,
28,983 compressed bytes. All **15 UTF-8 members / 394,886 uncompressed bytes**
are retained here unchanged. The ZIP itself is not committed. [ARTIFACT.tsv](ARTIFACT.tsv)
records its identity and [AUDIT.json](AUDIT.json) records the independent checks.

## Independent verification

- Normal and denied build metadata identify Genode source commit
  `d18ee9e917200b059549e21f2dd9a55bfe61a0eb`, source-tree digest
  `d520090fc4798c75fa38f7f34cd69dafbcfe2069929e6968b59da588fec42078`,
  and the same image hashes subsequently checked by their boot runners.
- All **40 distinct recorded source hashes** match exact `d9cbb44` Git objects;
  both real-I/O builds cover `imports/linux/pcspkr/SOURCE`. The imported Linux
  file remains unchanged at SHA-256
  `a73cf08b1543da44b905957ee35b92ddf5d55a4c283da98d24556636e121d492`.
- The real-I/O guest boots Genode 26.08 and records its explicit PASS, lifecycle
  checks and eight reversed-order samples. Every trial has 100 cycles,
  400 RPCs, 500 writes, 400 reads and 500 delay calls. No Genode errors occur.
- The denial guest reports no route to `IO_PORT` and Genode's explicit startup
  fail-stop. No speaker service is announced and no benchmark samples appear.
  This verifies refusal of authority, not a successful device operation.
- The independent model ELF has no I/O-port authority. Its eight shared-fixture
  rows each match 500,000 events, 3,500,000 operations, 1,093,750 modeled delay
  microseconds, checksum `64f59f6df89549d1` and control `a8`. The three fixture
  C compile commands have effective `-Os`, GCC 14.2.0 and the embedded/no-main
  define. No Windows/MS-PL driver is linked.

Raw guest logs and the model TSV were parsed independently and matched to the
JSON samples. Order, count/digest parity, all medians, log hashes, source pins
and compiler commands were checked. These are fresh CI results, separate from
the earlier local canonical records; they are not pooled.

| Scope | Import median | Reference median | Unit |
| --- | ---: | ---: | --- |
| Real-I/O service | 281,917 | 273,866.5 | guest us per 400-RPC trial |
| Shared component model | 587,864.5 | 259,268 | guest us per 500,000 modeled events |

The real-I/O comparison uses the same RPC/resource callbacks, locks, readbacks
and actual delays. The model uses in-memory operations and does not sleep.
Different scopes, compilers, execution modes and hosts prevent turning these
values into an OS capacity or native-driver ranking. The reference implementations
are the documented minimal contracts, not complete native Genode drivers.
