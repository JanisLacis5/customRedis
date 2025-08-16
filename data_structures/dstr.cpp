#include "dstr.h"
#include <algorithm>
#include <string.h>

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

size_t dstr_append(dstr **pstr, char *toadd, size_t toadd_s) {
    dstr *str = *pstr;
    size_t lpos = str->size;
    if (str->free < toadd_s) {
        size_t newlen = str->size + str->free + toadd_s;
        size_t incr = std::min(MAX_PREALOC, newlen);

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
