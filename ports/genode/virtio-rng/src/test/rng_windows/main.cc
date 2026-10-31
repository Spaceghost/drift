#include <base/component.h>
#include <resources.h>
#include <fixture.h>
#include <drift_windows_viorng.h>

namespace {
struct Main {
 Resources resources;
 drift_win_rng profile {};
 Expected_stream expected {};
 drift_win_rng_request first {}, second {}, active {}, queued {}, cancelled {};
 drift_win_rng_host host {this, completed};
 uint8_t a[258] {}, b[258] {}, c[66] {}, unused[66] {};
 unsigned notifications = 0;
 bool callback_error = false;
 static void completed(void *context, drift_win_rng_request *request) {
  auto &self = *static_cast<Main *>(context);
  ++self.notifications;
  if (self.profile.locked || request->phase != DRIFT_WIN_RNG_COMPLETE || request->completions != 1)
   self.callback_error = true;
 }
 bool await(drift_win_rng_request &r) {
  uint64_t const start = resources.timer.elapsed_us();
  while (r.phase != DRIFT_WIN_RNG_COMPLETE) {
   if (drift_windows_viorng_pump(&profile) < 0 || resources.error ||
       resources.timer.elapsed_us() - start > 1000000) return false;
   if (r.phase != DRIFT_WIN_RNG_COMPLETE) resources.timer.usleep(10);
  }
  return r.status == DRIFT_WIN_RNG_SUCCESS && r.completions == 1 && !callback_error;
 }
 bool submit(drift_win_rng_request &r, uint8_t *data, unsigned n) {
  int const status = drift_windows_viorng_read(&profile, &r, data, n, n);
  return status == DRIFT_WIN_RNG_PENDING || status == DRIFT_WIN_RNG_SUCCESS;
 }
 explicit Main(Genode::Env &env) : resources(env) {
  if (!resources.acquire()) { Genode::error("Windows RNG resources unavailable"); return; }
  drift_virtio_bar bars[6] {};
  for (unsigned i = 0; i < 6; ++i)
   bars[i] = {resources.region[i].ds.constructed() ? i : UINT32_MAX, resources.region[i].size};
  int const opened = drift_windows_viorng_open_function(&profile, &resources.hw, &resources.pci,
                       Resources::BDF, bars, &host);
  Genode::log("RNG_WINDOWS OPEN status=", opened, " transport=", profile.transport.state,
              " phase=", profile.phase, " live=", resources.live);
  if (opened || profile.phase != 2 || profile.transport.state != 5 || resources.error) {
   Genode::error("Windows source RNG open failed; retaining resources"); return;
  }
  a[0] = a[257] = b[0] = b[257] = c[0] = c[65] = unused[0] = unused[65] = 0xa5;
  uint64_t const start = resources.timer.elapsed_us();
  if (!submit(first, a + 1, 256) || !await(first) ||
      !submit(second, b + 1, 256) || !await(second)) {
   Genode::error("Windows source RNG read failed"); return;
  }
  uint64_t const elapsed = resources.timer.elapsed_us() - start;
  bool exact = expected.check(a + 1, 256) && expected.check(b + 1, 256);
  Genode::log("RNG_WINDOWS READ first=", first.information, " second=", second.information,
              " exact=", exact ? 1 : 0, " notifications=", notifications, " guest_us=", elapsed);
  if (!exact || first.information != 256 || second.information != 256 || notifications != 2 ||
      a[0] != 0xa5 || a[257] != 0xa5 || b[0] != 0xa5 || b[257] != 0xa5) {
   Genode::error("Windows source RNG exact bytes/canaries failed"); return;
  }
  if (!submit(active, c + 1, 64) || !submit(queued, unused + 1, 64) ||
      active.phase != DRIFT_WIN_RNG_ACTIVE || profile.active != &active || !profile.transfer.active ||
      queued.phase != DRIFT_WIN_RNG_QUEUED ||
      drift_windows_viorng_cancel(&profile, &queued) ||
      queued.status != DRIFT_WIN_RNG_CANCELLED || queued.completions != 1 || !await(active)) {
   Genode::error("Windows source RNG queued cancellation failed"); return;
  }
  exact &= expected.check(c + 1, 64);
  for (unsigned i = 0; i < 64; ++i) exact &= unused[i+1] == 0;
  if (!exact || active.information != 64 || notifications != 4 ||
      c[0] != 0xa5 || c[65] != 0xa5 || unused[0] != 0xa5 || unused[65] != 0xa5) {
   Genode::error("Windows source RNG cancellation payload/canary failed"); return;
  }
  Genode::log("RNG_WINDOWS CHECKSUM bytes=576 fnv64=", Genode::Hex(expected.checksum));
  if (expected.checksum != 0xa7001fd839da826full) {
   Genode::error("Windows source RNG whole-payload checksum failed"); return;
  }
  if (!submit(cancelled, unused + 1, 64) || cancelled.phase != DRIFT_WIN_RNG_ACTIVE ||
      profile.active != &cancelled || !profile.transfer.active ||
      drift_windows_viorng_cancel(&profile, &cancelled) ||
      cancelled.status != DRIFT_WIN_RNG_CANCELLED || cancelled.completions != 1 || notifications != 5) {
   Genode::error("Windows source RNG active cancellation failed"); return;
  }
  int const closed = drift_windows_viorng_close(&profile);
  Genode::log("RNG_WINDOWS CLOSE status=", closed, " transport=", profile.transport.state,
              " phase=", profile.phase, " live=", resources.live,
              " allocated=", resources.allocations, " released=", resources.releases,
              " notifications=", notifications);
  if (closed || profile.phase || profile.transport.state || resources.live || resources.error || callback_error || notifications != 5) {
   Genode::error("Windows source RNG close failed; retaining resources"); return;
  }
  bool cancelled_untouched = unused[0] == 0xa5 && unused[65] == 0xa5 &&
                             queued.information == 0 && cancelled.information == 0 &&
                             queued.completions == 1 && cancelled.completions == 1;
  for (unsigned i = 1; i <= 64; ++i) cancelled_untouched &= unused[i] == 0;
  if (!cancelled_untouched) {
   Genode::error("Windows source RNG cancelled output changed during close"); return;
  }
  if (!resources.release_device()) return;
  Genode::log("DRIFT_GENODE_RNG_WINDOWS PASS bytes=576 exact_prefix=576 queued_cancel=1 active_cancel=1 notifications=5 mappings=0");
 }
};
}
void Component::construct(Genode::Env &env) { static Main main(env); }
