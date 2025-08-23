#include <cstdio>
#include <cstdlib>
#include <string.h>
#include "dstr.h"
#include "utils/common.h"


dstr* dstr_init(size_t len) {
    dstr* str = (dstr*)malloc(sizeof(dstr) + len + 1);
    if (!str) {
        return NULL;
    }
    str->size = 0;
    str->free = len;
    str->buf[0] = '\0';
    return str;
}

size_t dstr_cap(dstr* str) {
    return str->free + str->size;
}

size_t dstr_assign(dstr **pstr, const char *toadd, size_t toadd_s) {
    dstr *str = *pstr;
    str->free = dstr_cap(str);
    str->size = 0;
    str->buf[0] = '\0';
    dstr_append(pstr, toadd, toadd_s);
}

uint8_t dstr_resize(dstr **pstr, size_t len, unsigned char pad) {
    dstr *str = *pstr;
    size_t cap = dstr_cap(str);
    if (str->size < len) {
        if (cap < len) {
            str = (dstr*)realloc(str, sizeof(dstr) + len + 1);
            if (!str) {
                return 1;
            }
            *pstr = str;
            cap = len;
        }
        memset(str->buf + str->size, pad, len - str->size);
    }

    str->free = cap - len;
    str->size = len;
    str->buf[len] = '\0';
    return 0;
}

size_t dstr_append(dstr **pstr, const char *toadd, size_t toadd_s) {
    dstr *str = *pstr;
    size_t lpos = str->size;
    if (str->free < toadd_s) {
        size_t newlen = str->size + str->free + toadd_s;
        size_t incr = min(MAX_PREALOC, newlen);

        newlen += incr;
        str->free = newlen - str->free;
        str = (dstr*)realloc(str, sizeof(dstr) + newlen + 1);
        if (!str) {
            return 0;
        }
    }

    memcpy(&str->buf[lpos], toadd, toadd_s);
    str->size += toadd_s;
    str->free -= toadd_s;
    str->buf[str->size] = '\0';
    *pstr = str;
    return toadd_s;
}
