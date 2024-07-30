cmake_minimum_required(VERSION 3.20)
include(set_compiler_options)
set(module_source_files
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/array_action.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/event_manager_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/type_erased_array_tests.cpp)
add_executable(klgl_tests ${module_source_files})
set_generic_compiler_options(klgl_tests PRIVATE)
target_link_libraries(klgl_tests PUBLIC klgl)
target_link_libraries(klgl_tests PRIVATE gtest_main)
target_include_directories(klgl_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/code/public)
target_include_directories(klgl_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private)
enable_testing()
include(GoogleTest)
gtest_discover_tests(klgl_tests)
