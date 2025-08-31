#include "data_structures/test_avl_tree.cpp"
#include "data_structures/test_dlist.cpp"
#include "data_structures/test_dstr.cpp"
#include "data_structures/test_hashmap.cpp"
#include "data_structures/test_heap.cpp"
#include "data_structures/test_hyperloglog.cpp"
#include "data_structures/test_zset.cpp"

int main() {
    run_all_hll();
    run_all_dstr();
}
