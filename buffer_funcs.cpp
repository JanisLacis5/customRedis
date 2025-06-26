#include <vector>
#include <stdint.h>

void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
    buf.insert(buf.end(), data, data+len);
}

void buf_consume(std::vector<uint8_t> &buf, size_t len) {
    buf.erase(buf.begin(), buf.begin() + len);
}

void buf_append_u8(std::vector<uint8_t> &buf, uint8_t data) {
    buf.push_back(data);
}

void buf_append_u32(std::vector<uint8_t> &buf, uint32_t data) {
    buf_append(buf, (uint8_t*)&data, 4);
}