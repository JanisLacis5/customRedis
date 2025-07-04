#include "entry.h"
#include "common.h"
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

void entry_del(Entry* entry) {
    if (entry->type == T_ZSET) {
        zset_clear(&entry->zset);
    }
    ent_rem_ttl(entry);
    delete entry;
}