/* SPDX-License-Identifier: MIT */
#include <drift_windows_viorng.h>
struct Imported_case {
 static constexpr char const *name = "windows";
 Resources &resources;
 drift_virtio_bar *bars;
 drift_win_rng profile {};
 drift_win_rng_request request {};
 drift_win_rng_host host {this, completed};
 unsigned notifications = 0;
 bool callback_error = false;
 Imported_case(Resources &r, drift_virtio_bar *b) : resources(r), bars(b) {}
 static void completed(void *p, drift_win_rng_request *request) {
  auto &s = *static_cast<Imported_case *>(p);
  ++s.notifications;
  if (s.profile.locked || request != &s.request || request->completions != 1 ||
      request->phase != DRIFT_WIN_RNG_COMPLETE) s.callback_error = true;
 }
 bool open() {
  if (profile.phase || profile.transport.state) return false;
  profile = {}; request = {}; notifications = 0; callback_error = false;
  int const n = drift_windows_viorng_open_function(&profile, &resources.hw, &resources.pci,
                  Resources::BDF, bars, &host);
  return !n && profile.phase == 2 && profile.transport.state == 5 && profile.transport.features == 0;
 }
 int read(uint8_t *p, unsigned n) {
  int const status = drift_windows_viorng_read(&profile, &request, p, n, n);
  if (status != DRIFT_WIN_RNG_PENDING && status != DRIFT_WIN_RNG_SUCCESS) return -1;
  uint64_t const start = resources.timer.elapsed_us();
  while (request.phase != DRIFT_WIN_RNG_COMPLETE) {
   if (drift_windows_viorng_pump(&profile) < 0 || resources.timer.elapsed_us()-start > 1000000) return -1;
   if (request.phase != DRIFT_WIN_RNG_COMPLETE) resources.timer.usleep(10);
  }
  if (request.status || request.information != n || request.completions != 1 || callback_error) return -1;
  return drift_windows_viorng_request_reset(&request) ? -1 : static_cast<int>(n);
 }
 int prefetch() { return 0; }
 bool close() {
  return !drift_windows_viorng_close(&profile) && !profile.phase && !profile.transport.state && !callback_error;
 }
};
