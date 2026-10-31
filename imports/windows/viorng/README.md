# Unchanged Virtio-Windows RNG driver

This profile builds all five original driver translation units, `viorng.h`
and `trace.h`, from virtio-win commit
`bd196cda83a50a5b2545dd4f85d25cc6238e10ae`. `SOURCE` and `SHA256SUMS` preserve
their identity. Each file's original BSD-3-Clause notice and the directory's
BSD-2-style license remain intact. The original files are not patched.

```sh
make -C imports/windows/viorng check sanitize lto-check
make -C imports/windows/viorng verify-generated NELUA="$PWD/.toolchain/nelua/nelua"
```

The C bridge provides typed entries/storage and a scoped allocator binding.
Nelua implements the bounded WDF/Virtio API personality. Original source owns
read scheduling decisions, SG submission, list/cancel processing, ISR/DPC
processing, result copying and power/cleanup callbacks. Unsupported WDF APIs
fail compilation. Trace generation is explicitly off; the unused diagnostic
formatter returns NOT_SUPPORTED rather than pretending to format data.

## Resources and use

`drift_windows_viorng.h` exposes caller-owned state and request storage.
Zero-initialize both before first use. Open
with generic `drift_hw`/`drift_pci` resources for a modern Virtio RNG device.
Submit `read`, call `pump` in the owner domain until completion, then inspect
the request's NTSTATUS and byte count. `host.completed` is notified outside
source callbacks and locks. No host callback supplies entropy or decodes queues.

The transport uses actual external coherent DMA and Drift's generic native
split queue. It never calls `drift_virtio_entropy`. CPU and device addresses
may differ. Used lengths and cookies are validated before original source
can see them; DMA is acquired before its result-copying code executes.

The device path is polling. A validated used-ring completion latches a virtual
WDF interrupt; the original ISR schedules its original DPC for later execution.
Neither a no-work pump nor an empty kick invents a completion. This does not
implement physical Windows interrupt routing or kernel IRQLs.

## Request and memory ownership

Calls across all instances run in one serialized owner domain: the C bridge's
scoped allocator binding is one global, non-atomic pointer. The framework keeps at most sixteen
outstanding/undelivered requests and one active source read. Notifications are
deferred until source callbacks and locks return. Retain request/output/state
storage through completion and the outer API return; do not reset/free it in
its completion notification. A completed request can then be reset explicitly.

The original driver has one shared 4 KiB DMA buffer. Cancelling a read completes
the request, but its descriptor and read-entry metadata remain owned until a
used completion or successful reset. Subsequent reads stay queued until that
ownership is reclaimed. Device-owned memory is never freed to make cancellation
appear synchronous. Short reads are permitted and capped at 4 KiB.

Final close purges requests, disables virtual callbacks, runs the original D0
exit/reset path and then cleanup. Failed reset quarantines DMA and metadata,
leaves a nonzero phase, and returns an error even though the upstream power
callback itself returns SUCCESS. Keep all resources and retry close after
quiescence becomes possible. Phase zero means resources are released.

This profile supports final D0 exit/removal. Transparent system suspend/requeue
is outside its public API; the unsupported stop-acknowledge requeue case records
NOT_SUPPORTED. It is not a general WDF power-policy implementation.

The upstream DMA-allocation behavior is preserved: D0 entry can return SUCCESS
after common-buffer allocation fails, but the next nonempty read reports
INSUFFICIENT_RESOURCES. Zero-length requests complete SUCCESS/0 in the framework,
as specified by the source's `AllowZeroLengthRequests=FALSE`; no device work is
performed for them.

## Verification boundaries

Models use the independent native register/DMA oracle with nonperiodic
xorshift32 device bytes (low byte per step); platform evidence uses its separately
pinned high-byte fixture. They cover actual SG/used-ring transfer, delayed
copying, cancellation with retained DMA, request bounds, malformed completions,
timeout, failed-reset quarantine/retry, callback gating, a completion callback
that submits another read, and instance isolation.
Normal, ASan/UBSan and O3/LTO runs exercise the same source behavior.

ELF undefined-symbol verification uses a final static, no-CRT audit image,
never executed, so the linker resolves its own GOT normally. PE uses the
MinGW relocatable audit with auto-import/pseudo-relocations disabled. No
symbols are filtered or allowed unresolved; normal library/test flags stay
unchanged. The ELF audit has no dynamic section or DT_NEEDED dependencies.

The generated C has no Nelua startup, heap or threads. Allocations are bounded
caller-owned pools and explicit platform DMA. A source-model pass is not
Windows `.sys` loading, Windows CNG/system-RNG registration, general KMDF/NT
compatibility or hardware qualification. Real device/OS evidence belongs to
the platform-specific test records, separately from these models.
