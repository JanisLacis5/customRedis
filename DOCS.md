# customRedis

A lightweight Redis-compatible in-memory data store written in C++. Supports basic string and sorted-set commands, key expiration, and a poll-based event loop with connection timeouts. Designed as a learning project in low-level networking, custom data structures, and concurrency.

---

## Table of Contents

1. [Features](#features)
2. [Directory Structure](#directory-structure)
3. [Build & Installation](#build--installation)
4. [Running the App](#running-the-app)
5. [Architecture Overview](#architecture-overview)
    - [Event Loop & Concurrency](#event-loop--concurrency)
    - [Connection Lifecycle](#connection-lifecycle)
7. [Command Reference](#command-reference)
    - [String Commands](#string-commands)
    - [Sorted-Set Commands](#sorted-set-commands)
    - [Expiration Commands](#expiration-commands)
9. [Limitations](#limitations)
10. [Future Work](#future-work)

---

# Features

- **Basic key–value operations**: `GET`, `SET`, `DEL`, `KEYS`
- **Sorted sets**: `ZADD`, `ZSCORE`, `ZREM`, `ZQUERY` (range query by score)
- **Key expiration**: `EXPIRE`, `TTL`, `PERSIST`
- **Non-blocking I/O** using `poll()` and configurable timeouts
- **Custom data structures**: hash map, min-heap for TTL, zset (AVL + heap), doubly linked list for timeouts
- **Thread pool** for offloading expensive operations
- **Modular CMake build** for portability

---

# Directory Structure
```text
customRedis/
├── CMakeLists.txt
├── server.h
├── server.cpp
├── redis_functions.h
├── redis_functions.cpp
├── buffer_funcs.h/.cpp       # request parsing, framing
├── out_helpers.h/.cpp        # RESP formatting helpers
├── threadpool.h/.cpp         # worker threads for background tasks
├── dlist.h/.cpp              # doubly linked lists (timeout queues)
└── data_structures/
    ├── hashmap.h/.cpp        # in-memory key→entry storage
    ├── heap.h/.cpp           # min-heap for TTL management
    ├── zset.h/.cpp           # sorted-set (AVL tree + heap)
    ├── avl_tree.h/.cpp       # AVL tree
```

---

# Build & Installation

1. **Prerequisites**
    - C++17 compiler (g++ / clang++)
    - CMake 3.10+

2. **Clone & Build**
   ```bash
   git clone <your-repo-url>
   cd customRedis
   mkdir build && cd build
   cmake ..
   cmake --build .
   ```

# Running the app

1. **Run the server**
   ```bash
   ./build/redis_server
   ```
2. **Compile client**
   ```bash
   g++ -Wall -Wextra client.cpp buffer_funcs.cpp -o client
   ```
3. **Run commands** \
   Example:
   ```bash
   $ ./client set root 1
   (null)
   $ ./client get root
   (str) 1
   ```

# Architecture Overview

## Event Loop & Concurrency

### `main()` (in `server.cpp`)

- Initializes global structures (`global_data`): hash map, timeout lists, thread pool.
- Creates a listening socket (with `SO_REUSEADDR`) and enters an infinite `poll()` loop.
- Aggregates `pollfd`s for the listener and each active connection.
- Dispatches ready events to `handle_read()`, `handle_write()`, or `close_conn()`.

### Thread Pool

- Configurable worker threads (`threadpool_init(&global_data.threadpool, 8)`).
- Offloads CPU-heavy tasks (e.g. range queries) to avoid blocking the event loop.

## Connection Lifecycle

### `Conn` struct (in `server.h`)

```cpp
struct Conn {
  int                   fd;
  bool                  want_read;
  bool                  want_write;
  bool                  want_close;
  std::vector<uint8_t>  incoming, outgoing;
  DListNode             idle_timeout, read_timeout, write_timeout;
  uint64_t              last_active_ms, last_read_ms, last_write_ms;
};
```

# Timeout Queues
Three doubly linked lists (`idle_list`, `read_list`, `write_list`) track connection deadlines.  
`process_timers()` advances time and closes idle or stalled connections.

# Command Reference
## String commands
| Command | Syntax              | Description             |
|---------|---------------------|-------------------------|
| SET     | `SET <key> <value>` | Store a string value    |
| GET     | `GET <key>`         | Retrieve a string value |
| DEL     | `DEL <key>`         | Delete a key            |
| KEYS    | `KEYS`              | List all keys           |

## HSet commands
| Command | Syntax                                 | Description                                                                                                         |
|---------|----------------------------------------|---------------------------------------------------------------------------------------------------------------------|
| HSET    | `HSET key field value [field value …]` | Set one or more `field`→`value` pairs in the hash stored at `key`. Returns the number of new fields added.          |
| HGET    | `HGET key field`                       | Get the value associated with `field` in the hash at `key`. Returns the value, or `nil` if the field doesn’t exist. |
| HDEL    | `HDEL key field [field …]`             | Remove one or more `field`s from the hash at `key`. Returns the number of fields that were removed.                 |
| HGETALL | `HGETALL key`                          | Retrieve all fields and values in the hash at `key`. Returns a list: `field1, value1, field2, value2, …`.           |

## Sorted Set commands
| Command | Syntax                                                   | Description                         |
|---------|----------------------------------------------------------|-------------------------------------|
| ZADD    | `ZADD <key> <score> <member>`                            | Add or update a member with a score |
| ZSCORE  | `ZSCORE <key> <member>`                                  | Get the score of a member           |
| ZREM    | `ZREM <key> <member>`                                    | Remove a member from a sorted set   |
| ZQUERY  | `ZQUERY <key> BY <min\max> <score> OFFSET <n> LIMIT <m>` | Query a score range with pagination |

## Expiration commands
| Command | Syntax                   | Description                               |
|---------|--------------------------|-------------------------------------------|
| EXPIRE  | `EXPIRE <key> <seconds>` | Set a key’s expiration in seconds         |
| TTL     | `TTL <key>`              | Get remaining TTL in seconds              |
| PERSIST | `PERSIST <key>`          | Remove expiration to make a key permanent |

# Limitations
- No on-disk persistence (RDB/AOF) yet
- Single-threaded event loop; thread pool only for off-loading
- Partial RESP support (RESP3 not implemented)
- No replication, clustering, or ACLs
- Only strings and sorted-sets; other data types not yet supported

# Future work
- Add RDB snapshot and AOF persistence
- Implement Hash, List, Set, Bitmap, HyperLogLog
- Support transactions (`MULTI` / `EXEC`), Lua scripting, Pub/Sub
- Introduce master–replica replication and automatic failover
- Enhance RESP3 compliance and binary framing
- Add `INFO`, `CLIENT` commands, and a benchmarking suite
