cmake_minimum_required(VERSION 3.20)
project(graph VERSION 1.0.0 LANGUAGES C)

# Updated to C99 to support for-loop variable declarations
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# FetchContent for external dependencies (SQLite style)
include(FetchContent)

# Unity Test Framework for comprehensive testing
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.5.2
)

# SQLite amalgamation (latest stable)
FetchContent_Declare(
    sqlite3
    URL https://www.sqlite.org/2024/sqlite-autoconf-3450100.tar.gz
    URL_HASH SHA1=650305e234add12fc1e6bef0b365d86a087b3d38
)

FetchContent_MakeAvailable(unity sqlite3)

# Configure Unity with double precision support
target_compile_definitions(unity PUBLIC
    UNITY_INCLUDE_DOUBLE
    UNITY_DOUBLE_PRECISION=1e-12
)

# Build SQLite3 as a static library
add_library(sqlite3_lib STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3_lib PUBLIC ${sqlite3_SOURCE_DIR})
# SQLite has intentional fallthrough cases, so disable implicit-fallthrough warnings for it
target_compile_options(sqlite3_lib PRIVATE -Wno-implicit-fallthrough)

# Compiler flags (SQLite standards: ANSI C-89, 80 char lines, 2-space indent)  
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Wextra -pedantic -fprofile-arcs -ftest-coverage -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable")

# SQLite-style warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-long-long")

# Code coverage support
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)
if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# Add coverage build type
set(CMAKE_C_FLAGS_COVERAGE 
    "-O0 -fprofile-arcs -ftest-coverage"
    CACHE STRING "Flags used by C compiler during coverage builds."
    FORCE)

# Include directories
include_directories(include)
include_directories(${sqlite3_SOURCE_DIR})

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# Enable testing
enable_testing()

# Configure CTest to allow running from source directory
set(CMAKE_CTEST_COMMAND ctest)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/CTestConfig.cmake.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/CTestConfig.cmake"
    @ONLY
)

# Custom target to run all tests
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_graph
    COMMENT "Running all tests..."
)

# Custom target for coverage
add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
    COMMAND ${CMAKE_CTEST_COMMAND} -R graph_coverage_tests
    COMMAND lcov --capture --directory . --output-file coverage.info
    COMMAND lcov --remove coverage.info '/usr/*' --output-file coverage.info
    COMMAND lcov --remove coverage.info '*/tests/*' --output-file coverage.info
    COMMAND genhtml coverage.info --output-directory coverage
    DEPENDS test_graph_coverage
    COMMENT "Generating coverage report..."
)