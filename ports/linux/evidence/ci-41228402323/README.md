# Fresh-runner Linux guest qualification

[Run 41228402323](https://github.com/Spaceghost/drift/actions/runs/41228402323),
job `124617281456`, tested exact source
`d9cbb4411c3378b96acc33fbf27cabb625419149` on a fresh Ubuntu 24.04.5 runner.
The Linux job completed successfully, including artifact upload.

The runner downloaded and checksum-verified its pinned kernel and private
QEMU dependencies, built the static payloads, and booted **Linux
6.12.107+deb13-amd64 x86_64**. Both unchanged-source models passed, including
Linux's real pthread contention case and all **584** Windows-profile checks.
Actual guest `ioperm` denial, bounded PIT/speaker I/O, readback, lifecycle,
recovery and clean guest poweroff also passed; QEMU exited **0**.

All eight reversed-order real-I/O trials have identical transcript digests and
counts: 200 events, 500 writes, 200 profile reads and 500 real one-microsecond
delay calls. Guest monotonic median elapsed time was **98,295 µs import** and
**100,434 µs reference** per trial. These are short QEMU-TCG diagnostics, not a
physical-device score or evidence of a speed win. They remain separate from
the earlier local canonical timings; no samples are pooled.

The original uploaded ZIP's GitHub SHA-256 was independently matched before
extraction. All eight uploaded UTF-8 members are retained byte for byte here;
source hashes, strict markers, paired ordering/counts and guest exit were
independently rechecked. `ARTIFACT.tsv` records identity and `SHA256SUMS`
covers the text archive. No image, kernel, SDK or executable is included.
