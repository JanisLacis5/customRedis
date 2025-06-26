#ifndef BUFFER_FUNCS_H
#define BUFFER_FUNCS_H

#include <vector>
#include <stdint.h>

void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len);
void buf_consume(std::vector<uint8_t> &buf, size_t len);
void buf_append_u8(std::vector<uint8_t> &buf, uint8_t data);
void buf_append_u32(std::vector<uint8_t> &buf, uint32_t data);
void buf_append_double(std::vector<uint8_t> &buf, double data)

#endif //BUFFER_FUNCS_H
