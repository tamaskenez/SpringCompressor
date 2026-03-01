# CLAUDE.md

## Build

Some dependencies are provided by Conan. These needs to be built beforehand. The following script installs conan dependencies, Debug and Release, into the `_d` directory.

```bash
./1_build_conan.sh
```

Other dependencies are vendored from the _submodules directory. To configure and build the project:

```bash
cmake -S . -B _b -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build _b
```

## Directory structure

The _submodules subdir contains mostly external projects which should not be changed, except for _submodules/meadow which is our own project and can be changed.

Some non-tracked dirs:

- _b for building
- _d for dependencies (3rd party libs)
- _o for test output

## Architecture

Avoid MacOS-specific code whenever possible.

## CMake hints

Set up the project for C++23.
Prefer adding source files to targets via globbing. Collect and set up source files like this:

```CMake
file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})
```

## Coding style

Use snake-case for symbols, except for type use CamelCase.
Call `clang-format -i` for *.cpp and *.h files after changing them.
When possible, use std::format, std::print, std::println instead of the older, overloaded "<<" operator based techniques.
