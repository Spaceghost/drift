/* A complete cooperative embedding seam. SPDX-License-Identifier: MIT
 * Compile with -ffreestanding; no allocator or C library is required.
 * The scheduler calls drift_tick; on an effect, its endpoint owns completion.
 * A kernel port supplies exactly the endpoint callback below. */
#include "drift.h"

typedef int32_t (*drift_endpoint)(void *context, const drift_effect *effect,
                                  const uint8_t *tx, uint8_t *rx,
                                  uint32_t *received);

typedef struct drift_device {
    drift_vm machine;
    drift_endpoint endpoint;
    void *context;
    uint8_t reply[DRIFT_MEMORY_MAX];
} drift_device;

/* The callback completes synchronously. An asynchronous OS can instead retain
 * the instance/ticket and call drift_complete from its completion queue; never
 * run or mutate a VM concurrently with another caller. */
int32_t drift_tick(drift_device *device, uint32_t fuel)
{
    drift_effect *effect;
    uint32_t received=0;
    int32_t state, status;
    if (!device || !device->endpoint) return DRIFT_EARG;
    state=drift_run(&device->machine,fuel);
    if (state != DRIFT_EFFECT) return state;
    effect=&device->machine.effect;
    status=device->endpoint(device->context,effect,
                            device->machine.memory+effect->tx,
                            device->reply,&received);
    state=drift_complete(&device->machine,effect->instance,effect->ticket,
                         status,device->reply,received);
    return state;
}
