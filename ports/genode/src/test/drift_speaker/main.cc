/* Real service/readback/lifecycle checks and paired trial samples. SPDX-License-Identifier: MIT */
#include <base/component.h>
#include <base/log.h>
#include <timer_session/connection.h>
#include <speaker.h>
struct Failed {};
static void require(bool value,const char *msg) { if(!value) { Genode::error(msg);throw Failed(); } }
void Component::construct(Genode::Env &env) {
 Timer::Connection clock(env);
 {
  Speaker::Connection c(env);
  require(c.tone(440)==0,"tone failed");require(c.control()==3,"real port enable readback failed");
  require(c.suspend()==0 && c.control()==0,"suspend/off failed");
  require(c.tone(1000)==0 && c.control()==3,"bell tone failed");
  require(c.tone(0)==0 && c.control()==0,"off failed");
  for(unsigned round=0;round<4;++round) {
   for(unsigned position=0;position<2;++position) {
    bool reference=((round+position)&1)!=0;
    unsigned writes=c.counter(0),reads=c.counter(1),delays=c.counter(2);
    auto start=clock.elapsed_us();
    for(unsigned i=0;i<100;++i) {
     require((reference?c.tone_reference(440):c.tone(440))==0,"benchmark tone failed");
     require(c.control()==3,"benchmark enable readback failed");
     require((reference?c.tone_reference(0):c.tone(0))==0,"benchmark off failed");
     require(c.control()==0,"benchmark off readback failed");
    }
    auto elapsed=clock.elapsed_us()-start;
    require(c.counter(0)-writes==500 && c.counter(1)-reads==400 && c.counter(2)-delays==500,"reference/import I/O transcript counts differ");
    Genode::log("DRIFT_GENODE_SAMPLE round=",round+1," position=",position+1," variant=",reference?"reference":"import"," cycles=100 rpc=400 writes=500 reads=400 delays=500 guest_us=",elapsed);
   }
  }
  require(c.control()==0,"final readback failed");
  require(c.close()==0,"close failed");
  require(c.tone(440)==-19,"closed driver accepted event");

 }
 clock.msleep(20);
 {
  Speaker::Connection c(env);
  require(c.tone(440)==0 && c.control()==3,"session restart failed");
  require(c.tone(0)==0 && c.control()==0,"restart stop failed");
 }
 Genode::log("Drift Linux pcspkr Genode REAL-IO PASS");env.parent().exit(0);
}
