#include "avl_tree.h"

void avl_init(AVLNode *node) {
    node->parent = NULL;
    node->left = NULL;
    node->right = NULL;
    node->height = 1;
    node->size = 1;
}
