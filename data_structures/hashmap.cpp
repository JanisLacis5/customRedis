#include <assert.h>
#include <cstdlib>

#include "hashmap.h"

#include <string.h>

#include "server.h"
#include "zset.h"

const size_t MAX_LOAD_FACTOR = 8;
const size_t REHASHING_WORK = 128;

static void h_init(HTab *htab, size_t n) {
    // Assert that n is a power of 2
    assert(n > 0 && ((n-1) & n) == 0);

    htab->tab = (HNode**)calloc(n, sizeof(HNode*));
    htab->mask = n-1;
    htab->size = 0;
}

static void hm_rehash(HMap *hmap) {
    hmap->older = hmap->newer;
    h_init(&hmap->newer, 2 * (hmap->older.mask + 1));
    hmap->migrate_pos = 0;
}

// Returns the &P->next where P is the previous node before `node`
static HNode** ht_lookup(HTab *htab, HNode *node) {
    if (!htab->tab) {
        return NULL;
    }

    size_t pos = node->hcode & htab->mask;
    HNode **slot = &htab->tab[pos];
    int count = 0;
    while (*slot) {
        HNode *curr = *slot;
        if (curr->hcode == node->hcode && !strcmp(node->key->buf, curr->key->buf)) {
            return slot;
        }
        slot = &curr->next;
    }
    return NULL;
}

static void hn_del_sync(HNode* hnode) {
    if (hnode->type == T_ZSET) {
        zset_clear(hnode->zset);
    }
    if (hnode->type == T_HSET) {
        hm_clear(&hnode->hmap);
    }
    delete hnode;
}

static void hn_del_wrapper(void *arg) {
    hn_del_sync((HNode*)arg);
}

static HNode* ht_delete(HTab* htab, HNode **from) {
    HNode *to_delete = *from;
    if (!to_delete) {
        return NULL;
    }
    *from = to_delete->next;
    htab->size--;

    size_t size = 0;
    if (to_delete->type == T_ZSET) {
        size = std::max(size, hm_size(&to_delete->zset->hmap));
    }
    if (to_delete->type == T_HSET) {
        size = std::max(size, hm_size(&to_delete->hmap));
    }

    const size_t LARGE_SIZE_TRESHOLD = 1000;
    if (size >= LARGE_SIZE_TRESHOLD) {
        threadpool_produce(&global_data.threadpool, &hn_del_wrapper, to_delete);
    }
    else {
        hn_del_sync(to_delete);
    }

    return to_delete;
}

static void ht_insert(HTab *htab, HNode *node) {
    size_t pos = node->hcode & htab->mask;
    HNode *next = htab->tab[pos]; // issue
    node->next = next;
    htab->tab[pos] = node;
    htab->size++;
}

static void hm_rehash_help(HMap *hmap) {
    size_t nwork = 0;
    while (nwork < REHASHING_WORK && hmap->older.size > 0) {
        HNode **slot = &hmap->older.tab[hmap->migrate_pos];
        if (!*slot) {
            hmap->migrate_pos++;
            continue;
        }
        HNode *node = ht_delete(&hmap->older, slot);
        ht_insert(&hmap->newer, node);
        nwork++;
    }
    if (hmap->older.tab && hmap->older.size == 0) {
        free(hmap->older.tab);
        hmap->older = HTab{};
    }
}

static bool h_foreach(HTab *htab, std::vector<dstr*> &arg) {
    for (size_t i = 0; i <= htab->mask; i++) {
        if (!htab->tab) {
            continue;
        }
        HNode *curr = htab->tab[i];
        while (curr) {
            arg.push_back(curr->key);
            curr = curr->next;
        }
    }
    return true;
}

HNode* hm_lookup(HMap *hmap, HNode* key) {
    HNode **slot = ht_lookup(&hmap->older, key);
    if (!slot) {
        slot = ht_lookup(&hmap->newer, key);
    }

    return slot ? *slot : NULL;
}

HNode* hm_delete(HMap* hmap, HNode* key) {
    HNode **slot = ht_lookup(&hmap->older, key);
    if (slot) {
        return ht_delete(&hmap->older, slot);
    }
    slot = ht_lookup(&hmap->newer, key);
    if (slot) {
        return ht_delete(&hmap->newer, slot);
    }
    return NULL;
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

void hm_keys(HMap* hmap, std::vector<dstr*> &arg) {
    h_foreach(&hmap->newer, arg);
    h_foreach(&hmap->older, arg);
}

HNode* new_node(dstr *key, uint32_t type) {
    HNode *node = new HNode();
    node->key = dstr_init(key->size);
    dstr_append(&node->key, key->buf, key->size);
    node->hcode = str_hash((uint8_t*)key->buf, key->size);
    node->type = type;
    if (type == T_STR) {
        node->val = dstr_init(0);
    }
    if (type == T_ZSET) {
        node->zset = new ZSet();
    }
    if (type == T_BITMAP) {
        node->bitmap = dstr_init(0);
    }
    return node;
}
