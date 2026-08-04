cmake_minimum_required(VERSION 3.28)

project(AmberRoom CXX)

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
    # Find Threads (необходимо только для многопоточного Boehm GC)
    find_package(Threads REQUIRED)
endif()

# Find Boehm GC
find_package(GC QUIET)
if(NOT GC_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GC IMPORTED_TARGET bdw-gc)
        if(GC_FOUND)
            add_library(GC::GC ALIAS PkgConfig::GC)
        endif()
    endif()
endif()
if(NOT GC_FOUND)
    message(FATAL_ERROR "Boehm GC not found. Install libgc or provide it via Conan.")
endif()

# Find GTest
find_package(GTest REQUIRED)

# target lib
add_library(amber_room_lib)

target_sources(amber_room_lib
    PUBLIC FILE_SET CXX_MODULES BASE_DIRS src
        FILES
            src/immutable_ptr/immutable_ptr.cppm
            src/memory/scoped_gc_redirection.cppm
)

target_include_directories(amber_room_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(amber_room_lib PUBLIC GC::GC)

if(AMBER_ROOM_MULTITHREADED)
    target_link_libraries(amber_room_lib PUBLIC Threads::Threads)
    target_compile_definitions(amber_room_lib PUBLIC GC_THREADS)
    message(STATUS "AmberRoom: Multithreading enabled (GC_THREADS is defined)")
else()
    message(STATUS "AmberRoom: Single-threaded mode (GC_THREADS is NOT defined)")
endif()


target_compile_features(amber_room_lib PRIVATE cxx_std_23)

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
