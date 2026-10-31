/* Independent guest-side deterministic I/O integrity oracle, not entropy. */
/* SPDX-License-Identifier: MIT */
#ifndef DRIFT_MINIX_RNG_FIXTURE_H
#define DRIFT_MINIX_RNG_FIXTURE_H
#include <stdint.h>
#define DRIFT_FIXTURE_SEED UINT32_C(0x6d2b79f5)
static inline uint8_t fixture_next(uint32_t *state) {
    uint32_t x=*state;x^=x<<13;x^=x>>17;x^=x<<5;*state=x;
    return (uint8_t)(x>>24);
}
static inline int fixture_compare(uint32_t *state,const uint8_t *data,unsigned n) {
    for(unsigned i=0;i<n;i++)if(data[i]!=fixture_next(state))return (int)i+1;
    return 0;
}
static inline uint64_t fixture_hash(uint64_t hash,const uint8_t *data,unsigned n) {
    for(unsigned i=0;i<n;i++)hash=(hash^data[i])*UINT64_C(1099511628211);
    return hash;
}
#endif
