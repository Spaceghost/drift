/* SPDX-License-Identifier: MIT
 * Shared workload and validation; include after exactly one Imported_case.
 * Each row times open,64 x512-byte read+validation, prefetch drain and close.
 * Both implementations use the same resources/compiler/features. */
struct Shootout {
 Resources resources;
 drift_virtio_bar bars[6] {};
 Imported_case imported {resources, bars};
 drift_virtio native {};
 Expected_stream expected {};
 uint8_t output[514] {};
 explicit Shootout(Genode::Env &env) : resources(env) {
  if (!resources.acquire()) { Genode::error("shootout resources unavailable"); return; }
  for (unsigned i = 0; i < 6; ++i)
   bars[i] = {resources.region[i].ds.constructed() ? i : UINT32_MAX, resources.region[i].size};
  uint64_t last = resources.timer.elapsed_us(), minimum = UINT64_MAX;
  unsigned zero_deltas = 0;
  for (unsigned i = 0; i < 128; ++i) {
   uint64_t const now = resources.timer.elapsed_us();
   if (now < last) { Genode::error("shootout clock moved backwards"); return; }
   if (now == last) ++zero_deltas;
   else if (now-last < minimum) minimum = now-last;
   last = now;
  }
  if (minimum == UINT64_MAX) { Genode::error("shootout clock did not advance"); return; }
  Genode::log("RNG_CLOCK unit_us=1 deltas=128 min_positive_us=", minimum, " zero_deltas=", zero_deltas);
  for (unsigned round = 0; round < 4; ++round) {
   for (unsigned position = 0; position < 2; ++position) {
    bool const candidate = (round + position) % 2 != 0;
    unsigned const allocations = resources.allocations, releases = resources.releases;
    uint64_t const offset = expected.offset;
    expected.checksum = 14695981039346656037ull;
    uint64_t const start = resources.timer.elapsed_us();
    bool const opened = candidate ? imported.open() :
      (!drift_virtio_open_function(&native, &resources.hw, &resources.pci, Resources::BDF, bars) &&
       native.state == 2 && native.features == 0 && native.device_id == 4);
    if (!opened || resources.error || resources.live != 2) {
     Genode::error("shootout open failed; retaining resources"); return;
    }
    for (unsigned call = 0; call < 64; ++call) {
     for (auto &byte : output) byte = 0xa5;
     int const n = candidate ? imported.read(output+1, 512) : drift_virtio_entropy(&native, output+1, 512);
     if (n != 512 || !expected.check(output+1, 512) || output[0] != 0xa5 || output[513] != 0xa5) {
      Genode::error("shootout exact payload/canary check failed; retaining resources"); return;
     }
    }
    int const prefetched = candidate ? imported.prefetch() : 0;
    if (prefetched < 0) { Genode::error("shootout prefetch completion failed"); return; }
    expected.discard(static_cast<unsigned>(prefetched));
    bool const closed = candidate ? imported.close() : !drift_virtio_close(&native);
    uint64_t const elapsed = resources.timer.elapsed_us() - start;
    if (!closed || resources.error || resources.live || resources.allocations-allocations != 2 ||
        resources.releases-releases != 2 || (candidate && imported.notifications !=
          (Genode::strcmp(Imported_case::name, "windows") == 0 ? 64u : 0u))) {
     Genode::error("shootout close/count check failed; retaining resources"); return;
    }
    Genode::log("RNG_SHOOTOUT round=", round, " position=", position,
                " variant=", candidate ? Imported_case::name : "native",
                " calls=64 bytes=32768 offset=", offset, " next_offset=", expected.offset,
                " prefetch=", prefetched, " features=1 allocations=2 releases=2 mappings=0 notifications=",
                candidate ? imported.notifications : 0, " guest_us=", elapsed,
                " fnv64=", Genode::Hex(expected.checksum));
   }
  }
  if (!resources.release_device()) return;
  Genode::log("DRIFT_GENODE_RNG_SHOOTOUT PASS profile=", Imported_case::name,
              " rows=8 delivered_bytes=262144 allocations=16 releases=16 mappings=0");
 }
};
