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
        server.cpp
        server.h
        buffer_funcs.cpp
        buffer_funcs.h
        redis_functions.cpp
        redis_functions.h
        out_helpers.cpp
        out_helpers.h
        threadpool.cpp
        threadpool.h
        tblock.cpp
        tblock.h
        data_structures/dlist.cpp
        data_structures/dlist.h
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
        ${CMAKE_SOURCE_DIR}/utils
        ${CMAKE_SOURCE_DIR}/data_structures
)
enable_sanitizers(customRedis)

add_executable(redis_server
        server.cpp
)

target_include_directories(redis_server PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(redis_server
        customRedis
)
enable_sanitizers(redis_server)
