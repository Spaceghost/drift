/* Independent ABI tests, deliberately not using the module assembler.
 * SPDX-License-Identifier: MIT */
#include "drift.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#ifdef NDEBUG
#error Tests require assertions.
#endif

enum { IMAGE_MAX = 32 + DRIFT_CODE_MAX * 8 + DRIFT_MEMORY_MAX };
static unsigned checks;
static uint64_t instance;
static uint32_t rng = 0xd41f7001u;
#define CHECK(expr) do { ++checks; assert(expr); } while (0)

typedef struct image {
    uint8_t bytes[IMAGE_MAX + 1];
    uint32_t count, memory, data, size;
} image;

static void put32(uint8_t *p, uint32_t x)
{
    p[0] = (uint8_t)x; p[1] = (uint8_t)(x >> 8);
    p[2] = (uint8_t)(x >> 16); p[3] = (uint8_t)(x >> 24);
}

static uint32_t random32(void)
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return rng;
}

static void init(image *m, uint32_t count, uint32_t memory,
                 uint32_t data, uint32_t caps, uint32_t bus)
{
    memset(m, 0, sizeof(*m));
    memcpy(m->bytes, "DRFT", 4);
    m->count = count; m->memory = memory; m->data = data;
    m->size = 32 + count * 8 + data;
    put32(m->bytes + 4, 1); put32(m->bytes + 8, count);
    put32(m->bytes + 12, memory); put32(m->bytes + 16, data);
    put32(m->bytes + 20, caps); put32(m->bytes + 28, bus);
}

static void ins(image *m, uint32_t pc, uint8_t op, uint8_t a,
                uint8_t b, uint8_t c, uint32_t imm)
{
    uint8_t *p = m->bytes + 32 + pc * 8;
    assert(pc < m->count);
    p[0] = op; p[1] = a; p[2] = b; p[3] = c;
    put32(p + 4, imm);
}

static int32_t load(drift_vm *v, image *m)
{
    return drift_load(v, m->bytes, m->size, 7, ++instance);
}

static void bad(image *m, int32_t error)
{
    drift_vm v = {0};
    CHECK(load(&v, m) == error);
    CHECK(drift_run(&v, 100) < 0);
}

static void test_loader(void)
{
    image m;
    drift_vm v = {0};
    init(&m, 1, 8, 0, 0, 0);
    CHECK(drift_load(NULL, m.bytes, m.size, 0, ++instance) == DRIFT_EARG);
    CHECK(drift_load(&v, NULL, m.size, 0, ++instance) == DRIFT_EFORMAT);
    CHECK(drift_load(&v, m.bytes, m.size, 0, 0) == DRIFT_EFORMAT);
    CHECK(drift_run(NULL, 1) == DRIFT_EARG);
    CHECK(drift_complete(NULL, 1, 1, 0, NULL, 0) == DRIFT_EARG);
    for (uint32_t n = 0; n < m.size; ++n)
        CHECK(drift_load(&v, m.bytes, n, 0, ++instance) == DRIFT_EFORMAT);
    ++m.size; bad(&m, DRIFT_EFORMAT); --m.size;
    m.bytes[0] = 'X'; bad(&m, DRIFT_EFORMAT); m.bytes[0] = 'D';
    put32(m.bytes + 4, 2); bad(&m, DRIFT_EFORMAT); put32(m.bytes + 4, 1);
    const uint32_t offsets[] = {8, 12, 16, 20, 24, 28};
    const uint32_t invalid[] = {UINT32_MAX, UINT32_MAX, 9, 8, 1, 4};
    const uint32_t originals[] = {1, 8, 0, 0, 0, 0};
    for (unsigned i = 0; i < sizeof(offsets) / sizeof(offsets[0]); ++i) {
        put32(m.bytes + offsets[i], invalid[i]); bad(&m, DRIFT_EFORMAT);
        put32(m.bytes + offsets[i], originals[i]);
    }
    put32(m.bytes + 8, 0); bad(&m, DRIFT_EFORMAT); put32(m.bytes + 8, 1);
    put32(m.bytes + 12, 0); bad(&m, DRIFT_EFORMAT); put32(m.bytes + 12, 8);
    ins(&m, 0, 255, 0, 0, 0, 0); bad(&m, DRIFT_EOP);
    for (unsigned reg = 0; reg < 3; ++reg) {
        ins(&m, 0, DRIFT_HALT, 0, 0, 0, 0);
        m.bytes[33 + reg] = 16; bad(&m, DRIFT_EOP);
    }
    for (uint8_t op = DRIFT_JMP; op <= DRIFT_JLT; ++op) {
        ins(&m, 0, op, 0, 0, 0, 1); bad(&m, DRIFT_EOP);
    }
    ins(&m, 0, DRIFT_EVENT, 0, 0, 0, 1); bad(&m, DRIFT_ECAP);
    ins(&m, 0, DRIFT_SLEEP, 0, 0, 0, 0); bad(&m, DRIFT_ECAP);
    ins(&m, 0, DRIFT_IO, 0, 0, 0, 0); bad(&m, DRIFT_ECAP);
    init(&m, 1, 8, 0, 1, DRIFT_BUS_NONE); bad(&m, DRIFT_EFORMAT);
    init(&m, 1, 8, 0, 1, DRIFT_BUS_SPI);
    CHECK(drift_load(&v, m.bytes, m.size, 0, ++instance) == DRIFT_ECAP);
    ins(&m, 0, DRIFT_IO, 0, 0, 0, 16); CHECK(load(&v, &m) < 0);

    /* Every byte, including entry PC and immediate words, is little endian. */
    init(&m, 3, 4, 4, 0, 0);
    ins(&m, 0, DRIFT_LIT, 0, 0, 0, 0x12345678u);
    ins(&m, 1, DRIFT_HALT, 0, 0, 0, 0);
    ins(&m, 2, DRIFT_LIT, 0, 0, 0, 99);
    m.bytes[56] = 0xfe; m.bytes[57] = 0xdc;
    m.bytes[58] = 0xba; m.bytes[59] = 0x98;
    CHECK(load(&v, &m) == DRIFT_READY);
    memset(m.bytes, 0, sizeof(m.bytes)); /* Loader owns an independent copy. */
    CHECK(drift_run(&v, 10) == DRIFT_DONE);
    CHECK(v.r[0] == 0x12345678u && v.pc == 2 && v.steps == 2);
    CHECK(v.memory[0] == 0xfe && v.memory[3] == 0x98 && v.memory[4] == 0);
    CHECK(drift_run(&v, 10) == DRIFT_DONE && v.steps == 2);
    init(&m, 2, 1, 0, 0, 0);
    ins(&m, 0, DRIFT_LIT, 0, 0, 0, 33);
    put32(m.bytes + 24, 1);
    CHECK(load(&v, &m) == DRIFT_READY && drift_run(&v, 5) == DRIFT_DONE);
    CHECK(v.r[0] == 0 && v.steps == 1);

    /* A failed replacement invalidates a previously runnable machine. */
    CHECK(load(&v, &m) == DRIFT_READY);
    m.bytes[0] = 0;
    CHECK(load(&v, &m) == DRIFT_EFORMAT);
    CHECK(drift_run(&v, 100) < 0);

    init(&m, DRIFT_CODE_MAX, DRIFT_MEMORY_MAX, DRIFT_MEMORY_MAX, 0, 0);
    put32(m.bytes + 24, DRIFT_CODE_MAX - 1);
    memset(m.bytes + 32 + DRIFT_CODE_MAX * 8, 0x5a, DRIFT_MEMORY_MAX);
    CHECK(load(&v, &m) == DRIFT_READY && drift_run(&v, 1) == DRIFT_DONE);
    CHECK(v.memory[0] == 0x5a && v.memory[DRIFT_MEMORY_MAX - 1] == 0x5a);
    init(&m, 1, 1, 0, 0, 0);
    CHECK(load(&v, &m) == DRIFT_READY);
    CHECK(v.memory[DRIFT_MEMORY_MAX - 1] == 0); /* No stale prior driver bytes. */
}

static void test_arithmetic(void)
{
    image m;
    drift_vm v = {0};
    init(&m, 19, 1, 0, 0, 0);
    ins(&m, 0, DRIFT_LIT, 2, 0, 0, UINT32_MAX);
    ins(&m, 1, DRIFT_LIT, 3, 0, 0, 1);
    ins(&m, 2, DRIFT_ADD, 4, 2, 3, 0);
    ins(&m, 3, DRIFT_SUB, 5, 4, 3, 0);
    ins(&m, 4, DRIFT_LIT, 6, 0, 0, 2);
    ins(&m, 5, DRIFT_MUL, 7, 2, 6, 0);
    ins(&m, 6, DRIFT_DIV, 8, 2, 6, 0);
    ins(&m, 7, DRIFT_LIT, 9, 0, 0, 63);
    ins(&m, 8, DRIFT_SHL, 10, 3, 9, 0);
    ins(&m, 9, DRIFT_SHR, 11, 2, 9, 0);
    ins(&m, 10, DRIFT_AND, 12, 2, 10, 0);
    ins(&m, 11, DRIFT_OR, 13, 10, 3, 0);
    ins(&m, 12, DRIFT_XOR, 14, 2, 10, 0);
    ins(&m, 13, DRIFT_MOV, 15, 14, 0, 0);
    ins(&m, 14, DRIFT_LIT, 0, 0, 0, 32);
    ins(&m, 15, DRIFT_SHL, 0, 3, 0, 0); /* Count modulo 32, not UB. */
    ins(&m, 16, DRIFT_MUL, 1, 10, 6, 0);
    ins(&m, 17, DRIFT_SUB, 1, 1, 3, 0);
    CHECK(load(&v, &m) == DRIFT_READY && drift_run(&v, 100) == DRIFT_DONE);
    CHECK(v.r[4] == 0 && v.r[5] == UINT32_MAX);
    CHECK(v.r[7] == UINT32_MAX - 1 && v.r[8] == 0x7fffffffu);
    CHECK(v.r[10] == 0x80000000u && v.r[11] == 1 && v.r[12] == 0x80000000u);
    CHECK(v.r[13] == 0x80000001u && v.r[14] == 0x7fffffffu);
    CHECK(v.r[15] == v.r[14] && v.r[0] == 1 && v.r[1] == UINT32_MAX);
    init(&m, 1, 1, 0, 0, 0);
    ins(&m, 0, DRIFT_DIV, 0, 0, 0, 0);
    CHECK(load(&v, &m) == DRIFT_READY && drift_run(&v, 1) == DRIFT_EDIVZERO);
    CHECK(drift_run(&v, 100) == DRIFT_EDIVZERO && v.steps == 1);
    CHECK(drift_complete(&v, v.instance, 1, 0, NULL, 0) == DRIFT_ESTATE);
}

static void test_memory(void)
{
    image m;
    drift_vm v = {0};
    init(&m, 8, 12, 0, 0, 0);
    ins(&m, 0, DRIFT_LIT, 2, 0, 0, 1);
    ins(&m, 1, DRIFT_LIT, 3, 0, 0, 0x89abcdefu);
    ins(&m, 2, DRIFT_ST32, 3, 2, 0, 2); /* Unaligned address 3. */
    ins(&m, 3, DRIFT_LD32, 4, 2, 0, 2);
    ins(&m, 4, DRIFT_ST8, 3, 2, 0, 10); /* Last allowed byte. */
    ins(&m, 5, DRIFT_LD8, 5, 2, 0, 10);
    ins(&m, 6, DRIFT_LD32, 0, 2, 0, 7); /* Last allowed word. */
    CHECK(load(&v, &m) == DRIFT_READY && drift_run(&v, 100) == DRIFT_DONE);
    CHECK(v.r[4] == 0x89abcdefu && v.r[5] == 0xefu);
    CHECK(v.memory[3] == 0xef && v.memory[4] == 0xcd && v.memory[5] == 0xab);
    CHECK(v.memory[6] == 0x89 && v.memory[11] == 0xef && v.r[0] == 0xef000000u);
    const uint32_t base[] = {0, 1, UINT32_MAX, 8, UINT32_MAX - 1};
    const uint32_t off[]  = {12, 11, 1, 1, 4};
    for (uint8_t op = DRIFT_LD8; op <= DRIFT_ST32; ++op) {
        for (unsigned i = 0; i < 5; ++i) {
            if (op < DRIFT_LD32 && i == 3) continue;
            init(&m, 3, 12, 0, 0, 0);
            ins(&m, 0, DRIFT_LIT, 2, 0, 0, base[i]);
            ins(&m, 1, op, 0, 2, 0, off[i]);
            CHECK(load(&v, &m) == DRIFT_READY);
            CHECK(drift_run(&v, 100) == DRIFT_EBOUNDS);
            CHECK(drift_run(&v, 100) == DRIFT_EBOUNDS && v.steps == 2);
        }
    }
}

static void test_control_flow(void)
{
    image m;
    drift_vm v = {0};
    init(&m, 7, 1, 0, 0, 0);
    ins(&m, 0, DRIFT_LIT, 2, 0, 0, UINT32_MAX);
    ins(&m, 1, DRIFT_JLT, 2, 0, 0, 5); /* Unsigned, never taken. */
    ins(&m, 2, DRIFT_JEQ, 0, 1, 0, 4);
    ins(&m, 3, DRIFT_LIT, 0, 0, 0, 99);
    ins(&m, 4, DRIFT_JNE, 0, 2, 0, 6);
    ins(&m, 5, DRIFT_LIT, 0, 0, 0, 33);
    CHECK(load(&v, &m) == DRIFT_READY && drift_run(&v, 100) == DRIFT_DONE);
    CHECK(v.r[0] == 0 && v.steps == 5);
    init(&m, 1, 1, 0, 0, 0);
    ins(&m, 0, DRIFT_JMP, 0, 0, 0, 0);
    CHECK(load(&v, &m) == DRIFT_READY);
    CHECK(drift_run(&v, 0) == DRIFT_QUOTA && v.steps == 0 && v.pc == 0);
    CHECK(drift_run(&v, 123) == DRIFT_QUOTA && v.steps == 123 && v.pc == 0);
    CHECK(drift_run(&v, 7) == DRIFT_QUOTA && v.steps == 130 && v.state == DRIFT_READY);
    ins(&m, 0, DRIFT_LIT, 0, 0, 0, 7); /* No HALT: end is a trap. */
    CHECK(load(&v, &m) == DRIFT_READY && drift_run(&v, 1) == DRIFT_QUOTA);
    CHECK(drift_run(&v, 1) == DRIFT_EBOUNDS && v.steps == 1);
}

static void xfer_image(image *m, uint32_t tx, uint32_t txlen,
                       uint32_t rx, uint32_t rxlen)
{
    init(m, 6, 16, 2, 1, DRIFT_BUS_SPI);
    ins(m, 0, DRIFT_LIT, 2, 0, 0, tx);
    ins(m, 1, DRIFT_LIT, 3, 0, 0, txlen);
    ins(m, 2, DRIFT_LIT, 4, 0, 0, rx);
    ins(m, 3, DRIFT_LIT, 5, 0, 0, rxlen);
    ins(m, 4, DRIFT_IO, 2, 3, 4, 5);
    m->bytes[32 + 6 * 8] = 0x9f;
    m->bytes[32 + 6 * 8 + 1] = 0xab;
}

static void test_effects(void)
{
    image m;
    drift_vm v = {0};
    uint8_t response[] = {0xef, 0x40, 0x18, 0xa5, 0x5a};
    xfer_image(&m, 0, 2, 8, 4);
    CHECK(load(&v, &m) == DRIFT_READY && drift_run(&v, 100) == DRIFT_EFFECT);
    drift_effect pending = v.effect;
    CHECK(pending.instance == v.instance && pending.ticket == 1 && pending.kind == DRIFT_XFER);
    CHECK(pending.tx == 0 && pending.tx_len == 2 && pending.rx == 8 && pending.rx_len == 4);
    CHECK(v.memory[pending.tx] == 0x9f && v.memory[pending.tx + 1] == 0xab);
    uint64_t steps = v.steps;
    CHECK(drift_run(&v, 100) == DRIFT_EFFECT && v.steps == steps);
    CHECK(memcmp(&pending, &v.effect, sizeof(pending)) == 0);
    CHECK(drift_complete(&v, v.instance + 1, 1, 0, response, 3) == DRIFT_ESTALE);
    CHECK(drift_complete(&v, v.instance, 2, 0, response, 3) == DRIFT_ESTALE);
    CHECK(drift_complete(&v, v.instance, 1, 0, response, 5) == DRIFT_EBOUNDS);
    CHECK(drift_complete(&v, v.instance, 1, 0, NULL, 1) == DRIFT_EARG);
    CHECK(drift_complete(&v, v.instance, 1, 1, NULL, 0) == DRIFT_EARG);
    CHECK(drift_complete(&v, v.instance, 1, -5, response, 1) == DRIFT_EARG);
    CHECK(v.state == DRIFT_EFFECT && v.memory[8] == 0 && v.memory[11] == 0);
    CHECK(memcmp(&pending, &v.effect, sizeof(pending)) == 0);
    CHECK(drift_complete(&v, v.instance, 1, 0, response, 3) == DRIFT_READY);
    CHECK(v.r[0] == 0 && v.r[1] == 3 && v.memory[8] == 0xef && v.memory[10] == 0x18);
    CHECK(v.memory[11] == 0 && v.pc == 5 && v.state == DRIFT_READY);
    response[0] = 0;
    CHECK(v.memory[8] == 0xef); /* Completion also owns a copy. */
    CHECK(drift_complete(&v, v.instance, 1, 0, NULL, 0) == DRIFT_ESTATE);
    CHECK(drift_run(&v, 1) == DRIFT_DONE);
    CHECK(load(&v, &m) == DRIFT_READY && drift_run(&v, 100) == DRIFT_EFFECT);
    CHECK(drift_complete(&v, pending.instance, pending.ticket, 0, NULL, 0) == DRIFT_ESTALE);
    CHECK(drift_complete(&v, v.instance, 1, -19, NULL, 0) == DRIFT_READY);
    CHECK(v.r[0] == (uint32_t)-19 && v.r[1] == 0 && v.memory[8] == 0);
    CHECK(drift_run(&v, 1) == DRIFT_DONE && v.r[0] == (uint32_t)-19);

    const uint32_t tx[] = {17, 15, UINT32_MAX, 0, 0};
    const uint32_t tl[] = {0, 2, 1, 0, 0};
    const uint32_t rx[] = {0, 0, 0, UINT32_MAX, 15};
    const uint32_t rl[] = {0, 0, 0, 1, 2};
    for (unsigned i = 0; i < 5; ++i) {
        xfer_image(&m, tx[i], tl[i], rx[i], rl[i]);
        CHECK(load(&v, &m) == DRIFT_READY && drift_run(&v, 100) == DRIFT_EBOUNDS);
    }
    xfer_image(&m, 16, 0, 16, 0); /* Empty span at end is valid. */
    CHECK(load(&v, &m) == DRIFT_READY && drift_run(&v, 100) == DRIFT_EFFECT);
    CHECK(drift_complete(&v, v.instance, 1, 0, NULL, 0) == DRIFT_READY);

    init(&m, 6, 1, 0, 6, 0);
    ins(&m, 0, DRIFT_LIT, 2, 0, 0, 0x1234);
    ins(&m, 1, DRIFT_LIT, 3, 0, 0, 1);
    ins(&m, 2, DRIFT_LIT, 4, 0, 0, UINT32_MAX);
    ins(&m, 3, DRIFT_EVENT, 2, 3, 4, 0xaabbccddu);
    ins(&m, 4, DRIFT_SLEEP, 2, 0, 0, 0);
    CHECK(load(&v, &m) == DRIFT_READY && drift_run(&v, 100) == DRIFT_EFFECT);
    CHECK(v.effect.kind == DRIFT_EMIT && v.effect.tag == 0xaabbccddu);
    CHECK(v.effect.x == 0x1234 && v.effect.y == 1 && v.effect.z == UINT32_MAX);
    CHECK(v.effect.tx_len == 0 && v.effect.rx_len == 0);
    CHECK(drift_complete(&v, v.instance, 1, 0, response, 1) == DRIFT_EARG);
    CHECK(drift_complete(&v, v.instance, 1, 0, NULL, 0) == DRIFT_READY);
    CHECK(drift_run(&v, 1) == DRIFT_EFFECT && v.effect.ticket == 2);
    CHECK(v.effect.kind == DRIFT_WAIT && v.effect.x == 0x1234 && v.effect.tag == 0);
    CHECK(v.effect.y == 0 && v.effect.z == 0 && v.effect.rx_len == 0);
    CHECK(drift_complete(&v, v.instance, 1, 0, NULL, 0) == DRIFT_ESTALE);
    CHECK(drift_complete(&v, v.instance, 2, 0, NULL, 0) == DRIFT_READY);
    CHECK(drift_run(&v, 1) == DRIFT_DONE);
}

static void test_overlapping_completion(void)
{
    image m;
    drift_vm v = {0};
    init(&m, 6, 16, 8, 1, DRIFT_BUS_STREAM);
    for (uint32_t i = 0; i < 8; ++i) m.bytes[32 + 6*8 + i] = (uint8_t)(i+1);
    ins(&m, 0, DRIFT_LIT, 2, 0, 0, 0);
    ins(&m, 1, DRIFT_LIT, 3, 0, 0, 0);
    ins(&m, 2, DRIFT_LIT, 4, 0, 0, 3);
    ins(&m, 3, DRIFT_LIT, 5, 0, 0, 4);
    ins(&m, 4, DRIFT_IO, 2, 3, 4, 5);
    ins(&m, 5, DRIFT_HALT, 0, 0, 0, 0);
    CHECK(load(&v, &m) == DRIFT_READY);
    CHECK(drift_run(&v, 10) == DRIFT_EFFECT);
    CHECK(drift_complete(&v, v.instance, v.effect.ticket, 0, v.memory+1, 4) == DRIFT_READY);
    const uint8_t right[] = {1,2,3,2,3,4,5,8};
    CHECK(memcmp(v.memory, right, sizeof(right)) == 0);
    ins(&m, 2, DRIFT_LIT, 4, 0, 0, 1);
    CHECK(load(&v, &m) == DRIFT_READY);
    CHECK(drift_run(&v, 10) == DRIFT_EFFECT);
    CHECK(drift_complete(&v, v.instance, v.effect.ticket, 0, v.memory+3, 4) == DRIFT_READY);
    const uint8_t left[] = {1,4,5,6,7,6,7,8};
    CHECK(memcmp(v.memory, left, sizeof(left)) == 0);
}

static void test_random_modules(void)
{
    image m;
    drift_vm v = {0};
    uint8_t response[DRIFT_MEMORY_MAX];
    memset(response, 0xa5, sizeof(response));
    for (unsigned round = 0; round < 30000; ++round) {
        uint32_t count = 1 + random32() % 64;
        uint32_t memory = 1 + random32() % 128;
        uint32_t data = random32() % (memory + 1);
        init(&m, count, memory, data, 7, 1);
        for (uint32_t pc = 0; pc < count; ++pc) {
            uint8_t op = (uint8_t)(random32() % 23);
            uint32_t imm = random32();
            if (op >= DRIFT_JMP && op <= DRIFT_JLT) imm %= count;
            else if (op == DRIFT_IO) imm %= 16;
            else if (op == DRIFT_LD8 || op == DRIFT_ST8 || op == DRIFT_LD32 || op == DRIFT_ST32)
                imm %= memory + 4;
            ins(&m, pc, op, random32() % 16, random32() % 16, random32() % 16, imm);
        }
        for (uint32_t i = 0; i < data; ++i)
            m.bytes[32 + count * 8 + i] = (uint8_t)random32();
        if ((round & 1) == 0) { /* Mutate both headers and instruction encoding. */
            unsigned mutations = 1 + random32() % 8;
            while (mutations--) m.bytes[random32() % m.size] ^= (uint8_t)random32();
        }
        int32_t status = load(&v, &m);
        if (status != DRIFT_READY) { CHECK(status < 0); continue; }
        for (unsigned turn = 0; turn < 12; ++turn) {
            status = drift_run(&v, 37);
            if (status == DRIFT_EFFECT) {
                uint32_t n = v.effect.rx_len;
                if (n) n = random32() % (n + 1);
                CHECK(n <= sizeof(response));
                CHECK(drift_complete(&v, v.instance, v.effect.ticket, 0, response, n) == DRIFT_READY);
            } else if (status != DRIFT_QUOTA) {
                CHECK(status == DRIFT_DONE || status < 0);
                break;
            }
        }
        CHECK(v.memory_len <= DRIFT_MEMORY_MAX && v.code_len <= DRIFT_CODE_MAX);
    }
}

int main(void)
{
    test_loader(); test_arithmetic(); test_memory(); test_control_flow();
    test_effects(); test_overlapping_completion(); test_random_modules();
    printf("vm: %u checks passed (30,000 generated/mutated modules)\n", checks);
    return 0;
}
