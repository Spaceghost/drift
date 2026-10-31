#include <base/component.h>
#include <base/log.h>
#include <io_port_session/connection.h>

namespace {
struct Main {
 Genode::Io_port_connection ports;
 unsigned read(unsigned bdf, unsigned off) {
  ports.outl(0xcf8, 0x80000000u | (bdf << 8) | (off & 0xfc));
  return ports.inl(0xcfc);
 }
 Main(Genode::Env &env) : ports(env, 0xcf8, 8) {
  Genode::log("PCI_PROBE host=", Genode::Hex(read(0, 0)),
              " pciexbar_lo=", Genode::Hex(read(0, 0x60)),
              " pciexbar_hi=", Genode::Hex(read(0, 0x64)));
  unsigned const bdf = 3u << 3;
  Genode::log("PCI_PROBE bdf=", Genode::Hex(bdf),
              " identity=", Genode::Hex(read(bdf, 0)),
              " class=", Genode::Hex(read(bdf, 8)),
              " subsystem=", Genode::Hex(read(bdf, 0x2c)));
  for (unsigned i = 0; i < 6; ++i)
   Genode::log("PCI_PROBE bar=", i, " value=", Genode::Hex(read(bdf, 0x10 + 4*i)));
  if (read(0, 0) != 0x29c08086 || read(bdf, 0) != 0x10441af4) {
   Genode::error("unexpected QEMU PCI identity");
   return;
  }
  Genode::log("DRIFT_GENODE_RNG_PROBE PASS");
 }
};
}
void Component::construct(Genode::Env &env) { static Main main(env); }
