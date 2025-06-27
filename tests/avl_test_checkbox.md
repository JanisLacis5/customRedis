- [ ] **Empty‐tree invariants**
    - Verify that an empty tree has `cnt == 0` and passes `avl_verify(NULL, root)`.

- [ ] **Single‐element insert/delete**
    - Insert one value, verify tree invariants and count, then delete it and verify the tree is empty again.

- [ ] **Delete non‐existent element**
    - Attempt to delete a value not in the tree; assert that `del(...)` returns `false` and the tree remains unchanged.

- [ ] **Sequential insert**
    - Insert values in strictly increasing (or decreasing) order (e.g. 0, 3, 6, 9, …). After each insert, run full `avl_verify` and check in-order traversal matches the expected sequence.

- [ ] **Random insert**
    - Insert **N** random values one by one, and after each insertion verify balance, `cnt`, height, parent pointers, and in-order extraction.

- [ ] **Random delete**
    - On a prebuilt tree of size **M**, pick random values (some present, some not). For each:
        1. Call `del(...)` and check return value matches presence.
        2. Verify the tree invariants and in-order contents.

- [ ] **Insertion at all “positions” (test_insert)**
    - For each `val` in `[0..sz)`:
        1. Build a tree containing all `[0..sz)` *except* `val`.
        2. Verify it.
        3. Insert `val`, then verify again.

- [ ] **Duplicate‐insert handling (test_insert_dup)**
    - For each `val` in `[0..sz)`:
        1. Build a tree with one copy of each `[0..sz)`.
        2. Insert `val` again (a duplicate).
        3. Verify that duplicate is allowed (multiset count increases) and tree remains balanced.

- [ ] **Sequential removal (test_remove)**
    - For each `val` in `[0..sz)`:
        1. Build a tree containing `[0..sz)`.
        2. Remove `val`, check `del(...)` returns `true`, then verify invariants and in-order contents.

- [ ] **Stress loop of mixed ops**
    - Perform a large number (e.g. 200–500) of random insertions and deletions. After each operation, verify all AVL invariants and multiset contents.

- [ ] **Bulk disposal**
    - Repeatedly delete the root (calling `avl_del(root)` and freeing) until the tree is empty, verifying invariants and no memory leaks at each step.
