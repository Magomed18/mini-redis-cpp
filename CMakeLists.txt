cmake_minimum_required(VERSION 3.16)
project(mini_redis LANGUAGES CXX)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_executable(mini_redis 
src/main.cpp
src/command_parser.cpp
src/key_value_store.cpp
src/winsock_runtime.cpp
src/socket_handle.cpp
src/command_executor.cpp
src/client_session.cpp
src/client_worker_pool.cpp
src/resp_parser.cpp
src/resp_encoder.cpp
)

add_executable(client_session_tests
    test/client_session_tests.cpp
    src/client_session.cpp
    src/command_executor.cpp
    src/command_parser.cpp
    src/key_value_store.cpp
    src/winsock_runtime.cpp
    src/socket_handle.cpp
    src/resp_parser.cpp
    src/resp_encoder.cpp
)

add_executable(key_value_store_tests
    test/key_value_store_tests.cpp
    src/key_value_store.cpp
)

add_executable(command_parser_tests
    test/command_parser_tests.cpp
    src/command_parser.cpp 
)

add_executable(command_executor_tests
    test/command_executor_tests.cpp
    src/command_executor.cpp
    src/key_value_store.cpp
    src/resp_encoder.cpp
)

add_executable(resp_parser_tests
    test/resp_parser_tests.cpp
    src/resp_parser.cpp
)

add_executable(resp_encoder_tests
    test/resp_encoder_tests.cpp
    src/resp_encoder.cpp

)

target_include_directories(key_value_store_tests PRIVATE src)
target_include_directories(command_parser_tests PRIVATE src)
target_include_directories(command_executor_tests PRIVATE src)
target_include_directories(resp_parser_tests PRIVATE src)
target_include_directories(resp_encoder_tests PRIVATE src)
target_include_directories(client_session_tests PRIVATE src)


add_test(
    NAME KeyValueStoreTests
    COMMAND key_value_store_tests
)

add_test(
    NAME CommandParserTests
    COMMAND command_parser_tests
)

add_test(
    NAME CommandExecutorTests
    COMMAND command_executor_tests
)

add_test(
    NAME RespParserTests
    COMMAND resp_parser_tests
)

add_test(
    NAME RespEncoderTests
    COMMAND resp_encoder_tests
)

add_test(
    NAME ClientSessionTests
    COMMAND client_session_tests
)

target_link_libraries(
    mini_redis
    PRIVATE Threads::Threads
)

target_link_libraries(
    client_session_tests PRIVATE Threads::Threads
)



if(WIN32)
    target_link_libraries(mini_redis PRIVATE ws2_32)
    target_link_libraries(client_session_tests PRIVATE ws2_32)
endif()

if(MSVC)
    add_compile_options(
        /W4
        /permissive-
    )
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
    )
endif()
