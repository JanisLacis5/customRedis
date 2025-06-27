#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <cstddef>
#include <stdint.h>

struct AVLNode {
    AVLNode *parent = NULL;
    AVLNode *left = NULL;
    AVLNode *right = NULL;
    uint32_t height = 0;  // height of the tree
    uint32_t size = 0;  // no. of nodes in the tree
};

inline uint32_t avl_size(AVLNode *node) {
    return node ? node->size : 0;
}

void avl_init(AVLNode *node);
uint32_t avl_height(AVLNode *node);
AVLNode* avl_fix(AVLNode *root);
AVLNode* avl_del(AVLNode *root, AVLNode *to_delete, bool (*is_smaller)(AVLNode*, AVLNode*));

#endif //AVL_TREE_H
