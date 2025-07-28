#include <assert.h>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>

#include "../data_structures/zset.h"

// Test basic zset operations
static void test_basic_operations() {
    printf("Testing basic zset operations...\n");
    
    ZSet zset;
    zset_new(&zset);
    
    // Test adding members
    assert(zset_add(&zset, "member1", 6, 1.0) == true);
    assert(zset_add(&zset, "member2", 6, 2.0) == true);
    assert(zset_add(&zset, "member3", 6, 3.0) == true);
    
    // Test score lookup
    double score;
    assert(zset_score(&zset, "member1", 6, &score) == true);
    assert(score == 1.0);
    
    assert(zset_score(&zset, "member2", 6, &score) == true);
    assert(score == 2.0);
    
    assert(zset_score(&zset, "member3", 6, &score) == true);
    assert(score == 3.0);
    
    // Test non-existent member
    assert(zset_score(&zset, "nonexistent", 11, &score) == false);
    
    zset_dispose(&zset);
}

// Test zset member removal
static void test_removal() {
    printf("Testing zset removal...\n");
    
    ZSet zset;
    zset_new(&zset);
    
    // Add several members
    zset_add(&zset, "member1", 7, 1.0);
    zset_add(&zset, "member2", 7, 2.0);
    zset_add(&zset, "member3", 7, 3.0);
    zset_add(&zset, "member4", 7, 4.0);
    
    // Test successful removal
    assert(zset_pop(&zset, "member2", 7) == true);
    
    // Verify it's gone
    double score;
    assert(zset_score(&zset, "member2", 7, &score) == false);
    
    // Verify others still exist
    assert(zset_score(&zset, "member1", 7, &score) == true);
    assert(score == 1.0);
    assert(zset_score(&zset, "member3", 7, &score) == true);
    assert(score == 3.0);
    assert(zset_score(&zset, "member4", 7, &score) == true);
    assert(score == 4.0);
    
    // Test removal of non-existent member
    assert(zset_pop(&zset, "nonexistent", 11) == false);
    
    zset_dispose(&zset);
}

// Test score updates
static void test_score_updates() {
    printf("Testing zset score updates...\n");
    
    ZSet zset;
    zset_new(&zset);
    
    // Add a member
    assert(zset_add(&zset, "member", 6, 1.0) == true);
    
    // Update with different score - should return false (member exists)
    assert(zset_add(&zset, "member", 6, 2.0) == false);
    
    // Verify score was updated
    double score;
    assert(zset_score(&zset, "member", 6, &score) == true);
    assert(score == 2.0);
    
    // Update again
    assert(zset_add(&zset, "member", 6, 5.5) == false);
    assert(zset_score(&zset, "member", 6, &score) == true);
    assert(score == 5.5);
    
    zset_dispose(&zset);
}

// Test range queries
static void test_range_queries() {
    printf("Testing zset range queries...\n");
    
    ZSet zset;
    zset_new(&zset);
    
    // Add members with various scores
    zset_add(&zset, "alice", 5, 1.0);
    zset_add(&zset, "bob", 3, 2.0);
    zset_add(&zset, "charlie", 7, 3.0);
    zset_add(&zset, "david", 5, 4.0);
    zset_add(&zset, "eve", 3, 5.0);
    
    // Test range query
    ZNode *out[10];
    size_t n = zset_query(&zset, 2.0, "", 0, 0, 10, out);
    
    // Should return members with scores >= 2.0
    assert(n >= 4);  // bob, charlie, david, eve (alice has score 1.0)
    
    // Verify they're in score order
    for (size_t i = 1; i < n; i++) {
        assert(out[i-1]->score <= out[i]->score);
    }
    
    // Test range query with offset
    n = zset_query(&zset, 1.0, "", 0, 2, 10, out);  // Skip first 2
    assert(n >= 3);  // Should have at least 3 remaining
    
    // Test range query with limit
    n = zset_query(&zset, 1.0, "", 0, 0, 2, out);   // Limit to 2
    assert(n == 2);
    
    zset_dispose(&zset);
}

// Test with duplicate scores
static void test_duplicate_scores() {
    printf("Testing zset with duplicate scores...\n");
    
    ZSet zset;
    zset_new(&zset);
    
    // Add members with same scores
    zset_add(&zset, "member_a", 8, 2.0);
    zset_add(&zset, "member_b", 8, 2.0);
    zset_add(&zset, "member_c", 8, 2.0);
    zset_add(&zset, "member_d", 8, 1.0);
    zset_add(&zset, "member_e", 8, 3.0);
    
    // All should be retrievable
    double score;
    assert(zset_score(&zset, "member_a", 8, &score) == true);
    assert(score == 2.0);
    assert(zset_score(&zset, "member_b", 8, &score) == true);
    assert(score == 2.0);
    assert(zset_score(&zset, "member_c", 8, &score) == true);
    assert(score == 2.0);
    
    // Test range query with duplicate scores
    ZNode *out[10];
    size_t n = zset_query(&zset, 2.0, "", 0, 0, 10, out);
    assert(n >= 4);  // member_a, member_b, member_c, member_e
    
    zset_dispose(&zset);
}

// Test empty zset operations
static void test_empty_zset() {
    printf("Testing empty zset operations...\n");
    
    ZSet zset;
    zset_new(&zset);
    
    // Test operations on empty zset
    double score;
    assert(zset_score(&zset, "member", 6, &score) == false);
    assert(zset_pop(&zset, "member", 6) == false);
    
    // Test range query on empty zset
    ZNode *out[10];
    size_t n = zset_query(&zset, 0.0, "", 0, 0, 10, out);
    assert(n == 0);
    
    zset_dispose(&zset);
}

// Test large zset
static void test_large_zset() {
    printf("Testing large zset...\n");
    
    ZSet zset;
    zset_new(&zset);
    
    const int num_members = 1000;
    
    // Add many members
    for (int i = 0; i < num_members; i++) {
        char member[20];
        snprintf(member, sizeof(member), "member_%d", i);
        double score = (double)(i % 100);  // Create some duplicate scores
        zset_add(&zset, member, strlen(member), score);
    }
    
    // Verify we can find all members
    for (int i = 0; i < num_members; i++) {
        char member[20];
        snprintf(member, sizeof(member), "member_%d", i);
        double score;
        assert(zset_score(&zset, member, strlen(member), &score) == true);
        assert(score == (double)(i % 100));
    }
    
    // Test range query on large set
    ZNode *out[50];
    size_t n = zset_query(&zset, 50.0, "", 0, 0, 50, out);
    assert(n == 50);  // Should get 50 results
    
    // Verify ordering
    for (size_t i = 1; i < n; i++) {
        assert(out[i-1]->score <= out[i]->score);
    }
    
    // Test removal from large set
    for (int i = 0; i < num_members; i += 10) {  // Remove every 10th member
        char member[20];
        snprintf(member, sizeof(member), "member_%d", i);
        assert(zset_pop(&zset, member, strlen(member)) == true);
    }
    
    // Verify removed members are gone
    for (int i = 0; i < num_members; i += 10) {
        char member[20];
        snprintf(member, sizeof(member), "member_%d", i);
        double score;
        assert(zset_score(&zset, member, strlen(member), &score) == false);
    }
    
    zset_dispose(&zset);
}

// Test edge case scores
static void test_edge_scores() {
    printf("Testing edge case scores...\n");
    
    ZSet zset;
    zset_new(&zset);
    
    // Test various edge case scores
    zset_add(&zset, "zero", 4, 0.0);
    zset_add(&zset, "negative", 8, -1.0);
    zset_add(&zset, "small", 5, 0.000001);
    zset_add(&zset, "large", 5, 999999.999);
    
    // Verify all can be retrieved
    double score;
    assert(zset_score(&zset, "zero", 4, &score) == true);
    assert(score == 0.0);
    
    assert(zset_score(&zset, "negative", 8, &score) == true);
    assert(score == -1.0);
    
    assert(zset_score(&zset, "small", 5, &score) == true);
    assert(score == 0.000001);
    
    assert(zset_score(&zset, "large", 5, &score) == true);
    assert(score == 999999.999);
    
    // Test range query with negative scores
    ZNode *out[10];
    size_t n = zset_query(&zset, -2.0, "", 0, 0, 10, out);
    assert(n == 4);  // All members should be >= -2.0
    
    zset_dispose(&zset);
}

int main() {
    printf("Starting zset tests...\n");
    
    test_basic_operations();
    printf("✓ Basic operations test passed\n");
    
    test_removal();
    printf("✓ Removal test passed\n");
    
    test_score_updates();
    printf("✓ Score updates test passed\n");
    
    test_range_queries();
    printf("✓ Range queries test passed\n");
    
    test_duplicate_scores();
    printf("✓ Duplicate scores test passed\n");
    
    test_empty_zset();
    printf("✓ Empty zset test passed\n");
    
    test_large_zset();
    printf("✓ Large zset test passed\n");
    
    test_edge_scores();
    printf("✓ Edge case scores test passed\n");
    
    printf("All zset tests passed!\n");
    return 0;
}