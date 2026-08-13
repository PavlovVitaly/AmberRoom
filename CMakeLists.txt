cmake_minimum_required(VERSION 3.28)

project(AmberRoom CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# enable c++ modules
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# include generated conan-files
if(EXISTS ${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
    include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AMBER_ROOM_MULTITHREADED "Enable multithreading support and thread-safe GC" ON)

if(AMBER_ROOM_MULTITHREADED)
    find_package(Threads REQUIRED)
endif()

# Find Boehm GC
find_package(BDWgc REQUIRED)
# Find Boost PFR
find_package(pfr REQUIRED)

# target lib
add_library(amber_room_lib)

target_sources(amber_room_lib
    PUBLIC FILE_SET CXX_MODULES BASE_DIRS src
        FILES
            src/immutable_ptr/immutable_ptr.cppm
            src/memory/scoped_gc_redirection.cppm
            src/memory/gc_allocators.cppm
            src/std_aliases/std_aliases.cppm
            src/restrictions/restrictions.cppm
)

target_include_directories(amber_room_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(amber_room_lib
    PUBLIC
        BDWgc::BDWgc
        pfr::pfr)

if(AMBER_ROOM_MULTITHREADED)
    target_link_libraries(amber_room_lib PUBLIC Threads::Threads)
    target_compile_definitions(amber_room_lib PUBLIC GC_THREADS)
    message(STATUS "AmberRoom: Multithreading enabled (GC_THREADS is defined)")
else()
    message(STATUS "AmberRoom: Single-threaded mode (GC_THREADS is NOT defined)")
endif()


target_compile_features(amber_room_lib PRIVATE cxx_std_23)

# Find GTest
find_package(GTest REQUIRED)

# target tests
add_executable(utests
    tests/immutable_ptr_tests.cpp
)

target_link_libraries(utests
    PRIVATE
        amber_room_lib
        GTest::gtest_main
)

if(AMBER_ROOM_MULTITHREADED)
    target_compile_definitions(utests PRIVATE AMBER_ROOM_TEST_MULTITHREADED)
endif()

# for integrations with CTest
enable_testing()
include(GoogleTest)
gtest_discover_tests(utests)

find_package(benchmark REQUIRED)
find_package(immer REQUIRED)

# =====================================================================
# ISOLATED TARGETS FOR BENCHMARKS
# =====================================================================

# Single-threaded trivial benchmark (memory allocation)
add_executable(bench_trivial benchmarks/immutable_ptr/immutable_ptr_trivial_bench.cpp)
target_link_libraries(bench_trivial PRIVATE amber_room_lib benchmark::benchmark immer::immer)

# Single-threaded benchmark with destructors
add_executable(bench_destructor benchmarks/immutable_ptr/immutable_ptr_obj_with_destructor_bench.cpp)
target_link_libraries(bench_destructor PRIVATE amber_room_lib benchmark::benchmark immer::immer)

# Single-threaded benchmark with strings (SSO and heap-allocated)
add_executable(bench_strings benchmarks/immutable_ptr/immutable_ptr_obj_with_string_bench.cpp)
target_link_libraries(bench_strings PRIVATE amber_room_lib benchmark::benchmark immer::immer)

# Multi-threaded trivial benchmark (Contention / TLAB performance)
add_executable(bench_mt_trivial benchmarks/immutable_ptr/multithread_immutable_ptr_trivial_bench.cpp)
target_link_libraries(bench_mt_trivial PRIVATE amber_room_lib benchmark::benchmark immer::immer)

# Multi-threaded benchmark with destructors
add_executable(bench_mt_destructor benchmarks/immutable_ptr/multithread_immutable_ptr_obj_with_destructor_bench.cpp)
target_link_libraries(bench_mt_destructor PRIVATE amber_room_lib benchmark::benchmark immer::immer)

# Multi-threaded benchmark with strings (Shared / Memory Contention)
add_executable(bench_mt_strings benchmarks/immutable_ptr/multithread_immutable_ptr_obj_with_string_bench.cpp)
target_link_libraries(bench_mt_strings PRIVATE amber_room_lib benchmark::benchmark immer::immer)
