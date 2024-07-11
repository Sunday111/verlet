cmake_minimum_required(VERSION 3.20)
include(set_compiler_options)
set(module_source_files
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/mesh/mesh_data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/name_cache/name.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/name_cache/name_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/opengl/debug/annotations.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/opengl/debug/gl_debug_messenger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/opengl/gl_api.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/os/os.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/read_file.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/shader/shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/shader/shader_define.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/shader/shader_uniform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/texture/texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/type_id_widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/application.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/common.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/macro/to_string.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/macro/warning_suppress.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/mesh/mesh_data.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/name_cache/name.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/name_cache/name_cache.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/opengl/debug/annotations.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/opengl/debug/gl_debug_messenger.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/opengl/gl_api.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/os/os.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/read_file.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/reflection/glm_reflect.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/reflection/matrix_reflect.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/reflection/register_types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/shader/define_handle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/shader/sampler_uniform.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/shader/shader.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/shader/shader_define.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/shader/shader_uniform.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/shader/uniform_handle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/template/class_member_traits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/template/get_enum_underlying.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/template/member_offset.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/template/on_scope_leave.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/template/tuple_type_by_index.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/template/type_to_gl_type.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/texture/texture.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/type_id_widget.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/window.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/wrap/wrap_glfw.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/code/public/klgl/wrap/wrap_imgui.hpp)
add_library(klgl STATIC ${module_source_files})
set_generic_compiler_options(klgl PRIVATE)
target_link_libraries(klgl PUBLIC edt
                                  fmt
                                  CppReflection
                                  nlohmann_json
                                  glad
                                  glfw
                                  imgui_glfw_cmake)
target_include_directories(klgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/code/public)
target_include_directories(klgl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private)
