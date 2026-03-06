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

## C++ Guidelines

Avoid platform-specific code whenever possible.

### Architecture

The JUCE framework pushes towards a certain relationship between AudioProcessor and AudioProcessorEditor:
The AudioProcessorEditor knows about the concrete AudioProcessor while the AudioProcessor is discouraged
to even talk to the generic AudioProcessorEditor. This makes the AudioProcessorEditor higher level.

In our case we push against this approach and prefer the AudioProcessor be higher-level and the one that knows about the other.

### Numeric conversion

The project has many C++ warnings enabled as errors (see clang_warnings.cmake) including
numeric conversion warnings. To silence the numeric conversion warnings, instead of
`static_cast` use the following custom functions (defined in <meadow/cppext.h>):

#### Sign conversion

Use sucast() and uscast() to convert between unsigned and signed types of the same size.

#### Integer -> integer

Use iicast<target-integer-type>() to narrow.

#### float -> integer

Use iround<integer-type>, ifloor<integer-type> or iceil<integer-type>.

#### float -> double

Use ffcast<float> to cast from double to float.

### Include order and style

```
#include "corresponding_header.h"

#include "headers_of_same_target.h"

#include "headers_from_this_project.h"

#include <third_party_headers.h>

#include <c++_standard_library_headers>

#include <c_standard_library_headers.h>
```

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
