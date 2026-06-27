# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Test

```bash
# Configure (uses Visual Studio 18 2026 generator)
cmake --preset debug

# Build
cmake --build --preset debug

# Run all tests
ctest --preset debug

# Run specific test binary directly (fastest for iteration)
./build/debug/tests/Debug/pml-tests.exe
./build/debug/tests/Debug/pml-builtins-smoke.exe
./build/debug/tests/Debug/pml-layer-test.exe
./build/debug/tests/Debug/pml-filter-test.exe

# Run a single GTest test case
./build/debug/tests/Debug/pml-tests.exe --gtest_filter="PMLRuntimeTest.ExecuteSimpleArithmetic"

# Add a new test: add source to tests/CMakeLists.txt with add_executable + gtest_discover_tests
```

- **pml.exe 输出路径**（Windows MSVC 多配置生成器）：
  ```
  ./build/debug/src/pml/cli/Debug/pml.exe
  ```
  PML源文件路径相对于CWD传入。`.pml`中的`render`输出路径相对于源文件所在目录。
- CMake options: `PML_BUILD_TESTS`, `PML_BUILD_CLI`, `PML_BUILD_MCP`, `PML_BUILD_GIF`, `PML_BUILD_SKIA`
- Skia is **pre-built** at `third_party/skia/out/` — see `CMakePresets.json` for paths
- Third-party (googletest, nlohmann_json, zlib, libpng, giflib) vendored in `third_party/`, CMake prefers local checkout over git clone
- Test helper: `tests/test_helpers.h` provides `pml::test::eval()` for quick GTest integration

## Architecture Overview

PML is a C++23 S-expression DSL for code-to-image generation. The pipeline:

```
source → Lexer → Parser → Expander → Evaluator → GraphicObject → RenderBackend → PNG/GIF
```

**Library dependency order** (bottom-up):
```
pml_core → pml_frontend → pml_evaluator → pml_graphics → pml_api
                           → pml_backend ↗               ↗ pml_layer
                                                pml_animation, pml_sprites, pml_asset
                                                pml_module, pml_filter
```

**Key subsystems:**

- **`pml_core`** — Foundation: `Value` (tagged union with small-string + Box for complex types), `Symbol`/`Keyword`, `Expr` (AST), `Arena` (bump allocator, reset per execute()), `PMLException` + `Result<T>` (= `std::expected<T, PMLException>`)
- **`pml_frontend`** — `Lexer` → `Parser` → `Expander` (macro expansion); arena-allocated AST
- **`pml_evaluator`** — Tree-walking interpreter with 21+ special forms, tail-call optimization (trampoline), lexical scoping via `Environment` (dense indexed varref cache)
- **`pml_graphics`** — `GraphicObject` (immutable scene-graph node with shape_type, params, fill/stroke, transform, children), `Canvas` (rendering context), tilemap, rough-style perturbation
- **`pml_backend`** — `RenderBackend` abstract base with `NullBackend` (always compiled), `SkiaBackend` (GPU, pre-built), `GifBackend`; registered via static initializers in `BackendRegistry`
- **`pml_layer`** — Layer composition, blend modes, spritesheet export, Aseprite format
- **`pml_filter`** — Image filters (blur, convolution, edge detect, layer styles)
- **`pml_sprites`** — Style/palette registries, character component system
- **`pml_animation`** — Timeline + easing functions
- **`pml_asset`** — Bitmap I/O and asset cache
- **`pml_api`** — `PMLRuntime` facade: owns `Environment` + `Arena` + `PMLContext` (canvas, timeline, styles, palettes, compositions). Thread-safe via `PMLContext::current()` thread-local pointer.
- **`pml_cli`** — CLI entry with file exec, REPL, watch mode, JSON output
- **`pml_mcp`** — JSON-RPC MCP server (stdio)

## Value System

`Value` is a tagged union: `Tag::{Nil, Int, Double, Bool, Object}`. Scalars inline; complex types via `shared_ptr<Box>` (preserves reference semantics). `Box::Kind` has ~20 variants (String, List, HashMap, Vector, Procedure, Builtin, Macro, Module, GraphicObject, Canvas, Transform, Mesh3D, Animation, Skeleton, Style, Palette, Timeline, Layer, Composition, ImageFilter).

## Language Quirks

- `let` = sequential bindings (use `let-par` for parallel)
- No `while`/`for` — use `dotimes` or `for`:
  - `(dotimes (i n) body...)` — iterate 0 to n-1
  - `(for (i start end [step]) body...)` — numeric for loop, start inclusive, end exclusive
- Explicit type conversion: `(->double x)`, `(->int x)`
- Integer division: `(div a b)` (alias for `quotient`)
- `add` order = z-order (later = above)
- `set-backend!` must precede `canvas`
- `render` output format from file extension (`.png`/`.gif`)
- Module imports resolve relative to the importing file, not CWD
- Macro system: `defmacro`, `macroexpand`, `gensym`, quasiquote/unquote
- Shader introspection: `(shader-uniforms handle)` lists all uniform names/types/sizes
- Shader pre-validation: `(shader-validate handle uniform-data)` checks size compatibility
- Shader composition: `(compose-with-child wrapper-sksl child)` / `(compose-with-children wrapper-sksl (list ...) :uniforms ...)` — GPU-side pipeline assembly
- Shader sampling: `(eval-shader handle x y)` returns `(r g b a)` for debugging

## Error Handling & Memory

- **Errors**: `Result<T> = std::expected<T, PMLException>` with factory functions: `type_error()`, `syntax_error()`, `unbound_error()`, `arity_error()`, `import_error()`. Exceptions only for truly unrecoverable (OOM, assertion)
- **Memory**: `Arena` bump allocator for execute()-lifetime data; objects that outlive (Procedure bodies, macros) use `make_list_heap()`. Arena is thread-local, set per execute() call
- **Code style**: LLVM-based `.clang-format`, 4-space indent, 100 cols, pointer/reference left-aligned, `SortIncludes: false`. Namespace `pml { ... }`, `FixNamespaceComments: true`

## PMLContext Design

`PMLContext` replaces old global singletons. Each `PMLRuntime` owns one, activated per thread via `PMLContextScope` RAII helper. Accessors in graphics/animation/sprites delegate to `PMLContext::current()`.

## Skeleton/IK System

Present in `src/pml/skeleton/` but **not registered into the runtime** (commented out in `api.cpp`). Leave disabled unless explicitly asked to enable it.
