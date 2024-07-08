cmake_minimum_required(VERSION 3.20)

project(YAE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(YAE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/deps/yae")
set(YAE_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${YAE_ROOT}/cmake")


# ---------------------------- External Dependencies -----------------------------

# https://github.com/Sunday111/EverydayTools for_yae
option(EDT_WITH_TESTS "" ON)
add_subdirectory(deps/yae/cloned_repositories/edt SYSTEM)

# https://github.com/google/googletest v1.14.0
option(INSTALL_GTEST "" OFF)
option(BUILD_GMOCK "" OFF)
add_subdirectory(deps/yae/cloned_repositories/google/googletest/1_14_0 SYSTEM)

# https://github.com/google/benchmark v1.8.3
option(BENCHMARK_ENABLE_TESTING "" OFF)
add_subdirectory(deps/yae/cloned_repositories/google/googlebench/1_8_3 SYSTEM)

# https://github.com/martinus/unordered_dense v4.1.2
add_subdirectory(deps/yae/cloned_repositories/martinus/unordered_dense SYSTEM)

# https://github.com/fmtlib/fmt 10.1.1
add_subdirectory(deps/yae/cloned_repositories/fmt/10_1_1 SYSTEM)

# https://github.com/Sunday111/CppReflection for_yae
add_subdirectory(deps/yae/cloned_repositories/CppReflection SYSTEM)

# --------------------------------- Own Modules ----------------------------------
add_subdirectory(deps/yae/modules/examples/ecs)
add_subdirectory(deps/yae/modules/examples/ecs_tests)
add_subdirectory(deps/yae/modules/examples/yae_example_library)
add_subdirectory(deps/yae/modules/examples/yae_example_header_only)
add_subdirectory(deps/yae/modules/examples/yae_example_tests_main)
add_subdirectory(deps/yae/modules/examples/yae_example_tests)
add_subdirectory(deps/yae/modules/examples/yae_example_benchmark)
add_subdirectory(deps/yae/modules/examples/yae_example_benchmark_main)
add_subdirectory(deps/yae/modules/examples/ecs_benchmark)
add_subdirectory(deps/yae/modules/examples/yae_example_executable)
add_subdirectory(modules/verlet)

enable_testing()
