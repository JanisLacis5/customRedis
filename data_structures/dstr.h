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
size_t dstr_append(dstr **pstr, char *toadd, size_t toadd_s);

#endif
