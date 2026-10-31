# MINIX modern Virtio RNG qualification

This kit boots the pinned MINIX 3.3.0 i386 image in private QEMU TCG. The RS service reserves a modern-only `1af4:1044` PCI function through MINIX's PCI service, maps its assigned BARs with `vm_map_phys`, and supplies generic `drift_hw`/`drift_pci` resources. Device negotiation and split-ring operations remain in the generated Nelua controller. No host entropy operation is exposed.

Run from the repository root on x86_64 Linux with Python 3, GCC with i386 code generation, GNU binutils, make, and `dpkg-deb`:

```sh
python ports/minix/virtio-rng/run.py --work "$PWD/build/minix-virtio-rng-raw" --profile raw
python ports/minix/virtio-rng/run.py --work "$PWD/build/minix-virtio-rng-linux" --profile linux
python ports/minix/virtio-rng/run.py --work "$PWD/build/minix-virtio-rng-windows" --profile windows
```

This downloads only SHA-pinned QEMU packages and the pinned MINIX ISO into the work directory, extracts the SDK, builds an i386 RS service, injects it into a copy of the ISO, and boots it. An existing private cache can be selected with `--iso`, `--sdk`, and `--qemu-root`. No system installation or host hardware privileges are required. The base image and existing PC-speaker qualification kit are unchanged.

Each profile uses a fresh QEMU boot and a separately linked RS executable. Linux and Windows source profiles are never linked into one binary. Their library targets verify the preserved upstream source hashes before compilation. The Linux source carries GPL-2.0-or-later obligations; the Virtio-Windows source preserves its upstream BSD notices and directory license.

| Profile | Actual guest checks |
| --- | --- |
| `raw` | Deferred modern transport probe, exact 128-byte device fixture, unrelated physical-map denial, injected reset timeout retaining DMA, real retry-reset and balanced release |
| `linux` | Unchanged Linux `virtio-rng.c`, exact 512 bytes and canaries, upstream freeze/restore, explicitly drain/discard one prefetch, another 64 exact fixture bytes at absolute offset 576, cancel-next-read, balanced release |
| `windows` | Unchanged Virtio-Windows RNG source files, exact 512 bytes and canaries, one request completion/notification, cancellation before completion dispatch retaining DMA, balanced release |

The raw transport qualification checks the actual device-written used ring and an exact 128-byte QEMU RNG fixture. The 1 MiB nonperiodic fixture is generated with xorshift32 seed `0x6d2b79f5`: unsigned `x ^= x << 13; x ^= x >> 17; x ^= x << 5`, then emit `x >> 24` for each byte. Its SHA-256 is `9be4f35563ede119bb4a85dd735a75da95b2d815d83a28e257efa93296d384a7`. An independent guest-side C generator compares every returned byte. This is a deterministic I/O integrity oracle, not cryptographic entropy. Linux drains exactly one post-read prefetch completion before freeze, so the restored read must start at absolute offset 576; no offset fallback or modulo matching is accepted. Completion is polled in the owning service; no hardware IRQ handler is installed. This is actual MINIX PCI/MMIO/DMA evidence in an emulator, not physical hardware qualification or a complete Linux kernel/WDF implementation. The Windows profile implements a closed source API on MINIX ILP32, not the Windows NT binary ABI, full WDF, PnP, CNG, or Windows IRQ/IRQL. Neither source profile imports its original operating-system kernel.

`resources.c` keeps CPU and bus addresses separate. `alloc_contig` allocates and preallocates contiguous page-aligned RAM, and MINIX `sys_umap` supplies its physical address. Each allocation is at most one 4 KiB coherent x86 page; an actual CPU memory barrier implements DMA acquire/release ordering. This adapter is scoped to QEMU x86 coherent RAM without an IOMMU and below-4-GiB PCI BARs. MMIO widths 1, 2, and 4 are used by the tested transport; width 8 is two ordered 32-bit accesses, without an atomicity guarantee. PCI command updates preserve the status halfword and existing unrelated command bits.

The driver resets before releasing device-owned DMA. If reset fails, the service retains its mappings, request metadata, and DMA; the shell does not stop that service. The runner may terminate the entire emulated machine. The raw harness deliberately drops one reset-status MMIO write after a real device completion. It verifies timeout, retained DMA, and refused provider cleanup, then restores real writes and retries reset successfully before balanced release. This is explicit host-call fault injection; it does not claim that QEMU refused a delivered reset. Generic provider cleanup refuses live DMA allocations.

The RS configuration permits only the device ID and required IPC/kernel services. A negative check attempts to map the unrelated local-APIC physical page and requires denial. UID 0 does not bypass the MINIX VM resource check.

The operating-system interfaces were checked against the image's exact source commit, [`588a35b9293dcbc89a45881940d00c2ae6d13801`](https://github.com/Stichting-MINIX-Research-Foundation/minix/tree/588a35b9293dcbc89a45881940d00c2ae6d13801):

- [`pci_reserve_a` and BAR discovery](https://github.com/Stichting-MINIX-Research-Foundation/minix/blob/588a35b9293dcbc89a45881940d00c2ae6d13801/minix/drivers/bus/pci/pci.c) grant exact BAR memory ranges and reject inaccessible 64-bit BARs.
- [`map_perm_check` / `do_map_phys`](https://github.com/Stichting-MINIX-Research-Foundation/minix/blob/588a35b9293dcbc89a45881940d00c2ae6d13801/minix/servers/vm/mmap.c) check those permissions before mapping.
- [`alloc_contig` / `free_contig`](https://github.com/Stichting-MINIX-Research-Foundation/minix/blob/588a35b9293dcbc89a45881940d00c2ae6d13801/minix/lib/libsys/alloc_util.c) use preallocated contiguous RAM and query its physical address.
- [RS service configuration parser](https://github.com/Stichting-MINIX-Research-Foundation/minix/blob/588a35b9293dcbc89a45881940d00c2ae6d13801/minix/commands/service/parse.c) implements the explicit PCI device ACL.

Run the paired actual-device diagnostic with the same private cache:

```sh
python ports/minix/virtio-rng/bench.py --work "$PWD/build/minix-virtio-rng-paired" --events 4096 --rounds 4 --iso "$PWD/build/minix-virtio-rng-raw/minix_R3.3.0-588a35b.iso" --sdk "$PWD/build/minix-virtio-rng-raw/sdk" --qemu-root "$PWD/build/minix-virtio-rng-raw/qemu-root"
```

Without the three cache arguments, `bench.py` obtains the same pinned image, SDK, and private QEMU itself. It builds separate native Nelua, Linux-source, and Windows-source executables with identical common `-Os` flags. Twelve fresh guests execute four alternating forward/reverse rounds. Each trial warms up with 64 bytes, then delivers 4096 reads of 64 bytes, checks every byte against the same nonperiodic stream, verifies canaries, and computes the same FNV64 `750af98b1e20c57d`. The measured loop includes this identical validation work and excludes boot, open, and close. All variants negotiate VERSION_1 only, use an eight-entry queue, and poll with a 10 us wait. Linux's unchanged source naturally posts one extra prefetch: total posted buffers are 4098 for Linux and 4097 for native/Windows, including warmup.

The actual MINIX `CLOCK_MONOTONIC` resolution is reported by `clock_getres`; the observed guest clock is about 16.67 ms. Zero durations and trials no longer than the ceiling of one reported resolution tick in microseconds are rejected. These are TCG device/read-loop diagnostics inside one RS service, not IPC service latency, physical throughput, or a ranking across different hosts or operating systems. The four quantized samples per variant warrant a range and median, not precise speedup claims. The default run takes roughly four minutes of guest boots after dependencies are cached; each guest has a 150-second limit.

For CI, run the three `run.py` profiles plus `bench.py --events 4096 --rounds 4`; collect the recursive `*.log`, `*.json`, and `*.txt` files. Do not upload the SDK, ISO, QEMU packages, or ELF files as text evidence. The raw serial logs and per-trial source/build manifests are the reviewable records. A workflow timeout of 20 minutes permits the normal downloads and twelve boots.
