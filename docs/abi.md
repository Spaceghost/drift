# Drift module ABI 1

This is the first experimental ABI. Incompatible changes require another
version. The file format does not use native C structure layout.

## Module

All multibyte fields are unsigned little-endian. The header is exactly 32 bytes.

| Offset | Bytes | Meaning |
| --- | --- | --- |
| 0 | 4 | ASCII `DRFT` |
| 4 | 4 | ABI version, currently 1 |
| 8 | 4 | Instruction count, 1..1024 |
| 12 | 4 | Accessible linear-memory length, 1..4096 |
| 16 | 4 | Initialized data length, 0..memory length |
| 20 | 4 | Required capability mask: transfer=1, event=2, wait=4 |
| 24 | 4 | Entry instruction index, less than instruction count |
| 28 | 4 | Bus: none=0, record stream=1, SPI=2, I2C=3 |

The header is followed by `count * 8` instruction bytes, then exactly the
initialized-data bytes. No trailing bytes are accepted. Data begins at guest
address zero; the rest of memory is cleared. Registers are initially zero.
The loader copies code and data. After a successful load the input image may
be released. The input image must not overlap the VM's storage. A failed load
invalidates any previously loaded driver.

There is no signature, checksum, device identity or cryptographic trust claim
in this format. Hosts determine which modules and endpoints they admit.

## Execution

Each instruction is `op:u8, a:u8, b:u8, c:u8, immediate:u32le`. All three
register fields must be below 16, including unused fields. Authors should zero
unused fields. Code is not mapped into guest memory. There is no guest stack,
native call instruction or host-pointer instruction.

Registers are unsigned 32-bit values. Arithmetic wraps modulo 2^32. Division
is unsigned floor division and traps on zero. Shift counts are masked with 31.
Loads/stores are explicitly little-endian and may be unaligned. An address
addition that would wrap cannot pass the bounds checks.

| Op | Name | Operation |
| --- | --- | --- |
| 0 | HALT | Stop; `r0` is the module's result word |
| 1 | LIT | `r[a] = immediate` |
| 2 | MOV | `r[a] = r[b]` |
| 3 | ADD | `r[a] = r[b] + r[c]` |
| 4 | SUB | `r[a] = r[b] - r[c]` |
| 5 | MUL | `r[a] = r[b] * r[c]` |
| 6 | DIV | `r[a] = r[b] / r[c]` |
| 7 | AND | `r[a] = r[b] & r[c]` |
| 8 | OR | `r[a] = r[b] \| r[c]` |
| 9 | XOR | `r[a] = r[b] ^ r[c]` |
| 10 | SHL | `r[a] = r[b] << (r[c] & 31)` |
| 11 | SHR | `r[a] = r[b] >> (r[c] & 31)` |
| 12 | LD8 | `r[a] = memory[r[b] + immediate]` |
| 13 | ST8 | Store low 8 bits of `r[a]` at that address |
| 14 | LD32 | Load 4 bytes at that address |
| 15 | ST32 | Store 4 bytes at that address |
| 16 | JMP | Jump to instruction `immediate` |
| 17 | JEQ | Jump if `r[a] == r[b]` |
| 18 | JNE | Jump if `r[a] != r[b]` |
| 19 | JLT | Jump if unsigned `r[a] < r[b]` |
| 20 | IO | Suspend for a transfer; see below |
| 21 | EVENT | Suspend with tag=`immediate`, x/y/z=`r[a/b/c]` |
| 22 | SLEEP | Suspend until a host wait of `r[a]` milliseconds completes |

All branch targets are checked at load time, including unreachable branches.
Falling off code traps. Memory and transfer ranges are checked at execution
time. Capability requirements are checked at load time for every effect
instruction. Those checks are ordinary code and remain active in release builds.

## Transfer semantics

IO uses `r[a]` as TX offset, `r[b]` as TX length, `r[c]` as RX offset and
`r[immediate]` as RX capacity. The immediate must be a register index below 16.
Ranges may be empty or overlap. Empty ranges may start at the end of memory.
The host must consume TX before completing RX; guest memory is suspended while
an effect is outstanding.

The host binds one endpoint per instance. A module cannot name a path, choose
another device or change bus configuration. The host applies its own operation
and length policy in addition to module capability bits.

| Bus | One transfer means |
| --- | --- |
| Record stream | One record read, at most RX capacity; supplied adapter requires TX length zero |
| SPI | TX bytes then RX clocks in one chip-select interval; discard input during TX, drive zero during RX |
| I2C | Write TX then repeated-start read RX at one host-bound address; omit empty phases; one final STOP |

I2C is a contract for future adapters, not an implemented port. ABI 1 does not
express simultaneous full-duplex SPI or multi-message I2C sequences. A host
must reject requests it cannot represent faithfully rather than split them
into semantically different transactions.

## Effects and completion

The machine issues exactly one effect at a time. An effect contains a
host-supplied nonzero instance identity and a monotonically increasing 32-bit
ticket. Ticket exhaustion traps before wraparound. The host must use a fresh
instance identity on every load; identity freshness is its responsibility.

The PC already points past the effect instruction. Repeated `drift_run` calls
while waiting return the same effect and ticket without executing anything.
They are observations, not instructions to perform an external action again.
Hosts must dispatch a ticket only once and serialize access to each VM.

`drift_complete` accepts only the outstanding instance/ticket. Status zero means
success; a negative status means transport or sink failure, with no response
bytes. Positive statuses are invalid. A successful short transfer is accepted
by the VM; the driver decides whether the protocol permits it. Returned bytes
cannot exceed RX capacity. Events and waits cannot return bytes.

The data pointer is borrowed only during completion; accepted data is copied
into memory. Overlapping source/destination data has memmove semantics. Input
buffers are not modified. Invalid completions leave VM state and memory intact.
After acceptance `r0` contains the status as a u32 bit pattern, `r1` contains the
actual response length, and all other registers retain their values. The state
becomes READY. Duplicate completion is rejected.

EMIT is acknowledged only after the event sink accepts the event. WAIT is
acknowledged no earlier than the requested interval, except explicitly virtual
time in replay. Instruction fuel does not bound wall time spent inside host I/O.
Real hosts need their own deadlines and detach/cancellation policy.

## Return values

| Value | Meaning |
| --- | --- |
| 0 READY | Runnable; successful load/completion |
| 1 EFFECT | Waiting on the effect in `vm.effect` |
| 2 QUOTA | Instruction budget exhausted; still runnable |
| 3 DONE | Halted; inspect `r0` |
| -1 EFORMAT | Invalid image or header |
| -2 EOP | Invalid opcode, register or branch target |
| -3 EBOUNDS | Code/memory range or response length violation |
| -4 ECAP | Missing capability or invalid IO length-register field |
| -5 ESTATE | No pending effect to complete |
| -6 ESTALE | Wrong instance or ticket |
| -7 EDIVZERO | Division by zero |
| -8 ETICKET | Ticket space exhausted |
| -9 EARG | Invalid API argument/completion form |

Execution traps are sticky. A new load is required to run again. QUOTA is a
scheduler result, not a sticky error. A zero budget executes no instruction.
The `steps` statistic counts executed instructions modulo 2^64. VM structures
are process-local host state; do not serialize them as portable snapshots.
