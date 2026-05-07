cmake_minimum_required(VERSION 3.16)
project(rvc_claude CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---------- Coverage (gcc/clang only) --------------------------------------
# Enable with -DRVC_COVERAGE=ON. Forces -O0 -g and threads --coverage through
# all rvc_* libraries and the gtest binaries. Used by CI's `coverage` job to
# enforce NFR-TEST-001 (>=85% line, >=75% branch).
option(RVC_COVERAGE "Build with gcov/gcc --coverage instrumentation" OFF)
if(RVC_COVERAGE)
    if(MSVC)
        message(FATAL_ERROR "RVC_COVERAGE requires gcc or clang, not MSVC")
    endif()
    message(STATUS "RVC_COVERAGE=ON  (instrumented build, -O0 -g --coverage)")
    add_compile_options(-O0 -g --coverage -fno-inline)
    add_link_options(--coverage)
endif()

# ---------- Layered targets (see arch/design/packages.md) -------------------
# rvc_ports: header-only port interfaces + value types.
add_library(rvc_ports INTERFACE)
target_include_directories(rvc_ports INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

# rvc_domain: navigation + power policies, depend on ports only.
add_library(rvc_domain STATIC
    src/domain/default_navigation_policy.cpp
    src/domain/default_cleaning_power_policy.cpp)
target_include_directories(rvc_domain PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(rvc_domain PUBLIC rvc_ports)

# rvc_app: CleaningCoordinator orchestration. Depends on ports + domain.
add_library(rvc_app STATIC
    src/app/cleaning_coordinator.cpp)
target_include_directories(rvc_app PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(rvc_app PUBLIC rvc_ports rvc_domain)

# rvc_tech: grid world adapters. Depends on ports only (not on app/domain).
add_library(rvc_tech STATIC
    src/tech/grid_world.cpp)
target_include_directories(rvc_tech PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(rvc_tech PUBLIC rvc_ports)

# rvc_core: convenience meta-target for tests / sim.
add_library(rvc_core INTERFACE)
target_link_libraries(rvc_core INTERFACE rvc_ports rvc_domain rvc_app rvc_tech)

# ---------- Simulator binary -----------------------------------------------
add_executable(rvc_sim tools/rvc_sim.cpp)
target_link_libraries(rvc_sim PRIVATE rvc_core)

# ---------- Tests ----------------------------------------------------------
option(RVC_BUILD_TESTS "Build GTest unit and integration tests" ON)
if(RVC_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    FetchContent_MakeAvailable(googletest)

    # Suppress warnings on GTest itself.
    if(MSVC)
        target_compile_options(gtest PRIVATE /W0)
        target_compile_options(gtest_main PRIVATE /W0)
        target_compile_options(gmock PRIVATE /W0)
        target_compile_options(gmock_main PRIVATE /W0)
    endif()

    add_subdirectory(tests)
endif()
