# Phase 1 — Unit tests for core data structures (P0)

## `data_structures/dstr.*`

* **Basics**: init with small/large capacities, append/assign, growth beyond `MAX_PREALOC`.
* **Resize semantics**: `dstr_resize` padding behavior; verify `size`, `free`, null-termination.
* **Zero/empty**: append empty, assign empty, repeated resizes to 0.
* **Misuse guards**: extremely large sizes (near `SIZE_MAX/4`) → expect failure / no UB.
* **Fuzzer mini**: random sequence of append/resize/assign vs a reference `std::string` (in tests only).

## `data_structures/dlist.*`

* **Empty list ops**: pop from empty, iterate empty (no segfaults).
* **Push/pop**: lpush/rpush symmetry, 1-element → 2-element transitions.
* **Remove arbitrary node**: middle, head, tail; after many inserts.
* **Iterator integrity**: forward/backward iteration equals inserted order.

## `data_structures/avl_tree.*`

* **Rotations**: insert sequences that force LL/LR/RL/RR; assert heights/balance.
* **Ordering**: inorder traversal sorted; lookup for present/absent keys.
* **Delete**: leaf, one-child, two-children; repeated deletes to empty.
* **Property-test**: insert N random unique keys, then delete in random order; ensure tree empties.

## `data_structures/hashmap.*`

* **Insert/lookup/delete**: collisions (synthetic keys with same hash), update same key, delete non-existent.
* **Rehash**: cross load-factor thresholds; verify all keys still reachable.
* **Iterate keys**: `hm_keys` returns all, no dups; stable under concurrent inserts (single-thread).
* **Type fields**: `HNode` variants (string, zset, set, list, bitmap, hll) correctly initialized/freed.
* **Edge**: extremely long keys; zero-length key if allowed.

## `data_structures/heap.*` (TTL min-heap)

* **Push/pop order**: min at top; stability for equal priorities.
* **Decrease/increase key**: if supported; otherwise remove+reinsert semantics verified.
* **Stress**: interleaved push/pop thousands; assert heap property.

## `data_structures/zset.*`

* **Insert semantics**: (member,score) new vs update score; idempotency on same score.
* **Lookup**: exact key; lower\_bound with tie-breaking by key.
* **Delete**: by `ZNode`; ensure removal from both AVL + HMap.
* **Range/offset**: `zset_offset` stepping forward/backward N; boundaries.
* **Cross-consistency**: count in AVL equals HMap count.

## `data_structures/hyperloglog.*`

* **Header**: magic, `enc`, card bytes layout; `HLL_DENSE_SIZE_BYTES` exact.
* **Add**: multiple identical values (no count bump), many unique values across registers.
* **Count**: monotonicity; approximate within expected relative error (\~1.04/√m with m=16384).
* **Merge**: commutative, idempotent; merged count ≥ each input, and close to combined union.
* **Fuzz**: random strings, merges of random partitions; regression snapshots for a few seeds.

# Phase 2 — Protocol & command behavior (P0)

Use the provided **client** or a thin C++/Python harness to speak your binary protocol.

## Keyspace

* **GET/SET/DEL**: basic, overwrite, delete absent key returns correct code; large values round-trip.
* **TTL/expire**: `set` with/without TTL + `do_expire`; deterministic clock to assert expiry & survival under activity.
* **Memory**: allocate/free checks with ASan under repetitive set/del cycles.

## Lists (if implemented via dlist)

* **LPUSH/RPUSH/LPOP/RPOP** on empty & 1-item; FIFO/LIFO expectations; mixed sides.
* **LRANGE**-style retrieval (if present): negative indices, out-of-range returns empty.

## Sets

* **SADD/SREM/SMEMBERS/SCARD**: uniqueness, removal of non-members, iteration returns all members (order not guaranteed).
* **Hash vs Set distinction**: ensure no cross-type contamination (type errors on wrong ops).

## Sorted sets (zset)

* **ZADD** (new/update), **ZREM**, **ZRANGE / ZRANGEBYSCORE** (if present): borders, equal scores ordering by key.
* **ZCARD, ZSCORE**: accurate after churn.

## Bitmaps

* **SETBIT/GETBIT/BITCOUNT**: varied ranges (byte mode too), negative indices (translated to tail), sparse growth zeros.

## HyperLogLog

* **PFADD/PFCOUNT/PFMERGE**: duplicates don’t increase count; merge equals union; rough error bounds.

## Error paths

* **Arity**: wrong arg counts produce error tag.
* **Type errors**: operating on key with wrong type yields correct error.
* **Overflow/format**: malformed integers/doubles rejected; boundary doubles for zset scores.

# Phase 3 — Server I/O & lifecycle (P1)

* **Framing**: partial sends/reads; coalesce multiple replies; ensure `incoming`/`outgoing` buffer logic correct (use randomized chunk sizes).
* **Backpressure**: simulate slow client; verify write readiness toggling and no busy loop.
* **Idle timeout**: deterministic time; connection closed after `IDLE_TIMEOUT_MS`.
* **Many clients**: 100–1000 short-lived clients doing small ops; verify no fd leak (`fd_to_conn` shrinks, lists clean).
* **Close semantics**: server-initiated close (idle/error) vs client-initiated; resources reclaimed.

# Phase 4 — Fuzzing (P1 → P0 for robustness)

* **Protocol fuzzer**: libFuzzer target that feeds random byte streams into your request decoder (the code that fills/consumes `Conn::incoming` and parses commands). Assertions: no crashes, no timeouts, no infinite loops.
* **HLL/bitmap fuzz**: random sequences of PFADD/PFMERGE/PFCOUNT and SETBIT/GETBIT/BITCOUNT; compare to a slow reference (Python set for HLL “true cardinality”, Python `bitarray` for bitmaps) inside the fuzzer where feasible.
* **Hashmap/zset structural fuzz**: random operations with a mirrored reference (std::unordered\_map + std::map\<pair\<double,key>>), checking invariants after each step.

# Phase 5 — Concurrency & threadpool (P1)

* **Threadpool**: enqueue N work items; ensure all run exactly once; shutdown behavior if you add it later.
* **Race detection**: run the whole suite with **TSan**; add tests that stress concurrent command handling if your server uses multiple threads in stateful paths (if not, keep to threadpool unit tests).
* **Locking invariants**: if you add locks to HMap/ZSet later, add acquire-order tests to avoid deadlocks (instrument with try-lock & timeouts).

# Phase 6 — Performance baselines (P2)

Using Google Benchmark:

* **Micro-bench**: dstr append/resize, hashmap lookup/insert at various load factors, AVL insert/delete, HLL add/count.
* **Macro-bench**: SET/GET throughput single client vs 64 clients (small/large payloads), ZADD-heavy and PFADD-heavy workloads.
* **Regressions**: store golden medians; alert on >10–15% regressions.

# Phase 7 — Long-run & fault injection (P2)

* **Soak test**: 1–4 hours mixed workload (SET/GET/DEL/ZADD/PFADD/SETBIT) with random TTLs; ensure stable memory (no steady growth with ASan/heap profile).
* **Faults**: random kill client mid-request, drop packets (if you add a shim), simulate ENOBUFS/short writes; server remains healthy.

# Specific test ideas (short list to start with)

* **Hash collisions**: craft keys with `hcode` collisions → insert 1k colliding keys, ensure all retrievable.
* **ZSet tie-break**: multiple members with identical scores — confirm order by key where applicable.
* **BITCOUNT windows**: byte-mode ranges that straddle byte boundaries; negative start/end resolving to valid slices.
* **HLL merge idempotency**: `merge(A, A)` ≈ `A`; `count(merge(A,B))` within expected error of union.
* **TTL heap**: insert many nodes with strictly increasing TTL, then retroactively “advance clock” and pop in order.
* **Server framing**: send half a command, wait, then send remainder; ensure exactly one reply.
