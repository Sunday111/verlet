cmake_minimum_required(VERSION 3.20)
include(set_compiler_options)
set(module_source_files
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/float_range.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/int_range.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/math.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/matrix.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/matrix_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/measure_time.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/position_lookup.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/wrap_raylib.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/wrap_raylib.hpp)
add_executable(verlet ${module_source_files})
set_generic_compiler_options(verlet PRIVATE)
target_link_libraries(verlet PRIVATE eigen
                                     raylib
                                     fmt)
target_include_directories(verlet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/code/public)
target_include_directories(verlet PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private)
