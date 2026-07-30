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

# Linking deps
find_package(GTest REQUIRED)

# target lib
add_library(amber_room_lib)

target_sources(amber_room_lib
    PUBLIC FILE_SET CXX_MODULES BASE_DIRS src
        FILES
            src/const_ptr/const_ptr.cppm
)

target_include_directories(amber_room_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(amber_room_lib PRIVATE cxx_std_23)

# target tests
add_executable(utests
    tests/const_ptr_tests.cpp
)

target_link_libraries(utests
    PRIVATE
        amber_room_lib
        GTest::gtest_main
)

# for integrations with CTest
enable_testing()
include(GoogleTest)
gtest_discover_tests(utests)
