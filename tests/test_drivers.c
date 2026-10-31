/* Protocol golden tests: real capsules, real Nelua VM, simulated transport.
 * SPDX-License-Identifier: MIT */
#include "drift.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); \
} } while (0)

typedef struct module {
    uint8_t bytes[32 + DRIFT_CODE_MAX*8 + DRIFT_MEMORY_MAX];
    uint32_t size;
} module;

typedef struct fixture {
    drift_vm vm;
    uint32_t ticket;
} fixture;

typedef struct event { uint32_t tag, x, y, z; } event;
static uint64_t instance = 100;
static unsigned cases;

static void read_module(module *m, const char *path)
{
    FILE *file = fopen(path, "rb");
    CHECK(file != NULL);
    m->size = (uint32_t)fread(m->bytes, 1, sizeof(m->bytes), file);
    CHECK(!ferror(file) && fgetc(file) == EOF);
    CHECK(fclose(file) == 0);
    CHECK(m->size >= 32);
}

static void fresh(fixture *f, module *m, uint32_t bus)
{
    memset(f, 0xa5, sizeof(*f));
    f->ticket = 0;
    CHECK(drift_load(&f->vm, m->bytes, m->size,
                    DRIFT_CAP_XFER | DRIFT_CAP_EMIT, ++instance) == DRIFT_READY);
    CHECK(f->vm.bus == bus && f->vm.caps == (DRIFT_CAP_XFER | DRIFT_CAP_EMIT));
    ++cases;
}

/* Small quanta deliberately split protocol loops and state updates. A pending
 * effect must remain stable regardless of further run calls or their budget. */
static int32_t advance(fixture *f)
{
    int32_t state;
    unsigned n = 0;
    if (f->vm.state == DRIFT_EFFECT) {
        CHECK(f->vm.effect.ticket == f->ticket);
        return DRIFT_EFFECT;
    }
    do {
        state = drift_run(&f->vm, 7);
        CHECK(++n < 10000);
    } while (state == DRIFT_QUOTA);
    CHECK(state == DRIFT_EFFECT || state == DRIFT_DONE);
    if (state == DRIFT_EFFECT) {
        drift_effect e = f->vm.effect;
        uint64_t steps = f->vm.steps;
        CHECK(e.instance == f->vm.instance && e.ticket == ++f->ticket);
        CHECK(drift_run(&f->vm, 0) == DRIFT_EFFECT);
        CHECK(drift_run(&f->vm, 1000) == DRIFT_EFFECT);
        CHECK(f->vm.steps == steps);
        CHECK(f->vm.effect.instance == e.instance && f->vm.effect.ticket == e.ticket);
        CHECK(f->vm.effect.kind == e.kind && f->vm.effect.tx == e.tx);
        CHECK(f->vm.effect.tx_len == e.tx_len && f->vm.effect.rx == e.rx);
        CHECK(f->vm.effect.rx_len == e.rx_len && f->vm.effect.tag == e.tag);
        CHECK(f->vm.effect.x == e.x && f->vm.effect.y == e.y && f->vm.effect.z == e.z);
    }
    return state;
}

static void complete(fixture *f, int32_t status, const uint8_t *data, uint32_t size)
{
    uint8_t copy[DRIFT_MEMORY_MAX];
    CHECK(size <= sizeof(copy));
    if (size) memcpy(copy, data, size);
    CHECK(drift_complete(&f->vm, f->vm.instance, f->ticket, status,
                         size ? copy : NULL, size) == DRIFT_READY);
    CHECK(f->vm.r[0] == (uint32_t)status && f->vm.r[1] == size);
    /* The VM must have copied the completion; this host buffer is ephemeral. */
    memset(copy, 0xcc, size);
}

static void request(fixture *f, const uint8_t *tx, uint32_t tx_len, uint32_t rx_len)
{
    CHECK(advance(f) == DRIFT_EFFECT);
    CHECK(f->vm.effect.kind == DRIFT_XFER);
    CHECK(f->vm.effect.tx_len == tx_len && f->vm.effect.rx_len == rx_len);
    CHECK(f->vm.effect.tag == 0 && f->vm.effect.x == 0);
    CHECK(f->vm.effect.y == 0 && f->vm.effect.z == 0);
    CHECK(f->vm.effect.tx <= f->vm.memory_len);
    CHECK(tx_len <= f->vm.memory_len - f->vm.effect.tx);
    CHECK(f->vm.effect.rx <= f->vm.memory_len);
    CHECK(rx_len <= f->vm.memory_len - f->vm.effect.rx);
    if (tx_len) CHECK(memcmp(f->vm.memory + f->vm.effect.tx, tx, tx_len) == 0);
}

static void expect_event(fixture *f, event expected, int32_t sink_status)
{
    CHECK(advance(f) == DRIFT_EFFECT);
    CHECK(f->vm.effect.kind == DRIFT_EMIT);
    CHECK(f->vm.effect.tx_len == 0 && f->vm.effect.rx_len == 0);
    CHECK(f->vm.effect.tag == expected.tag && f->vm.effect.x == expected.x);
    CHECK(f->vm.effect.y == expected.y && f->vm.effect.z == expected.z);
    complete(f, sink_status, NULL, 0);
}

static void done(fixture *f, uint32_t result)
{
    uint64_t steps;
    CHECK(advance(f) == DRIFT_DONE);
    CHECK(f->vm.r[0] == result);
    steps = f->vm.steps;
    CHECK(drift_run(&f->vm, 1000) == DRIFT_DONE && f->vm.steps == steps);
}

static void jedec_tests(module *m)
{
    fixture f;
    const uint8_t command[] = {0x9f};
    const uint8_t id[][3] = {{0xef, 0x40, 0x18}, {0x20, 0xba, 0x19}};
    const uint8_t absent[][3] = {{0, 0, 0}, {0xff, 0xff, 0xff}};
    unsigned i;
    for (i = 0; i < 2; ++i) {
        fresh(&f, m, DRIFT_BUS_SPI);
        request(&f, command, 1, 3);
        complete(&f, 0, id[i], 3);
        expect_event(&f, (event){2, id[i][0], id[i][1], id[i][2]}, 0);
        done(&f, 0);
    }
    for (i = 0; i < 2; ++i) {
        fresh(&f, m, DRIFT_BUS_SPI);
        request(&f, command, 1, 3);
        complete(&f, 0, absent[i], 3);
        done(&f, 2);
    }
    for (i = 0; i < 3; ++i) {
        fresh(&f, m, DRIFT_BUS_SPI);
        request(&f, command, 1, 3);
        complete(&f, 0, id[0], i);
        done(&f, 1);
    }
    fresh(&f, m, DRIFT_BUS_SPI);
    request(&f, command, 1, 3);
    complete(&f, -19, NULL, 0);
    done(&f, (uint32_t)-19);

    fresh(&f, m, DRIFT_BUS_SPI);
    request(&f, command, 1, 3);
    complete(&f, 0, id[0], 3);
    expect_event(&f, (event){2, 0xef, 0x40, 0x18}, -5);
    done(&f, (uint32_t)-5);
}

static void report(fixture *f, const uint8_t data[8], const event *events, size_t n)
{
    size_t i;
    request(f, NULL, 0, 8);
    complete(f, 0, data, 8);
    for (i = 0; i < n; ++i) expect_event(f, events[i], 0);
    /* No unlisted event may hide between reports. Leave the next read pending. */
    request(f, NULL, 0, 8);
}

static void keyboard_stop(fixture *f)
{
    request(f, NULL, 0, 8);
    complete(f, -19, NULL, 0);
    done(f, (uint32_t)-19);
}

static void keyboard_sets(module *m)
{
    fixture f;
    const uint8_t empty[8] = {0};
    const uint8_t a[8] = {0, 0, 4};
    const uint8_t ab[8] = {0, 0, 4, 5};
    const uint8_t ba[8] = {0, 0, 5, 4};
    const uint8_t bba[8] = {0, 0, 5, 5, 4};
    const uint8_t b[8] = {0, 0, 5};
    const uint8_t cc[8] = {0, 0, 6, 6};
    const event a_down[] = {{1, 4, 1, 0}};
    const event b_down[] = {{1, 5, 1, 0}};
    const event a_up[] = {{1, 4, 0, 0}};
    const event b_up_c_down[] = {{1, 5, 0, 0}, {1, 6, 1, 0}};
    const event c_up[] = {{1, 6, 0, 0}};
    fresh(&f, m, DRIFT_BUS_STREAM);
    report(&f, empty, NULL, 0);
    report(&f, a, a_down, 1);
    report(&f, a, NULL, 0);
    report(&f, ab, b_down, 1);
    report(&f, ba, NULL, 0);
    report(&f, bba, NULL, 0);
    report(&f, b, a_up, 1);
    report(&f, cc, b_up_c_down, 2);
    report(&f, empty, c_up, 1);
    keyboard_stop(&f);
}

static void keyboard_rollover(module *m)
{
    fixture f;
    const uint8_t a[8] = {0, 0, 4};
    const uint8_t empty[8] = {0};
    const uint8_t error[4][8] = {
        {0xff, 0, 1, 1, 1, 1, 1, 1}, /* ErrorRollOver */
        {0xff, 0, 2, 2, 2, 2, 2, 2}, /* POSTFail */
        {0xff, 0, 3, 3, 3, 3, 3, 3}, /* ErrorUndefined */
        {0xff, 0, 5, 0, 0, 1}        /* Even one error slot invalidates it. */
    };
    const event down[] = {{1, 4, 1, 0}}, up[] = {{1, 4, 0, 0}};
    unsigned i;
    fresh(&f, m, DRIFT_BUS_STREAM);
    report(&f, a, down, 1);
    for (i = 0; i < 4; ++i) {
        report(&f, error[i], NULL, 0);
        report(&f, a, NULL, 0);
    }
    report(&f, empty, up, 1);
    keyboard_stop(&f);
}

static void keyboard_modifiers(module *m)
{
    fixture f;
    const uint8_t all[8] = {0xff}, empty[8] = {0};
    const uint8_t shift_a[8] = {2, 0, 4}, alt_b[8] = {4, 0, 5};
    const event shifted[] = {{1, 225, 1, 2}, {1, 4, 1, 2}};
    const event altered[] = {{1, 225, 0, 4}, {1, 226, 1, 4},
                             {1, 4, 0, 4}, {1, 5, 1, 4}};
    const event released[] = {{1, 226, 0, 0}, {1, 5, 0, 0}};
    event down[8], up[8];
    unsigned i;
    for (i = 0; i < 8; ++i) {
        down[i] = (event){1, 224+i, 1, 0xff};
        up[i] = (event){1, 224+i, 0, 0};
    }
    fresh(&f, m, DRIFT_BUS_STREAM);
    report(&f, all, down, 8);
    report(&f, all, NULL, 0);
    report(&f, empty, up, 8);
    report(&f, shift_a, shifted, 2);
    report(&f, alt_b, altered, 4);
    report(&f, empty, released, 2);
    keyboard_stop(&f);
}

static void keyboard_errors(module *m)
{
    fixture f;
    const uint8_t report_bytes[8] = {2, 0, 4};
    unsigned i;
    /* Every short length is a protocol error, including zero-length success. */
    for (i = 0; i < 8; ++i) {
        fresh(&f, m, DRIFT_BUS_STREAM);
        request(&f, NULL, 0, 8);
        complete(&f, 0, report_bytes, i);
        done(&f, 1);
    }
    fresh(&f, m, DRIFT_BUS_STREAM);
    keyboard_stop(&f);

    fresh(&f, m, DRIFT_BUS_STREAM);
    request(&f, NULL, 0, 8);
    complete(&f, 0, report_bytes, 8);
    expect_event(&f, (event){1, 225, 1, 2}, -5);
    done(&f, (uint32_t)-5); /* No key event after a rejected modifier event. */
}

int main(int argc, char **argv)
{
    module jedec, keyboard;
    CHECK(argc == 3);
    read_module(&jedec, argv[1]);
    read_module(&keyboard, argv[2]);
    jedec_tests(&jedec);
    keyboard_sets(&keyboard);
    keyboard_rollover(&keyboard);
    keyboard_modifiers(&keyboard);
    keyboard_errors(&keyboard);
    printf("drivers: %u protocol scenarios passed\n", cases);
    return 0;
}
