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
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/gui/imgui_helpers.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/instance_painter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/instance_painter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/mesh_vertex.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/object.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/object_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/object_pool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/physics/collision_solvers/collision_solver.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/physics/collision_solvers/collision_solver_mt.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/physics/collision_solvers/collision_solver_mt.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/physics/collision_solvers/collision_solver_st.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/physics/collision_solvers/collision_solver_st.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/physics/verlet_solver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/physics/verlet_solver.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/ranges.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/tagged_id_hash.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/tools/delete_objects_tool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/tools/delete_objects_tool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/tools/move_objects_tool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/tools/move_objects_tool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/tools/spawn_objects_tool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/tools/spawn_objects_tool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/tools/tool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/verlet_app.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/verlet_app.hpp)
add_executable(verlet_fast ${module_source_files})
set_generic_compiler_options(verlet_fast PRIVATE)
target_link_libraries(verlet_fast PRIVATE klgl
                                          unordered_dense)
target_include_directories(verlet_fast PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/code/public)
target_include_directories(verlet_fast PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private)
add_custom_command(TARGET verlet_fast
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/content"
    $<TARGET_FILE_DIR:verlet_fast>/content)
