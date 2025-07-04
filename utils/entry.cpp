#include "entry.h"
#include "common.h"
#include "../threadpool.h"
#include "../server.h"

bool entry_eq(HNode *node, HNode *key) {
    Entry *ent = container_of(node, Entry, node);
    Lookup *keydata = container_of(key, Lookup, node);
    return ent->key == keydata->key;
}

Entry* new_entry(std::string &key, uint32_t type) {
    Entry *entry = new Entry();
    entry->key = key;
    entry->type = type;
    entry->node.hcode = str_hash((uint8_t*)key.data(), key.size());

    return entry;
}

static void entry_del_sync(Entry* entry) {
    if (entry->type == T_ZSET) {
        zset_clear(&entry->zset);
    }
    delete entry;
}

static void entry_del_wrapper(void *arg) {
    entry_del_sync((Entry*)arg);
}

void entry_del(Entry *entry) {
    // Unlink from data structures
    ent_rem_ttl(entry);

    size_t size = entry->type == T_ZSET ? hm_size(&entry->zset.hmap) : 0;
    const size_t LARGE_SIZE_TRESHOLD = 1000;
    if (size >= LARGE_SIZE_TRESHOLD) {
        threadpool_produce(&global_data.threadpool, &entry_del_wrapper, entry);
    }
    else {
        entry_del_sync(entry);
    }
}