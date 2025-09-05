#include <assert.h>
#include <stdio.h>
#include <cstdint>
#include <cstdlib>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "../../data_structures/dstr.h"

const size_t too_large_size = MAX_STR_SIZE + 1;

static void assert_str(dstr *str, size_t size, size_t free) {
    assert(str->size == size);
    assert(str->free == free);
    assert(str->buf[size] == '\0');
}

static void test_dstr_init(size_t size) {
    // TOO LARGE
    errno = 0;
    dstr *str = dstr_init(too_large_size);
    assert(errno == STR_ERR_TOO_LARGE);
    assert(str == NULL); 

    // OK
    str = dstr_init(size);
    assert_str(str, 0, size);
    free(str);
}

static void test_dstr_cap(size_t size) {
    dstr *str = dstr_init(size);
    size_t str_cap = dstr_cap(str);
    assert(str_cap == size);
    free(str);
} 

static void test_dstr_append() {
    // TOO LARGE
    dstr *str = dstr_init(0);
    uint32_t err = dstr_append(&str, "", too_large_size); // "" because error is expected
    assert(err == STR_ERR_TOO_LARGE);
    assert_str(str, 0, 0);

    // OK
    char to_append[] = "just a string";
    size_t to_append_size = strlen(to_append);

    err = dstr_append(&str, to_append, to_append_size);
    assert(err == STR_OK);
    assert(str->size == to_append_size);
    assert(!strcmp(str->buf, to_append));
    free(str);
}

static void test_dstr_resize_grow(size_t resize_to) {
    // TOO LARGE
    dstr *str = dstr_init(0);
    uint32_t err = dstr_resize(&str, too_large_size, '\0');
    assert(err == STR_ERR_TOO_LARGE);
    assert_str(str, 0, 0);

    // OK
    err = dstr_resize(&str, resize_to, 'j');
    assert(err == STR_OK);
    for (size_t i = 0; i < resize_to; i++) {
        assert(str->buf[i] == 'j');
    }
    assert(str->buf[resize_to] == '\0');
    free(str);
}

static void test_dstr_resize_shrink(size_t resize_to) {
    // TOO LARGE
    dstr *str = dstr_init(MAX_STR_SIZE);
    uint32_t err = dstr_resize(&str, too_large_size, '\0');
    assert(err == STR_ERR_TOO_LARGE);
    assert_str(str, 0, MAX_STR_SIZE);

    // OK
    err = dstr_resize(&str, resize_to, 'j');
    assert(err == STR_OK);
    for (size_t i = 0; i < resize_to; i++) {
        assert(str->buf[i] == 'j');
    }
    assert(str->buf[resize_to] == '\0');
    free(str);
}

static void test_dstr_assign(size_t size) {
    dstr *str = dstr_init(0);

    // TOO LARGE
    uint32_t err = dstr_assign(&str, "", too_large_size);
    assert(err == STR_ERR_TOO_LARGE);
    assert_str(str, 0, 0);

    // OK
    char *tmp = (char*)malloc(size + 1);
    memset(tmp, 'j', size);
    tmp[size] = '\0';

    err = dstr_assign(&str, tmp, size);
    assert(err == STR_OK);
    assert_str(str, size, str->free);
    free(tmp);
    free(str);
}

int run_all_dstr() {
    srand(time(NULL));

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
    size_t size_grid[] = {1, 100, (1 << 15), MAX_STR_SIZE};
    for (size_t size: size_grid) {
        test_dstr_init(size);
        printf("[dstr]: dstr_init() passed! (1/6)\n");
        test_dstr_cap(size);
        printf("[dstr]: dstr_cap() passed! (2/6)\n");
        test_dstr_resize_grow(size);
        printf("[dstr]: dstr_resize_grow() passed! (3/6)\n");
        test_dstr_resize_shrink(size);
        printf("[dstr]: dstr_resize_shrink() passed! (4/6)\n");
        test_dstr_assign(size);
        printf("[dstr]: dstr_assign() passed! (5/6)\n");
    }
    test_dstr_append();
    printf("[dstr]: dstr_append() passed! (6/6)\n");
    printf("[dstr]: ALL DSTR TESTS PASSED!\n");
    return 0;
}
