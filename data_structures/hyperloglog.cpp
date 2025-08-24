#include <cstdint>
#include <math.h>
#include <string.h>
#include "hyperloglog.h"
#include "utils/common.h"

static inline uint8_t get_reg(dstr *hll, uint32_t reg_no) {

}

static inline void set_reg(dstr *hll, uint32_t req_no, uint32_t value) {

}

static uint32_t cnt_zero_regs(dstr *hll) {
    uint32_t zero_reg_cnt = 0;
    for (uint32_t reg_no = 0; reg_no < REGISTER_CNT; reg_no++) {
        uint8_t reg = get_reg(hll, reg_no);
        if (!reg) {
            zero_reg_cnt++;
        }
    }

    return zero_reg_cnt;
}

static bool cache_valid(dstr *hll) {
    return !(hll->buf[15] & (1u << 7));
}

static uint64_t get_cache(dstr *hll) {

}

static void set_cache(dstr *hll, uint64_t value) {

}

static void invalidate_cache(dstr *hll) {
    hll->buf[15] |= (1 << 7);
}

static void validate_cache(dstr *hll) {
    hll->buf[15] &= ~(1 << 7);
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

void hll_init(dstr **hll_p) { // assumes hll not initialized (hll == NULL)
    dstr *hll = dstr_init(HLL_HEADER_SIZE_BYTES + HLL_DENSE_SIZE_BYTES);
    
    // This is not a typical string so the size is set and there are no free characters
    hll->size = hll->free;
    hll->free = 0;

    // Zero-out all registers
    memset(hll->buf + HLL_HEADER_SIZE_BYTES, 0, HLL_DENSE_SIZE_BYTES);

    // Create the header
    dstr_append(&hll, "HYLL", 4); 
    hll->buf[4] = HLL_DENSE;
    for (uint32_t i = 8; i < 16; i++) {
        hll->buf[i] = 0;
    }
    hll->buf[15] |= (1u << 7); // set the msb so the cached cardinality is invalid
    
    *hll_p = hll;
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
    estimate += 0.5l; // 0.5l for rounding when casted to int

    set_cache(hll, (uint64_t)estimate);
    return estimate;
}

uint8_t hll_add(dstr *hll, dstr *val) {
    uint64_t hash = str_hash((uint8_t*)val->buf, val->size);
    uint32_t reg_no = hash >> HLL_Q;
    uint64_t experimental = hash << HLL_P;
    
    // Count leading zeroes + 1
    uint8_t zeroes = 1;
    for (; zeroes <= HLL_Q; zeroes++) {
        if (experimental & (1ull << 63)) {
            break; // 1-bit found
        }
        experimental <<= 1;
    }

    uint8_t reg = get_reg(hll, reg_no);
    if (reg < zeroes) {
        set_reg(hll, reg_no, zeroes);
        invalidate_cache(hll); // cached value is not valid because hll has changed 
        return 1;
    }
        
    validate_cache(hll); // cached value is valid because hll did not change
    return 0;
}

void hll_merge(dstr *hll1, dstr *hll2) {

}
