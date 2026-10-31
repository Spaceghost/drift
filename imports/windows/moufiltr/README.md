# Unchanged Windows mouse-filter source

This opt-in profile compiles Microsoft's original `moufiltr.c` and `moufiltr.h`
without source edits. `SOURCE` pins Windows-driver-samples commit
`f4c1ff842b6a0001152f44cb61788b216a1c7627`; `SHA256SUMS` covers both files and
the retained MS-PL license. The resulting artifact includes MS-PL source and is
not MIT-only. It is separate from Drift's normal native archive/install.

```sh
make -C imports/windows/moufiltr check sanitize lto-check
make -C imports/windows/moufiltr verify-generated NELUA="$PWD/.toolchain/nelua/nelua"
```

`compat.nelua` owns registration, object identity, lifecycle, input-buffer
validation, pending requests and completion ownership. C defines the closed API
and typed entry/storage bindings. The original source still performs CONNECT,
PS/2 hook replacement, packet/ISR forwarding and DISCONNECT rejection.
Unimplemented WDF APIs fail compilation; there is no generic success stub.

## Execution contract

Use a serialized owner domain that permits synchronous reentry on another
request. There is no global current context or nonreentrant gate. Each handle
and request identifies its own device. Up to 64 requests may remain outstanding,
and dispatch nesting is bounded to 64. This supports the sample's parallel
queue: a lower CONNECT send may synchronously issue a HOOK request back into
the same filter. Cross-thread callers must marshal into that owner domain.
This is not an implementation of general KMDF scheduling or Windows IRQLs.

A successful `send` transfers ownership before the lower callback executes;
the callback may complete inline or later through
`drift_windows_moufiltr_complete`. A failed send leaves completion to the
original driver. Completion is delivered once; a contradictory failed send
after completion is reported as a protocol error. STATUS_PENDING is an
acceptance state, not an allowed final completion status.

Keep request storage alive until the outer control/complete API returns,
including after notification. Do not free or reset it inside the notification.
Retain request buffers and state until outstanding requests finish. CONNECT
retains the upper class object, callback context and function code beyond its
request's completion; they must remain alive until the connection is quiesced
and state closed. Marshal every raw installed service/ISR callback to the same
owner domain, and drain active hook calls before close. Raw hook calls bypass
the wrapper's depth counter, so close cannot detect their activity. The imported sample's
DISCONNECT deliberately returns STATUS_NOT_IMPLEMENTED; this profile preserves
that behavior and does not invent a disconnect protocol.

The input and internal callback records are a closed source-API representation,
not the Windows kernel binary ABI. There is no `.sys` loader, IRP stack, PnP
manager, actual i8042 interrupt path or OS-visible mouse device in these tests.
The same policy applies to `PAGED_CODE`/SAL/debug metadata: the model has no
pageable kernel code or real IRQL. An actual OS adapter must enforce its entry,
resource and execution rules.

## Evidence

Tests invoke the real source: registration failures, nested lower-stack IOCTLs,
multiple asynchronous requests, inline and deferred completion, failed sends,
duplicate completion, CONNECT sharing, original DISCONNECT rejection, packet
and ISR callback forwarding, invalid buffers, two-instance isolation and limits.
Normal and ASan/UBSan builds run allocation-failure link wrappers. LTO tests
omit only those wrappers because link-time optimization can bypass linker
interposition; the actual request/callback tests still run.

Objects use caller-owned fixed storage and a local `-fno-strict-aliasing` policy,
with C size/alignment assertions. The combined normal objects have no undefined
symbols, allocator, Nelua startup or hidden threads. A host-model pass is not
Windows kernel loading, hardware qualification or a Genode/MINIX port.

Microsoft's [sample documentation](https://learn.microsoft.com/en-us/samples/microsoft/windows-driver-samples/mouse-input-wdf-filter-driver-moufiltr/)
and [hook IOCTL contract](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntdd8042/ni-ntdd8042-ioctl_internal_i8042_hook_mouse)
describe the original Windows integration. Preserve a source/kernel/profile
pin and complete real target tests before advertising another driver subset.
