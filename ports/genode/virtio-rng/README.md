# Genode Virtio RNG imports

These separate Genode components exercise the unchanged Linux and Windows
Virtio RNG sources against QEMU's actual modern PCI device, through the same
generic Drift resources. The components do not register an OS entropy service.
Linux and Windows source personalities are linked into separate ELF images.
Their upstream notices and checksums remain in their import directories.

| Scenario | What the guest checks |
| --- | --- |
| `rng_probe` | Actual PCI config identity, PCIEXBAR and BAR readbacks, independently checked against QMP |
| `rng_resources` | Three deferred-ready raw opens, coherent ring allocation, reset, full reclamation and Platform release |
| `rng_resources_denied` | A valid Platform session whose policy grants no device; no consumer BAR mapping or DMA allocation |
| `rng_linux` | Exact initial 512 bytes, source freeze/reopen/restore, 64 exact postrestore bytes, cancellation and complete reset/reclamation |
| `rng_windows` | Exact 576 bytes, original source ISR/DPC after real used-ring completion, deferred notifications, queued/active cancellation and complete reset/reclamation |
| `rng_linux_shootout` | Four reversed-order native Nelua/Linux source pairs, identical delivered-byte workload with independent byte checks |
| `rng_windows_shootout` | Four reversed-order native Nelua/Windows source pairs, identical delivered-byte workload with independent byte checks |

The provider in `include/resources.h` supplies PCI config, bounded MMIO,
coherent DMA, ordering and time only. Platform supplies the BAR dataspaces;
Genode 26.08's separate Dma service supplies each bus address. CPU addresses
are obtained independently from attached dataspaces. The x86 guest uses cached,
cache-coherent DMA and full compiler/CPU fences. It does not claim an IOMMU
mapping or physical interrupt route. Completion polling runs in the component
owner domain. Windows' virtual interrupt latch is set by the framework only
after the native transport returns a real used-ring completion.

The framework owns queue layout, feature negotiation, payload allocation and
class behavior. The provider neither opens the entropy fixture nor implements
an entropy read. QEMU's device backend alone reads the 1 MiB fixture
xorshift32 seed `0x6d2b79f5`, unsigned32 shifts `13,17,5`, high byte per step, SHA-256
`9be4f35563ede119bb4a85dd735a75da95b2d815d83a28e257efa93296d384a7`.
A fresh QEMU process resets the fixture for every scenario. Linux explicitly completes its additional 64-byte prefetch before freeze,
which discards the unread bytes at offsets 512–575. Its restored read must
match absolute offsets 576–639. Each guest independently generates expected
bytes and validates a whole returned-payload checksum plus canaries.
This is a device-path correctness test, not a randomness-quality test.

The resource description is deliberately limited to the measured QEMU 8.2.2
q35 topology: `00:03.0`, modern `1af4:1044`, PCIEXBAR `0xb0000001`, BAR1
`0xfebf1000/4096`, BAR4 `0xfe000000/16384`. `rng_probe` obtains the values
through actual CF8/CFC reads and independent QMP resource reporting.
The main component checks the real PCI identity and address values again.
Platform owns ECAM and enables/disables the function; the consumer's exclusive
CF8/CFC session exposes callbacks restricted to its BDF and command bits 0–2.
This recipe is not a general PC device enumerator.

Explicit successful shutdown is required. A failed reset retains DMA and
device ownership; attempted destruction while either remains stops the owner
thread before member teardown. Successful close leaves no live DMA, then the
consumer unmaps its BARs and releases the Platform device. The guest verifies
PCI decoding/bus mastering are cleared, and QMP independently reports disabled
BAR decoding. The fault models in the import and native suites cover reset
failure; this QEMU recipe checks successful real-device quiescence.

Use the exact SDK and toolchain pinned in [../SDK](../SDK). The supplied SDK
must match its complete verified source-tree digest. No SDK edits, external
contrib ports, host installations, privileged host devices or networking are
required. Expect, Tcl, the SDK's host dependencies and a working x86 QEMU TCG
installation are needed as described in [the existing Genode recipe](../README.md).

For each scenario above:

```sh
python3 ports/genode/virtio-rng/build.py \
  --genode-dir /path/to/verified-genode-26.08 \
  --build-dir build/genode-virtio-rng \
  --toolchain-prefix /path/to/tool/25.05/bin/genode-x86- \
  --scenario rng_linux
python3 ports/genode/virtio-rng/boot.py \
  --genode-dir /path/to/verified-genode-26.08 \
  --build-dir build/genode-virtio-rng --scenario rng_linux \
  --qemu /path/to/qemu-system-x86_64 \
  --qemu-data /path/to/share/qemu \
  --bios /path/to/share/seabios/bios-256k.bin \
  --module-dir /path/to/lib/x86_64-linux-gnu/qemu
```

Build metadata records the immutable SDK digest, compiler, image digest and
source inputs. Inputs must stay unchanged throughout the build and still
match before boot. Concurrent builders/runners for the same output are
rejected. The runner validates detailed guest assertions and independent QMP
results; a timeout or pass marker alone is insufficient. It saves the guest
log, QEMU diagnostic log and structured result beside the build metadata.
Records are text evidence only; SDKs, fixture binaries and guest images remain
reproducible build artifacts.

Each shootout row delivers 64 client reads of 512 bytes. The shared harness
times open, reads, exact-data/checksum/canary validation, source prefetch drain,
reset and reclamation. Both variants use the same component, generic resources,
GCC flags (`-O2`), VERSION_1-only negotiation, eight-entry split ring and polling.
Individual samples, absolute fixture offsets and payload checksums are saved
in the guest log and TSV; the runner independently recomputes every checksum
and checks the reversed pair order before accepting a result.
This is a fresh-instance, cold-lifecycle benchmark with no driver-workload
warmup. Machine/device caches may warm across successive rows. It is not a
steady-state measurement.

Linux's unchanged source has a 64-byte data buffer, so it needs eight device
transfers per 512-byte client read and prefetches an additional 64 bytes at the
end. The harness completes and counts that prefetch before reset. Native Nelua
and Windows use larger payload buffers and submit the requested 512 bytes.
These are intrinsic differences in the compared implementations. Equal useful
bytes do not imply equal DMA-request counts. The measurements cover controller,
framework and source strategies, rather than isolating compiler/language speed.
The two paired ELFs remain separate; their timings are not pooled or used to
rank operating systems. Functional-test read timings are separate diagnostics.
The Timer RPC returns integer microseconds. Each shootout records 128
consecutive-read deltas, the smallest positive observed delta and zero-delta
count; this includes RPC/read overhead and does not measure the underlying
clock's intrinsic resolution. Every accepted row has positive elapsed time.
CPU utilization, peak memory, latency percentiles and physical interrupt
latency are not measured.
