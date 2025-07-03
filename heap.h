#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

struct HeapNode {
    int64_t val;
};

void heap_fix(std::vector<HeapNode*> &heap, size_t pos);

#endif
