cmake_minimum_required(VERSION 3.10)
project(RedisClone C CXX)

add_library(customRedis STATIC
    client.cpp
    server.cpp
        hashmap.cpp
        hashmap.h
        buffer_funcs.cpp
        buffer_funcs.h
        redis_functions.cpp
        redis_functions.h
)
target_include_directories(customRedis PUBLIC
    ${CMAKE_SOURCE_DIR}
)
