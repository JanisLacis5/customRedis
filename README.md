# Redis Clone

This is a clone of the popular in-memory Redis database built using mainly C, a bit of C++ (as little as possible). My
goal is to build a clone of Redis that has the main features from real Redis database, for example, main commands for
the global database (`get`, `set` etc.), commands for data strucutres: `hash map`, `sorted set`, `linked list`,
`hash set` `bitmap` and `hyperloglog`, add TTL to keys, transactions, pub/sub etc. (for further details on plans for
this project, see [todo](todo.md)). What I want to get from this project is an in-depth understanding of the real
Redis database, so I can efficiently use it in the real world, get experience with low-level programming and expand my
knowledge on data structures and algorithms.

---

## Table of contents

1. [Example Video](#example-video)
2. [Main Features](#main-features)
3. [Documentation](#documentation)
4. [Benchmarks](#benchmarks)
5. [Build & Run](#build--run)
6. [Current Progress & Future work](#current-progress--future-work)

---

## Example Video

![Demo](assets/demo.gif)

---

## Main Features

- Clone supports many commands (more in [DOCS.md](DOCS.md)) for the following data structures: *hash map*, *sorted set*,
  *linked list*, *hash set*, *bitmap* and *hyperloglog*.
- All data structures are built from scrach - all listed in the previous point + dynamic strings, AVL tree (basis for
  sorted set), heap for TTL's
- Custom-built CLI (to build, see [Build & Run](#build--run) section)

---

## Documentation

For more technical details, see the [documentation](DOCS.md)

---

## Benchmarks

Performance testing and profiling results will be published here once performance tests will be implemented.

---

## Build & Run

1. Compile and run the backend:

  ```bash
  mkdir build 
  cd build
  cmake ..
  cmake --build .
  ./redis_server
   ```

2. Compile and run the client:

  ```bash
      g++ -Wall -Wextra client.cpp buffer_funcs.cpp utils/common.h data_structures/dstr.cpp -Iutils -o client  
      ./client
  ```

Now you are in the CLI and you can run commands (see [Example Video](#example-video)) for an example.

---

## Current Progress & Future Work

For more details on future work, see [todo](todo.md)

- [x] Implement core Redis data structures, for example, hyperloglog, dynamic strings etc.
- [x] Create commands for data structures implemented in the previous point.
- [ ] Create tests for data structures and commands.
- [ ] Write performance tests and compare benchmarks vs the real Redis database.
- [ ] Implement scipting and transactions.
- [ ] Implement persistance and durability.
- [ ] Implement replication.