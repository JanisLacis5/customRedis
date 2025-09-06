#ifndef OUT_HELPERS_H
#define OUT_HELPERS_H

#include <string.h>
#include "server.h"

enum Tags {
    TAG_INT = 0,
    TAG_STR = 1,
    TAG_ARR = 2,
    TAG_NULL = 3,
    TAG_ERROR = 4,
    TAG_DOUBLE = 5
};

// Response status codes
enum ResponseStatusCodes {
    RES_OK = 0,
    RES_ERR = 1,    // error
    RES_NX = 2,     // key not found
};

void out_arr(Conn *conn, uint32_t len);
void out_int(Conn *conn, uint32_t nr);
void out_double(Conn *conn, double dbl);
void out_str(Conn *conn, const char *str, uint32_t size);
void out_not_found(Conn *conn);
void out_err(Conn *conn, const char *err_mes);
size_t out_unknown_arr(Conn *conn);
void out_null(Conn *conn);

#endif
