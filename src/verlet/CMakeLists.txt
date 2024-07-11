cmake_minimum_required(VERSION 3.20)
include(set_compiler_options)
set(module_source_files
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/float_range.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/int_range.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/application.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/macro/to_string.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/macro/warning_suppress.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/mesh/mesh_data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/mesh/mesh_data.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/name_cache/name.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/name_cache/name.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/name_cache/name_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/name_cache/name_cache.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/opengl/debug/annotations.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/opengl/debug/annotations.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/opengl/debug/gl_debug_messenger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/opengl/debug/gl_debug_messenger.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/opengl/gl_api.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/opengl/gl_api.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/os/os.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/os/os.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/read_file.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/read_file.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/reflection/glm_reflect.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/reflection/matrix_reflect.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/reflection/register_types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/shader/define_handle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/shader/sampler_uniform.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/shader/shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/shader/shader.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/shader/shader_define.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/shader/shader_define.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/shader/shader_uniform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/shader/shader_uniform.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/shader/uniform_handle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/template/class_member_traits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/template/get_enum_underlying.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/template/member_offset.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/template/on_scope_leave.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/template/tuple_type_by_index.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/template/type_to_gl_type.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/texture/texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/texture/texture.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/type_id_widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/type_id_widget.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/window.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/wrap/wrap_glfw.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/klgl/wrap/wrap_imgui.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/math.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/matrix.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/matrix_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/measure_time.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/position_lookup.hpp)
add_executable(verlet ${module_source_files})
set_generic_compiler_options(verlet PRIVATE)
target_link_libraries(verlet PRIVATE fmt
                                     CppReflection
                                     nlohmann_json
                                     glad
                                     glfw
                                     imgui_glfw_cmake)
target_include_directories(verlet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/code/public)
target_include_directories(verlet PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private)
add_custom_command(TARGET verlet
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/content"
    $<TARGET_FILE_DIR:verlet>/content)
