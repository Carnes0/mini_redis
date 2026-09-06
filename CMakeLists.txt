cmake_minimum_required(VERSION 3.20)
project(mini_redis VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

add_library(mini_redis_core
    src/protocol/resp.cpp
    src/storage/store.cpp
    src/commands/dispatcher.cpp
    src/server/thread_pool.cpp
)
target_include_directories(mini_redis_core PUBLIC include)
if(MSVC)
    target_compile_options(mini_redis_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(mini_redis_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(mini_redis src/main.cpp src/server/tcp_server.cpp)
target_link_libraries(mini_redis PRIVATE mini_redis_core ws2_32)

function(add_mini_test name source)
    add_executable(${name} ${source})
    target_include_directories(${name} PRIVATE tests)
    target_link_libraries(${name} PRIVATE mini_redis_core)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_mini_test(protocol_test tests/protocol_test.cpp)
add_mini_test(storage_test tests/storage_test.cpp)
add_mini_test(commands_test tests/commands_test.cpp)
add_mini_test(thread_pool_test tests/thread_pool_test.cpp)

