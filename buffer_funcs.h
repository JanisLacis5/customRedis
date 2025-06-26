#ifndef BUFFER_FUNCS_H
#define BUFFER_FUNCS_H

#include <vector>
#include <stdint.h>

void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len);
void buf_consume(std::vector<uint8_t> &buf, size_t len);

#endif //BUFFER_FUNCS_H
