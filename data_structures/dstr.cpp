#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <string.h>
#include <errno.h>
#include "dstr.h"
#include "utils/common.h"


dstr* dstr_init(size_t len) {
    if (len > MAX_STR_SIZE) {
        errno = STR_ERR_TOO_LARGE;
        return NULL;
    }
    dstr* str = (dstr*)malloc(sizeof(dstr) + len + 1);
    if (!str) {
        errno = STR_ERR_ALLOC_FAIL;
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

uint32_t dstr_assign(dstr **pstr, const char *toadd, size_t toadd_s) {
    dstr *tmp = dstr_init(dstr_cap(*pstr));
    if (!tmp) {
        return errno;
    }

    uint32_t err = dstr_append(&tmp, toadd, toadd_s);
    if (err) {
        free(tmp);
        return err;
    }
    
    dstr *old = *pstr;
    *pstr = tmp;
    free(old);
    return STR_OK;

uint32_t dstr_resize(dstr **pstr, size_t len, unsigned char pad) {
    if (len > MAX_STR_SIZE) {
        return STR_ERR_TOO_LARGE;
    }
    dstr *str = *pstr;
    size_t cap = dstr_cap(str);
    if (str->size < len) {
        if (cap < len) {
            str = (dstr*)realloc(str, sizeof(dstr) + len + 1);
            if (!str) {
                return STR_ERR_ALLOC_FAIL;
            }
            *pstr = str;
            cap = len;
        }
        memset(str->buf + str->size, pad, len - str->size);
    }

    str->free = cap - len;
    str->size = len;
    str->buf[len] = '\0';
    return STR_OK;
}

uint32_t dstr_append(dstr **pstr, const char *toadd, size_t toadd_s) {
    if (toadd_s + dstr_cap(*pstr) > MAX_STR_SIZE) {
        return STR_ERR_TOO_LARGE;
    }

    dstr *str = *pstr;
    size_t lpos = str->size;
    if (str->free < toadd_s) {
        size_t newlen = str->size + str->free + toadd_s;
        size_t incr = dmin(MAX_STR_PREALOC, newlen);

        newlen += incr;
        str->free = newlen - str->free;
        str = (dstr*)realloc(str, sizeof(dstr) + newlen + 1);
        if (!str) {
            return STR_ERR_ALLOC_FAIL;
        }
    }

    memcpy(&str->buf[lpos], toadd, toadd_s);
    str->size += toadd_s;
    str->free -= toadd_s;
    str->buf[str->size] = '\0';
    *pstr = str;
    return STR_OK;
}
