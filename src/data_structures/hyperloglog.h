#ifndef HYPERLOGLOG_H
#define HYPERLOGLOG_H

#include <stdint.h>
#include "dstr.h"

#define HASH_BITS_CNT 64
#define HLL_P 14
#define HLL_Q (HASH_BITS_CNT - HLL_P)
#define REGISTER_CNT 16384
#define BIAS_CORRECTION (0.7213 / (1. + 1.079 / REGISTER_CNT))
#define HLL_HEADER_SIZE_BYTES 16
#define HLL_DENSE_SIZE_BYTES (6 * REGISTER_CNT / 8) // 2^14 (16384) 6 bit register
#define HLL_SPARSE_MAX_BYTES 3000

enum HLLEncoding {
    HLL_DENSE = 0,
    HLL_SPARSE = 1
};

void hll_init(dstr **phll);
uint8_t hll_add(dstr **phll, dstr *val);
uint64_t hll_count(dstr *hll);
void hll_merge(dstr *hll1, dstr *hll2);

#endif
