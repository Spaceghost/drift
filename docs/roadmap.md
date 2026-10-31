# The next working pieces

Keep the machine small by adding capabilities only for a demonstrated driver.

Shipped: read-only flash backup with per-block double reads, a descriptor-checked
Linux keyboard monitor, private capture/replay, and installable commands.
These use the existing VM ABI; neither adds a core instruction.

1. **Qualify the first physical endpoints.** JEDEC probe on a named SPI flash
   part, then boot-keyboard reports through an actual negotiated USB endpoint.
   Publish traces, disconnect tests and exact host versions.
2. **Port the host to another OS.** Implement the same effect contract in a
   small OS or a microkernel driver server. Run the unchanged module binaries.
   Driver source changes during a port are evidence of a contract gap.
3. **Add a useful I2C device.** Implement one bound-address adapter and a
   datasheet-backed sensor/RTC module. Verify repeated-start, shorts and errors.
4. **Make authoring pleasant.** Add a disassembler, symbolic diagnostics and
   structured Nelua helpers once two unrelated drivers expose repeated patterns.
   Preserve the bytecode format as an inspectable output.
5. **Reach controller-class hardware deliberately.** Prototype a simple
   MMIO/interrupt device with explicit region handles and memory-order rules.
   DMA needs host-managed buffers, ownership and IOMMU decisions. Native/AOT
   execution needs equivalent effect and memory semantics plus measured benefit.

Do not start with a loader for arbitrary existing kernel driver binaries.
Linux, Windows and BSD drivers depend on their kernels' APIs, scheduling,
memory and synchronization models. Reusing those drivers is a separate
compatibility project, not a feature hidden inside this interpreter.

Related work worth studying before expanding scope includes UDI, DDE/DDEKit,
rump kernels, WebAssembly runtimes, eBPF interpreters, libusb and libsigrok.
Drift's present contribution is a tiny, complete implementation of its own
explicit driver/host boundary. No novelty claim is made over those systems.
