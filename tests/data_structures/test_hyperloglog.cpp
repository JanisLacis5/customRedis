#include "../../data_structures/hyperloglog.cpp"
#include "hyperloglog.h"

// === Small helpers for crafting flags ===
static uint8_t make_val_flag(uint8_t val /*1..32*/, uint8_t cnt /*1..4*/) {
    assert(val >= 1 && val <= 32);
    assert(cnt >= 1 && cnt <= 4);
    return (uint8_t)((1u<<7) | ((uint8_t)(val - 1) << 2) | (uint8_t)(cnt - 1));
}

static uint8_t make_zero_flag(uint32_t cnt /*1..64*/) {
    assert(cnt >= 1 && cnt <= 64);
    return (uint8_t)(cnt - 1);
}

static void write_xzero_bytes(uint8_t *msb, uint8_t *lsb, uint32_t cnt /*1..16384*/) {
    assert(cnt >= 1 && cnt <= 16384);
    uint32_t raw = cnt - 1;
    uint8_t hi6  = (raw >> 8) & 63;
    *msb = ((1u << 6) | hi6);
    *lsb = (raw & 255);
}

// === GENERAL HELPER FUNCTION TESTS
void test_get_enc() {
    dstr *hll = NULL;
    hll_init(&hll);

    assert(get_enc(hll) == HLL_SPARSE);

    hll->buf[4] = HLL_DENSE;
    assert(get_enc(hll) == HLL_DENSE);
    
    free(hll);
}

void test_set_enc() {
    dstr *hll = NULL;
    hll_init(&hll);

    set_enc(hll, HLL_DENSE);
    assert(hll->buf[4] == HLL_DENSE);

    set_enc(hll, HLL_SPARSE);
    assert(hll->buf[4] == HLL_SPARSE);

    free(hll);
}

void test_cache_valid_and_invalidate() {
    dstr *hll = NULL;
    hll_init(&hll);

    // Manually validate cache (clear bit7 at byte 15)
    hll->buf[15] &= ~(1u << 7);
    assert(cache_valid(hll));

    // Invalidate and check
    invalidate_cache(hll);
    assert(!cache_valid(hll));

    // Clear again and re-check
    hll->buf[15] &= ~(1u << 7);
    assert(cache_valid(hll));

    free(hll);
}

void test_is_val_and_is_zero() {
    uint8_t f_val = make_val_flag(5, 3);
    assert(is_val(f_val));
    assert(!is_zero(f_val));

    uint8_t f_zero = make_zero_flag(1);
    assert(!is_val(f_zero));
    assert(is_zero(f_zero));

    uint8_t msb, lsb;
    write_xzero_bytes(&msb, &lsb, 100);
    assert(!is_val(msb));
    assert(!is_zero(msb));
}

void test_val_value_and_val_cnt() {
    uint8_t f1 = make_val_flag(1, 1);
    assert(val_value(f1) == 1);
    assert(val_cnt(f1) == 1);

    uint8_t f2 = make_val_flag(32, 4);
    assert(val_value(f2) == 32);
    assert(val_cnt(f2) == 4);

    uint8_t f3 = make_val_flag(12, 2);
    assert(val_value(f3) == 12);
    assert(val_cnt(f3) == 2);
}

void test_zero_cnt() {
    assert(zero_cnt(make_zero_flag(1)) == 1);
    assert(zero_cnt(make_zero_flag(64)) == 64);

    assert(zero_cnt(make_zero_flag(7)) == 7);
    assert(zero_cnt(make_zero_flag(33)) == 33);
}

void test_xzero_cnt() {
    uint8_t msb, lsb;

    write_xzero_bytes(&msb, &lsb, 1);
    assert(xzero_cnt(msb, lsb) == 1);

    write_xzero_bytes(&msb, &lsb, 16384);
    assert(xzero_cnt(msb, lsb) == 16384);

    // Check that masking out bit6 works (i.e., msb may have bit6 set and we still decode)
    write_xzero_bytes(&msb, &lsb, 256);
    assert(xzero_cnt(msb, lsb) == 256);

    // A couple of mid values
    write_xzero_bytes(&msb, &lsb, 1337);
    assert(xzero_cnt(msb, lsb) == 1337);

    write_xzero_bytes(&msb, &lsb, 8192);
    assert(xzero_cnt(msb, lsb) == 8192);
}

void test_set_cache_and_get_cache() {
    dstr *hll = NULL;
    hll_init(&hll);

    // After set_cache, cache must be valid and round-trip should succeed.
    uint64_t vals[] = {
        0ull,
        1ull,
        123456789ull,
        (1ull<<40) + 12345ull,
        (1ull<<63) - 1ull // max allowed by your assert
    };
    for (size_t i = 0; i < sizeof(vals)/sizeof(vals[0]); i++) {
        set_cache(hll, vals[i]);
        assert(cache_valid(hll));           // top bit of byte 15 must be 0
        uint64_t got = get_cache(hll);
        assert(got == vals[i]);
    }

    // If we invalidate, we must not call get_cache (it asserts).
    invalidate_cache(hll);
    assert(!cache_valid(hll));

    free(hll);
}

int main(void) {
    test_get_enc();
    test_set_enc();
    test_cache_valid_and_invalidate();
    test_is_val_and_is_zero();
    test_val_value_and_val_cnt();
    test_zero_cnt();
    test_xzero_cnt();
    test_set_cache_and_get_cache();
    return 0;
}

