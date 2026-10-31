/* Portable flash capsule tests against an independent simulated SPI NOR.
 * SPDX-License-Identifier: MIT */
#include "drift.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned cases;
static uint64_t checks, instance = 2000;
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); \
} } while (0)

static uint8_t image[32 + DRIFT_CODE_MAX * 8 + DRIFT_MEMORY_MAX];
static uint32_t image_size;
typedef struct fixture { drift_vm vm; uint32_t ticket; } fixture;

static void fresh(fixture *f)
{
    memset(f, 0, sizeof(*f));
    CHECK(drift_load(&f->vm, image, image_size,
                    DRIFT_CAP_XFER | DRIFT_CAP_EMIT, ++instance) == DRIFT_READY);
    CHECK(f->vm.bus == DRIFT_BUS_SPI);
    ++cases;
}

/* Tiny fuel slices split comparison loops. Waiting must never repeat I/O. */
static int32_t advance(fixture *f)
{
    int32_t state;
    unsigned turns = 0;
    do {
        state = drift_run(&f->vm, 7);
        CHECK(++turns < 1000);
    } while (state == DRIFT_QUOTA);
    CHECK(state == DRIFT_EFFECT || state == DRIFT_DONE);
    if (state == DRIFT_EFFECT) {
        drift_effect effect = f->vm.effect;
        uint64_t steps = f->vm.steps;
        CHECK(effect.instance == f->vm.instance && effect.ticket == ++f->ticket);
        CHECK(drift_run(&f->vm, 1000) == DRIFT_EFFECT);
        CHECK(f->vm.steps == steps);
        CHECK(memcmp(&effect, &f->vm.effect, sizeof(effect)) == 0);
    }
    return state;
}

static void complete(fixture *f, int32_t status, const uint8_t *bytes, uint32_t n)
{
    uint8_t copy[256];
    CHECK(n <= sizeof(copy));
    if (n) memcpy(copy, bytes, n);
    CHECK(drift_complete(&f->vm, f->vm.instance, f->ticket,
                        status, n ? copy : NULL, n) == DRIFT_READY);
    memset(copy, 0xcc, sizeof(copy));
}

static void done(fixture *f, int32_t result)
{
    uint64_t steps;
    CHECK(advance(f) == DRIFT_DONE);
    CHECK(f->vm.r[0] == (uint32_t)result);
    steps = f->vm.steps;
    CHECK(drift_run(&f->vm, 1000) == DRIFT_DONE && f->vm.steps == steps);
}

static void request(fixture *f, const uint8_t *tx, uint32_t tx_len,
                    uint32_t rx_len, uint32_t rx_offset)
{
    CHECK(advance(f) == DRIFT_EFFECT);
    CHECK(f->vm.effect.kind == DRIFT_XFER);
    CHECK(f->vm.effect.tx_len == tx_len && f->vm.effect.rx_len == rx_len);
    CHECK(f->vm.effect.rx == rx_offset);
    CHECK(f->vm.effect.tx <= f->vm.memory_len);
    CHECK(tx_len <= f->vm.memory_len - f->vm.effect.tx);
    CHECK(memcmp(f->vm.memory + f->vm.effect.tx, tx, tx_len) == 0);
}

static void identify(fixture *f, const uint8_t id[3], uint32_t n, int32_t status)
{
    const uint8_t command = 0x9f;
    request(f, &command, 1, 3, 16);
    complete(f, status, id, n);
}

static void id_event(fixture *f, const uint8_t id[3], int32_t status)
{
    CHECK(advance(f) == DRIFT_EFFECT);
    CHECK(f->vm.effect.kind == DRIFT_EMIT && f->vm.effect.tag == 2);
    CHECK(f->vm.effect.x == id[0] && f->vm.effect.y == id[1]);
    CHECK(f->vm.effect.z == id[2]);
    complete(f, status, NULL, 0);
}

static uint8_t pattern(uint32_t offset)
{
    return (uint8_t)(offset ^ (offset >> 8) ^ (offset >> 16) ^ 0x5a);
}

static void read_request(fixture *f, uint32_t offset, unsigned pass)
{
    const uint8_t command[] = {0x03, (uint8_t)(offset >> 16),
                              (uint8_t)(offset >> 8), (uint8_t)offset};
    request(f, command, sizeof(command), 256, pass ? 272 : 16);
}

static void data_event(fixture *f, uint32_t offset, int32_t status)
{
    uint32_t i;
    CHECK(advance(f) == DRIFT_EFFECT);
    CHECK(f->vm.effect.kind == DRIFT_EMIT && f->vm.effect.tag == 3);
    CHECK(f->vm.effect.x == offset && f->vm.effect.y == 256);
    CHECK(f->vm.effect.z <= f->vm.memory_len);
    CHECK(f->vm.effect.y <= f->vm.memory_len - f->vm.effect.z);
    for (i = 0; i < 256; ++i)
        CHECK(f->vm.memory[f->vm.effect.z + i] == pattern(offset + i));
    complete(f, status, NULL, 0);
}

static void full_backup(unsigned capacity)
{
    fixture f;
    uint8_t id[] = {0xef, 0x40, (uint8_t)capacity}, bytes[256];
    uint32_t offset, i, limit = UINT32_C(1) << capacity;
    unsigned pass;
    fresh(&f);
    identify(&f, id, 3, 0);
    id_event(&f, id, 0);
    for (offset = 0; offset < limit; offset += 256) {
        for (i = 0; i < 256; ++i) bytes[i] = pattern(offset + i);
        for (pass = 0; pass < 2; ++pass) {
            read_request(&f, offset, pass);
            complete(&f, 0, bytes, sizeof(bytes));
        }
        data_event(&f, offset, 0);
    }
    done(&f, 0);
}

static void identity_errors(void)
{
    fixture f;
    const uint8_t valid[3] = {0xef, 0x40, 16};
    const uint8_t invalid[][3] = {{0, 0, 0}, {255, 255, 255},
                                  {0, 0x40, 16}, {255, 0x40, 16}};
    const uint8_t capacity[] = {0, 15, 25, 255};
    uint8_t id[] = {0xef, 0x40, 16};
    unsigned i;
    for (i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i) {
        fresh(&f); identify(&f, invalid[i], 3, 0); done(&f, 2);
    }
    for (i = 0; i < sizeof(capacity); ++i) {
        id[2] = capacity[i];
        fresh(&f); identify(&f, id, 3, 0); done(&f, 3);
    }
    for (i = 0; i < 3; ++i) {
        fresh(&f); identify(&f, valid, i, 0); done(&f, 1);
    }
    fresh(&f); identify(&f, valid, 0, -19); done(&f, -19);
    fresh(&f); identify(&f, valid, 3, 0); id_event(&f, valid, -5); done(&f, -5);
    /* Every supported capacity reaches its first read, without allocation or
     * a size-dependent module. Abort after checking that read to bound tests. */
    for (i = 16; i <= 24; ++i) {
        id[2] = (uint8_t)i;
        fresh(&f); identify(&f, id, 3, 0); id_event(&f, id, 0);
        read_request(&f, 0, 0); complete(&f, -125, NULL, 0); done(&f, -125);
    }
}

static void read_errors(void)
{
    fixture f;
    const uint8_t id[3] = {0xef, 0x40, 16};
    const uint32_t lengths[] = {0, 1, 255};
    const unsigned mismatches[] = {0, 127, 255};
    uint8_t bytes[256];
    unsigned pass, i, j;
    for (j = 0; j < sizeof(bytes); ++j) bytes[j] = pattern(j);
    for (pass = 0; pass < 2; ++pass) {
        for (i = 0; i < sizeof(lengths)/sizeof(lengths[0]); ++i) {
            fresh(&f); identify(&f, id, 3, 0); id_event(&f, id, 0);
            read_request(&f, 0, 0);
            if (pass) {
                complete(&f, 0, bytes, 256);
                read_request(&f, 0, 1);
            }
            complete(&f, 0, bytes, lengths[i]); done(&f, 1);
        }
        fresh(&f); identify(&f, id, 3, 0); id_event(&f, id, 0);
        read_request(&f, 0, 0);
        if (pass) { complete(&f, 0, bytes, 256); read_request(&f, 0, 1); }
        complete(&f, -110, NULL, 0); done(&f, -110);
    }
    for (i = 0; i < sizeof(mismatches)/sizeof(mismatches[0]); ++i) {
        fresh(&f); identify(&f, id, 3, 0); id_event(&f, id, 0);
        read_request(&f, 0, 0); complete(&f, 0, bytes, 256);
        read_request(&f, 0, 1);
        bytes[mismatches[i]] ^= 1;
        complete(&f, 0, bytes, 256);
        bytes[mismatches[i]] ^= 1;
        done(&f, 4); /* No data event and no next read after a mismatch. */
    }
    fresh(&f); identify(&f, id, 3, 0); id_event(&f, id, 0);
    read_request(&f, 0, 0); complete(&f, 0, bytes, 256);
    read_request(&f, 0, 1); complete(&f, 0, bytes, 256);
    data_event(&f, 0, -28); done(&f, -28);
}

int main(int argc, char **argv)
{
    FILE *file;
    CHECK(argc == 2);
    file = fopen(argv[1], "rb");
    CHECK(file != NULL);
    image_size = (uint32_t)fread(image, 1, sizeof(image), file);
    CHECK(!ferror(file) && fgetc(file) == EOF);
    CHECK(fclose(file) == 0);
    full_backup(16);
    full_backup(24); /* Exercise carry into all three address bytes and the end. */
    identity_errors();
    read_errors();
    printf("flash: %u scenarios, %" PRIu64 " checks passed\n", cases, checks);
    return 0;
}
