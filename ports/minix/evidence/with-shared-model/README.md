# Verified MINIX guest run

The public `ports/minix/run.py` completed with exit 0. The raw serial stream and
parsed `result.json` retain all component checks and timing rows.

- Actual MINIX 3.3.0 i386 guest, booted with QEMU 8.2.2 TCG and `nobeep=1`.
- Linux source model passes its normal/failure/lifecycle/reentry cases; its
  pthread contention case is explicitly omitted.
- Separate Windows-source executable passes **584 checks** in MINIX userspace.
  This is a WDF model, not a Windows kernel API implementation.
- The same shared 500,000-event component fixture used on the other OSes emits
  eight nonzero `CLOCK_MONOTONIC` measurements. Every row has 3,500,000 modeled
  operations, checksum `64f59f6df89549d1`, final control `a8`, and 1,093,750
  modeled delay microseconds. `shared-model-parsed.tsv` is parsed from serial
  whitespace; `serial.log` retains the original bytes.
- The actual MINIX RS service passes checked hardware I/O, PIT2 mode/gate
  readback, unrelated-port `0x80` denial and removal. Eight reversed trials each
  execute 10,000 events: 10,000 reads, 40,000 writes and 40,000 real delays;
  all normalized transcript hashes are `2932125812`. Gate bits are reset and
  verified before every variant. The prior unequal-precondition comparison is
  retained under `../initial-state-mismatch/` and is rejected.

The shared model medians are 550,000 μs imported / 350,000 μs reference per
500,000 mixed events. The distinct actual-PIO loop medians are 850,000 μs
imported / 858,333.5 μs reference per 10,000 tone events. These are coarse 60-Hz
TCG component diagnostics from this run, not service request latency, a
cross-OS ranking or physical-speaker qualification. Small PIO differences are
within the timing granularity/noise; no speed win is claimed.

Source/artifact hashes, exact compiler flags, independently licensed payload
hashes, image changes and build logs accompany the raw output. The base ISO,
SDK, compiled programs and test ISO are generated artifacts, not committed.
