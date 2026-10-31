/* SPDX-License-Identifier: MIT
 * Guest-side expected-data generator, independent of the Python QEMU writer.
 * Used only by test applications; the generic resource provider does not
 * include this file or access the fixture. */
#ifndef DRIFT_GENODE_RNG_FIXTURE_H
#define DRIFT_GENODE_RNG_FIXTURE_H
#include <stdint.h>
struct Expected_stream {
 uint32_t state = 0x6d2b79f5u;
 uint64_t offset = 0, checksum = 14695981039346656037ull;
 uint8_t next() {
  state ^= state << 13; state ^= state >> 17; state ^= state << 5;
  ++offset; return static_cast<uint8_t>(state >> 24);
 }
 void discard(unsigned n) { for (unsigned i = 0; i < n; ++i) next(); }
 bool check(uint8_t const *bytes, unsigned n) {
  bool valid = true;
  for (unsigned i = 0; i < n; ++i) {
   valid &= bytes[i] == next(); checksum ^= bytes[i]; checksum *= 1099511628211ull;
  }
  return valid;
 }
};
#endif
