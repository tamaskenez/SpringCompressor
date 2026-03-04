cmake_minimum_required(VERSION 3.31)
project(SpringCompressor VERSION 0.1.0)

if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/_d")
endif()

enable_testing()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GTest REQUIRED CONFIG)

set(TESTING_OUTPUT_DIR "${PROJECT_SOURCE_DIR}/_o")
file(MAKE_DIRECTORY ${TESTING_OUTPUT_DIR})

add_subdirectory(_submodules/JUCE)

# Mark all JUCE targets as SYSTEM so their headers don't trigger project warnings.
function(mark_juce_as_system dir)
    get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
    foreach(subdir IN LISTS subdirs)
        mark_juce_as_system("${subdir}")
    endforeach()
    get_property(targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        get_target_property(_aliased "${target}" ALIASED_TARGET)
        if(NOT _aliased)
            set_target_properties("${target}" PROPERTIES SYSTEM TRUE)
        endif()
    endforeach()
endfunction()
mark_juce_as_system("${CMAKE_CURRENT_SOURCE_DIR}/_submodules/JUCE")

include(clang_warnings.cmake)

set(CMAKE_COMPILE_WARNING_AS_ERROR 1)

add_subdirectory(_submodules/meadow)
add_subdirectory(src)
