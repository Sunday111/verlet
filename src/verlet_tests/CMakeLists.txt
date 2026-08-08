cmake_minimum_required(VERSION 3.20)
include(set_compiler_options)
set(module_source_files
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/object_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/verlet_solver.cpp)
add_executable(verlet_tests ${module_source_files})
set_generic_compiler_options(verlet_tests PRIVATE)
target_link_libraries(verlet_tests PRIVATE verlet_lib
                                           gtest_main)
target_include_directories(verlet_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/code/public)
target_include_directories(verlet_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private)
enable_testing()
include(GoogleTest)
gtest_discover_tests(verlet_tests)
