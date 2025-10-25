## TODO List
Note: This file is written by ChatGPT, editted by me afterward.

1. **More Core Data-Type Commands (Low Effort)**

Adding the other “built-in” Redis types will instantly beef up your feature set:

| Data Type   | Commands to Add                      | Rough Effort | Done? |
|-------------|--------------------------------------|--------------|-------|
| Hashes      | HSET / HGET / HDEL / HGETALL         | Easy         | YES   |
| Lists       | LPUSH / RPUSH / LPOP / RPOP / LRANGE | Easy–Medium  | YES   |
| Sets        | SADD / SREM / SMEMBERS / SCARD       | Easy         | YES   |
| Bitmaps     | SETBIT / GETBIT / BITCOUNT           | Medium       | YES   |
| HyperLogLog | PFADD / PFCOUNT                      | Medium       | YES   |

**Why?** These are the staple Redis types.

---

2. **Scripting & Transactions (Medium Effort)**

Once you have multiple types, you can add:

- **Multi-command transactions**
    - `MULTI` / `EXEC` / `DISCARD` / `WATCH`
    - Buffer commands server-side and apply atomically.

- **Lua scripting**
    - `EVAL` / `EVALSHA`
    - Embed Lua (e.g. via Lua C API).

**Why?** Transactions demonstrate command-buffering and atomicity; scripting shows that I can integrate an interpreter.

---

3. **Pub/Sub & Streams (Medium–Hard)**

- **Pub/Sub**
    - `SUBSCRIBE` / `UNSUBSCRIBE` / `PUBLISH`
    - Maintain subscriber lists per channel; fan-out messages.

- **Streams** (if you’re feeling very ambitious)
    - `XADD` / `XREAD` / `XLEN` / `XRANGE`
    - Under the hood: a log-structured sequence of entries, indexed by ID.

**Why?** Messaging patterns are fundamental in distributed systems; learn networking and memory-management skills.

---

4. **Persistence & Durability (Medium–Hard)**

So far everything lives in RAM. To keep data across restarts:

- **RDB-style snapshots**
    - Periodically write the entire in-memory state to disk and load it at startup.

- **AOF (Append-Only File)**
    - Log every write command to a file. On restart, replay the log. Support “rewrite”/compaction to prevent unbounded growth.
    - Configurable fsync via `CONFIG SET appendfsync {always, everysec, no}`.

**Why?** Durability is a hallmark of production-ready servers.

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
