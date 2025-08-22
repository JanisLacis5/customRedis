#ifndef DSTR_H
#define DSTR_H
#include <stdint.h>
#include <cstddef>

const size_t MAX_STR_PREALOC = 1024 * 1024; // 1 MB
const size_t MAX_STR_SIZE = (1ull << 30); // 1 GB

enum StrFuncReturns {
    STR_OK = 0,
    STR_ERR_TOO_LARGE = 1,
    STR_ERR_ALLOC_FAIL = 2
};

struct dstr {
    size_t size;
    size_t free;
    char buf[];
};

dstr* dstr_init(size_t len);
size_t dstr_cap(dstr *str);
uint32_t dstr_resize(dstr **pstr, size_t len, unsigned char pad);
uint32_t dstr_assign(dstr **pstr, const char *toadd, size_t toadd_s);
uint32_t dstr_append(dstr **pstr, const char *toadd, size_t toadd_s);

#endif
