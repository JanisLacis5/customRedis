#include "hashmap.h"
#include <assert.h>
#include <cstdlib>

void h_init(HTab *htab, size_t n) {
    // Assert that n is a power of 2
    assert(n > 0 && ((n-1) & n == 0));

    htab->tab = (HNode**)calloc(n, sizeof(HNode));
    htab->mask = n-1;
    htab->size = 0;
}

// Returns the &P->next where P is the previous node before `node`
HNode** ht_lookup(HTab *htab, HNode *node, bool (*eq)(HNode*, HNode*)) {
    if (!htab->tab) {
        return NULL;
    }

    size_t pos = node->hcode & htab->mask;
    HNode **slot = &htab->tab[pos];
    while (*slot) {
        HNode *curr = *slot;
        if (curr->hcode == node->hcode && eq(curr, node)) {
            return slot;
        }
        slot = &curr->next;
    }

    return NULL;
}

HNode* ht_delete(HTab* htab, HNode **from) {
    HNode *to_delete = *from;
    *from = to_delete->next;
    htab->size--;
    return to_delete;
}

HNode* hm_lookup(HMap *hmap, HNode* key, bool (*eq)(HNode*, HNode*)) {

}

void ht_insert(HTab *htab, HNode *node) {
    size_t pos = node->hcode & htab->mask;
    HNode *next = htab->tab[pos];
    node->next = next;
    htab->tab[pos] = node;
    htab->size++;
}

void hm_insert(HMap* hmap, HNode* node) {

}

HNode* hm_delete(HMap* hmap, HNode* key, bool (*eq)(HNode*, HNode*)) {

}

void hm_clear(HMap* hmap) {

}

size_t hm_size(HMap* hmap) {
    return hmap->newer.size() + hmap->older.size();
}




