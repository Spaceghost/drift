/* Minimal tone session and matched benchmark interface. SPDX-License-Identifier: MIT */
#ifndef DRIFT_GENODE_SPEAKER_H
#define DRIFT_GENODE_SPEAKER_H
#include <session/session.h>
#include <base/connection.h>
#include <base/rpc_client.h>
namespace Speaker {
struct Session : Genode::Session {
 static const char *service_name() { return "Drift_speaker"; }
 static constexpr unsigned CAP_QUOTA=2;
 virtual int tone(int)=0;
 virtual int tone_reference(int)=0;
 virtual unsigned counter(unsigned)=0;
 virtual int control()=0;
 virtual int suspend()=0;
 virtual int close()=0;
 GENODE_RPC(Rpc_tone,int,tone,int);
 GENODE_RPC(Rpc_reference,int,tone_reference,int);
 GENODE_RPC(Rpc_counter,unsigned,counter,unsigned);
 GENODE_RPC(Rpc_control,int,control);
 GENODE_RPC(Rpc_suspend,int,suspend);
 GENODE_RPC(Rpc_close,int,close);
 GENODE_RPC_INTERFACE(Rpc_tone,Rpc_reference,Rpc_counter,Rpc_control,Rpc_suspend,Rpc_close);
};
struct Connection : Genode::Connection<Session>, Genode::Rpc_client<Session> {
 Connection(Genode::Env &env) : Genode::Connection<Session>(env,Label(),Genode::Ram_quota{8192},Args(" ")), Genode::Rpc_client<Session>(cap()) { }
 int tone(int hz) override { return call<Session::Rpc_tone>(hz); }
 int tone_reference(int hz) override { return call<Session::Rpc_reference>(hz); }
 unsigned counter(unsigned kind) override { return call<Session::Rpc_counter>(kind); }
 int control() override { return call<Session::Rpc_control>(); }
 int suspend() override { return call<Session::Rpc_suspend>(); }
 int close() override { return call<Session::Rpc_close>(); }
};
}
#endif
