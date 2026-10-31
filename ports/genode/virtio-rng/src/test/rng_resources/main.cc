#include <base/component.h>
#include <resources.h>
#include <drift_virtio.h>

namespace {
struct Main {
 Resources resources;
 explicit Main(Genode::Env &env) : resources(env) {
  if (!resources.acquire()) {
   if (!resources.error && !resources.device.valid() && !resources.live)
    Genode::log("DRIFT_GENODE_RNG_RESOURCES DENIED PASS allocations=0 mappings=0");
   return;
  }
  drift_virtio_bar bars[6] {};
  for (unsigned i = 0; i < 6; ++i)
   bars[i] = {resources.region[i].ds.constructed() ? i : UINT32_MAX,
              resources.region[i].size};
  for (unsigned round = 0; round < 3; ++round) {
   drift_virtio transport {};
   int const opened = drift_virtio_raw_open_function(&transport, &resources.hw,
                  &resources.pci, Resources::BDF, bars, 4, 1);
   Genode::log("RNG_HANDSHAKE round=", round, " open=", opened,
               " state=", transport.state, " live=", resources.live);
   if (opened || transport.state != 4 || resources.live != 1 || resources.error) {
    Genode::error("raw transport handshake failed; retaining all resources");
    return;
   }
   /* Deliberately no class operations or DRIVER_OK: this is the initial
    * coherent-ring allocation/feature-negotiation resource proof only. */
   int const closed = drift_virtio_close(&transport);
   Genode::log("RNG_QUIESCENCE round=", round, " close=", closed,
               " state=", transport.state, " live=", resources.live,
               " allocated=", resources.allocations, " released=", resources.releases);
   if (closed || transport.state || resources.live || resources.error) {
    Genode::error("reset failed; retaining resource ownership");
    return;
   }
  }
  if (!resources.release_device()) return;
  Genode::log("DRIFT_GENODE_RNG_RESOURCES PASS allocations=", resources.allocations,
              " releases=", resources.releases, " mappings=", resources.live);
 }
};
}
void Component::construct(Genode::Env &env) { static Main main(env); }
