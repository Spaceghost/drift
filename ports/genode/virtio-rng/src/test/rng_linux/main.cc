#include <base/component.h>
#include <resources.h>
#include <fixture.h>
#include <drift_linux_virtio_rng.h>

namespace {
uint64_t checksum(uint8_t const *p, unsigned n) {
 uint64_t value = 14695981039346656037ull;
 for (unsigned i = 0; i < n; ++i) { value ^= p[i]; value *= 1099511628211ull; }
 return value;
}
struct Main {
 Resources resources;
 drift_virtio transport {};
 drift_linux_virtio_rng profile {};
 Expected_stream expected {};
 uint8_t first[258] {}, second[258] {};
 uint8_t restored_bytes[66] {};
 explicit Main(Genode::Env &env) : resources(env) {
  if (!resources.acquire()) { Genode::error("Linux RNG resources unavailable"); return; }
  drift_virtio_bar bars[6] {};
  for (unsigned i = 0; i < 6; ++i)
   bars[i] = {resources.region[i].ds.constructed() ? i : UINT32_MAX, resources.region[i].size};
  int opened = drift_virtio_raw_open_function(&transport, &resources.hw, &resources.pci,
                  Resources::BDF, bars, 4, 1);
  if (!opened) opened = drift_linux_virtio_rng_open(&profile, &transport);
  Genode::log("RNG_LINUX OPEN status=", opened, " transport=", transport.state,
              " phase=", profile.phase, " live=", resources.live);
  if (opened || transport.state != 5 || profile.phase != 2 || resources.error) {
   Genode::error("Linux source RNG open failed; retaining resources"); return;
  }
  first[0] = first[257] = second[0] = second[257] = 0xa5;
  uint64_t const start = resources.timer.elapsed_us();
  int const a = drift_linux_virtio_rng_read(&profile, first + 1, 256, 1);
  int const b = drift_linux_virtio_rng_read(&profile, second + 1, 256, 1);
  uint64_t const elapsed = resources.timer.elapsed_us() - start;
  bool const exact = expected.check(first + 1, 256) && expected.check(second + 1, 256);
  Genode::log("RNG_LINUX READ first=", a, " second=", b,
              " hash_first=", Genode::Hex(checksum(first + 1, 256)),
              " hash_second=", Genode::Hex(checksum(second + 1, 256)),
              " guest_us=", elapsed);
  if (a != 256 || b != 256 || !exact || resources.error ||
      first[0] != 0xa5 || first[257] != 0xa5 || second[0] != 0xa5 || second[257] != 0xa5) {
   Genode::error("Linux source RNG payload or canary validation failed"); return;
  }
  /* The source prefetches once when its last data is consumed. Complete that
   * actual transfer before freeze, which discards the source's unread64 bytes.
   * The next open therefore starts at absolute fixture offset576. */
  uint64_t const prefetch_start = resources.timer.elapsed_us();
  int prefetched = 0;
  while (profile.transfer.active) {
   prefetched = drift_linux_virtio_rng_service(&profile);
   if (prefetched < 0 || resources.timer.elapsed_us() - prefetch_start > 1000000) {
    Genode::error("Linux source RNG prefetch completion failed"); return;
   }
   if (profile.transfer.active) resources.timer.usleep(10);
  }
  if (prefetched != 1 || profile.transfer.length != 64) {
   Genode::error("Linux source RNG prefetch shape failed"); return;
  }
  expected.discard(64);
  Genode::log("RNG_LINUX PREFETCH completion=", prefetched,
              " bytes=", profile.transfer.length, " next_offset=", expected.offset);
  int const frozen = drift_linux_virtio_rng_freeze(&profile);
  Genode::log("RNG_LINUX FREEZE status=", frozen, " transport=", transport.state,
              " phase=", profile.phase, " live=", resources.live);
  if (frozen || transport.state || profile.phase != 4 || resources.live || resources.error) {
   Genode::error("Linux source RNG freeze failed; retaining resources"); return;
  }
  int restored = drift_virtio_raw_open_function(&transport, &resources.hw, &resources.pci,
                   Resources::BDF, bars, 4, 1);
  if (!restored) restored = drift_linux_virtio_rng_restore(&profile, &transport);
  restored_bytes[0] = restored_bytes[65] = 0xa5;
  int const after = restored ? restored : drift_linux_virtio_rng_read(&profile, restored_bytes + 1, 64, 1);
  bool const restored_exact = expected.check(restored_bytes + 1, 64);
  Genode::log("RNG_LINUX RESTORE status=", restored, " bytes=", after,
              " first=", static_cast<unsigned>(restored_bytes[1]), " offset=576 transport=", transport.state,
              " phase=", profile.phase, " live=", resources.live);
  if (restored || after != 64 || !restored_exact || resources.error ||
      restored_bytes[0] != 0xa5 || restored_bytes[65] != 0xa5) {
   Genode::error("Linux source RNG restore payload failed; retaining resources"); return;
  }
  Genode::log("RNG_LINUX CHECKSUM bytes=576 fnv64=", Genode::Hex(expected.checksum));
  if (expected.checksum != 0xacb83090e9c1a505ull) {
   Genode::error("Linux source RNG whole-payload checksum failed"); return;
  }
  uint64_t const before_cancel = checksum(first + 1, 256);
  if (drift_linux_virtio_rng_cancel(&profile) ||
      drift_linux_virtio_rng_read(&profile, first + 1, 256, 1) != -4 ||
      checksum(first + 1, 256) != before_cancel || first[0] != 0xa5 || first[257] != 0xa5) {
   Genode::error("Linux source RNG cancellation failed"); return;
  }
  int const closed = drift_linux_virtio_rng_close(&profile);
  Genode::log("RNG_LINUX CLOSE status=", closed, " transport=", transport.state,
              " phase=", profile.phase, " live=", resources.live,
              " allocated=", resources.allocations, " released=", resources.releases);
  if (closed || transport.state || profile.phase || resources.live || resources.error) {
   Genode::error("Linux source RNG close failed; retaining resources"); return;
  }
  if (!resources.release_device()) return;
  Genode::log("DRIFT_GENODE_RNG_LINUX PASS bytes=576 exact=576 freeze_restore=1 cancellation=1 mappings=0");
 }
};
}
void Component::construct(Genode::Env &env) { static Main main(env); }
