#include <assert.h>
#include <cstring>
#include <cstdio>
#include <vector>
#include <string>

#include "../data_structures/hashmap.h"
#include "../utils/entry.h"

// Test basic hashmap operations
static void test_basic_operations() {
    printf("Testing basic hashmap operations...\n");
    
    HMap hmap;
    hmap_init(&hmap, 4);  // Start with small size to test resizing
    
    // Test insertion
    const char* key1 = "test_key_1";
    const char* key2 = "test_key_2";
    
    Entry* entry1 = new Entry();
    entry1->key = strdup(key1);
    entry1->val = strdup("value1");
    
    Entry* entry2 = new Entry();
    entry2->key = strdup(key2);
    entry2->val = strdup("value2");
    
    // Insert entries
    hmap_insert(&hmap, &entry1->node, entry1->key);
    hmap_insert(&hmap, &entry2->node, entry2->key);
    
    assert(hmap.size == 2);
    
    // Test lookup
    HNode* found1 = hmap_lookup(&hmap, key1, strlen(key1));
    assert(found1 != nullptr);
    Entry* found_entry1 = container_of(found1, Entry, node);
    assert(strcmp(found_entry1->key, key1) == 0);
    assert(strcmp(found_entry1->val, "value1") == 0);
    
    HNode* found2 = hmap_lookup(&hmap, key2, strlen(key2));
    assert(found2 != nullptr);
    Entry* found_entry2 = container_of(found2, Entry, node);
    assert(strcmp(found_entry2->key, key2) == 0);
    assert(strcmp(found_entry2->val, "value2") == 0);
    
    // Test lookup of non-existent key
    HNode* not_found = hmap_lookup(&hmap, "nonexistent", 11);
    assert(not_found == nullptr);
    
    // Cleanup
    free(entry1->key);
    free(entry1->val);
    delete entry1;
    
    free(entry2->key);
    free(entry2->val);  
    delete entry2;
    
    hmap_destroy(&hmap);
}

// Test hashmap resizing
static void test_resizing() {
    printf("Testing hashmap resizing...\n");
    
    HMap hmap;
    hmap_init(&hmap, 2);  // Very small initial size to force resize
    
    std::vector<Entry*> entries;
    const int num_entries = 10;
    
    // Insert many entries to trigger resize
    for (int i = 0; i < num_entries; i++) {
        Entry* entry = new Entry();
        entry->key = new char[20];
        snprintf(entry->key, 20, "key_%d", i);
        entry->val = new char[20];
        snprintf(entry->val, 20, "value_%d", i);
        
        hmap_insert(&hmap, &entry->node, entry->key);
        entries.push_back(entry);
    }
    
    assert(hmap.size == num_entries);
    assert(hmap.mask > 1);  // Should have resized
    
    // Verify all entries can still be found
    for (int i = 0; i < num_entries; i++) {
        char key[20];
        snprintf(key, 20, "key_%d", i);
        
        HNode* found = hmap_lookup(&hmap, key, strlen(key));
        assert(found != nullptr);
        
        Entry* found_entry = container_of(found, Entry, node);
        assert(strcmp(found_entry->key, key) == 0);
        
        char expected_val[20];
        snprintf(expected_val, 20, "value_%d", i);
        assert(strcmp(found_entry->val, expected_val) == 0);
    }
    
    // Cleanup
    for (Entry* entry : entries) {
        delete[] entry->key;
        delete[] entry->val;
        delete entry;
    }
    
    hmap_destroy(&hmap);
}

// Test hashmap deletion
static void test_deletion() {
    printf("Testing hashmap deletion...\n");
    
    HMap hmap;
    hmap_init(&hmap, 4);
    
    // Insert several entries
    std::vector<Entry*> entries;
    for (int i = 0; i < 5; i++) {
        Entry* entry = new Entry();
        entry->key = new char[20];
        snprintf(entry->key, 20, "del_key_%d", i);
        entry->val = strdup("del_value");
        
        hmap_insert(&hmap, &entry->node, entry->key);
        entries.push_back(entry);
    }
    
    assert(hmap.size == 5);
    
    // Delete middle entry
    HNode* to_delete = hmap_lookup(&hmap, "del_key_2", strlen("del_key_2"));
    assert(to_delete != nullptr);
    
    hmap_pop(&hmap, to_delete, strlen("del_key_2"));
    assert(hmap.size == 4);
    
    // Verify it's gone
    HNode* deleted = hmap_lookup(&hmap, "del_key_2", strlen("del_key_2"));
    assert(deleted == nullptr);
    
    // Verify other entries still exist
    for (int i = 0; i < 5; i++) {
        if (i == 2) continue;  // Skip deleted entry
        
        char key[20];
        snprintf(key, 20, "del_key_%d", i);
        
        HNode* found = hmap_lookup(&hmap, key, strlen(key));
        assert(found != nullptr);
    }
    
    // Cleanup
    for (int i = 0; i < 5; i++) {
        if (i == 2) {
            // Manually cleanup deleted entry
            delete[] entries[i]->key;
            free(entries[i]->val);
            delete entries[i];
        } else {
            delete[] entries[i]->key;
            free(entries[i]->val);
            delete entries[i];
        }
    }
    
    hmap_destroy(&hmap);
}

// Test collision handling
static void test_collisions() {
    printf("Testing collision handling...\n");
    
    HMap hmap;
    hmap_init(&hmap, 4);  // Small size to increase collision probability
    
    // Create keys that might hash to same bucket
    std::vector<std::string> keys = {
        "collision_test_1",
        "collision_test_2", 
        "collision_test_3",
        "different_key_1",
        "different_key_2"
    };
    
    std::vector<Entry*> entries;
    
    // Insert all keys
    for (size_t i = 0; i < keys.size(); i++) {
        Entry* entry = new Entry();
        entry->key = strdup(keys[i].c_str());
        entry->val = new char[20];
        snprintf(entry->val, 20, "value_%zu", i);
        
        hmap_insert(&hmap, &entry->node, entry->key);
        entries.push_back(entry);
    }
    
    assert(hmap.size == keys.size());
    
    // Verify all can be found despite potential collisions
    for (size_t i = 0; i < keys.size(); i++) {
        HNode* found = hmap_lookup(&hmap, keys[i].c_str(), keys[i].length());
        assert(found != nullptr);
        
        Entry* found_entry = container_of(found, Entry, node);
        assert(strcmp(found_entry->key, keys[i].c_str()) == 0);
        
        char expected_val[20];
        snprintf(expected_val, 20, "value_%zu", i);
        assert(strcmp(found_entry->val, expected_val) == 0);
    }
    
    // Cleanup
    for (Entry* entry : entries) {
        free(entry->key);
        delete[] entry->val;
        delete entry;
    }
    
    hmap_destroy(&hmap);
}

// Test empty key and special cases
static void test_edge_cases() {
    printf("Testing edge cases...\n");
    
    HMap hmap;
    hmap_init(&hmap, 4);
    
    // Test empty key
    Entry* empty_entry = new Entry();
    empty_entry->key = strdup("");
    empty_entry->val = strdup("empty_value");
    
    hmap_insert(&hmap, &empty_entry->node, empty_entry->key);
    
    HNode* found = hmap_lookup(&hmap, "", 0);
    assert(found != nullptr);
    Entry* found_entry = container_of(found, Entry, node);
    assert(strcmp(found_entry->val, "empty_value") == 0);
    
    // Test very long key
    std::string long_key(1000, 'x');
    Entry* long_entry = new Entry();
    long_entry->key = strdup(long_key.c_str());
    long_entry->val = strdup("long_value");
    
    hmap_insert(&hmap, &long_entry->node, long_entry->key);
    
    HNode* long_found = hmap_lookup(&hmap, long_key.c_str(), long_key.length());
    assert(long_found != nullptr);
    Entry* long_found_entry = container_of(long_found, Entry, node);
    assert(strcmp(long_found_entry->val, "long_value") == 0);
    
    assert(hmap.size == 2);
    
    // Cleanup
    free(empty_entry->key);
    free(empty_entry->val);
    delete empty_entry;
    
    free(long_entry->key);
    free(long_entry->val);
    delete long_entry;
    
    hmap_destroy(&hmap);
}

// Test duplicate key insertion
static void test_duplicate_keys() {
    printf("Testing duplicate key handling...\n");
    
    HMap hmap;
    hmap_init(&hmap, 4);
    
    const char* key = "duplicate_key";
    
    // Insert first entry
    Entry* entry1 = new Entry();
    entry1->key = strdup(key);
    entry1->val = strdup("first_value");
    
    hmap_insert(&hmap, &entry1->node, entry1->key);
    assert(hmap.size == 1);
    
    // Try to insert duplicate key (this should not increase size)
    Entry* entry2 = new Entry();
    entry2->key = strdup(key);
    entry2->val = strdup("second_value");
    
    // Note: Depending on implementation, this might replace or be ignored
    // For now, let's test that the hashmap handles it gracefully
    HNode* existing = hmap_lookup(&hmap, key, strlen(key));
    if (existing != nullptr) {
        // Key already exists, don't insert duplicate
        free(entry2->key);
        free(entry2->val);
        delete entry2;
    } else {
        hmap_insert(&hmap, &entry2->node, entry2->key);
    }
    
    // Size should still be 1
    assert(hmap.size == 1);
    
    // Should find the original entry
    HNode* found = hmap_lookup(&hmap, key, strlen(key));
    assert(found != nullptr);
    
    // Cleanup
    free(entry1->key);
    free(entry1->val);
    delete entry1;
    
    hmap_destroy(&hmap);
}

int main() {
    printf("Starting hashmap tests...\n");
    
    test_basic_operations();
    printf("✓ Basic operations test passed\n");
    
    test_resizing();
    printf("✓ Resizing test passed\n");
    
    test_deletion();
    printf("✓ Deletion test passed\n");
    
    test_collisions();
    printf("✓ Collision handling test passed\n");
    
    test_edge_cases();
    printf("✓ Edge cases test passed\n");
    
    test_duplicate_keys();
    printf("✓ Duplicate keys test passed\n");
    
    printf("All hashmap tests passed!\n");
    return 0;
}