cmake_minimum_required(VERSION 3.31)
project(SpringCompressor VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(_submodules/JUCE)
add_subdirectory(src)
