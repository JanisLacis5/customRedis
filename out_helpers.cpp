#include <stdint.h>
#include <string>

#include "out_helpers.h"

void out_arr(Conn *conn, uint32_t len) {
    buf_append_u8(conn->outgoing, TAG_ARR);
    buf_append_u32(conn->outgoing, len);
}

void out_int(Conn *conn, uint32_t nr) {
    buf_append_u8(conn->outgoing, TAG_INT);
    buf_append_u32(conn->outgoing, nr);
}

void out_double(Conn *conn, double dbl) {
    buf_append_u8(conn->outgoing, TAG_DOUBLE);
    buf_append_double(conn->outgoing, dbl);
}

void out_str(Conn *conn, char *str, uint32_t size) {
    buf_append_u8(conn->outgoing, TAG_STR);
    buf_append_u32(conn->outgoing, size);
    buf_append(conn->outgoing, (uint8_t*)str, size);
}

void out_not_found(Conn *conn) {
    buf_rem_last_res_code(conn->outgoing);
    buf_append_u32(conn->outgoing, RES_NX);
    buf_append_u8(conn->outgoing, TAG_NULL);
}

void out_err(Conn *conn, const std::string &err_mes) {
    // Replace the defalut OK tag with ERR tag
    buf_rem_last_res_code(conn->outgoing);
    buf_append_u32(conn->outgoing, RES_ERR);

    // Add error message
    buf_append_u8(conn->outgoing, TAG_ERROR);
    buf_append_u32(conn->outgoing, err_mes.size());
    buf_append(conn->outgoing, (uint8_t*)err_mes.data(), err_mes.size());
}

void out_null(Conn *conn) {
    buf_append_u8(conn->outgoing, TAG_NULL);
}

size_t out_unknown_arr(Conn *conn) {
    buf_append_u8(conn->outgoing, TAG_ARR);
    buf_append_u32(conn->outgoing, 0); // to be updated
    return conn->outgoing.size() - 4;
}