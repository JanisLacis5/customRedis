#include <assert.h>
#include <cstdlib>
#include <string.h>
#include "../../data_structures/dstr.h"


int main() {
    for (int len = 0; len < 10; len++) {
        dstr *str = dstr_init(len);
        assert(str->size == 0);
        assert(str->free == len);
        assert(str->buf[0] == '\0');
        assert(!strcmp(str->buf, ""));
        free(str);
    }
}
