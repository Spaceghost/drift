# Fresh-run MINIX guest qualification in CI

The [MINIX matrix job](https://github.com/Spaceghost/drift/actions/runs/41228402323/job/124617281533)
passed at exact source commit `d9cbb4411c3378b96acc33fbf27cabb625419149`.
This record concerns that job; other jobs in the workflow have their own results.
The public command was `python3 ports/minix/run.py --work "$PWD/build/qualification-minix"`
on a fresh Ubuntu 24.04 runner. The actual guest run took **26.204 seconds**,
after image/tool downloads and compilation. No checks were disabled or changed.

The artifact independently verifies:

- MINIX 3.3.0 i386 boot and `nobeep=1`.
- Linux source model success, with the **pthread contention case explicitly
  omitted**. Reentry/lifecycle and fault cases remain exercised.
- A **separate** Windows Moufiltr executable passing **584 checks** in MINIX
  userspace. It models WDF and establishes no Windows kernel ABI compatibility.
- Eight shared component-model rows, each with 500,000 events, 3,500,000 modeled
  operations, digest `64f59f6df89549d1`, final control `a8` and 1,093,750 modeled
  delay microseconds. All `CLOCK_MONOTONIC` timings are nonzero.
- Eight actual RS-service PIO rows, each with 10,000 events, 10,000 checked
  reads, 40,000 writes, 40,000 real delays and normalized digest `2932125812`.
  PIT2 mode/gate readback, unrelated-port `0x80` denial, and service removal
  pass. All seven parsed verification categories are true.

| Distinct component | Imported median | Direct-reference median |
| --- | ---: | ---: |
| Shared port model, 500,000 mixed events | 633,333 μs | 400,000 μs |
| Actual MINIX PIO loop, 10,000 tone events | 750,000 μs | 716,667 μs |

Both use QEMU TCG, four reverse-order rounds and coarse 60-Hz guest timing.
The PIO measurements include checked register access and real delays inside one
service. They are component diagnostics, not service-request latency, physical
speaker qualification, a cross-OS performance ranking, or a speed-win claim.
Only gate bits 0–1 of port `0x61` are included in the normalized transcript;
other timer/status bits vary. Every variant begins with verified gates off.

All **20 original UTF-8 artifact members** are retained byte for byte. The ZIP
SHA-256 matched GitHub's published artifact digest:
`9c6bfaa7ea42e86ecf15cf11ad32db45384dd8644f62bb9e3f0d4c7d7e89e282`.
The serial bytes match `result.json`'s recorded digest. A separate read-only audit
reproduced raw rows, order/count/digest checks, source hashes against the tested
Git commit, the base-image pin, independent payload identities, authority-denial
and cleanup markers. `ARTIFACT.json`, `AUDIT.json` and `SHA256SUMS` identify and
cover this archive. Generated SDKs, images and binaries are not committed.
