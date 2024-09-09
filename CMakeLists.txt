cmake_minimum_required(VERSION 3.20)

project(Verlet)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Set output directories for binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
# Set output directories for archives
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
# Set output directories for libraries
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

set(YAE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/deps/yae")
set(YAE_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${YAE_ROOT}/cmake")


include(yae_lto)
enable_lto_globally()


# ---------------------------- External Dependencies -----------------------------

# https://github.com/Sunday111/ass main
option(ASS_ENABLE_TESTING "" ON)
option(ASS_FETCH_GOOGLE_TESTS "" OFF)
add_subdirectory(cloned_dependencies/Sunday111/ass SYSTEM)

# https://github.com/Sunday111/EverydayTools master
option(EDT_WITH_TESTS "" ON)
add_subdirectory(cloned_dependencies/Sunday111/EverydayTools SYSTEM)

# https://github.com/fmtlib/fmt 10.1.1
add_subdirectory(cloned_dependencies/fmt/10_1_1 SYSTEM)

# https://github.com/Sunday111/CppReflection for_yae
add_subdirectory(cloned_dependencies/Sunday111/CppReflection SYSTEM)

# https://github.com/nlohmann/json develop
add_subdirectory(cloned_dependencies/nlohmann/json SYSTEM)

# https://github.com/Sunday111/glad main
add_subdirectory(cloned_dependencies/Sunday111/glad SYSTEM)

# https://github.com/glfw/glfw master
add_subdirectory(cloned_dependencies/glfw/glfw SYSTEM)

# https://github.com/Sunday111/imgui_glfw_cmake main
add_subdirectory(cloned_dependencies/Sunday111/imgui_glfw_cmake SYSTEM)

# https://github.com/jeremy-rifkin/cpptrace main
add_subdirectory(cloned_dependencies/jeremy-rifkin/cpptrace SYSTEM)

# https://github.com/martinus/unordered_dense v4.1.2
add_subdirectory(cloned_dependencies/martinus/unordered_dense SYSTEM)

# https://github.com/Neargye/magic_enum v0.9.6
add_subdirectory(cloned_dependencies/Neargye/magic_enum SYSTEM)

# https://github.com/google/benchmark v1.8.3
option(BENCHMARK_ENABLE_TESTING "" OFF)
add_subdirectory(cloned_dependencies/google/benchmark/1_8_3 SYSTEM)

# https://github.com/google/googletest v1.15.0
option(INSTALL_GTEST "" OFF)
option(BUILD_GMOCK "" OFF)
add_subdirectory(cloned_dependencies/google/googletest/1_15_0 SYSTEM)

# https://github.com/ericniebler/range-v3 0.12.0
add_subdirectory(cloned_dependencies/range_v3 SYSTEM)

# --------------------------------- Own Modules ----------------------------------
add_subdirectory(cloned_dependencies/Sunday111/klgl/klgl)
add_subdirectory(src/verlet)
add_subdirectory(deps/yae/modules/examples/ecs)
add_subdirectory(deps/yae/modules/examples/ecs_benchmark)
add_subdirectory(deps/yae/modules/examples/ecs_tests)
add_subdirectory(deps/yae/modules/examples/yae_example_benchmark)
add_subdirectory(deps/yae/modules/examples/yae_example_benchmark_main)
add_subdirectory(deps/yae/modules/examples/yae_example_library)
add_subdirectory(deps/yae/modules/examples/yae_example_header_only)
add_subdirectory(deps/yae/modules/examples/yae_example_executable)
add_subdirectory(deps/yae/modules/examples/yae_example_tests)
add_subdirectory(deps/yae/modules/examples/yae_example_tests_main)
add_subdirectory(cloned_dependencies/Sunday111/klgl/benchmarks/mat_mul_sse)
add_subdirectory(cloned_dependencies/Sunday111/klgl/benchmarks/rotators)
add_subdirectory(cloned_dependencies/Sunday111/klgl/examples/geometry_shader)
add_subdirectory(cloned_dependencies/Sunday111/klgl/examples/minimal_cube)
add_subdirectory(cloned_dependencies/Sunday111/klgl/examples/minimal_quad)
add_subdirectory(cloned_dependencies/Sunday111/klgl/examples/painter2d)
add_subdirectory(cloned_dependencies/Sunday111/klgl/examples/textured_quad)
add_subdirectory(cloned_dependencies/Sunday111/klgl/examples/two_textures)
add_subdirectory(cloned_dependencies/Sunday111/klgl/tests)

enable_testing()
