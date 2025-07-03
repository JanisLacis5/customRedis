#include <vector>
#include "heap.h"

#include <algorithm>

static size_t heap_left(uint32_t pos) {
    return pos * 2 + 1;
}

static size_t heap_right(uint32_t pos) {
    return pos * 2 + 2;
}

static size_t heap_parent(uint32_t pos) {
    return (pos - 1) >> 1;
}

void heap_up(std::vector<HeapNode*> &heap, size_t pos) {
    if (pos == 0) {
        return;
    }

    size_t parent = heap_parent(pos);
    if (heap[parent]->val > heap[pos]->val) {
        std::swap(heap[parent], heap[pos]);
        heap_up(heap, parent);
    }
}

void heap_down(std::vector<HeapNode*> &heap, size_t pos) {
    if (pos >= heap.size()) {
        return;
    }

    size_t left_ch = heap_left(pos);
    size_t right_ch = heap_right(pos);

    if (left_ch < heap.size()) {
        int64_t min_val = heap[left_ch]->val;
        if (right_ch < heap.size()) {
            min_val = std::min(min_val, heap[right_ch]->val);
        }

        if (heap[pos]->val > min_val) {
            size_t next_pos = min_val == heap[left_ch]->val ? left_ch : right_ch;
            std::swap(heap[pos], heap[next_pos]);
            heap_down(heap, next_pos);
        }
    }
}

void heap_fix(std::vector<HeapNode*> &heap, size_t pos) {
    HeapNode *node = heap[pos];
    size_t parent_pos = heap_parent(pos);
    size_t left_ch = heap_left(pos);
    size_t right_ch = heap_right(pos);

    // Fix parent being larger
    heap_up(heap, pos);

    // Fix node being larger than children
    if (left_ch < heap.size()) {
        int64_t min_val = heap[left_ch]->val;
        if (parent_pos < heap.size()) {
            min_val = std::min(min_val, heap[parent_pos]->val);
        }

        if (node->val > min_val) {
            heap_down(heap, pos);
        }
    }
}


