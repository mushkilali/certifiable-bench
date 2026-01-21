##############################################################################
# CMakeLists.txt — certifiable-bench
#
# Production-standard CMake configuration for deterministic ML benchmarking.
#
# @traceability CB-MATH-001
#
# Copyright © 2026 The Murray Family Innovation Trust. All rights reserved.
# SPDX-License-Identifier: GPL-3.0-or-later
##############################################################################

cmake_minimum_required(VERSION 3.16)

project(certifiable-bench
    VERSION 1.0.0
    DESCRIPTION "Performance benchmarking for deterministic ML inference"
    LANGUAGES C
)

#─────────────────────────────────────────────────────────────────────────────
# Build Options
#─────────────────────────────────────────────────────────────────────────────

option(CB_BUILD_TESTS    "Build unit tests"           ON)
option(CB_BUILD_EXAMPLES "Build examples"             ON)
option(CB_ENABLE_RDTSC   "Enable x86 RDTSC backend"   ON)
option(CB_ENABLE_CNTVCT  "Enable ARM64 CNTVCT backend" ON)
option(CB_ENABLE_RISCV   "Enable RISC-V cycle backend" ON)

#─────────────────────────────────────────────────────────────────────────────
# C Standard and Compiler Settings
#─────────────────────────────────────────────────────────────────────────────

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Export compile_commands.json for IDE/tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#─────────────────────────────────────────────────────────────────────────────
# Compiler Flags — Strict for Safety-Critical Code
#─────────────────────────────────────────────────────────────────────────────

add_compile_options(
    # Warnings as errors for production quality
    -Wall
    -Wextra
    -Wpedantic
    -Werror

    # Additional strictness
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wold-style-definition
    -Wdouble-promotion

    # Determinism: disable floating-point optimisations
    -ffp-contract=off
    -fno-fast-math

    # No common symbols (link-time errors for duplicate definitions)
    -fno-common

    # Position-independent code for library
    -fPIC
)

# Debug vs Release
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
    add_definitions(-DCB_DEBUG=1)
else()
    add_compile_options(-O2 -DNDEBUG)
endif()

#─────────────────────────────────────────────────────────────────────────────
# Platform Detection
#─────────────────────────────────────────────────────────────────────────────

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
    set(CB_PLATFORM "x86_64")
    add_definitions(-DCB_PLATFORM_X86_64=1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64|arm64")
    set(CB_PLATFORM "arm64")
    add_definitions(-DCB_PLATFORM_ARM64=1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv64|riscv32")
    set(CB_PLATFORM "riscv")
    add_definitions(-DCB_PLATFORM_RISCV=1)
else()
    set(CB_PLATFORM "unknown")
    message(WARNING "Unknown platform: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

message(STATUS "Platform: ${CB_PLATFORM}")

#─────────────────────────────────────────────────────────────────────────────
# Include Directories
#─────────────────────────────────────────────────────────────────────────────

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

#─────────────────────────────────────────────────────────────────────────────
# Source Files
#─────────────────────────────────────────────────────────────────────────────

# Timer module (SRS-001)
set(TIMER_SOURCES
    src/timer/timer.c
)

# Metrics module (SRS-002)
set(METRICS_SOURCES
    src/metrics/metrics.c
)

# Platform module (SRS-006)
set(PLATFORM_SOURCES
    src/platform/platform.c
)

# Verify module (SRS-004)
set(VERIFY_SOURCES
    src/verify/verify.c
)

# Runner module (SRS-003)
set(RUNNER_SOURCES
    src/runner/runner.c
)

# Report module (SRS-005)
set(REPORT_SOURCES
    src/report/report.c
)

# All library sources
set(LIB_SOURCES
    ${TIMER_SOURCES}
    ${METRICS_SOURCES}
    ${PLATFORM_SOURCES}
    ${VERIFY_SOURCES}
    ${RUNNER_SOURCES}
    ${REPORT_SOURCES}
)

#─────────────────────────────────────────────────────────────────────────────
# Library Target
#─────────────────────────────────────────────────────────────────────────────

add_library(certifiable-bench STATIC ${LIB_SOURCES})

target_include_directories(certifiable-bench
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link math library if needed
target_link_libraries(certifiable-bench PUBLIC m)

#─────────────────────────────────────────────────────────────────────────────
# Tests
#─────────────────────────────────────────────────────────────────────────────

if(CB_BUILD_TESTS)
    enable_testing()

    # Timer tests
    add_executable(test_timer tests/unit/test_timer.c)
    target_link_libraries(test_timer PRIVATE certifiable-bench)
    add_test(NAME test_timer COMMAND test_timer)

    # Metrics tests
    add_executable(test_metrics tests/unit/test_metrics.c)
    target_link_libraries(test_metrics PRIVATE certifiable-bench)
    add_test(NAME test_metrics COMMAND test_metrics)

    # Platform tests
    add_executable(test_platform tests/unit/test_platform.c)
    target_link_libraries(test_platform PRIVATE certifiable-bench)
    add_test(NAME test_platform COMMAND test_platform)

    # Verify tests
    add_executable(test_verify tests/unit/test_verify.c)
    target_link_libraries(test_verify PRIVATE certifiable-bench)
    add_test(NAME test_verify COMMAND test_verify)

    # Runner tests
    add_executable(test_runner tests/unit/test_runner.c)
    target_link_libraries(test_runner PRIVATE certifiable-bench)
    add_test(NAME test_runner COMMAND test_runner)

    # Report tests
    add_executable(test_report tests/unit/test_report.c)
    target_link_libraries(test_report PRIVATE certifiable-bench)
    add_test(NAME test_report COMMAND test_report)

    # Custom target to run all tests
    add_custom_target(test-all
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS test_timer test_metrics test_platform test_verify test_runner test_report
        COMMENT "Running all certifiable-bench tests"
    )
endif()

#─────────────────────────────────────────────────────────────────────────────
# Examples
#─────────────────────────────────────────────────────────────────────────────

if(CB_BUILD_EXAMPLES)
    # Example: basic benchmark — placeholder
    # add_executable(example_basic examples/basic_benchmark.c)
    # target_link_libraries(example_basic PRIVATE certifiable-bench)
endif()

#─────────────────────────────────────────────────────────────────────────────
# Installation
#─────────────────────────────────────────────────────────────────────────────

install(TARGETS certifiable-bench
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY docs/
    DESTINATION share/doc/certifiable-bench
    FILES_MATCHING
        PATTERN "*.md"
        PATTERN "*.txt"
)

#─────────────────────────────────────────────────────────────────────────────
# Summary
#─────────────────────────────────────────────────────────────────────────────

message(STATUS "")
message(STATUS "certifiable-bench configuration:")
message(STATUS "  Version:      ${PROJECT_VERSION}")
message(STATUS "  Platform:     ${CB_PLATFORM}")
message(STATUS "  Build type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Compiler:   ${CMAKE_C_COMPILER}")
message(STATUS "  Build tests:  ${CB_BUILD_TESTS}")
message(STATUS "  Build examples: ${CB_BUILD_EXAMPLES}")
message(STATUS "")
