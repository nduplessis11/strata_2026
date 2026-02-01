# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

This project uses CMake with presets. Requires GCC 14+ for C++23 features (`std::print`, `std::stacktrace`).

```bash
# Linux (GCC)
cmake --preset gcc-debug
cmake --build --preset gcc-debug

# Linux (GCC Release)
cmake --preset gcc-release
cmake --build --preset gcc-release

# Cross-compile to Windows (MinGW)
cmake --preset mingw64-debug
cmake --build --preset mingw64-debug

# Windows (MSVC)
cmake --preset msvc-debug
cmake --build --preset msvc-debug
```

**Executables output to:**
- Linux: `build/<preset>/strata_demo`
- Windows cross: `build/<preset>/strata_demo.exe`
- Windows native: `out/build/<preset>/strata_demo.exe`

## Architecture

**Header-only library** in `include/` with a demo app in `src/main.cpp`.

### Error Handling (`include/error.hpp`)

Two-level error system under `strata::err`:

- **`expect(bool, msg)`** - Invariant assertions that terminate on failure. Use for conditions that should never be false.
- **`ensure(bool, msg)`** - Contract checks that throw `std::runtime_error`. Use for recoverable/fallible operations.
- **`debug_expect(predicate, msg)`** - Debug-only assertions compiled out in release (`NDEBUG`). Supports lazy evaluation via callables.

All functions support formatted messages: `expect(cond, "value: {}", val)` with compile-time format string validation.

### Logging (`include/log.hpp`)

Simple `strata::log::Level` enum (trace, debug, info, warn, error, fatal, off) with `to_string()` conversion.

## Code Conventions

- Namespace structure: `strata::` with nested `err`, `log`, `err::detail`
- C++23 with GNU extensions (`gnu++23` by default, configurable via `STRATA_USE_GNU_EXTENSIONS`)
- `[[nodiscard]]` on utility functions
- Requires clauses for template constraints
- `#pragma once` for header guards

## CMake Options

- `STRATA_USE_GNU_EXTENSIONS` (ON) - Use gnu++23 vs c++23
- `STRATA_WARNINGS` (ON) - Enable extra compiler warnings
- `STRATA_COPY_MINGW_RUNTIME_DLLS` (ON) - Copy MinGW DLLs for Windows builds
