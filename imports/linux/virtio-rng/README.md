# Unchanged Linux Virtio RNG source import

This opt-in profile compiles Linux v6.18 `drivers/char/hw_random/virtio-rng.c`
without changing its bytes. The source owns probe, registration, scatterlist
submission, entropy reads, completion handling, freeze/restore, and removal.
Drift implements the bounded framework underneath those calls in Nelua.

[SOURCE](SOURCE) pins Linux commit
`7d0a66e4bb9081d75c82ec4957c50034cb0ea449`.
The driver SHA-256 is
`720433a85ce0ce6f84e85842b54d23c7c8e9bad792c66b1066accbaa7fb95a75`.
`make source-check` verifies both the driver and its accompanying license.
From the repository root, `sh tools/import-linux.sh virtio-rng` restores
missing upstream files from the pinned revision after checksum verification;
it refuses to overwrite local edits or linked destination files.
The driver is GPL-2.0-or-later; the framework/wrapper are MIT. The combined
archive is a separate GPL source import, outside the default MIT native library.

The subset is one modern Virtio RNG device with one split queue, one input
scatterlist element, and one outstanding 64-byte transfer. The generic raw
Virtio transport negotiates VERSION_1 and creates the queue with DRIVER_OK
unset. The unchanged source sets DRIVER_OK through `virtio_device_ready()`.
The source uses a 64-byte data buffer; the framework maps it
through a real coherent DMA bounce buffer, publishes the descriptor, kicks
the device, and acquires/copies returned DMA before invoking the upstream
callback. Host adapters supply PCI, bounded MMIO/register access, coherent
DMA, synchronization and monotonic time. There is no host entropy operation
and this profile never calls Drift's native entropy class helper.

Completions are polled from actual used-ring entries. No physical interrupt
handler, Linux module loader, scheduler, kernel ABI, entropy-pool injection,
or general Linux driver environment is provided. The private headers expose
only APIs used by this source; an unknown API fails compilation. IDA has
16 real reusable leases, allocation has one caller-owned source slot per
instance, and completion counts, registration and unregister/reset semantics
are implemented. A global non-spinning gate rejects overlapping or nested
profile calls with `-EBUSY`; it protects the bounded source's shared IDA and
implicit framework context. Callers separately serialize each native transport.

The upstream `unsigned long` pointer casts require LP64 or ILP32. Linux and
Genode x86-64 and MINIX i386 fit this contract; Windows x64 LLP64 is explicitly
rejected at compile time. Changing or truncating the upstream casts would
break the unchanged-source claim. Generic raw Virtio itself supports 32-bit
and 64-bit callers with 64-bit device bus addresses.

Use [drift_linux_virtio_rng.h](drift_linux_virtio_rng.h) from C or C++:

1. Zero-initialize the caller-owned native transport and profile state.
2. Bind the device's generic resources and call
   `drift_virtio_raw_open_function(..., 4, 1)` (or the raw MMIO attach).
3. Pass that deferred-ready transport to `drift_linux_virtio_rng_open()`.
4. Call `read()` with 1..4096 bytes and a blocking/nonblocking flag.
   `service()` optionally pumps a completion without a read. A nonblocking
   read returns zero while pending. A negative read result can follow a
   copied/consumed prefix; ignore the output on error.
5. `cancel()` is an atomic request that interrupts the active or next blocking
   read with `-EINTR`; it neither consumes pending DMA nor affects nonblocking
   reads. Retain the open instance while another context requests cancellation.
6. `close()` must succeed before releasing state or resources. Failed reset
   retains the source slot, IDA lease, queue, transfer context and payload in
   quarantine. Retry close after the host/device can establish quiescence.

Freeze invokes the actual upstream freeze/removal path. After successful
freeze, reopen a raw deferred-ready transport on the same retained resources
and pass it to `restore()`. Prefetched bytes may be discarded by upstream
freeze; entropy-stream offsets are not a lifecycle guarantee. A failed bind
can retain resources in phase 5 until close succeeds. Phase 0 never retains
profile DMA. If open rejects before binding, the caller still owns and closes
the deferred native transport. State and transport are trusted caller-owned
storage and must not be modified during their lifetime.

From this directory, with the repository's pinned Nelua compiler bootstrapped:

```sh
make check sanitize verify-generated
make library
```

`check` verifies the upstream hashes, executes actual-source tests and requires
an empty undefined-symbol audit for the combined freestanding objects. Tests
cover reads, genuine model DMA with distinct CPU/bus addresses, delayed and
malformed completions, allocation and ID exhaustion, ignored upstream return
values, registration errors, timeout/reset quarantine and retry, freeze/restore
failures, nested/concurrent rejection, and concurrent cancellation. Faults use
linker wrappers in the test binary; production code has no fault switches.
`closed-api-check` checks that an unsupported Linux API fails compilation.
`sanitize` runs the same model under ASan/UBSan. Generated C comes from
`compat.nelua`; `make regen` updates it and `verify-generated` compares it.
All import compilation uses `-fno-strict-aliasing` for Linux-style typed source
slots. Host model success alone does not qualify a physical device or an OS
port; OS kits preserve their own guest runs and provenance separately.
