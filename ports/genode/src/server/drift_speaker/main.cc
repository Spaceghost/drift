/* Native Genode capability backend; imported Linux source remains unchanged.
 * SPDX-License-Identifier: MIT */
#include <base/component.h>
#include <base/log.h>
#include <base/heap.h>
#include <base/mutex.h>
#include <base/rpc_server.h>
#include <io_port_session/connection.h>
#include <timer_session/connection.h>
#include <root/component.h>
#include <speaker.h>
#include "drift_linux_pcspkr.h"
struct Driver {
 Genode::Io_port_connection pit,system;
 Timer::Connection timer;
 Genode::Mutex mutex{};
 drift_linux_pcspkr state{};
 drift_linux_pcspkr_host host;
 unsigned writes=0,reads=0,waits=0;
 static int32_t read(void *p,uint16_t port) {
  Driver &d=*static_cast<Driver *>(p);
  if(port!=0x61) return -22;
  ++d.reads;return d.system.inb(port);
 }
 static int32_t write(void *p,uint16_t port,uint8_t value) {
  Driver &d=*static_cast<Driver *>(p);
  if(port==0x42||port==0x43) d.pit.outb(port,value);
  else if(port==0x61) d.system.outb(port,value);
  else return -22;
  ++d.writes;return 0;
 }
 static int32_t lock(void *p,unsigned long *flags) {
  static_cast<Driver *>(p)->mutex.acquire();*flags=0;return 0;
 }
 static int32_t unlock(void *p,unsigned long) {
  static_cast<Driver *>(p)->mutex.release();return 0;
 }
 static int32_t wait(void *p,uint32_t us) {
  Driver &d=*static_cast<Driver *>(p);d.timer.usleep(us);++d.waits;return 0;
 }
 Driver(Genode::Env &env):pit(env,0x42,2),system(env,0x61,1),timer(env),host{this,read,write,lock,unlock,wait} { }
 int open() { return drift_linux_pcspkr_open(&state,&host); }
 int tone(int hz) { return drift_linux_pcspkr_event(&state,DRIFT_PCSPKR_EV_SND,DRIFT_PCSPKR_TONE,hz); }
 int control() { Genode::Mutex::Guard guard(mutex);++reads;return system.inb(0x61)&3; }
 int reference(int hz) {
  if(state.phase!=2) return -19;
  unsigned count=(hz>20 && hz<32767)?1193182u/(unsigned)hz:0;
  unsigned long flags=0;int error=lock(this,&flags);if(error) return error;
  auto w=[&](uint16_t p,uint8_t v) { if(!error) error=write(this,p,v); };
  auto delay=[&]() { if(!error) error=wait(this,1); };
  auto r=[&]() { int v=0;if(!error) { v=read(this,0x61);if(v<0||v>255) error=v<0?v:-5; } return v; };
  if(count) { w(0x43,0xb6);delay();w(0x42,count&255);delay();w(0x42,(count>>8)&255);int v=r();delay();w(0x61,(uint8_t)(v|3));delay(); }
  else { int v=r();delay();w(0x61,(uint8_t)(v&252)); }
  int released=unlock(this,flags);return error?error:released;
 }
 int suspend() { return drift_linux_pcspkr_suspend(&state); }
 int close() { int r=drift_linux_pcspkr_close(&state);Genode::log("Drift speaker close status=",r," writes=",writes," reads=",reads," delays=",waits);return r; }
};
struct Session : Genode::Rpc_object<Speaker::Session> {
 Driver &driver;
 Session(Driver &d):driver(d) { if(driver.open()!=0) throw Genode::Service_denied(); }
 ~Session() { driver.close(); }
 int tone(int hz) override { return driver.tone(hz); }
 int tone_reference(int hz) override { return driver.reference(hz); }
 unsigned counter(unsigned kind) override { return kind==0?driver.writes:kind==1?driver.reads:kind==2?driver.waits:0; }
 int control() override { return driver.control(); }
 int suspend() override { return driver.suspend(); }
 int close() override { return driver.close(); }
};
struct Root : Genode::Root_component<Session,Genode::Single_client> {
 Driver &driver;
 Create_result _create_session(const char *) override { return *new(md_alloc()) Session(driver); }
 Root(Genode::Env &env,Genode::Allocator &a,Driver &d):Genode::Root_component<Session,Genode::Single_client>(env.ep(),a),driver(d) { }
};
struct Main {
 Genode::Env &env;
 Genode::Sliced_heap heap{env.ram(),env.rm()};
 Driver driver{env};
 Root root{env,heap,driver};
 Main(Genode::Env &e):env(e) { env.parent().announce(env.ep().manage(root));Genode::log("Drift Linux pcspkr resource-backed service ready"); }
};
void Component::construct(Genode::Env &env) { static Main main(env); }
