# CLAUDE.md

## Build

```bash
cmake -S . -B _b -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build _b
```

## Directory structure

The _submodules subdir contains only external projects which should not be changed.

## Architecture

Avoid MacOS-specific code whenever possible.

## CMake hints
Set up the project for C++26.
Prefer adding source files to targets via globbing. Collect and set up source files like this:

```CMake
file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})
```

## Coding style

Use snake-case for symbols, except for type use CamelCase.
Call `clang-format -i` for *.cpp and *.h files after changing them.
