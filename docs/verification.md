# Verification

## Native controller kit

The native library is compiled from Nelua and tested through independent
register/DMA models, real emulated devices, and a booted freestanding kernel.
The common resource interface does not implement device protocols for the
library: those live in the portable driver source.

```sh
make native-check
make native-sanitize
make qemu-check
make kernel-check
```

The native suite includes 4,096 Virtio checks, 303,876 NVMe checks, 138,525
AHCI checks, 6,435 Ethernet checks, 425 CDC checks, 2,824 hub checks and 1,595,440
common-I/O checks, plus PCI and xHCI model scenarios. These assertion counts
include repeated data/range checks; the real-device paths below are separate
integration evidence.

The real controller checks use QEMU 8.2.2 under TCG, without KVM or privileged
host device access. Scratch disk images and isolated network connections are
created for each run. The qtest host maps separate CPU/DMA shadows onto guest
RAM, while the reference kernel uses coherent identity-mapped DMA. Both execute
the same generated controller code. The reference kernel is a booted x86 OS
example; these are two resource hosts, **not two independent OS ports**.

The kernel performs disk read/write/flush/readback/restore through AHCI, NVMe,
Virtio and USB storage, plus Ethernet frame exchange through Virtio, Intel
82540EM and RTL8139. The scripts check individual completion markers and QEMU's
final exit result; a missing controller or failed operation fails the run.

Independent models cover nonidentity and high DMA addresses, allocation failure,
malformed completions, wraparound, timeout, failed reset and retained/quarantined
DMA. The shadow models enforce publish/acquire calls. They do not simulate all
real cache hardware; the resource contract requires coherent or uncached DMA.

The initial APIs poll and serialize requests. Physical hardware qualification,
interrupt-driven operation, multicore contention, throughput benchmarks and a
second independent OS integration are not established. USB and NIC support is
limited to the subsets named in their guides. The library is a new implementation,
not an audited production replacement for a mature kernel's complete drivers.

## Original protocol machine


Date: 2026-10-30. These results describe this source release, not a hardware
certification or a complete additional operating-system port.

## Reference environment

Linux x86-64; GCC 13.3.0; C11; `-Os` for release measurements. Nelua sources
were compiled using the clean Spaceghost fork checkout at
`3c29419776dfdca194fa7e9c572498264fe5c654`. `make verify-generated` compares the
generated C and all three assembled modules with freshly generated copies.

## Checks that passed

| Check | Evidence |
| --- | --- |
| Independent VM tests | 87,213 assertions, including 30,000 deterministic generated/mutated modules |
| Original module execution | 22 JEDEC/keyboard scenarios with exact transfer/event expectations |
| Verified flash | 36 scenarios, 24,016,109 assertions; complete 64 KiB and 16 MiB reads; mismatch/short/error coverage |
| HID descriptor proof | 123 assertions and 20,000 malformed input cases |
| Complete tools | Keyboard capture/replay, held-key cleanup, private capture permissions; flash image byte comparison, no-clobber and partial-file cleanup |
| Host CLI | Golden replay output; wrong command, short response, extra trace row, bus mismatch and failed stdout rejected |
| ASan + UBSan | VM and protocol tests pass with no reported violations |
| Release build | `-Wall -Wextra -Werror`; explicit checks remain enabled in generated code |
| Core dependencies | `nm -u build/drift.o` is empty |
| Freestanding x86-64 | Unchanged JEDEC module runs in a no-libc executable and returns the expected event |
| Freestanding i386 | Same source and module compile/link without libc; runtime execution unavailable here |
| Generated artifacts | Byte comparisons pass after regeneration from Nelua sources |
| Guided example | Documented success/error outputs checked; modified Nelua module runs in both hosts; example passes ASan + UBSan |

Leak detection is disabled in the sanitizer target because the reference
environment does not allow LeakSanitizer to inspect its process-task metadata.
AddressSanitizer and UndefinedBehaviorSanitizer remain enabled. The VM itself
does not allocate.

## Measured footprint

The reference core object has **2,318 bytes of text**, zero data/BSS and zero
undefined symbols. The ELF object file includes metadata and is larger than
that text section. A `drift_vm` occupies **12,448 bytes** on this 64-bit host,
including the fixed-capacity code and memory arenas. A host effect occupies
48 bytes. Use `make size` to measure your own build.

The flash module is 67 instructions and 569 bytes, with 528 bytes of accessible
guest RAM. It still occupies one fixed-capacity VM instance.

The JEDEC module is 25 instructions and 233 bytes. The keyboard module is 68
instructions and 576 bytes. A compact loop implementation reduced the keyboard
module from 3,120 bytes while retaining the unchanged protocol test expectations.

## Review fixes

Independent review caught a real acknowledgment bug: an event sent to a failing
stdout sink was reported as accepted. The host now checks and flushes output
before acknowledging the ticket; `/dev/full` is a regression case. Review also
led to explicit SPI mode validation, nonblocking stream reads and defined
overlapping-response copy semantics with regression assertions.

## Not established

No physical SPI flash or USB keyboard was connected. The trace fixtures are
constructed examples. SPI controller behavior, electrical timing, USB
enumeration, boot-protocol negotiation and detach behavior on actual hardware
still need qualification. Windows, macOS, WebAssembly and a second OS have not
been run. TCC and Zig CC have not been tested. A 32-bit executable was linked successfully, but
the reference kernel cannot execute it.

The [CI workflow](../.github/workflows/check.yml) repeats the build, sanitizer,
freestanding and generated-artifact checks. See
[Actions](https://github.com/Spaceghost/drift/actions) for current results.

The tests exercise the stated VM and protocol contracts. They are not a proof
of correctness, a security audit or a general native-kernel-driver compatibility
claim.
