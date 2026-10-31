# Unchanged Linux PC speaker driver

This opt-in experiment compiles Linux v6.18's `pcspkr.c` **without modifying
one byte**. `SOURCE` pins the commit; `SHA256SUMS` covers the original source
and GPL license. The compatibility runtime is Nelua, with generated C included
for ordinary C builds. `driver.c` supplies explicit lifecycle registration and
a C11 atomic, non-spinning call gate. No Linux module binary is loaded.

```sh
make -C imports/linux/pcspkr check
make -C imports/linux/pcspkr sanitize
make -C imports/linux/pcspkr verify-generated
```

The separate `build/linux-pcspkr/libdrift-linux-pcspkr.a` is not included in
Drift's default MIT libraries or installation. The imported source is
**GPL-2.0-only**. Keep its copyright and license with redistributed combined
artifacts; this experiment does not relicense the Linux driver as MIT.

Give `drift_linux_pcspkr_open` zero-initialized caller-owned state and the host
callbacks in `drift_linux_pcspkr.h`. The upstream driver handles probe, sound
events, suspend, shutdown and removal. The compatibility layer supplies one
fixed input-device slot, registration bookkeeping, checked I/O and error
propagation. It has no allocator, scheduler, background thread or Nelua runtime.
The tiny Linux headers cover only this pinned driver; unsupported APIs have no
catch-all success stubs. A negative compile test rejects `request_irq`.

The host must own PIT channel 2 and the speaker control port, supply real
serialization/interrupt-state preservation through `lock`/`unlock`, and a real
one-microsecond delay. The exact allowed operations are writes to ports
`0x42`, `0x43`, `0x61` and reads from `0x61`. Delayed Linux I/O operations use
the host's delay callback rather than granting access to port `0x80`.
Competing or nested entry returns `-16`; it never spins or invokes the host.
Do not access the state or release resources while a call is in progress.

On a resource error the call returns the host's negative error. A failed
unlock retains the saved flags; subsequent ordinary entry is barred and
`close` retries that unlock before further I/O. A failed speaker stop retains
the instance/resources, blocks new sound events and permits another `close`.
Retry a failed unlock in the original lock-owning execution context; saved
flags do not make mutex ownership or interrupt state transferable to another
thread. The host must reject or explicitly arrange any such recovery.
Do not zero or reuse a failed instance to bypass recovery.

The import profile uses `-fno-strict-aliasing`, as Linux does, because fixed
caller-owned slots hold Linux C objects. Static assertions check their sizes
and alignment. This flag is local to the import and its tests.

Tests call the actual upstream probe and event functions. They check metadata,
the exact independent PIT/port transcript, bell/tone ranges, upper control-bit
preservation, suspend/shutdown/remove, allocation and registration failures,
every resource failure, recovery, reentry and a real competing pthread. The
linked object has no undefined symbols. Sanitizers check host execution.
For the GNU MinGW COFF partial-link audit only, automatic DLL data imports and
runtime pseudo-relocations are disabled. GNU ld otherwise synthesizes a CRT
relocation helper when its LTO plugin and auto-import mode are active.
The strict zero-undefined-symbol check remains, and the executable model tests
retain their normal platform linker behavior.

**Status:** host model proof only. No PC speaker hardware, QEMU device, Genode
component or MINIX service has been booted by these tests. Both OSes still
need their own resource adapter and actual boot evidence. This driver neither
establishes arbitrary Linux driver compatibility nor changes Drift's VM ABI.
