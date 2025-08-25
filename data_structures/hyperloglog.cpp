#include <cstdint>
#include <math.h>
#include <string.h>
#include <assert.h>
#include "hyperloglog.h"
#include "utils/common.h"

// === GENERAL HELPER FUNCTIONS ===
static uint8_t get_enc(dstr *hll) {
    return hll->buf[4];
}

static void set_enc(dstr *hll, uint8_t enc) {
    hll->buf[4] = enc;
}

static bool cache_valid(dstr *hll) {
    return !(hll->buf[15] & (1u << 7));
}

static void invalidate_cache(dstr *hll) {
    hll->buf[15] |= (1 << 7);
}

static bool is_val(uint8_t flag) {
    return flag & (1u << 7);
}

static bool is_zero(uint8_t flag) {
    return !(flag & (1u << 7)) && !(flag & (1u << 6));
}

static uint8_t val_value(uint8_t flag) {
    flag &= ~(1u << 7);
    return (flag >> 2) + 1;
}

static uint8_t val_cnt(uint8_t flag) {
    flag &= ~(1u << 7);
    return (flag & 3) + 1;
}

static uint8_t zero_cnt(uint8_t flag) {
    return flag + 1;
}

static uint32_t xzero_cnt(uint8_t msb, uint8_t lsb) {
    msb &= ~(1u << 6);
    uint32_t cnt = (msb << 8) | lsb;
    return cnt + 1;
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

// === DENSE HELPER FUNCTIONS ===
static inline uint8_t get_reg(dstr *hll, uint32_t reg_no) {
    // Get the position of the byte where the first bit of the register is located
    uint32_t b0 = 6 * reg_no / 8 + HLL_HEADER_SIZE_BYTES;
    uint8_t fb = 6 * reg_no % 8; // first bit idx in the byte (lsb = 0)
    
    uint32_t buf0 = hll->buf[b0];
    uint32_t buf1 = hll->buf[b0 + 1]; // wont overflow because there is '\0' at hll->size

    // Get the register at the 6 lsb + garbage at 2 msb
    uint32_t reg = (buf0 >> fb) | (buf1 << (8 - fb));
    
    // Clamp the output to uint8_t 
    return reg & UINT8_MAX;
} 

static inline void set_reg(dstr *hll, uint32_t reg_no, uint32_t value) {
    uint32_t b0 = 6 * reg_no / 8 + HLL_HEADER_SIZE_BYTES;
    uint8_t fb = 6 * reg_no % 8;

    uint32_t buf0 = hll->buf[b0];
    uint32_t buf1 = hll->buf[b0 + 1];

    buf0 &= ~(63 << fb);
    buf0 |= value << fb;
    hll->buf[b0] = buf0 & UINT8_MAX; // clamp the output to 8 bit int

    buf1 &= ~(63 >> (8 - fb));
    buf1 |= value >> (8 - fb);
    hll->buf[b0 + 1] = buf1 & UINT8_MAX;
}

static uint8_t add_dense(dstr *hll, uint32_t reg_no, uint8_t val) {
    uint8_t reg = get_reg(hll, reg_no);
    if (reg < val) {
        set_reg(hll, reg_no, val);
        invalidate_cache(hll); // cached value is not valid because hll has changed 
        return 1;
    }

    return 0; 
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

static long double estimate_cnt_d(dstr *hll) {
    long double sum = 0.0l;
    for (uint32_t reg_no = 0; reg_no < REGISTER_CNT; reg_no++) {
        uint8_t curr_reg = get_reg(hll, reg_no);
        sum += 1.0l / (1ull << curr_reg);
    }
    
    long double pref = BIAS_CORRECTION * REGISTER_CNT * REGISTER_CNT;
    return pref * (1.0l / sum);
}

static void dense_init(dstr **target) {
    dstr *hll = dstr_init(HLL_HEADER_SIZE_BYTES + HLL_DENSE_SIZE_BYTES);
    
    // Zero-out all registers
    memset(hll->buf, 0, HLL_HEADER_SIZE_BYTES + HLL_DENSE_SIZE_BYTES);

    // Create the header
    memcpy(hll->buf, "HYLL", 4);
    hll->buf[4] = HLL_DENSE;
    hll->buf[15] |= (1u << 7); // set the msb so the cached cardinality is invalid
    
    // This is not a typical string so the size is set and there are no free characters
    hll->size = HLL_HEADER_SIZE_BYTES + HLL_DENSE_SIZE_BYTES;
    hll->free = 0;
    hll->buf[hll->size] = '\0';

    *target = hll;
}

static void densify(dstr **phll) {
    dstr *hll = *phll;
    if (get_enc(hll) == HLL_DENSE) {
        return;
    }
    
    dstr *dhll = NULL;
    dense_init(&dhll);
    
    uint32_t reg_no = 0;
    for (uint32_t flag_no = HLL_HEADER_SIZE_BYTES; flag_no < hll->size; flag_no++) {
        uint32_t flag = hll->buf[flag_no];
        uint32_t cnt = 0;

        if (is_val(flag)) { 
            uint8_t val = val_value(flag);
            cnt = val_cnt(flag);
            while (cnt--) {
                set_reg(dhll, reg_no, val);
                reg_no++;
            }
        }
        else if (is_zero(flag)) {
            cnt = zero_cnt(flag);
        }
        else { // XZERO
            cnt = xzero_cnt(flag, hll->buf[++flag_no]);
        }
        reg_no += cnt;
    }
    free(*phll);
    *phll = dhll;
}

// === SPARSE HELPER FUNCTIONS ===
static long double estimate_cnt_s(dstr *hll) {
    long double sum = 0.0l;
    for (uint32_t flag_no = HLL_HEADER_SIZE_BYTES; flag_no < hll->size; flag_no++) {
        long double val = 1.0l;
        uint32_t flag = hll->buf[flag_no];
        uint32_t cnt = 0;

        if (is_val(flag)) {
            uint32_t flag_val = val_value(flag);
            cnt = val_cnt(flag);
            val = 1.0l / (1ull << flag_val);
        }
        else if (is_zero(flag)) {
            cnt = zero_cnt(flag);
        }        
        else { // XZERO
            cnt = xzero_cnt(flag, hll->buf[++flag_no]);
        }
        sum += val * cnt;
    }
    
    long double pref = BIAS_CORRECTION * REGISTER_CNT * REGISTER_CNT;
    return pref * (1.0l / sum); 
}

static uint8_t add_sparse(dstr *hll, uint32_t reg_no, uint8_t val) {
    if (val-- > 32) { // decrement val because it will be set as val-1
        densify(&hll);
        return add_dense(hll, reg_no, val);
    }

    uint8_t start = 0;
    uint8_t end = 0;
    uint8_t *prev = NULL;
    uint8_t *curr = NULL;
    uint8_t *next = NULL;
    uint32_t idx = 0;
    uint8_t izero = 0;
    uint8_t ival = 0;
    uint8_t ixzero = 0;
    uint8_t *hll_end = (uint8_t*)hll->buf + hll->size;

    for (uint32_t fn = 0; fn < hll->size; fn++) {
        uint8_t flag = hll->buf[fn];
        start = idx;
        if (is_val(flag)) {
            ival = 1;
            idx += val_cnt(flag);
        }
        else if (is_zero(flag)) {
            izero = 1;
            idx += zero_cnt(flag);
        }
        else {
            ixzero = 1;
            idx += xzero_cnt(flag, hll->buf[++fn]);
        }

        if (idx > reg_no) {
            end = idx - 1;
            curr = &flag;
            if (fn + 1 < hll->size) {
                next = (uint8_t*)&hll->buf[fn + 1];
            }
            break;
        }
        prev = (uint8_t*)&hll->buf[fn];
        ival = 0; izero = 0; ixzero = 0;
    }

    // Nothing changes - register at reg_no has a larger or equal value to val
    if (ival && val_value(*curr) >= val) {
        return 0;
    }
    
    // Current register is a VAL with count 1 and value < val
    if (ival && val_cnt(*curr) == 1) {
        *curr &= ~(31 << 2); // clear the previous value
        *curr |= val << 2; // add the new value
        invalidate_cache(hll);
        return 1;
    }

    // Replace the ZERO flag with count 1 with VAL flag with count 1
    if (izero && zero_cnt(*curr) == 1) {
        *curr = (1u << 7) | (val << 2) | 1;  
    }

    /* General case
        It is guaranteed that currently *curr is VAL with count > 1,
        ZERO with count > 1 or XZERO
        
        1) Split the flag that is holding the current reg_no
        2) Set the 1st half in the tmp array
        3) Set the new val in the tmp array
        4) Set the other half in the tmp array

        tmp array's max size is 5 because the worst case is
        XZERO - VAL - XZERO flag configuration
    */
    uint8_t tmp[5];
    uint8_t *tp = tmp;
    if (ival) {
        uint8_t pval = val_value(*curr);

        // Split the PVAL (previous VAL)
        if (start != reg_no) {
            uint8_t count = reg_no - start;
            *tp++ |= (1u << 7) | (pval << 2) | pval;
        }

        // Set VAL
        *tp++ |= (1u << 7) | (val << 2) | 1;
        
        // Set the other half of PVAL
        if (end != reg_no) {
            uint8_t count = end - reg_no;
            *tp++ |= (1u << 7) | (pval << 2) | count;    
        }
    }
    else {
        // Split the ZERO or XZERO
        if (reg_no != start) {
            uint32_t count = reg_no - start;
            if (count > 64) { // set XZERO
                *tp++ |= (1u << 6) | (val >> 8);
                *tp++ |= val & 255;
            }
            else { // set ZERO
                *tp++ |= val;
            }
        }
        
        // Set val
        *tp++ = (1u << 7) | (val << 2) | 1;

        // Add the other half of the split
        if (reg_no != end) {
            uint32_t count = end - reg_no;
            if (count > 64) { // set XZERO
                *tp++ |= (1u << 6) | (val >> 8);
                *tp++ |= val & 255;
            }
            else { // set ZERO
                *tp++ |= val;
            }
        }
    }

    // Merge changes into hll
    uint8_t tmp_len = tp - tmp;
    uint8_t old_len = ixzero ? 2 : 1;
    uint8_t delta = tmp_len - old_len;

    if (delta) {
        dstr_resize(&hll, delta, 0);
    }
    if (delta && next) {
        memmove(next + delta, next, hll_end - next);
    }
    memcpy(prev + 1, tmp, tmp_len);
    hll_end += delta;
    
    // Merge adjacent VAL flags (try 5 starting from prev)
    uint8_t *p = prev ? prev : (uint8_t*)hll->buf + HLL_HEADER_SIZE_BYTES;
    uint8_t scanlen = 5;

    while (p < hll_end && scanlen--) {
        if (is_zero(*p)) {
            p++;
            continue;
        }
        if (!is_val(*p)) { // XZERO
            p += 2;
            continue;
        }

        if (p + 1 < hll_end && is_val(*(p + 1))) {
            uint8_t v1 = val_value(*p);
            uint8_t v2 = val_value(*(p+1));
            
            if (v1 == v2) {
                uint8_t len = val_cnt(*p) + val_cnt(*(p + 1));
                if (len <= 4) {
                    *p &= 0;
                    *p = (1u << 7) | (v1 << 2) | len;
                    memmove(p, p+1, hll_end - p);
                    hll->buf[--hll->size] = '\0';
                    hll->free++;
                    continue;
                }
            }
        }
        p++;
    }
    invalidate_cache(hll);
    return 1;
}

// === API ===
void hll_init(dstr **hll_p) { // assumes hll not initialized (hll == NULL)
    dstr *hll = dstr_init(HLL_HEADER_SIZE_BYTES + 2); // 2 bytes for XZERO flag
    memset(hll->buf, 0, HLL_HEADER_SIZE_BYTES + 2); 

    // Create the header
    memcpy(hll->buf, "HYLL", 4);
    hll->buf[4] = HLL_SPARSE;
    hll->buf[15] |= (1u << 7); // set the msb so the cached cardinality is invalid
    
    // Set all registers to zero (XZERO FLAG)
    uint32_t runlen = REGISTER_CNT - 1;
    hll->buf[16] = 0b01000000 | (runlen >> 8); // 01 + upper 6 bits
    hll->buf[17] = runlen & 255; // lower 8 bits

    // This is not a typical string so the size is set and there are no free characters
    hll->size = HLL_HEADER_SIZE_BYTES + 2;
    hll->free = 0;
    hll->buf[hll->size] = '\0'; 

    *hll_p = hll;
}

// Helper for choosing the estimate helper function
static long double estimate_cnt(dstr *hll) {
    if (get_enc(hll) == HLL_DENSE) {
        return estimate_cnt_d(hll);
    }
    return estimate_cnt_s(hll);
}

uint64_t hll_count(dstr *hll) {
    // If cache is valid, return cache
    if (cache_valid(hll)) {
        return get_cache(hll);
    }

    long double estimate = estimate_cnt(hll);
    
    // small range correction for dense hll
    if (get_enc(hll) == HLL_DENSE && estimate <= 2.5l * REGISTER_CNT) {
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
    uint8_t enc = get_enc(hll);

    // Count leading zeroes + 1
    uint8_t zeroes = 1;
    for (; zeroes <= HLL_Q; zeroes++) {
        if (experimental & (1ull << 63)) {
            break; // 1-bit found
        }
        experimental <<= 1;
    }

    uint8_t ret = enc == HLL_DENSE ? add_dense(hll, reg_no, zeroes) : add_sparse(hll, reg_no, zeroes);

    if (enc == HLL_SPARSE && hll->size > HLL_SPARSE_MAX_BYTES) {
        densify(&hll);
    }
    return ret;
}

void hll_merge(dstr *dest, dstr *src) {
    bool changed = false;
    for (uint32_t reg_no = 0; reg_no < REGISTER_CNT; reg_no++) {
        uint8_t reg1 = get_reg(dest, reg_no);
        uint8_t reg2 = get_reg(src, reg_no);
        if (reg1 < reg2) {
            changed = true;
        }
        set_reg(dest, reg_no, dmax(reg1, reg2));
    }
    if (changed) {
        invalidate_cache(dest);
    }
}

