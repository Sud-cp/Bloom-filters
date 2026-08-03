// MurmurHash3 was written by Austin Appleby and is placed in the public
// domain. The author disclaims copyright to this source code.

#include "MurmurHash3.h"
#include <cstring>

namespace {

inline uint32_t rotl32(uint32_t x, int8_t r) {
    return (x << r) | (x >> (32 - r));
}

inline uint32_t fmix32(uint32_t h) {
    h ^= h >> 16;
    h *= 0x85ebca6bU;
    h ^= h >> 13;
    h *= 0xc2b2ae35U;
    h ^= h >> 16;
    return h;
}

inline uint32_t getBlock32(const uint8_t* p, int i) {
    uint32_t block;
    std::memcpy(&block, p + i * 4, sizeof(block));
    return block;
}

} // namespace

void MurmurHash3_x86_32(const void* key, int len, uint32_t seed, void* out) {
    const uint8_t* data = static_cast<const uint8_t*>(key);
    const int nblocks = len / 4;

    uint32_t h1 = seed;

    const uint32_t c1 = 0xcc9e2d51U;
    const uint32_t c2 = 0x1b873593U;

    // body
    for (int i = 0; i < nblocks; ++i) {
        uint32_t k1 = getBlock32(data, i);

        k1 *= c1;
        k1 = rotl32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = rotl32(h1, 13);
        h1 = h1 * 5 + 0xe6546b64U;
    }

    // tail
    const uint8_t* tail = data + nblocks * 4;
    uint32_t k1 = 0;

    switch (len & 3) {
        case 3: k1 ^= static_cast<uint32_t>(tail[2]) << 16; [[fallthrough]];
        case 2: k1 ^= static_cast<uint32_t>(tail[1]) << 8;  [[fallthrough]];
        case 1: k1 ^= static_cast<uint32_t>(tail[0]);
                k1 *= c1;
                k1 = rotl32(k1, 15);
                k1 *= c2;
                h1 ^= k1;
    }

    // finalization
    h1 ^= static_cast<uint32_t>(len);
    h1 = fmix32(h1);

    std::memcpy(out, &h1, sizeof(h1));
}
