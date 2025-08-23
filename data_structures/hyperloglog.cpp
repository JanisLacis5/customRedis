#include <cstdint>
#include <math.h>
#include "hyperloglog.h"

static inline uint8_t get_reg(dstr *hll, uint32_t reg_no) {

}

static inline void set_reg(dstr *hll, uint32_t req_no, uint32_t value) {

}

static uint32_t cnt_zero_regs(dstr *hll) {

}

static bool cache_valid(dstr *hll) {
    return !(hll->buf[15] & (1u << 7));
}

static uint64_t get_cache(dstr *hll) {

}

static void set_cache(dstr *hll, uint64_t value) {

}

static long double estimate_cnt(dstr *hll) {
    long double sum = 0.0l;
    for (uint32_t reg_no = 0; reg_no < REGISTER_CNT; reg_no++) {
        uint8_t curr_reg = get_reg(hll, reg_no);
        sum += 1.0l / (1ull << curr_reg);
    }
    
    long double pref = BIAS_CORRECTION * REGISTER_CNT * REGISTER_CNT;
    return pref * (1.0l / sum);
}

void hll_init(dstr *hll) { // assumes hll not initialized (hll == NULL)
    hll = dstr_init(HLL_HEADER_SIZE_BYTES + HLL_DENSE_SIZE_BYTES);
    
    // Create the header
    dstr_append(&hll, "HYLL", 4);
    hll->buf[4] = HLL_DENSE;
    for (uint32_t i = 8; i < 16; i++) {
        hll->buf[i] = 0;
    }
    hll->buf[15] |= (1u << 7); // set the msb so the cached cardinality is invalid
}

uint64_t hll_count(dstr *hll) {
    // If cache is valid, return cache
    if (cache_valid(hll)) {
        return get_cache(hll);
    }

    long double estimate = estimate_cnt(hll);

    if (estimate <= 2.5l * REGISTER_CNT) {  // small range correction
        uint32_t zero_regs = cnt_zero_regs(hll);
        if (zero_regs) {
            estimate = logl((long double)REGISTER_CNT / zero_regs) * REGISTER_CNT;
        }
    }

    return estimate + 0.5l; // 0.5l for rounding
}

uint8_t hll_add(dstr *hll, dstr *val) {
    
}

void hll_merge(dstr *hll1, dstr *hll2) {

}
