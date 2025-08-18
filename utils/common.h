#ifndef COMMON_H
#define COMMON_H

// ptr = pointer to a struct's member
// T = type of the enclosing struct
// member = id for the field in T that ptr points to
#define container_of(ptr, T, member) ((T *)( (char *)ptr - offsetof(T, member) ))

#define max(a,b)             \
({                           \
__typeof__ (a) _a = (a); \
__typeof__ (b) _b = (b); \
_a > _b ? _a : _b;       \
})

#define min(a,b)             \
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
    T_BITMAP = 5
};

// FNV hash
inline uint64_t str_hash(const uint8_t *data, size_t len) {
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x01000193;
    }
    return h;
}

#endif
