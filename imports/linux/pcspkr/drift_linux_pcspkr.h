/* Explicit host resources for the opt-in Linux PC speaker import.
 * SPDX-License-Identifier: MIT */
#ifndef DRIFT_LINUX_PCSPKR_H
#define DRIFT_LINUX_PCSPKR_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct drift_linux_pcspkr_host {
    void *context;
    /* Only ports 0x42/0x43 (write) and 0x61 (read/write) are requested. */
    int32_t (*read8)(void *, uint16_t);
    int32_t (*write8)(void *, uint16_t, uint8_t);
    /* Supply real serialization and the target's required interrupt policy.
     * Return 0 or a negative error. Flags are opaque to Drift. */
    int32_t (*lock)(void *, unsigned long *flags);
    int32_t (*unlock)(void *, unsigned long flags);
    /* The Linux _p I/O operations need a post-access delay. */
    int32_t (*wait_us)(void *, uint32_t);
} drift_linux_pcspkr_host;

typedef struct drift_linux_pcspkr {
    drift_linux_pcspkr_host *host;
    uint32_t phase, allocated, registered, locked;
    int32_t error;
    unsigned long irq_flags;
    /* Caller-owned fixed storage, checked against the shim types at build time. */
    uint64_t input_slot[32], platform_slot[8];
} drift_linux_pcspkr;

/* Zero-initialize once. Keep state/host/resources alive through close.
 * The upstream driver is trusted native GPL code, outside Drift's VM.
 * Calls on any instance are serialized by one non-spinning atomic gate;
 * concurrent or nested calls return -16 (EBUSY), without invoking callbacks.
 * Resources must remain exclusively assigned to this speaker/PIT channel.
 * Hardware/lock failures are sticky for the current call and returned to the
 * caller; close can be retried if stopping the speaker fails. If unlock fails,
 * retry close in the original lock-owning execution context. The host must
 * reject or arrange recovery if its mutex/IRQ state is context-bound. */
int32_t drift_linux_pcspkr_open(drift_linux_pcspkr *, drift_linux_pcspkr_host *);
int32_t drift_linux_pcspkr_event(drift_linux_pcspkr *, uint32_t type, uint32_t code, int32_t value);
int32_t drift_linux_pcspkr_suspend(drift_linux_pcspkr *);
int32_t drift_linux_pcspkr_shutdown(drift_linux_pcspkr *);
int32_t drift_linux_pcspkr_close(drift_linux_pcspkr *);
enum { DRIFT_PCSPKR_EV_SND=18, DRIFT_PCSPKR_BELL=1, DRIFT_PCSPKR_TONE=2 };
#ifdef __cplusplus
}
#endif
#endif
