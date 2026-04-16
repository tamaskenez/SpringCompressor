# CLAUDE.md

## Directory structure

The _submodules subdirectory contains external dependencies which should not be changed, except for
_submodules/meadow which is our own project and can be changed.

The following directories are not tracked by git:

- _b: build directory
- _d: local install dir for dependencies (3rd party libs)
- _o: test output

Claude should use the `_b` directory for building.

## Build

Some dependencies are provided by Conan. These need to be built beforehand. The following script installs Conan
dependencies, Debug and Release, into the `_d` directory.

```bash
./1_build_conan.sh
```

Other dependencies are vendored from the _submodules directory. To configure and build the project:

```bash
cmake -S . -B _b -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build _b
```

## Coding Guidelines

Avoid platform-specific code whenever possible.

### Architecture

In our projects there is a central, main component which

- receives all messages in a central dispatcher
- initiates all state transitions
- delegates work to subcomponents by instructing them, without any inversion of control

The UI component

- is primarily a passive drawing surface
- doesn't make any decisions
- user actions (button click, etc..) are mostly routed back to the main component which decides what to do
- might get direct but read-only access to the application state
- ideally reports user actions with asynchronous messages but direct function call to an interface implemented by the
  main component is also possible in simpler cases.

These are guidelines, the starting point. They can be violated when the cost/benefit ratio is better otherwise.

Example in case of JUCE AudioProcessor and AudioProcessorEditor: The strong coupling is from the AudioProcessor towards
the editor. The other way is loose coupling.

### Conventions

Some frequency parameter names are postfixed with "_hps" which means half-cycles per second, that is, normalized to the
Nyquist frequency (MATLAB convention).

## C++ Coding guidelines

- Use snake-case for symbols, except for type use CamelCase.
- Call `clang-format -i` for *.cpp and *.h files after changing them.
- When possible, use std::format, std::print, std::println instead of the older, overloaded "<<" operator based techniques.
- Always put for-loop bodies in curly-braces.
- The header <meadow/cppext.h> lifts a certain subset of symbols from the std namespace into the global
  namespace. In this project we can leave out the `std::` prefixes on those core features that should really be part of
  the language instead of the standard library. This is to reduce line noise in the code.
- include order: start with closest header (cpp's own header) and proceed towards farther headers (third-party, system).
- include style: use double quotes for in-project headers, angle-brackets for everything else.
- the branches of an `if` statements must always use curly-braces, even for single-line branches.

### Numeric conversions

The project has many C++ warnings enabled as errors (see clang_warnings.cmake) including
numeric conversion warnings. To silence the numeric conversion warnings, instead of
`static_cast` use the following custom functions detailed below (defined in <meadow/cppext.h>). Some of these custom
casting functions have a debug check and restrict their parameter types.

- Sign conversion: Use sucast() and uscast() to convert between unsigned and signed types of the same size.
- Integer -> integer: Use iicast<target-integer-type>() to narrow and optionally change signedness, too.
- float -> integer: Use iround<integer-type>, ifloor<integer-type> or iceil<integer-type>.
- float -> double: Use ffcast<float> to cast from double to float.


## CMake style

Set up the project for C++23.
Prefer adding source files to targets via globbing. Collect and set up source files like this:

```CMake
file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})
```

## MATLAB coding guidelines

For now keep the scripts Octave-compatible. However, let me know if using a MATLAB feature would result in significant
benefits. In that case we'll switch to MATLAB.
Current Octave version is 11.1.0

