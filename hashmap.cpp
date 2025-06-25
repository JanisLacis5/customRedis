#include "hashmap.h"
#include <assert.h>
#include <cstdlib>

const size_t MAX_LOAD_FACTOR = 8;
const size_t REHASHING_WORK = 128;

void h_init(HTab *htab, size_t n) {
    // Assert that n is a power of 2
    assert(n > 0 && ((n-1) & n) == 0);

    htab->tab = (HNode**)calloc(n, sizeof(HNode*));
    htab->mask = n-1;
    htab->size = 0;
}

void hm_rehash(HMap *hmap) {
    hmap->older = hmap->newer;
    h_init(&hmap->newer, 2 * (hmap->older.mask + 1));
    hmap->migrate_pos = 0;
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

HNode* hm_lookup(HMap *hmap, HNode* key, bool (*eq)(HNode*, HNode*)) {
    HNode **slot = ht_lookup(&hmap->older, key, eq);
    if (!slot) {
        slot = ht_lookup(&hmap->newer, key, eq);
    }

    return slot ? *slot : NULL;
}

HNode* ht_delete(HTab* htab, HNode **from) {
    HNode *to_delete = *from;
    if (!to_delete) {
        return NULL;
    }
    *from = to_delete->next;
    htab->size--;
    return to_delete;
}

HNode* hm_delete(HMap* hmap, HNode* key, bool (*eq)(HNode*, HNode*)) {
    HNode **slot = ht_lookup(&hmap->older, key, eq);
    if (slot) {
        return ht_delete(&hmap->older, slot);
    }
    slot = ht_lookup(&hmap->newer, key, eq);
    if (slot) {
        return ht_delete(&hmap->newer, slot);
    }
    return NULL;
}

void ht_insert(HTab *htab, HNode *node) {
    size_t pos = node->hcode & htab->mask;
    HNode *next = htab->tab[pos];
    node->next = next;
    htab->tab[pos] = node;
    htab->size++;
}

void hm_rehash_help(HMap *hmap) {
    size_t nwork = 0;
    while (nwork < REHASHING_WORK && hmap->older.size > 0) {
        HNode **slot = &hmap->older.tab[hmap->migrate_pos];
        if (!*slot) {
            hmap->migrate_pos++;
            continue;
        }
        ht_insert(&hmap->newer, ht_delete(&hmap->older, slot));
        nwork++;
    }
    if (hmap->older.tab && hmap->older.size == 0) {
        free(hmap->older.tab);
        hmap->older = HTab{};
    }
}

void hm_insert(HMap* hmap, HNode* node) {
    if (!hmap->newer.tab) {
        h_init(&hmap->newer, 4);
    }
    ht_insert(&hmap->newer, node);

    if (!hmap->older.tab) {
        size_t treshold = (hmap->newer.mask + 1) * MAX_LOAD_FACTOR;
        if (hmap->newer.size >= treshold) {
            hm_rehash(hmap);
        }
    }
    hm_rehash_help(hmap);
}


void hm_clear(HMap* hmap) {
    free(hmap->older.tab);
    free(hmap->newer.tab);
    *hmap = HMap{};
}

size_t hm_size(HMap* hmap) {
    return hmap->newer.size + hmap->older.size;
}




