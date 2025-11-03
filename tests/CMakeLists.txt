cmake_minimum_required(VERSION 3.13)
project(RedisClone C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SANITIZE "Enable Address/UB sanitizers" ON)
function(enable_sanitizers target)
    if (NOT SANITIZE)
        return()
    endif ()

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE
                -g -O1 -fno-omit-frame-pointer
                -fsanitize=address,undefined
                -fno-sanitize-recover=all
        )
        target_link_options(${target} PRIVATE
                -fsanitize=address,undefined
        )
    elseif (MSVC)
        # VS 2019+ with MSVC toolset supports ASan
        target_compile_options(${target} PRIVATE /Zi /Od /fsanitize=address)
        target_link_options(${target} PRIVATE /DEBUG /fsanitize=address)
    endif ()
endfunction()

add_library(customRedis STATIC
        ../src/buffer_funcs.cpp
        ../src/buffer_funcs.h
        ../src/redis_functions.cpp
        ../src/redis_functions.h
        ../src/out_helpers.cpp
        ../src/out_helpers.h
        ../src/data_structures/dlist.cpp
        ../src/data_structures/dlist.h
        ../src/threadpool.cpp
        ../src/threadpool.h
        ../src/data_structures/hashmap.cpp
        ../src/data_structures/hashmap.h
        ../src/data_structures/avl_tree.cpp
        ../src/data_structures/avl_tree.h
        ../src/data_structures/zset.cpp
        ../src/data_structures/zset.h
        ../src/data_structures/heap.cpp
        ../src/data_structures/heap.h
        ../src/utils/common.h
        ../src/data_structures/hyperloglog.cpp
        ../src/data_structures/hyperloglog.h
        ../src/data_structures/dstr.cpp
        ../src/data_structures/dstr.h
)
target_include_directories(customRedis PUBLIC
        ${CMAKE_SOURCE_DIR}/../src
        ${CMAKE_SOURCE_DIR}/../src/utils
        ${CMAKE_SOURCE_DIR}/../src/data_structures
)
enable_sanitizers(customRedis)

add_executable(tests
        main.cpp
)

target_include_directories(tests PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(tests
        customRedis
)

enable_sanitizers(tests)
