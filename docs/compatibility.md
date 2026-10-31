# Driver compatibility and qualification

Drift's source-import goal is to keep selected Linux and Windows driver files
unchanged while implementing their required APIs in explicit compatibility
profiles. Rebuild for each OS/architecture. Kernel-module binary compatibility
(`.ko` or `.sys`) is a separate project, not implied by these source profiles.

The first Linux profile imports [PC speaker](linux-imports.md). The first
Windows profile imports Microsoft's [KMDF mouse filter](../imports/windows/moufiltr).
These are small compatibility proofs, not the high-value device lineup. Windows requires its
own API family: WDM, KMDF, NDIS, StorPort and UMDF are different contracts.
Providing a few familiar function names does not implement them.

## Useful targets

This order uses source availability, hardware utility and repeatable tests.
It is not a market-share ranking.

| Stage | Linux source | Windows source | Acceptance target |
| --- | --- | --- | --- |
| Queue foundation | `virtio-rng.c` | virtio-win `viorng` | Same QEMU entropy device; real queues, completion, DMA and teardown |
| Network | `virtio_net` | virtio-win `NetKVM` | Same QEMU NIC; packet integrity, throughput, latency, drops and CPU |
| Storage | `virtio_blk` | virtio-win `viostor` | Same temporary disk and controller; integrity before throughput/latency |
| Physical-controller model | AHCI/libata | Microsoft's StorAHCI | Same QEMU AHCI controller, with Drift's native AHCI as another baseline |
| Physical 2.5GbE | Intel `igc`, Realtek `r8169` | Qualify available matching source first | Named I225/I226 or RTL8125 hardware; no substitute revision |

[virtio-win](https://github.com/virtio-win/kvm-guest-drivers-windows)
provides actual open Windows drivers; preserve each directory's license and
every source header. Microsoft's [storage examples](https://learn.microsoft.com/en-us/windows-hardware/drivers/samples/storage-driver-samples)
include StorAHCI. Its [Realtek NetAdapterCx sample](https://github.com/microsoft/NetAdapter-Cx-Driver-Samples)
targets **RTL8168D revision 03**, not RTL8125. Intel's
[I225/I226 support matrix](https://www.intel.com/content/www/us/en/support/articles/000055236/ethernet-products/gigabit-ethernet-controllers-up-to-2-5gbe.html)
does not establish that its Windows vendor-driver source is available.

Wi-Fi and GPU imports need substantial surrounding subsystems and often
firmware. They must earn separate profiles and device qualification; source
availability or a successful compile alone does not establish support.

## Four separate OS targets

| Target | Required evidence |
| --- | --- |
| Linux | Actual executable/service; resource acquisition, device I/O and teardown; kernel module loading only if that frontend is used |
| Windows | Native executable/service or signed/test-signed guest driver, as appropriate; real Windows requests, resources, PnP/power/cancellation semantics |
| MINIX | Booted image, launched service, checked IPC/grants/kernel calls and device ownership |
| Genode | Booted selected base platform, component/session routes and real platform resources |

Host-model CI on Windows is a native Windows executable built with MinGW; it
is not a WDK build, loaded `.sys`, or hardware test. The same distinction
applies to host models on Linux and guest models on MINIX/Genode.

Record source and toolchain pins, upstream hashes, compiler flags, artifact
hashes, OS/image/kernel, emulator/device versions and actual commands. A
blocked or failed target remains in the results. No absent OS is represented
by a mock bearing its name.

## Verification gates

1. **Source identity and API semantics:** byte-identical upstream files;
   registration, lifecycle, error propagation and every implemented API;
   unknown APIs fail explicitly.
2. **Ownership and failure:** DMA/cache ordering, completion exactly once,
   partial I/O, cancellation, reentry/concurrency, unplug, reset and failed
   quiescence; no freeing device-owned memory after an unproven stop.
3. **OS/device execution:** boot and execute the real backend, including
   denied resources, shutdown/restart and externally checked data.
4. **Measurements:** only after correctness. Keep component, host-model,
   emulated-device and physical-device numbers distinct.

## Shootout protocol

Use the same controller, transport/features, disk/image contents, data,
transfer sizes, queue depth, emulator version/accelerator and resource budget.
Use the original driver's supported configuration and record every deviation.
Separate cold startup, steady I/O and recovery. Never benchmark a no-op
completion or silently replace a failed implementation.

Repeat comparable runs in reversed order with an explicit warm-up and fixed
measurement window. Preserve individual results as well as medians. Capture
throughput, latency percentiles, CPU, memory, errors and data-integrity checks;
network tests also retain loss/duplication, storage tests retain durable
write/read verification, and RNG tests establish I/O behavior rather than
claiming cryptographic entropy from a statistical smoke test.

Four OSes on different hosts, or KVM versus TCG, are different environments.
Such runs establish usability and provide diagnostics; they do not by
themselves establish a fair performance ranking. Timing a register model is
a component benchmark, not a hardware-driver shootout.

## Reproduce the current evidence

```sh
make linux-check windows-check
make linux-sanitize windows-sanitize
make linux-bench
python3 ports/linux/run.py
python3 ports/minix/run.py
```

The [Genode kit](../ports/genode) uses an independently pinned source SDK and
cross-toolchain. Its [component model](../ports/genode/component-model) is a
separate guest program with no I/O-port authority. MINIX also runs that shared
500,000-event fixture. CI publishes the native Linux and Windows results.
The common fixture verifies identical transcripts before reporting all eight
reversed-order samples; its 794-check API test rejects failed, frozen and
backward clocks and propagates output failures.

Real-I/O paired trials are retained separately in [Linux](../ports/linux/evidence),
[MINIX](../ports/minix/evidence/with-shared-model) and
[Genode](../ports/genode/records). Their references use the same callbacks and
register sequence; none is a full native-kernel driver stack. Different trial
sizes and guest configurations preclude an OS ranking. Short timer/emulator
measurements establish no speed advantage.

The source profiles above are implemented. The network/storage/DMA targets in
the selection table are **not yet imported or qualified**. Windows kernel
loading, general WDF/NDIS/StorPort compatibility and physical device
qualification remain unverified. Do not infer those from a green host-model job.
