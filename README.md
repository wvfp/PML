# PML C++23 — Picture Markup Language (Rewrite)

C++23 rewrite of PML, a Lisp-style DSL for code-to-image generation.
Skia GPU backend, giflib animation export, nlohmann-json for serialization.

## Build

```bash
# Configure (no vcpkg needed — third-party libs are git-cloned by CMake
# on first run, or reused from third_party/<name>/ if present)
cmake --preset debug

# Build
cmake --build --preset debug

# Run tests
ctest --preset debug
```

## Dependencies

- **Skia** — pre-built static libs at `G:/Project/skia/out/llvm.x64.debug/`
  (override via `PML_SKIA_DIR` / `PML_SKIA_OUT` cache variables). The top-level
  `CMakeLists.txt` imports `skia.lib` + `skcms.lib` (+ sksg, skshaper, svg,
  skottie, skresources, jsonreader, expat, bentleyottmann) — no Skia
  compilation is performed by us.
- **giflib** — fetched via CMake `FetchContent` (HTTPS) into
  `third_party/giflib/` if not already present.
- **nlohmann-json** — header-only, vendored under `third_party/json/`.
- **Google Test** — vendored under `third_party/googletest/`.
- **libpng** — vendored under `third_party/libpng/`.
- **Cairo** — *optional* (`PML_BUILD_CAIRO=OFF` by default; cairo on Windows
  MSVC is fragile because it needs pixman/zlib/freetype).
- **freetype** — *optional* (`PML_BUILD_FREETYPE=OFF` by default; not used yet).

No vcpkg required. The `pml_third_party()` helper in the top-level
`CMakeLists.txt` prefers a local `third_party/<name>/` checkout and falls back
to a `FetchContent_Declare` with HTTPS so it works behind firewalls that block
SSH (which vcpkg/git@github.com needs).

## License

MIT

## Behavior Differences

See [BEHAVIOR_DIFFERENCES.md](BEHAVIOR_DIFFERENCES.md) for known differences between the Python and C++ implementations.
