# Drift

Keep the runtime and driver authoring tools in Nelua. Generated C is a build
artifact, never the source of truth. Host adapters may use small amounts of C.
No allocator, libc, operating-system API, native pointer, clock, or hidden
thread in the VM core. Keep protocol logic in modules, transport in hosts.

All input validation is explicit and survives release builds. Bytecode is a
little-endian format, independent of C structure layout. Host authority is
bound to one endpoint per instance. One effect, one completion. An instance
identifier is never reused. Read docs/abi.md before changing the instruction
set or effect semantics; ABI changes require a version bump.

Prefer complete, readable mechanisms over frameworks. Add tests for new
semantics and fault paths. Never advertise simulated device tests as hardware
qualification or host tests as proof of a complete operating-system port.
