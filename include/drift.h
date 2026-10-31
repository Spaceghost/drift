/* Drift: a portable driver machine. SPDX-License-Identifier: MIT */
#ifndef DRIFT_H
#define DRIFT_H
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRIFT_ABI 1u
#define DRIFT_CODE_MAX 1024u
#define DRIFT_MEMORY_MAX 4096u
#define DRIFT_REGS 16u

enum { DRIFT_READY=0, DRIFT_EFFECT=1, DRIFT_QUOTA=2, DRIFT_DONE=3 };
enum { DRIFT_EFORMAT=-1, DRIFT_EOP=-2, DRIFT_EBOUNDS=-3,
       DRIFT_ECAP=-4, DRIFT_ESTATE=-5, DRIFT_ESTALE=-6,
       DRIFT_EDIVZERO=-7, DRIFT_ETICKET=-8, DRIFT_EARG=-9 };
enum { DRIFT_XFER=1, DRIFT_EMIT=2, DRIFT_WAIT=3 };
enum { DRIFT_CAP_XFER=1, DRIFT_CAP_EMIT=2, DRIFT_CAP_WAIT=4 };
enum { DRIFT_BUS_NONE=0, DRIFT_BUS_STREAM=1, DRIFT_BUS_SPI=2,
       DRIFT_BUS_I2C=3 };
enum { DRIFT_HALT=0, DRIFT_LIT, DRIFT_MOV, DRIFT_ADD, DRIFT_SUB,
       DRIFT_MUL, DRIFT_DIV, DRIFT_AND, DRIFT_OR, DRIFT_XOR,
       DRIFT_SHL, DRIFT_SHR, DRIFT_LD8, DRIFT_ST8, DRIFT_LD32,
       DRIFT_ST32, DRIFT_JMP, DRIFT_JEQ, DRIFT_JNE, DRIFT_JLT,
       DRIFT_IO, DRIFT_EVENT, DRIFT_SLEEP };

typedef struct drift_insn {
    uint8_t op, a, b, c;
    uint32_t imm;
} drift_insn;

/* One effect, one completion. No native pointer crosses the module boundary. */
typedef struct drift_effect {
    uint64_t instance;
    uint32_t ticket, kind;
    uint32_t tx, tx_len, rx, rx_len;
    uint32_t tag, x, y, z;
} drift_effect;

/* Caller-owned, fixed-size, reentrant. Fields are host-private, not an ABI
 * serialization format. Do not mutate a live machine or run it concurrently. */
typedef struct drift_vm {
    uint32_t r[DRIFT_REGS];
    drift_insn code[DRIFT_CODE_MAX];
    uint8_t memory[DRIFT_MEMORY_MAX];
    drift_effect effect;
    uint64_t instance, steps;
    uint32_t pc, code_len, memory_len, caps, bus, sequence;
    int32_t state;
} drift_vm;

/* Instance must be fresh and nonzero on every load, even after failure/reset.
 * Input module is copied; caller may release it after this call. The image
 * buffer must not overlap the VM. Neither input buffer is modified.
 * Failure invalidates the previous machine. grants is a host policy mask. */
int32_t drift_load(drift_vm *, uint8_t *, uint32_t size,
                   uint32_t grants, uint64_t instance);
int32_t drift_run(drift_vm *, uint32_t fuel);
/* status <= 0: zero success, negative transport error. Successful data is copied
 * only after ticket and bounds checks. A rejected completion leaves the effect
 * pending. Overlapping source/destination data has memmove semantics.
 * On acceptance r0 = status (u32 bits), r1 = received byte count. */
int32_t drift_complete(drift_vm *, uint64_t instance, uint32_t ticket,
                       int32_t status, uint8_t *data, uint32_t size);

#ifdef __cplusplus
}
#endif
#endif
