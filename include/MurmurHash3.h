// MurmurHash3 was written by Austin Appleby and is placed in the public
// domain. The author disclaims copyright to this source code.
//
// This is the 32-bit variant (x86_32), which is what we use to derive
// the two independent hash functions for the Bloom filter's double
// hashing scheme.

#ifndef MURMURHASH3_H
#define MURMURHASH3_H

#include <cstdint>
#include <cstddef>

void MurmurHash3_x86_32(const void* key, int len, uint32_t seed, void* out);

#endif // MURMURHASH3_H
