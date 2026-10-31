/* Portable component benchmark interface. SPDX-License-Identifier: MIT */
#ifndef DRIFT_BENCH_LINUX_IMPORT_H
#define DRIFT_BENCH_LINUX_IMPORT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define DRIFT_LINUX_IMPORT_BENCH_DEFAULT_EVENTS 500000U
#define DRIFT_LINUX_IMPORT_BENCH_ROUNDS 4U
#define DRIFT_LINUX_IMPORT_BENCH_SAMPLES 8U
#define DRIFT_LINUX_IMPORT_BENCH_SCOPE "component-model-no-hardware"

enum drift_linux_import_bench_variant {
    DRIFT_LINUX_IMPORT_BENCH_REFERENCE=0,
    DRIFT_LINUX_IMPORT_BENCH_IMPORTED=1
};
typedef struct drift_linux_import_bench_sample {
    uint32_t sample, round, position, variant, events, control;
    uint64_t elapsed_us, operations, modeled_delay_us, checksum;
} drift_linux_import_bench_sample;

/* Return zero and store monotonic wall microseconds, or return an error.
 * Genode supplies its clock here. It need not link the hosted main below. */
typedef int32_t (*drift_linux_import_bench_clock)(void *, uint64_t *);
typedef int32_t (*drift_linux_import_bench_emit)(void *,
    const drift_linux_import_bench_sample *);

/* Models port accesses and mandatory delays; does not access hardware or sleep.
 * Events must be a multiple of 16, between 16 and 4000000. Registration and
 * teardown are outside timing. The imported event path is compared with an
 * independent direct PIT reference, NOT a native Linux driver measurement.
 * All exact preflight transcripts and all timed-pair digests/counts must match
 * before emit is called. Four rounds reverse order each time. Sample pointers
 * are borrowed for the duration of emit only. Returns zero on success, -22 for
 * bad arguments, -3 for clock failure, -5 for model/driver failure, -6 for a
 * comparison mismatch, or the nonzero emitter error. No allocation or threads.
 * Define DRIFT_LINUX_IMPORT_BENCH_NO_MAIN when embedding this implementation. */
int32_t drift_bench_linux_import_run(drift_linux_import_bench_clock, void *,
    drift_linux_import_bench_emit, void *, uint32_t events);

#ifdef __cplusplus
}
#endif
#endif
