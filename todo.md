complile using

```bash
gcc -Wall -Wextra -g src/main.c src/hashtable.c -o main.o && ./main.o
```

# Roadmap for Building Redis from Scratch in C

This roadmap will guide you through building a Redis-like in-memory key-value store from scratch. It is broken down into phases, starting from the core data structures to networking and optimizations.

## Phase 1: Core Data Structures

-   Goal: Implement the fundamental data structures that store key-value pairs in memory.

### Hash Table Implementation (Core Data Store):

-   _DONE_ Implement a dynamic hash table with separate chaining or open addressing.
    Support operations: SET, GET, DEL, EXISTS.

-   _DONE_ Linked List (Foundation for Lists)
    Implement a doubly linked list to support list operations (LPUSH, RPUSH, LPOP, RPOP).
    Optimize for fast insertions and deletions.

-   Dynamic Strings (Like sds in Redis)
    Implement a custom string structure (char\* with metadata) for efficient memory management.
    Support resizing, reference counting, and garbage collection.

-   Expiry System (TTL for Keys)
    Implement a time-based expiration mechanism using a priority queue or sorted list.
    Periodically remove expired keys in a background process.

## Phase 2: Basic Command Processing

-   Goal: Add an interactive command system to handle client queries.

### Command Parser

-   Implement a simple command-line interface (CLI).
    Parse input into commands and arguments.

-   Command Execution Engine
    Maintain a command registry (command_name → function_pointer).
    Implement basic commands: SET, GET, DEL, EXISTS, EXPIRE.

## Phase 3: Networking (Turning It Into a Server)

-   Goal: Implement a lightweight network server to handle client connections.

### Basic TCP Server

-   Use socket(), bind(), listen(), and accept() to handle client connections.
    Handle multiple clients using select() or epoll().
-   Client-Server Protocol
    Implement a simple text-based protocol (\*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n).
    Support pipelining for batch command execution.

-   Multi-Threading or Event Loop (Like libevent)
    Implement an event loop using epoll() (Linux) or kqueue() (BSD/macOS).
    Optionally use threads for multi-core processing.

## Phase 4: Advanced Data Structures

-   Goal: Implement more Redis-like data structures.

### Sorted Sets (ZSET)

-   Implement a skip list for sorted set operations (ZADD, ZRANGE, ZREM).
-   Support fast range queries.
-   Bitmaps and HyperLogLog
-   Implement bitwise operations (SETBIT, GETBIT).
-   Implement HyperLogLog for approximate cardinality estimation.
-   Persistence (Snapshot & Logging)
-   Implement RDB (snapshot persistence): Serialize memory state to a file.
-   Implement AOF (Append-Only File) logging for durability.

## Phase 5: Optimization & Scalability

-   Goal: Improve performance and scalability.

### Memory Optimizations

-   Use memory pools to reduce fragmentation.
-   Implement compression for stored values.
-   Replication (Primary-Replica Model)
-   Implement master-slave replication using TCP.
-   Clustering & Sharding
-   Distribute data across multiple nodes using a hash-based partitioning strategy.

## Bonus: Enhancements

### Ideas to make it even better:

-   Implement Lua scripting support.
-   Add TLS encryption for secure connections.
-   Implement pub/sub (publish-subscribe) for real-time messaging.
