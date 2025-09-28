#ifndef DARRAY_H 
#define DARRAY_H
#include <stdlib.h>

struct darr_header {
    size_t size;
    size_t free;
};

#define darr_init(array_ptr, size, type_size) \
    array_ptr = malloc

#define darr_append(array, curr_size, capacity, type_size) \
    



#endif
