#include <base/component.h>
#include <resources.h>
#include <fixture.h>
#include <linux_case.h>
#include <shootout.h>
void Component::construct(Genode::Env &env) { static Shootout main(env); }
