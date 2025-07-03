#include <assert.h>
#include <vector>

#include "../heap.h"

std::vector<HeapNode*> heap;

void insert(int64_t val) {
    HeapNode *h = new HeapNode();
    h->val = val;
    heap.push_back(h);
    heap_fix(heap, heap.size()-1);
}

void del(uint32_t pos) {
    std::swap(heap[pos], heap[heap.size()-1]);
    heap.pop_back();
    heap_fix(heap, pos);
}

int main() {
    // Insert one value
    insert(10);
    assert(heap.size() == 1);
    assert(heap[0]->val == 10);

    // Insert a smaller value
    insert(9);
    assert(heap.size() == 2);
    assert(heap[0]->val == 9);

    // Insert larger value
    insert(20);
    assert(heap.size() == 3);
    assert(heap[0]->val == 9);

    // Delete the root of the tree
    del(0);
    assert(heap.size() == 2);
    assert(heap[0]->val == 10);

    // Delete non-root
    del(1);
    assert(heap.size() == 1);
    assert(heap[0]->val == 10);
}