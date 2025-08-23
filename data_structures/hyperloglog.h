#ifndef HYPERLOGLOG_H
#define HYPERLOGLOG_H
#include <stdint.h>
#include "dstr.h"

const uint32_t REGISTER_CNT = 16384;
const int32_t HLL_HEADER_SIZE_BYTES = 16;
const int32_t HLL_DENSE_SIZE_BYTES = 6 * REGISTER_CNT / 8; // 2^14 (16384) 6 bit registers
const double BIAS_CORRECTION = 0.7213 / (1. + 1.079 / REGISTER_CNT);
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

uint8_t hll_add(dstr *hll, dstr *val);
uint64_t hll_count(dstr *hll);
void hll_merge(dstr *hll1, dstr *hll2);


#endif
