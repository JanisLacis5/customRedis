cmake_minimum_required(VERSION 3.10)
project(RedisClone C CXX)

add_library(Unity STATIC
    vendor/Unity/src/unity.c
)
target_include_directories(Unity PUBLIC
    vendor/Unity/src
)

add_library(customRedis STATIC
    ${CMAKE_SOURCE_DIR}/src/LinkedList.c
    ${CMAKE_SOURCE_DIR}/src/hashtable.c
        src/tcp/client.cpp
        src/tcp/server.cpp
)
target_include_directories(customRedis PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(main
    src/main.c
    src/LinkedList.c
    src/hashtable.c
)

enable_testing()

file(GLOB TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/tests/dataStructures/*.c
)

add_executable(tests
    ${TEST_SOURCES}
)
target_include_directories(tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    vendor/Unity/src
)
target_link_libraries(tests PRIVATE
    Unity
    customRedis
)

add_test(NAME all_unit_tests COMMAND unit_tests)
