/* SPDX-License-Identifier: MIT */
#include <drift_linux_virtio_rng.h>
struct Imported_case {
 static constexpr char const *name = "linux";
 Resources &resources;
 drift_virtio_bar *bars;
 drift_virtio transport {};
 drift_linux_virtio_rng profile {};
 unsigned notifications = 0;
 Imported_case(Resources &r, drift_virtio_bar *b) : resources(r), bars(b) {}
 bool open() {
  if (profile.phase || transport.state) return false;
  profile = {}; transport = {}; notifications = 0;
  int n = drift_virtio_raw_open_function(&transport, &resources.hw, &resources.pci,
              Resources::BDF, bars, 4, 1);
  if (!n) n = drift_linux_virtio_rng_open(&profile, &transport);
  return !n && profile.phase == 2 && transport.state == 5 && transport.features == 0;
 }
 int read(uint8_t *p, unsigned n) { return drift_linux_virtio_rng_read(&profile, p, n, 1); }
 int prefetch() {
  uint64_t const start = resources.timer.elapsed_us();
  int delivered = 0;
  while (profile.transfer.active) {
   delivered = drift_linux_virtio_rng_service(&profile);
   if (delivered < 0 || resources.timer.elapsed_us()-start > 1000000) return -1;
   if (profile.transfer.active) resources.timer.usleep(10);
  }
  return delivered == 1 && profile.transfer.length == 64 ? 64 : -1;
 }
 bool close() {
  return !drift_linux_virtio_rng_close(&profile) && !profile.phase && !transport.state;
 }
};
