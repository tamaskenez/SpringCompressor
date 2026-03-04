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

# Suppress warnings on JUCE amalgam source files compiled inside project targets.
# JUCE adds module sources as INTERFACE_SOURCES to module targets; they are then
# compiled as part of the consuming plugin target. We walk each project target's
# transitive link libraries, collect all JUCE INTERFACE_SOURCES, and disable
# warnings on them in the context of the consuming target.
# Runs deferred so all targets are fully configured first.
function(_juce_collect_interface_sources lib result_var visited_var)
    if("${lib}" IN_LIST ${visited_var})
        return()
    endif()
    list(APPEND ${visited_var} "${lib}")
    set(${visited_var} "${${visited_var}}" PARENT_SCOPE)

    get_target_property(iface_sources "${lib}" INTERFACE_SOURCES)
    if(iface_sources)
        list(APPEND ${result_var} ${iface_sources})
        set(${result_var} "${${result_var}}" PARENT_SCOPE)
    endif()

    get_target_property(link_libs "${lib}" INTERFACE_LINK_LIBRARIES)
    if(link_libs)
        foreach(dep IN LISTS link_libs)
            if(NOT dep MATCHES "^\\$<" AND TARGET "${dep}")
                _juce_collect_interface_sources("${dep}" ${result_var} ${visited_var})
                set(${result_var} "${${result_var}}" PARENT_SCOPE)
                set(${visited_var} "${${visited_var}}" PARENT_SCOPE)
            endif()
        endforeach()
    endif()
endfunction()

function(_suppress_juce_source_warnings_in_dir dir)
    get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
    foreach(subdir IN LISTS subdirs)
        _suppress_juce_source_warnings_in_dir("${subdir}")
    endforeach()
    get_property(targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        get_target_property(type "${target}" TYPE)
        if(type STREQUAL "INTERFACE_LIBRARY" OR type STREQUAL "UTILITY")
            continue()
        endif()
        set(visited "")
        set(juce_sources "")

        # Case 1: JUCE files added directly as PRIVATE sources to the target.
        get_target_property(own_sources "${target}" SOURCES)
        if(own_sources)
            foreach(source IN LISTS own_sources)
                if(source MATCHES ".*[/\\\\]JUCE[/\\\\].*" OR source MATCHES ".*[/\\\\]VST3_SDK[/\\\\].*")
                    list(APPEND juce_sources "${source}")
                endif()
            endforeach()
        endif()

        # Case 2: JUCE module files added as INTERFACE_SOURCES of linked libraries.
        get_target_property(link_libs "${target}" LINK_LIBRARIES)
        if(link_libs)
            foreach(lib IN LISTS link_libs)
                if(NOT lib MATCHES "^\\$<" AND TARGET "${lib}")
                    _juce_collect_interface_sources("${lib}" juce_sources visited)
                endif()
            endforeach()
        endif()

        foreach(source IN LISTS juce_sources)
            if(NOT source MATCHES "^\\$<")
                set_source_files_properties("${source}"
                    TARGET_DIRECTORY "${target}"
                    PROPERTIES COMPILE_FLAGS "-w")
            endif()
        endforeach()
    endforeach()
endfunction()
cmake_language(DEFER CALL _suppress_juce_source_warnings_in_dir "${CMAKE_CURRENT_SOURCE_DIR}/src")
