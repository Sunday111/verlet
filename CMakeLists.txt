cmake_minimum_required(VERSION 3.20)

project(Verlet)

set(CMAKE_CXX_EXTENSIONS OFF)

set(YAE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/deps/yae")
set(YAE_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${YAE_ROOT}/cmake")

add_compile_options(/std:c++latest)

# ---------------------------- External Dependencies -----------------------------

# https://github.com/Sunday111/EverydayTools for_yae
option(EDT_WITH_TESTS "" ON)
add_subdirectory(cloned_modules/edt SYSTEM)

# https://github.com/Sunday111/CppReflection for_yae
add_subdirectory(cloned_modules/CppReflection SYSTEM)

# https://github.com/fmtlib/fmt 10.1.1
add_subdirectory(cloned_modules/fmt/10_1_1 SYSTEM)

# https://github.com/google/benchmark v1.8.3
option(BENCHMARK_ENABLE_TESTING "" OFF)
add_subdirectory(cloned_modules/google/googlebench/1_8_3 SYSTEM)

# https://github.com/google/googletest v1.14.0
option(INSTALL_GTEST "" OFF)
option(BUILD_GMOCK "" OFF)
add_subdirectory(cloned_modules/google/googletest/1_14_0 SYSTEM)

# https://github.com/martinus/unordered_dense v4.1.2
add_subdirectory(cloned_modules/martinus/unordered_dense SYSTEM)

# https://github.com/nlohmann/json develop
add_subdirectory(cloned_modules/nlohmann/json SYSTEM)

# https://github.com/Sunday111/glad main
add_subdirectory(cloned_modules/glad SYSTEM)

# https://github.com/glfw/glfw master
add_subdirectory(cloned_modules/glfw SYSTEM)

# https://github.com/Sunday111/imgui_glfw_cmake main
add_subdirectory(cloned_modules/imgui_glfw_cmake SYSTEM)

# https://github.com/jeremy-rifkin/cpptrace main
add_subdirectory(cloned_modules/cpptrace SYSTEM)

# --------------------------------- Own Modules ----------------------------------
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
add_subdirectory(src/klgl)
add_subdirectory(src/verlet)
add_subdirectory(src/verlet_fast)

enable_testing()
