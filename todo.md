## TODO List

1. **More Core Data-Type Commands (Low Effort / Big Resume Impact)**

Adding the other “built-in” Redis types will instantly beef up your feature set:

| Data Type  | Commands to Add                        | Rough Effort |
|------------|----------------------------------------|--------------|
| Hashes     | HSET / HGET / HDEL / HGETALL           | Easy         |
| Lists      | LPUSH / RPUSH / LPOP / RPOP / LRANGE   | Easy–Medium  |
| Sets       | SADD / SREM / SMEMBERS / SCARD         | Easy         |
| Bitmaps    | SETBIT / GETBIT / BITCOUNT             | Medium       |
| HyperLogLog| PFADD / PFCOUNT                        | Medium       |

**Why?** These are the staple Redis types. Even basic implementations (e.g. a `std::unordered_map<string,string>` for hashes, a `deque<string>` for lists, a `bitset` or `vector<bool>` for bitmaps) demonstrate mastery of C++ data structures and will make your clone feel *complete*.

---

2. **Scripting & Transactions (Medium Effort / Big Leverage)**

Once you have multiple types, you can add:

- **Multi-command transactions**
    - `MULTI` / `EXEC` / `DISCARD` / `WATCH`
    - Buffer commands server-side and apply atomically.

- **Lua scripting**
    - `EVAL` / `EVALSHA`
    - Embed Lua (e.g. via Lua C API).

**Why?** Transactions demonstrate command-buffering and atomicity; scripting shows you can integrate an interpreter, a huge résumé tick.

---

3. **Pub/Sub & Streams (Medium–Hard)**

- **Pub/Sub**
    - `SUBSCRIBE` / `UNSUBSCRIBE` / `PUBLISH`
    - Maintain subscriber lists per channel; fan-out messages.

- **Streams** (if you’re feeling very ambitious)
    - `XADD` / `XREAD` / `XLEN` / `XRANGE`
    - Under the hood: a log-structured sequence of entries, indexed by ID.

**Why?** Messaging patterns are fundamental in distributed systems; they push your networking and memory-management skills.

---

4. **Persistence & Durability (Medium–Hard)**

So far everything lives in RAM. To keep data across restarts:

- **RDB-style snapshots**
    - Periodically write your entire in-memory state to disk and load it at startup.

- **AOF (Append-Only File)**
    - Log every write command to a file. On restart, replay the log.
    - Support “rewrite”/compaction to prevent unbounded growth.
    - Configurable fsync via `CONFIG SET appendfsync {always, everysec, no}`.

**Why?** Durability is a hallmark of production­-ready servers.

---

5. **Replication & High Availability (Hard)**

Once you have persistence, you can replicate it:

- **Master → Replica streaming**
    - On startup, replicas sync via a bulk RDB dump + incremental AOF.

- **Sentinel-style failover (or simple Raft)**
    - Elect a new master if the existing one goes down.

- **Cluster sharding**
    - Divide your keyspace into slots and route commands across nodes.

**Why?** Demonstrates distributed systems knowledge and fault tolerance.

---

6. **Protocol & Tooling Polish (Ongoing)**

Even as you build features, polish the user experience:

- **Full RESP2/RESP3 protocol**
    - Proper binary framing, pipelining, intuitive error handling.

- **INFO command**
    - Expose uptime, memory usage, command statistics.

- **CLIENT commands**
    - `CLIENT LIST` / `CLIENT KILL` for connection management.

- **Benchmarks & Tests**
    - Automated unit- and integration-tests for every command.
    - A simple benchmarking harness (compare ops/sec vs. Redis under tiny workloads).

- **Docs, Docker & CI**
    - A rock-solid README with examples, a Dockerfile, and a GitHub Actions pipeline.

**Why?** Hiring managers notice well-tested, well-documented projects with CI and clear metrics.
