cmake_minimum_required(VERSION 3.10)
project(RedisClone C CXX)

add_library(customRedis STATIC
    server.cpp
        data_structures/hashmap.cpp
        data_structures/hashmap.h
    buffer_funcs.cpp
    buffer_funcs.h
        data_structures/avl_tree.cpp
        data_structures/avl_tree.h
    redis_functions.cpp
    redis_functions.h
    server.h
        data_structures/zset.cpp
        data_structures/zset.h
    out_helpers.cpp
    out_helpers.h
    dlist.cpp
    dlist.h
        data_structures/heap.cpp
        data_structures/heap.h
        utils/entry.cpp
        utils/entry.h
        utils/common.h
)
target_include_directories(customRedis PUBLIC
    ${CMAKE_SOURCE_DIR}
)

add_executable(redis_server
        server.cpp
)

target_include_directories(redis_server PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(redis_server
        customRedis
)
