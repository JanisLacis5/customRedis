#include <vector>
#include "../data_structures/heap.h"

#include <algorithm>

static size_t heap_left(size_t pos) {
    return pos * 2 + 1;
}

static size_t heap_right(size_t pos) {
    return pos * 2 + 2;
}

static int32_t heap_parent(size_t pos) {
    return (pos - 1) >> 1;
}

void heap_up(std::vector<HeapNode> &heap, size_t pos) {
    HeapNode node = heap[pos];
    while (pos > 0 && node.val <= heap[heap_parent(pos)].val) {
        size_t parent = heap_parent(pos);
        heap[pos] = heap[parent];
        *heap[pos].pos_ref = pos;
        pos = parent;
    }
    heap[pos] = node;
    *heap[pos].pos_ref = pos;
}

void heap_down(std::vector<HeapNode> &heap, size_t pos) {
    size_t size = heap.size();
    HeapNode node = heap[pos];

    while (pos < size) {
        size_t left_ch = heap_left(pos);
        size_t right_ch = heap_right(pos);
        uint64_t min_val = node.val;

        if (left_ch < size) min_val = std::min(min_val, heap[left_ch].val);
        if (right_ch < size) min_val = std::min(min_val, heap[right_ch].val);

        if (node.val == min_val) {
            break;
        }

        size_t next_pos = left_ch;
        if (right_ch < heap.size() && heap[right_ch].val < heap[left_ch].val) {
            next_pos = left_ch;
        }

        heap[pos] = heap[next_pos];
        *heap[pos].pos_ref = pos;
        pos = next_pos;
    }
    heap[pos] = node;
    *heap[pos].pos_ref = pos;
}

void heap_fix(std::vector<HeapNode> &heap, size_t pos) {
    HeapNode node = heap[pos];
    size_t parent_pos = heap_parent(pos);
    if (pos > 0 && heap[parent_pos].val > heap[pos].val) {
        heap_up(heap, pos);
    }
    else if (pos < heap.size()) {
        heap_down(heap, pos);
    }
}


