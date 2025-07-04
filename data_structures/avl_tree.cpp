#include <assert.h>
#include <algorithm>

#include "avl_tree.h"

uint32_t avl_height(AVLNode *node) {
    if (!node) {
        return 0;
    }
    return 1 + std::max(avl_height(node->left), avl_height(node->right));
}

static void update_node(AVLNode *node) {
    node->height = 1 + std::max(avl_height(node->left), avl_height(node->right));
    node->size = 1 + avl_size(node->left) + avl_size(node->right);
}

static AVLNode* rotate_left(AVLNode *curr_root) {
    AVLNode *new_root = curr_root->right;
    AVLNode *inner = new_root->left;

    // Update children
    curr_root->right = inner;
    new_root->left = curr_root;

    // Update parent
    new_root->parent = curr_root->parent;
    curr_root->parent = new_root;

    // Update children parents
    if (inner) {
        inner->parent = curr_root;
    }

    update_node(curr_root);
    update_node(new_root);
    return new_root;
}

static AVLNode* rotate_right(AVLNode *curr_root) {
    AVLNode *new_root = curr_root->left;
    AVLNode *inner = new_root->right;

    // Update children
    curr_root->left = new_root->right;
    new_root->right = curr_root;

    // Update parent
    new_root->parent = curr_root->parent;
    curr_root->parent = new_root;

    // Update inner parent
    if (inner) {
        inner->parent = curr_root;
    }

    update_node(curr_root);
    update_node(new_root);
    return new_root;
}

static AVLNode* del_simple(AVLNode *to_delete) {
    // This function can only be called if the node has only one child
    assert(!to_delete->left || !to_delete->right);

    AVLNode *parent = to_delete->parent;
    AVLNode *child = to_delete->left ? to_delete->left : to_delete->right;
    if (child) {
        child->parent = parent;
    }
    if (!parent) {
        return child;
    }

    if (parent->left == to_delete) {
        parent->left = child;
    }
    else {
        parent->right = child;
    }
    return avl_fix(parent);
}

void avl_init(AVLNode *node) {
    node->parent = NULL;
    node->left = NULL;
    node->right = NULL;
    node->height = 1;
    node->size = 1;
}

// Initially called on the updated node, walks up to the root
AVLNode* avl_fix(AVLNode *node) {
    while (true) {
        // Fixed tree
        AVLNode **from = &node;
        AVLNode *parent = node->parent;
        if (parent) {
            from = parent->left == node ? &parent->left : &parent->right;
        }

        uint32_t left = avl_height(node->left);
        uint32_t right = avl_height(node->right);
        update_node(node);
        if (left == right + 2) {
            if (avl_height(node->left->left) < avl_height(node->left->right)) {
                node->left = rotate_left(node->left);
            }
            node = rotate_right(node);
        }
        else if (right == left + 2) {
            if (avl_height(node->right->right) < avl_height(node->right->left)) {
                node->right = rotate_right(node->right);
            }
            node = rotate_left(node);
        }

        *from = node;
        if (!parent) {
            return node;
        }
        node = parent;
    }
}

AVLNode* avl_del(AVLNode *to_delete) {
    if (!to_delete->left || !to_delete->right) {
        return del_simple(to_delete);
    }

    // Find the next largest node in the tree
    AVLNode *victim = to_delete->right;
    while (victim->left) {
        victim = victim->left;
    }
    AVLNode *root = del_simple(victim);

    // Swap victim with the deletable node
    *victim = *to_delete; // gives left, right, parent

    // Update children parent
    if (victim->left) {
        victim->left->parent = victim;
    }
    if (victim->right) {
        victim->right->parent = victim;
    }

    // Attach victim to the parent
    AVLNode *parent = victim->parent;
    AVLNode **from = &root;
    if (parent) {
        from = parent->left == to_delete ? &parent->left : &parent->right;
    }
    *from = victim;

    return avl_fix(root);
}