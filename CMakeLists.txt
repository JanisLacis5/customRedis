cmake_minimum_required(VERSION 3.10)
project(RedisClone C CXX)

add_library(customRedis STATIC
    server.cpp
    server.h
    buffer_funcs.cpp
    buffer_funcs.h
    redis_functions.cpp
    redis_functions.h
    out_helpers.cpp
    out_helpers.h
        data_structures/dlist.cpp
        data_structures/dlist.h
    threadpool.cpp
    threadpool.h
        data_structures/hashmap.cpp
        data_structures/hashmap.h
        data_structures/avl_tree.cpp
        data_structures/avl_tree.h
        data_structures/zset.cpp
        data_structures/zset.h
        data_structures/heap.cpp
        data_structures/heap.h
        utils/common.h
        data_structures/hyperloglog.cpp
        data_structures/hyperloglog.h
        data_structures/dstr.cpp
        data_structures/dstr.h
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
