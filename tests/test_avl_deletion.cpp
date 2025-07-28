#include <algorithm>
#include <assert.h>
#include <cstdio>
#include <vector>
#include <set>

#include "../data_structures/avl_tree.h"

#define container_of(ptr, T, member) ((T *)( (char *)ptr - offsetof(T, member) ))

struct Data {
    AVLNode node;
    uint32_t data = 0;
};

struct Container {
    AVLNode *root = NULL;
};

static int avl_verify(AVLNode *parent, AVLNode *node) {
    if (!node) {
        return 0;
    }

    // verify parent
    assert(node->parent == parent);
    avl_verify(node, node->left);
    avl_verify(node, node->right);

    // verify size
    assert(node->size == 1 + avl_size(node->left) + avl_size(node->right));

    // verify height
    uint32_t l = avl_height(node->left);
    uint32_t r = avl_height(node->right);
    assert(l == r || l == r + 1 || r == l + 1);
    assert(node->height == 1 + std::max(l, r));

    // verify value
    uint32_t val = container_of(node, Data, node)->data;
    if (node->left) {
        uint32_t l_val = container_of(node->left, Data, node)->data;
        assert(l_val < val);
    }
    if (node->right) {
        uint32_t r_val = container_of(node->right, Data, node)->data;
        assert(r_val > val);
    }
    return 0;
}

static void add(Container &c, const uint32_t &val) {
    Data *data = new Data();
    avl_init(&data->node);
    data->data = val;

    AVLNode **from = &c.root;
    AVLNode *curr = NULL;
    while (*from) {
        curr = *from;
        uint32_t cval = container_of(curr, Data, node)->data;
        from = cval < data->data ? &curr->right : &curr->left;
    }
    *from = &data->node;
    data->node.parent = curr;
    avl_fix(&data->node);
}

static bool del(Container &c, const uint32_t &val) {
    AVLNode *curr = c.root;
    while (curr) {
        uint32_t cval = container_of(curr, Data, node)->data;
        if (val == cval) {
            break;
        }
        curr = val < cval ? curr->left : curr->right;
    }
    if (!curr) {
        return false;
    }

    avl_del(curr);
    if (curr == c.root) {
        c.root = curr->parent;
    }
    delete container_of(curr, Data, node);
    return true;
}

static void dispose(Container &c) {
    while (c.root) {
        Data *data = container_of(c.root, Data, node);
        avl_del(c.root);
        c.root = c.root->parent;
        delete data;
    }
}

// Test deletion of leaf nodes
static void test_leaf_deletion() {
    Container c;
    
    // Create tree: 10, 5, 15, 3, 7, 12, 20
    add(c, 10);
    add(c, 5);
    add(c, 15);
    add(c, 3);
    add(c, 7);
    add(c, 12);
    add(c, 20);
    
    avl_verify(NULL, c.root);
    assert(avl_size(c.root) == 7);
    
    // Delete leaf nodes
    assert(del(c, 3) == true);
    avl_verify(NULL, c.root);
    assert(avl_size(c.root) == 6);
    
    assert(del(c, 7) == true);
    avl_verify(NULL, c.root);
    assert(avl_size(c.root) == 5);
    
    assert(del(c, 12) == true);
    avl_verify(NULL, c.root);
    assert(avl_size(c.root) == 4);
    
    assert(del(c, 20) == true);
    avl_verify(NULL, c.root);
    assert(avl_size(c.root) == 3);
    
    dispose(c);
}

// Test deletion of nodes with one child
static void test_single_child_deletion() {
    Container c;
    
    // Create unbalanced tree to ensure single child scenarios
    add(c, 10);
    add(c, 5);
    add(c, 15);
    add(c, 3);
    add(c, 12);
    add(c, 20);
    add(c, 1);
    
    avl_verify(NULL, c.root);
    
    // Delete node with one child (5 has only left child 3->1)
    assert(del(c, 5) == true);
    avl_verify(NULL, c.root);
    
    // Delete node with one child (15 could have right child after rebalancing)
    assert(del(c, 15) == true);
    avl_verify(NULL, c.root);
    
    dispose(c);
}

// Test deletion of root node
static void test_root_deletion() {
    Container c;
    
    // Single node tree
    add(c, 10);
    assert(del(c, 10) == true);
    assert(c.root == NULL);
    
    // Two node tree
    add(c, 10);
    add(c, 5);
    assert(del(c, 10) == true);
    avl_verify(NULL, c.root);
    assert(avl_size(c.root) == 1);
    
    dispose(c);
}

// Test deletion causing rebalancing
static void test_deletion_rebalancing() {
    Container c;
    
    // Create a tree that will need rebalancing after deletion
    std::vector<uint32_t> values = {50, 30, 70, 20, 40, 60, 80, 10, 25, 35, 45};
    
    for (uint32_t val : values) {
        add(c, val);
    }
    
    avl_verify(NULL, c.root);
    uint32_t initial_size = avl_size(c.root);
    
    // Delete nodes that should trigger rebalancing
    assert(del(c, 10) == true);
    avl_verify(NULL, c.root);
    
    assert(del(c, 25) == true);
    avl_verify(NULL, c.root);
    
    assert(del(c, 30) == true);
    avl_verify(NULL, c.root);
    
    assert(avl_size(c.root) == initial_size - 3);
    
    dispose(c);
}

// Test random deletion pattern
static void test_random_deletions() {
    Container c;
    std::set<uint32_t> reference;
    
    // Add many values
    for (uint32_t i = 1; i <= 100; i++) {
        add(c, i);
        reference.insert(i);
    }
    
    avl_verify(NULL, c.root);
    assert(avl_size(c.root) == 100);
    
    // Delete every 3rd element
    for (uint32_t i = 3; i <= 100; i += 3) {
        assert(del(c, i) == true);
        reference.erase(i);
        avl_verify(NULL, c.root);
        assert(avl_size(c.root) == reference.size());
    }
    
    // Delete some more elements
    std::vector<uint32_t> to_delete = {1, 5, 7, 11, 13, 17, 19, 23, 29};
    for (uint32_t val : to_delete) {
        if (reference.count(val)) {
            assert(del(c, val) == true);
            reference.erase(val);
            avl_verify(NULL, c.root);
            assert(avl_size(c.root) == reference.size());
        }
    }
    
    dispose(c);
}

// Test deletion of non-existent elements
static void test_nonexistent_deletion() {
    Container c;
    
    // Empty tree
    assert(del(c, 10) == false);
    
    // Tree with some elements
    add(c, 10);
    add(c, 5);
    add(c, 15);
    
    assert(del(c, 20) == false);  // Greater than all
    assert(del(c, 1) == false);   // Less than all
    assert(del(c, 7) == false);   // In between existing
    
    avl_verify(NULL, c.root);
    assert(avl_size(c.root) == 3);
    
    dispose(c);
}

int main() {
    printf("Testing AVL tree deletion...\n");
    
    test_leaf_deletion();
    printf("✓ Leaf deletion tests passed\n");
    
    test_single_child_deletion();
    printf("✓ Single child deletion tests passed\n");
    
    test_root_deletion();
    printf("✓ Root deletion tests passed\n");
    
    test_deletion_rebalancing();
    printf("✓ Deletion rebalancing tests passed\n");
    
    test_random_deletions();
    printf("✓ Random deletion tests passed\n");
    
    test_nonexistent_deletion();
    printf("✓ Non-existent deletion tests passed\n");
    
    printf("All AVL deletion tests passed!\n");
    return 0;
}