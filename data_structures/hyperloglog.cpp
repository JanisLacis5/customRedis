#include "hyperloglog.h"

static inline uint8_t get_reg(dstr *hll, uint32_t reg_no) {

}

static inline void set_reg(dstr *hll, uint32_t req_no, uint32_t value) {

}

static uint64_t estimate_cnt(dstr *hll) {
    long double sum = 0.0l;
    for (uint32_t reg_no = 0; reg_no < REGISTER_CNT; reg_no++) {
        uint8_t curr_reg = get_reg(hll, reg_no);
        sum += 1.0l / (1ull << curr_reg);
    }
    
    long double pref = BIAS_CORRECTION * REGISTER_CNT * REGISTER_CNT;
    long double estimate = pref * (1.0l / sum);
    return estimate + 0.5l; // rounding
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


