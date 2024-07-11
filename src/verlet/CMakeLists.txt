cmake_minimum_required(VERSION 3.20)
include(set_compiler_options)
set(module_source_files
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/common.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/measure_time.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/mesh_vertex.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/position_lookup.hpp)
add_executable(verlet ${module_source_files})
set_generic_compiler_options(verlet PRIVATE)
target_link_libraries(verlet PRIVATE klgl)
target_include_directories(verlet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/code/public)
target_include_directories(verlet PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private)
add_custom_command(TARGET verlet
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/content"
    $<TARGET_FILE_DIR:verlet>/content)
