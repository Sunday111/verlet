cmake_minimum_required(VERSION 3.20)
include(set_compiler_options)
set(module_source_files
    ${CMAKE_CURRENT_SOURCE_DIR}/code/private/main.cpp)
add_executable(verlet_video ${module_source_files})
set_generic_compiler_options(verlet_video PRIVATE)
target_link_libraries(verlet_video PRIVATE verlet_lib)
target_include_directories(verlet_video PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/code/public)
target_include_directories(verlet_video PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private)
add_custom_target(verlet_video_copy_files ALL
    COMMAND ${Python3_EXECUTABLE} "${YAE_SUPPORT_ROOT}/scripts/stage_directories.py"
            --destination "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/content"
            --source "${CMAKE_CURRENT_SOURCE_DIR}/content"
            --manifest "${CMAKE_CURRENT_BINARY_DIR}/verlet_video_copy_files_content.manifest"
    COMMENT "stage content of verlet_video"
    VERBATIM)
add_dependencies(verlet_video verlet_video_copy_files)
