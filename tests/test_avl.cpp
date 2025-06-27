#include <algorithm>
#include <assert.h>
#include "../avl_tree.h"

#define container_of(ptr, T, member) ((T *)( (char *)ptr - offsetof(T, member) ))

struct Data {
    AVLNode node;
    uint32_t data = 0;
};

struct Container {
    AVLNode *root = NULL;
};

static void avl_verify(AVLNode *parent, AVLNode *node) {
    if (!node) {
        return;
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
}

// static void add() {
//
// }

int main() {
    Container c;
    avl_verify(NULL, c.root);
    assert(c.root->size == 0);
}