# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
cmake -S . -B _b -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build _b
```

JUCE is included as a git submodule at `_submodules/JUCE`. If it's missing, run:
```bash
git submodule update --init --recursive
```

## Linting / Formatting

```bash
./clang-format-all.sh
```

## Architecture

Avoid macOS-specific code whenever possible.