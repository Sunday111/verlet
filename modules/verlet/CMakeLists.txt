cmake_minimum_required(VERSION 3.20)
include(set_compiler_options)
set(module_source_files
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/main.cpp)
add_executable(verlet ${module_source_files})
set_generic_compiler_options(verlet PRIVATE)
target_link_libraries(verlet PRIVATE yae_example_header_only
                                     yae_example_library
                                     fmt)
target_include_directories(verlet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/code/public)
target_include_directories(verlet PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private)
