#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

struct HeapNode {
    uint64_t val;  // unsigned because it is used for timeouts
    size_t *pos_ref;  // points to Entry::heap_idx
};

void heap_fix(std::vector<HeapNode> &heap, size_t pos);

#endif
