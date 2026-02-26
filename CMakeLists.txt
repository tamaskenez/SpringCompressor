cmake_minimum_required(VERSION 3.31)
project(SpringCompressor VERSION 0.1.0)

if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/_d")
endif()

enable_testing()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(_submodules/JUCE)

find_package(GTest REQUIRED CONFIG)

add_subdirectory(src)
