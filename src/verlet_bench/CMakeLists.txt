cmake_minimum_required(VERSION 3.20)
include(set_compiler_options)
set(module_source_files
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/main.cpp)
add_executable(verlet_bench ${module_source_files})
set_generic_compiler_options(verlet_bench PRIVATE)
target_link_libraries(verlet_bench PRIVATE verlet_lib)
target_include_directories(verlet_bench PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/code/public)
target_include_directories(verlet_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private)
