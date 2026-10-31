/* A complete host for the walkthrough. The SPI endpoint is simulated.
 * SPDX-License-Identifier: MIT */
#include "drift.h"
#include <inttypes.h>
#include <stdio.h>

static drift_vm vm;
static uint8_t image[32 + DRIFT_CODE_MAX * 8 + DRIFT_MEMORY_MAX + 1];

static int fail(const char *message)
{
    fprintf(stderr, "identify: %s\n", message);
    return 1;
}

int main(int argc, char **argv)
{
    FILE *file;
    size_t size;
    int bad;
    int32_t state;
    if (argc != 2) return fail("usage: identify MODULE.drv");
    file = fopen(argv[1], "rb");
    if (!file) return fail("cannot open module");
    size = fread(image, 1, sizeof(image), file);
    bad = ferror(file);
    if (fclose(file)) bad = 1;
    if (bad || size == sizeof(image)) return fail("cannot read module");

    /* This process loads once. A host that reloads must use a fresh ID. */
    state = drift_load(&vm, image, (uint32_t)size,
                       DRIFT_CAP_XFER | DRIFT_CAP_EMIT, 1);
    if (state != DRIFT_READY) return fail("module rejected");
    if (vm.bus != DRIFT_BUS_SPI) return fail("this endpoint requires SPI");

    /* Eight instructions per turn; at most 1,024 instructions in this demo. */
    while (vm.steps < 1024) {
        uint8_t reply[] = {0xef, 0x40, 0x18};
        uint32_t received = 0;
        drift_effect *effect = &vm.effect;
        state = drift_run(&vm, (uint32_t)(1024 - vm.steps < 8 ? 1024 - vm.steps : 8));
        if (state == DRIFT_QUOTA) continue;
        if (state == DRIFT_DONE) {
            if (fclose(stdout)) return fail("cannot close output");
            return vm.r[0] ? fail("driver reported failure") : 0;
        }
        if (state != DRIFT_EFFECT) return fail("machine trapped");

        if (effect->kind == DRIFT_XFER) {
            /* A real port replaces this reply with one SPI transaction.
             * The endpoint permits only read-only JEDEC identification. */
            if (effect->tx_len != 1 || effect->rx_len != 3 ||
                vm.memory[effect->tx] != 0x9f)
                return fail("transaction refused by endpoint");
            received = sizeof(reply);
            if (puts("transfer 9f -> ef 40 18") == EOF || fflush(stdout))
                return fail("cannot write output");
        } else if (effect->kind == DRIFT_EMIT) {
            if (printf("event %" PRIu32 ": %02" PRIx32 " %02" PRIx32
                       " %02" PRIx32 "\n", effect->tag,
                       effect->x, effect->y, effect->z) < 0 || fflush(stdout))
                return fail("cannot deliver event");
        } else return fail("unsupported effect");

        /* Acknowledge exactly this request, after the operation succeeds.
         * Completion copies reply bytes; the local array can then expire. */
        state = drift_complete(&vm, effect->instance, effect->ticket,
                               0, reply, received);
        if (state != DRIFT_READY) return fail("completion rejected");
    }
    return fail("instruction budget exhausted");
}
