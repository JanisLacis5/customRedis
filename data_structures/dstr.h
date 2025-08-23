#ifndef DSTR_H
#define DSTR_H
#include <stdint.h>
#include <cstddef>

const size_t MAX_PREALOC = 1024 * 1024; // 1 MB

struct dstr {
    size_t size;
    size_t free;
    char buf[];
};

dstr* dstr_init(size_t len);
size_t dstr_cap(dstr *str);
uint8_t dstr_resize(dstr **pstr, size_t len, unsigned char pad);
size_t dstr_assign(dstr **pstr, const char *toadd, size_t toadd_s);
size_t dstr_append(dstr **pstr, const char *toadd, size_t toadd_s);

#endif
