#include <cstdint>
#include <math.h>
#include <string.h>
#include <assert.h>
#include "hyperloglog.h"
#include "utils/common.h"

static inline uint8_t get_reg(dstr *hll, uint32_t reg_no) {
    // Get the position of the byte where the first bit of the register is located
    uint32_t b0 = 6 * reg_no / 8 + HLL_HEADER_SIZE_BYTES;
    uint8_t fb = 6 * reg_no % 8; // first bit idx in the byte (lsb = 0)
    
    uint32_t buf0 = hll->buf[b0];
    uint32_t buf1 = 0;
    if (b0 + 1 < hll->size) {
        buf1 = hll->buf[b0 + 1];
    }

    // Get the register at the 6 lsb + garbage at 2 msb
    uint32_t reg = (buf0 >> fb) | (buf1 << (8 - fb));
    
    // Clamp the output to uint8_t 
    return reg & UINT8_MAX;
} 

static inline void set_reg(dstr *hll, uint32_t req_no, uint32_t value) {

}

static bool cache_valid(dstr *hll) {
    return !(hll->buf[15] & (1u << 7));
}

static void invalidate_cache(dstr *hll) {
    hll->buf[15] |= (1 << 7);
}

static void validate_cache(dstr *hll) {
    hll->buf[15] &= ~(1 << 7);
}

static void set_cache(dstr *hll, uint64_t value) {
    assert(value < (1ull << 63)); // Value has to fit in 63 bits
    
    // Because value fits in 63 bits, msb of the value will be a 0-bit therefore
    // cache flag will be unset which means that it is valid (expected)
    for (uint8_t i = 8; i < 16; i++) {
        // Clear the byte
        hll->buf[i] = value & 0xFF;
        value >>= 8;
    }
}

static uint64_t get_cache(dstr *hll) {
    assert(cache_valid(hll)); // if cache is invalid, this function returns garbage
    
    uint64_t cache = 0;
    for (uint8_t i = 0; i < 8; i++) {
        cache |= (uint64_t)hll->buf[8 + i] << (8 * i);
    }
    
    return cache;
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
