/* SPDX-License-Identifier: MIT
 * Genode/x86 resources only. No Virtio registers, queues, or class behavior.
 * All calls run serially in the component owner domain. */
#ifndef DRIFT_GENODE_RNG_RESOURCES_H
#define DRIFT_GENODE_RNG_RESOURCES_H
#include <base/attached_dataspace.h>
#include <base/log.h>
#include <base/sleep.h>
#include <dma_session/buffer.h>
#include <io_port_session/connection.h>
#include <io_mem_session/client.h>
#include <platform_session/connection.h>
#include <timer_session/connection.h>
#include <drift_hw.h>
#include <drift_pci.h>

struct Resources
{
 static constexpr unsigned BDF = 3u << 3;
 static constexpr unsigned SLOTS = 32;
 struct Region {
  Genode::Constructible<Genode::Attached_dataspace> ds {};
  Genode::size_t start = 0, size = 0;
 } region[6] {};
 struct Allocation {
  Genode::Constructible<Dma::Buffer> buffer {};
  drift_dma mapping {};
 } allocation[SLOTS] {};
 Genode::Env &env;
 Platform::Connection platform;
 Genode::Constructible<Dma::Connection> dma {};
 Genode::Constructible<Genode::Io_port_connection> config {};
 Timer::Connection timer;
 Genode::Capability<Platform::Device_interface> device {};
 uint64_t next_token = 1;
 unsigned live = 0, allocations = 0, releases = 0, syncs = 0;
 bool error = false;
 drift_hw hw {};
 drift_pci pci {};

 explicit Resources(Genode::Env &e) : env(e), platform(e), timer(e)
 {
  hw = {this, read_cb, write_cb, dma_cb, release_cb, sync_cb, time_cb, wait_cb};
  pci = {this, pci_read_cb, pci_write_cb};
 }
 ~Resources()
 {
  /* Explicit successful shutdown is required. In particular a failed reset
   * must not run Dma::Buffer destructors or close its managing session. The
   * component's owner thread stops with all members still alive. */
  if (live || device.valid()) {
   Genode::error("resource teardown attempted before quiescence; quarantined");
   Genode::sleep_forever();
  }
 }

 bool acquire()
 {
  platform.acquire_device(Platform::Session::Device_name("00:03.0")).with_result(
   [&] (auto cap) { device = cap; }, [&] (auto) { fail("Platform acquisition allocation error"); });
  if (!device.valid()) return false;
  config.construct(env, 0xcf8, 8);
  dma.construct(env);
  if (config_read(BDF, 0) != 0x10441af4u) return fail("PCI identity");
  /* Fixed QEMU topology: values independently measured by the probe and QMP.
   * The platform driver owns ECAM and BAR programming; this provider never
   * assigns BARs. CF8/CFC is exclusively owned and callbacks restrict BDF. */
  if (config_read(0, 0x60) != 0xb0000001u || config_read(0, 0x64) != 0)
   return fail("PCIEXBAR differs from measured topology");
  if ((config_read(BDF, 0x14) & ~15u) != 0xfebf1000u ||
      (config_read(BDF, 0x20) & ~15u) != 0xfe000000u || config_read(BDF, 0x24))
   return fail("BARs differ from measured topology");
  unsigned index = 0;
  platform.with_node([&] (Genode::Node const &nodes) {
   nodes.for_each_sub_node("device", [&] (Genode::Node const &node) {
    if (node.attribute_value("name", Genode::String<32>()) != "00:03.0") return;
    node.for_each_sub_node("io_mem", [&] (Genode::Node const &mem) {
     unsigned const bar = mem.attribute_value<unsigned>("pci_bar", 6);
     uint64_t const size = mem.attribute_value<uint64_t>("size", 0);
     if (bar >= 6 || !size || size > UINT32_MAX || region[bar].ds.constructed()) {
      fail("invalid Platform BAR resource"); return;
     }
     Platform::Device_interface::Range range;
     auto cap = device.call<Platform::Device_interface::Rpc_io_mem>(index++, range);
     if (!cap.valid() || range.size != size) { fail("BAR mapping denied"); return; }
     Genode::Io_mem_session_client io(cap);
     region[bar].ds.construct(env.rm(), io.dataspace());
     if (range.start > region[bar].ds->size() ||
         range.size > region[bar].ds->size() - range.start) {
      fail("BAR exceeds attached dataspace"); return;
     }
     region[bar].start = range.start;
     region[bar].size = range.size;
     Genode::log("RNG_RESOURCE BAR index=", bar, " bytes=", size,
                 " cpu=", Genode::Hex((Genode::addr_t)address(bar)));
    });
   });
  });
  return !error && region[4].ds.constructed();
 }

 bool release_device()
 {
  if (live) return fail("retaining device and DMA: mappings still live");
  for (auto &r : region) { r.ds.destruct(); r.size = 0; }
  if (device.valid()) platform.release_device(device);
  device = {};
  if (config.constructed() && (config_read(BDF, 4) & 7))
   return fail("platform release did not disable PCI decoding and bus mastering");
  config.destruct();
  dma.destruct();
  return !error;
 }
 bool fail(char const *why) { error = true; Genode::error(why); return false; }
 uint8_t *address(unsigned r) {
  return region[r].ds->local_addr<uint8_t>() + region[r].start;
 }
 bool valid(unsigned r, unsigned off, unsigned width) {
  return r < 6 && region[r].ds.constructed() &&
         (width == 1 || width == 2 || width == 4 || width == 8) &&
         off % width == 0 && off <= region[r].size && width <= region[r].size-off;
 }
 uint32_t config_read(unsigned bdf, unsigned off) {
  config->outl(0xcf8, 0x80000000u | bdf << 8 | off);
  return config->inl(0xcfc);
 }
 static uint32_t pci_read_cb(void *p, uint32_t bdf, uint32_t off) {
  auto &r = *static_cast<Resources *>(p);
  if (!r.config.constructed() || bdf != BDF || off > 252 || (off & 3)) {
   r.fail("invalid PCI config read"); return UINT32_MAX;
  }
  return r.config_read(bdf, off);
 }
 static void pci_write_cb(void *p, uint32_t bdf, uint32_t off, uint32_t value) {
  auto &r = *static_cast<Resources *>(p);
  /* Framework may request command bits 0..2, never reassign resources. */
  if (!r.config.constructed() || bdf != BDF || off != 4) {
   r.fail("invalid PCI config write"); return;
  }
  uint32_t const old = r.config_read(bdf, off);
  if ((value & 0xffff0000u) || ((value ^ old) & 0xfff8u)) {
   r.fail("PCI command exceeds resource authority"); return;
  }
  r.config->outl(0xcf8, 0x80000000u | bdf << 8 | off);
  r.config->outl(0xcfc, value);
 }
 static uint64_t read_cb(void *p, uint32_t n, uint32_t off, uint32_t width) {
  auto &r = *static_cast<Resources *>(p);
  if (!r.valid(n, off, width)) { r.fail("invalid MMIO read"); return UINT64_MAX; }
  volatile uint8_t *a = r.address(n) + off;
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  uint64_t value = width == 1 ? *a : width == 2 ? *reinterpret_cast<volatile uint16_t *>(a) :
                   width == 4 ? *reinterpret_cast<volatile uint32_t *>(a) :
                                *reinterpret_cast<volatile uint64_t *>(a);
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  return value;
 }
 static void write_cb(void *p, uint32_t n, uint32_t off, uint32_t width, uint64_t value) {
  auto &r = *static_cast<Resources *>(p);
  if (!r.valid(n, off, width)) { r.fail("invalid MMIO write"); return; }
  volatile uint8_t *a = r.address(n) + off;
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  if (width == 1) *a = static_cast<uint8_t>(value);
  if (width == 2) *reinterpret_cast<volatile uint16_t *>(a) = static_cast<uint16_t>(value);
  if (width == 4) *reinterpret_cast<volatile uint32_t *>(a) = static_cast<uint32_t>(value);
  if (width == 8) *reinterpret_cast<volatile uint64_t *>(a) = value;
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
 }
 Allocation *lookup(drift_dma const *mapping) {
  if (!mapping) return nullptr;
  for (auto &a : allocation)
   if (a.buffer.constructed() && a.mapping.token == mapping->token &&
       a.mapping.data == mapping->data && a.mapping.address == mapping->address &&
       a.mapping.size == mapping->size) return &a;
  return nullptr;
 }
 static int32_t dma_cb(void *p, uint32_t size, uint32_t align, drift_dma *out) {
  auto &r = *static_cast<Resources *>(p);
  if (!out || !r.device.valid() || !r.dma.constructed() || !size || size > 1048576 ||
      !align || (align & (align-1)) || align > 4096 || !r.next_token) return -1;
  for (auto &a : r.allocation) {
   if (a.buffer.constructed()) continue;
   try { a.buffer.construct(*r.dma, size, Genode::CACHED); }
   catch (...) { return -1; }
   auto *cpu = a.buffer->local_addr<uint8_t>();
   uint64_t const bus = a.buffer->bus_addr();
   if (!cpu || !bus || ((Genode::addr_t)cpu & (align-1)) || (bus & (align-1))) {
    a.buffer.destruct(); return -1;
   }
   a.mapping = {cpu, bus, size, r.next_token++};
   *out = a.mapping;
   Genode::bzero(cpu, size);
   __atomic_thread_fence(__ATOMIC_SEQ_CST);
   ++r.live; ++r.allocations;
   Genode::log("RNG_RESOURCE DMA token=", out->token, " bytes=", size,
               " cpu=", Genode::Hex((Genode::addr_t)cpu), " bus=", Genode::Hex(bus));
   return 0;
  }
  return -1;
 }
 static void release_cb(void *p, drift_dma *mapping) {
  auto &r = *static_cast<Resources *>(p);
  auto *a = r.lookup(mapping);
  if (!a) { r.fail("release of unowned DMA mapping"); return; }
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  a->buffer.destruct(); a->mapping = {}; *mapping = {};
  --r.live; ++r.releases;
 }
 static void sync_cb(void *p, drift_dma *mapping, uint32_t off, uint32_t length, uint32_t direction) {
  auto &r = *static_cast<Resources *>(p);
  if (!r.lookup(mapping) || off > mapping->size || length > mapping->size-off ||
      direction < DRIFT_DMA_TO_DEVICE || direction > DRIFT_DMA_BIDIRECTIONAL) {
   r.fail("invalid DMA synchronization"); return;
  }
  /* x86/QEMU DMA is cache coherent. This full compiler+CPU fence covers
   * publication and acquisition; no streaming-cache fiction is involved. */
  __atomic_thread_fence(__ATOMIC_SEQ_CST); ++r.syncs;
 }
 static uint64_t time_cb(void *p) { return static_cast<Resources *>(p)->timer.elapsed_us(); }
 static void wait_cb(void *p, uint32_t us) { static_cast<Resources *>(p)->timer.usleep(us); }
};
#endif
