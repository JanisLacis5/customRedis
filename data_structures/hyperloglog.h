#ifndef HYPERLOGLOG_H
#define HYPERLOGLOG_H
#include <stdint.h>
#include <string>

const int32_t HLL_HEADER_SIZE_BYTES = 16;
const int32_t HLL_DENSE_SIZE_BYTES = 6 * 16384 / 8; // 2^14 (16384) 6 bit registers
enum HLLEncoding {
    HLL_DENSE = 0,
    HLL_SPARSE = 1
};

struct HLLHeader {
    uint8_t magic[4];
    uint8_t enc;
    uint8_t unused[3];
    uint8_t card[8];
};

uint8_t hll_add(std::string &hll, std::string &val);
uint64_t hll_count(std::string &hll);
void hll_merge(std::string &hll1, std::string &hll2);


#endif
