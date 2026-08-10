cmake_minimum_required(VERSION 3.16)
project(mini_redis LANGUAGES CXX)

enable_testing()

add_executable(mini_redis 
src/main.cpp
src/command_parser.cpp
src/key_value_store.cpp
)

add_executable(key_value_store_tests
    test/key_value_store_tests.cpp
    src/key_value_store.cpp
)

add_executable(command_parser_tests
    test/command_parser_tests.cpp
    src/command_parser.cpp 
)

target_include_directories(key_value_store_tests PRIVATE src)
target_include_directories(command_parser_tests PRIVATE src)

add_test(
    NAME KeyValueStoreTests
    COMMAND key_value_store_tests
)

add_test(
    NAME CommandParserTests
    COMMAND command_parser_tests
)