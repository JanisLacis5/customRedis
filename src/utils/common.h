#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stddef.h>

// ptr = pointer to a struct's member
// T = type of the enclosing struct
// member = id for the field in T that ptr points to
#define container_of(ptr, T, member) ((T *)( (char *)ptr - offsetof(T, member) ))

#define dmax(a,b)             \
({                           \
__typeof__ (a) _a = (a); \
__typeof__ (b) _b = (b); \
_a > _b ? _a : _b;       \
})

#define dmin(a,b)             \
({                           \
__typeof__ (a) _a = (a); \
__typeof__ (b) _b = (b); \
_a < _b ? _a : _b;       \
})

enum EntryTypes {
    T_STR = 0,
    T_ZSET = 1,
    T_HSET = 2,
    T_LIST = 3,
    T_SET = 4,
    T_BITMAP = 5,
    T_HLL = 6
};

// FNV hash
// inline uint64_t str_hash(const uint8_t *data, size_t len) {
//     uint32_t h = 0x811C9DC5;
//     for (size_t i = 0; i < len; i++) {
//         h = (h + data[i]) * 0x01000193;
//     }
//     return h;
// }


static inline uint64_t murmurHash64A(const uint8_t *key, size_t len, uint64_t seed) {
    const uint64_t m = 0xc6a4a7935bd1e995ULL;
    const int r = 47;

    uint64_t h = seed ^ (len * m);

    const uint64_t *data = (const uint64_t*)key;
    const uint64_t *end = data + (len / 8);

    while (data != end) {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char *data2 = (const unsigned char*)data;

    switch (len & 7) {
        case 7:
            h ^= uint64_t(data2[6]) << 48;
        case 6:
            h ^= uint64_t(data2[5]) << 40;
        case 5:
            h ^= uint64_t(data2[4]) << 32;
        case 4:
            h ^= uint64_t(data2[3]) << 24;
        case 3:
            h ^= uint64_t(data2[2]) << 16;
        case 2:
            h ^= uint64_t(data2[1]) << 8;
        case 1:
            h ^= uint64_t(data2[0]);
            h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

inline uint64_t str_hash(const uint8_t *key, size_t len) {
    return murmurHash64A(key, len, 0xadc83b19ULL);
}
#endif
