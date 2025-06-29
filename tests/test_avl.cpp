#include <algorithm>
#include <assert.h>
#include <cstdio>

#include "../avl_tree.h"

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
    // Create data object
    Data *data = new Data();
    avl_init(&data->node);
    data->data = val;

    // Insert the node in the tree
    AVLNode **from = &c.root;
    AVLNode *curr = NULL;
    while (*from) {
        curr = *from;
        uint32_t cval = container_of(curr, Data, node)->data;
        from = cval < data->data ? &curr->left : &curr->right;
    }
    *from = &data->node;
    data->node.parent = curr;
    avl_fix(&data->node);
}

int main() {
    Container c;

    // Add and then remove a value to test an empty tree
    add(c, 1);
    int err = avl_verify(NULL, c.root);
    if (err) {
        printf("Error no 1\n");
        return -1;
    }

    printf("All tests passed!\n");
}