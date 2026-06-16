# PML C++23 — Picture Markup Language (Rewrite)

C++23 rewrite of PML, a Lisp-style DSL for code-to-image generation.
Skia GPU backend, giflib animation export, nlohmann-json for serialization.

## Build

```bash
# Prerequisites
export VCPKG_ROOT=/path/to/vcpkg

# Configure
cmake --preset debug

# Build
cmake --build --preset debug

# Run tests
ctest --preset debug
```

## Dependencies

- **Skia** — GPU-accelerated 2D rendering (replaces Cairo + stb_image)
- **giflib** — GIF export for animations
- **nlohmann-json** — JSON serialization
- **Google Test** — Unit testing

Managed via [vcpkg](https://github.com/microsoft/vcpkg).

## License

MIT

## Behavior Differences

See [BEHAVIOR_DIFFERENCES.md](BEHAVIOR_DIFFERENCES.md) for known differences between the Python and C++ implementations.
