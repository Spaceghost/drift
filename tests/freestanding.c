/* Same module, no libc, two host word sizes. SPDX-License-Identifier: MIT */
#include "drift.h"

__asm__(".pushsection .rodata\n"
        ".global test_module, test_module_end\n"
        "test_module:\n.incbin \"modules/jedec.drv\"\n"
        "test_module_end:\n.popsection\n");
extern uint8_t test_module[], test_module_end[];
static drift_vm vm;

static int test(void)
{
    uint8_t id[3] = {0xef, 0x40, 0x18};
    if (drift_load(&vm, test_module, (uint32_t)(test_module_end-test_module), 3, 1)) return 1;
    if (drift_run(&vm, 1000) != DRIFT_EFFECT) return 2;
    if (vm.bus != DRIFT_BUS_SPI || vm.effect.kind != DRIFT_XFER ||
        vm.effect.tx_len != 1 || vm.effect.rx_len != 3 ||
        vm.memory[vm.effect.tx] != 0x9f) return 3;
    if (drift_complete(&vm, 1, vm.effect.ticket, 0, id, 3)) return 4;
    if (drift_run(&vm, 1000) != DRIFT_EFFECT) return 5;
    if (vm.effect.kind != DRIFT_EMIT || vm.effect.tag != 2 ||
        vm.effect.x != 0xef || vm.effect.y != 0x40 || vm.effect.z != 0x18) return 6;
    if (drift_complete(&vm, 1, vm.effect.ticket, 0, 0, 0)) return 7;
    if (drift_run(&vm, 1000) != DRIFT_DONE || vm.r[0] != 0) return 8;
    return 0;
}

void _start(void)
{
    int result = test();
#if defined(__x86_64__)
    __asm__ volatile("syscall" : : "a"(60), "D"(result) : "rcx", "r11", "memory");
#elif defined(__i386__)
    __asm__ volatile("int $0x80" : : "a"(1), "b"(result) : "memory");
#else
#error This freestanding execution test uses the x86 Linux exit ABI.
#endif
    __builtin_unreachable();
}
