# Actual virtual-PCI Virtio RNG qualification

This runner executes the native Nelua Virtio RNG driver, the byte-identical imported Linux `virtio-rng.c`, and the five byte-identical Windows `viorng` source translation units against a real QEMU modern Virtio RNG PCI function. The host process supplies generic resources through `hosts/virtio_qtest.[ch]`: bounded PCI/BAR access, independent CPU-shadow DMA mappings into guest RAM, ownership synchronization, and monotonic time. The drivers own feature negotiation, descriptors, queues, completion handling, and lifecycle. The resource provider never supplies entropy or implements a device operation.

Run all three variants on Linux with a C11 GCC-compatible compiler and Python 3:

```sh
python3 ports/linux/virtio-rng/run.py --work build/virtio-rng-proof \
  --variants native,linux,windows --requests 64 --rounds 4
```

The default path downloads and privately extracts the existing SHA-pinned QEMU 8.2.2 package recipe through `ports/linux/run.py`. It installs no host packages. An existing private extraction can be selected with `--qemu-root DIR`. To use an explicitly prepared QEMU executable, pass `--qemu PATH` instead.

Native Windows uses the same Python runner and controller code, with a native **i686** GCC-compatible compiler and native x64 QEMU:

```powershell
python ports/linux/virtio-rng/run.py --work build/virtio-rng-proof `
  --qemu path/to/qemu-system-x86_64.exe --cc path/to/i686-w64-mingw32-gcc.exe `
  --variants native,linux,windows --requests 64 --rounds 4
```

The runner directly invokes the compiler with argument lists; it needs no make, shell, archiver, or cross-host executable emulation. The three controllers must have the same native target. i686 avoids the Linux profile's unsupported Windows LLP64 ABI; this is a bounded source compatibility profile, not a general Linux ABI emulator. Windows prerequisites are documented separately in `ports/windows/virtio-rng/`.

Each trial starts a fresh QEMU process and a separate loopback EGD fixture. The fixture supplies a nonperiodic xorshift32 byte stream (seed `0x6d2b79f5`, shifts 13/17/5, high byte) only to QEMU's actual RNG backend. The controller obtains every tested byte through the real PCI function, descriptors, guest RAM and used ring. It validates a separate DMA shadow roundtrip before opening any device queue, eight 32-byte warmup reads, then the requested number of 32-byte measured reads, two output canaries per read, completion results, reset/close state, and balanced DMA releases. This is a data-integrity fixture, not a cryptographic entropy claim.

QEMU runs a tiny CLI/HLT firmware loop so device run-state and timers remain active while no guest code configures PCI. `--stopped` deliberately exercises QEMU's stopped state, which does not provide the normal RNG completion behavior on the pinned version. No guest OS or production host device is involved. All sockets are loopback and unprivileged; no root permissions are needed.

Rounds alternate the complete variant order and retain individual measurements. The measured read loop starts after warmup; open, warmup and close are timed separately. Each timing window must be positive. Controller clocks use CLOCK_MONOTONIC on POSIX and QueryPerformanceCounter on Windows, report their source resolution, and expose integer microsecond timestamps. Source hashes are compared before/after compilation and after all trials. Upstream source checksums are checked before compilation. The aggregate JSON records the exact compiler commands and controller/QEMU/source hashes alongside raw per-trial logs and EGD transaction offsets.

The implementations deliver equal application workloads with different fetch, prefetch, polling and synchronization costs. In particular, the current native driver synchronizes a full 4,096-byte buffer on each small read, while the source profiles use smaller transfers. Results include TCP, shadow-copy and QEMU overhead and must be read with the recorded sync/EGD counts. They do not establish physical driver speed or cross-ISA performance. CPU utilization, RSS, request latency percentiles and entropy quality are not collected.

The archived [local Linux x86-64 evidence](evidence/local-linux-x86_64/README.md) contains twelve passing canonical trials. Native Windows execution, Genode/MINIX guests, source-profile model tests and kernel driver loading are separate qualifications; this runner proves actual virtual-PCI/DMA use from ordinary native host processes.
