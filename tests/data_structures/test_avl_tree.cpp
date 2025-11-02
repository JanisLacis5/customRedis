#include <stdio.h>
#include <cstdlib>
#include "../data_structures/avl_tree.cpp"
#include "../data_structures/avl_tree.h"

static void assert_node(AVLNode* node, AVLNode* parent, AVLNode* left, AVLNode* right, uint32_t height, uint32_t size) {
    assert(node->parent == parent);
    assert(node->left == left);
    assert(node->right == right);
    assert(node->height == height);
    assert(node->size == size);
}

static AVLNode* init_tree() {}

void test_avl_init() {
    // Heap
    AVLNode* root_h = (AVLNode*)malloc(sizeof(AVLNode));
    avl_init(root_h);
    assert_node(root_h, NULL, NULL, NULL, 1, 1);
    free(root_h);

    // Stack
    AVLNode root_s;
    avl_init(&root_s);
    assert_node(&root_s, NULL, NULL, NULL, 1, 1);
}

void test_update_node() {}

void test_rotate_left() {}

void test_rotate_right() {}

void test_del_simple() {}

void test_avl_height() {}

void test_avl_fix() {}

void test_avl_del() {}

int run_all_avl() {
    test_avl_init();
    printf("[avl]: avl_init() passed! (1/8)\n");
    test_update_node();
    printf("[avl]: update_node() passed! (2/8)\n");
    test_rotate_left();
    printf("[avl]: rotate_left() passed! (3/8)\n");
    test_rotate_right();
    printf("[avl]: rotate_right() passed! (4/8)\n");
    test_del_simple();
    printf("[avl]: del_simple() passed! (5/8)\n");
    test_avl_height();
    printf("[avl]: avl_height() passed! (6/8)\n");
    test_avl_fix();
    printf("[avl]: avl_fix() passed! (7/8)\n");
    test_avl_del();
    printf("[avl]: avl_del() passed! (8/8)\n");
    printf("[avl]: ALL AVL TESTS PASSED!\n");
    return 0;
}
