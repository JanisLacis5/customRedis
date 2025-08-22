#include <assert.h>
#include <cstdint>
#include <cstdlib>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "data_structures/dstr.h"

static void rand_str(char *dest, size_t length) {
    char charset[] = "0123456789"
                     "abcdefghijklmnopqrstuvwxyz"
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    while (length-- > 0) {
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        *dest++ = charset[index];
    }
    *dest = '\0';
}

static void test_dstr_init() {
    size_t too_large_size = (1ull << 31);
    size_t rand_size = rand() % MAX_STR_SIZE;
    
    // TOO LARGE
    errno = 0;
    dstr *str = dstr_init(too_large_size);
    assert(errno == STR_ERR_TOO_LARGE);
        
    // OK
    str = dstr_init(rand_size);
    assert(str->size == 0);
    assert(str->free == rand_size);
    assert(str->buf[0] == '\0');
}

static void test_dstr_cap() {
    size_t rand_size = rand() % MAX_STR_SIZE;
    size_t rand_str_size = rand() % rand_size;

    dstr *str = dstr_init(rand_size);
    size_t str_cap = dstr_cap(str);
    assert(str_cap == rand_size);

    // Add a random string
    rand_str(str->buf, rand_str_size);
    assert(str->size == rand_str_size);
    assert(str->free == rand_size - rand_str_size);
    assert(dstr_cap(str) == rand_size);
} 

static void test_dstr_append() {
    size_t too_large_size = (1ull << 31);

    // TOO LARGE
    dstr *str = dstr_init(0);
    uint32_t err = dstr_append(&str, "", too_large_size); // "" because error is expected
    assert(err == STR_ERR_TOO_LARGE);

    // OK
    char to_append[] = "just a string";
    size_t to_append_size = strlen(to_append);

    err = dstr_append(&str, to_append, to_append_size);
    assert(err == STR_OK);
    assert(str->size == to_append_size);
    assert(!strcmp(str->buf, to_append));
}

int main() {
    srand(time(NULL));
    size_t too_large_size = (1ull << 31);

    // Test if the string overall works with different sizes
    for (int len = 0; len < 10; len++) {
        dstr *str = dstr_init(len);
        assert(str->size == 0);
        assert(str->free == len);
        assert(str->buf[0] == '\0');
        assert(!strcmp(str->buf, ""));
        free(str);
    }

    // Each function individually
    test_dstr_init();
    test_dstr_cap();
    test_dstr_append();
}
