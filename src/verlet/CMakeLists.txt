cmake_minimum_required(VERSION 3.20)
include(set_compiler_options)
set(module_source_files
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/coloring/object_color_function.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/coloring/spawn_color/spawn_color_strategy.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/coloring/spawn_color/spawn_color_strategy_rainbow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/coloring/spawn_color/spawn_color_strategy_rainbow.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/coloring/tick_color/tick_color_strategy.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/coloring/tick_color/tick_color_strategy_velocity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/coloring/tick_color/tick_color_strategy_velocity.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/gui/app_gui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/gui/app_gui.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/instance_painter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/instance_painter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/object.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/object_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/object_pool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/physics/verlet_solver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/physics/verlet_solver.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/ranges.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/threading/batch_thread_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/threading/batch_thread_pool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/tools/delete_objects_tool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/tools/delete_objects_tool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/tools/move_objects_tool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/tools/move_objects_tool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/tools/spawn_objects_tool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/tools/spawn_objects_tool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/tools/tool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/verlet_app.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/verlet_app.hpp)
add_executable(verlet ${module_source_files})
set_generic_compiler_options(verlet PRIVATE)
target_link_libraries(verlet PRIVATE klgl)
target_include_directories(verlet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/code/public)
target_include_directories(verlet PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private)
add_custom_target(verlet_copy_files ALL
    ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/content" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/content
)
add_dependencies(verlet_copy_files verlet)
