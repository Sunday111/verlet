cmake_minimum_required(VERSION 3.20)
include(set_compiler_options)
set(module_source_files
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/camera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/camera.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/coloring/object_color_function.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/coloring/spawn_color/spawn_color_strategy.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/coloring/spawn_color/spawn_color_strategy_rainbow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/coloring/spawn_color/spawn_color_strategy_rainbow.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/coloring/tick_color/tick_color_strategy.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/coloring/tick_color/tick_color_strategy_velocity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/coloring/tick_color/tick_color_strategy_velocity.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/emitters/emitter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/emitters/emitter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/emitters/radial_emitter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/emitters/radial_emitter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/gui/app_gui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/gui/app_gui.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/instance_painter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/instance_painter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/object.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/object_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/object_pool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/physics/verlet_solver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/physics/verlet_solver.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/ranges.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/threading/batch_thread_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/threading/batch_thread_pool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/tools/delete_objects_tool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/tools/delete_objects_tool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/tools/move_objects_tool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/tools/move_objects_tool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/tools/spawn_objects_tool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/tools/spawn_objects_tool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/tools/tool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/verlet_app.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/verlet/verlet_app.hpp)
add_library(verlet_lib STATIC ${module_source_files})
set_generic_compiler_options(verlet_lib PRIVATE)
target_link_libraries(verlet_lib PUBLIC klgl)
target_include_directories(verlet_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/code/public)
target_include_directories(verlet_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private)
add_custom_target(verlet_lib_copy_files ALL
    ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/content" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/content
)
add_dependencies(verlet_lib_copy_files verlet_lib)
