# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

The project currently builds a single static library target: `SpringCompressorEngine`.

JUCE is included as a git submodule at `_submodules/JUCE`. If it's missing, run:
```bash
git submodule update --init --recursive
```

## Linting / Formatting

```bash
./clang-format-all.sh
```

This runs `clang-format -i` on all git-tracked `.cpp`, `.h`, and `.hpp` files using the `.clang-format` config at the project root. Key style rules: 4-space indent, 120 column limit, Linux brace style, left pointer alignment.

## Tests

No test infrastructure is configured yet.

## Architecture

The project is an audio dynamic range compressor plugin, intended to use the JUCE framework for audio I/O and plugin format support (VST/AU/LV2).

For now the project won't be tested on Windows, Linux but we need to avoid any MacOS specific code whenever possible.

**Core engine (`src/engine/`):**
- `engine.h` — Abstract `Engine` base class with a `make_engine()` factory function
- `engine.cpp` — Concrete `EngineImpl` struct returned by the factory; currently a stub

The factory pattern + abstract base class separates the public interface from implementation details, following a pimpl-style approach.

**Experimental MATLAB script (`matlab/p1.m`):**
An unspecified experiment MATLAB script, can be ignorede for now.

**Research references** are in `docs/notes.txt`, pointing to Giannoulis et al. (2013) on parameter automation in dynamic range compressors as the primary academic reference.
