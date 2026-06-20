# pml-cpp Agent Guide

> **Read this first.** This file describes the layout, build, conventions, and
> "gotchas" specific to the C++23 port. The canonical, **production** version
> of PML still lives in `../pml/` (Python 3.10+). This directory is the
> C++ rewrite — it is a **port**, not a replacement.

---

## 1. Project layout

```
pml-cpp/                                   # C++23 rewrite (this directory)
├── AGENTS.md                              # You are here.
├── CMakeLists.txt                         # Top-level CMake (third_party + skia)
├── CMakePresets.json                      # debug / release
├── third_party/                           # Vendored deps (preferred path)
│   ├── freetype/  googletest/  json/  libpng/   # present locally
│   └── giflib/                            # auto-cloned by CMake if missing
├── src/pml/
│   ├── core/        # Expr/Value/Procedure, errors, embedded stdlib bytes
│   ├── frontend/    # Lexer, Parser, Expander
│   ├── evaluator/   # 20 special forms, BuiltinProcedure, Environment
│   ├── graphics/    # GraphicObject, Canvas, AffineTransform, render dispatch
│   ├── graphics3d/  # Vec3, Mat4, Transform3D, Camera3D, Mesh3D, 3D primitives
│   ├── backend/     # RenderBackend ABC + registry; cairo/, gif/, skia/ backends
│   ├── sprites/     # Style, Palette (no components/ yet — see §5)
│   ├── animation/   # Easing + Timeline + Animation
│   ├── skeleton/    # Joint, SkeletonTemplate/Instance, FABRIK + CCD IK
│   ├── module/      # ModuleLoader + load_embedded_stdlib()
│   ├── api/         # PMLRuntime facade (execute / execute_file / render_sprite)
│   ├── cli/         # main.cpp + repl.cpp (file / REPL / --json / --watch / -o)
│   └── mcp/         # pml-mcp JSON-RPC server
├── tests/           # builtins_smoke.cpp (134 cases) + GTest stub
└── build/{debug,release}/                 # Out-of-source build dirs
```

The Python reference lives at `../pml/`. When in doubt, that directory is
the source of truth for semantics — see §3 for the comparison.

---

## 2. Build & run

```powershell
cd w:\Project\PML\pml-cpp

# Configure (Visual Studio 18 2026, x64 Debug)
cmake --preset debug

# Build
cmake --build --preset debug

# Run smoke tests (134 / 134 pass)
.\build\debug\tests\Debug\pml-builtins-smoke.exe

# CLI
.\build\debug\bin\Debug\pml.exe            # REPL
.\build\debug\bin\Debug\pml.exe file.pml   # execute file
.\build\debug\bin\Debug\pml.exe file.pml --json -o .\out
.\build\debug\bin\Debug\pml-mcp.exe        # MCP server (stdio)
```

### Build options

| Option                | Default | Notes |
|-----------------------|---------|-------|
| `PML_BUILD_TESTS`     | ON      | pml-tests GTest + builtins_smoke |
| `PML_BUILD_CLI`       | ON      | `pml.exe` |
| `PML_BUILD_MCP`       | ON      | `pml-mcp.exe` |
| `PML_BUILD_GIF`       | ON      | gif export backend (needs giflib) |
| `PML_BUILD_CAIRO`     | OFF     | cairo render backend (fragile on MSVC) |
| `PML_BUILD_SKIA`      | ON      | Skia GPU backend (needs `PML_SKIA_DIR` / `PML_SKIA_OUT`) |
| `PML_BUILD_FREETYPE`  | OFF     | freetype (not used yet) |
| `PML_SKIA_DIR`        | —       | Path to skia source root (needs `include/`). Default `G:/Project/skia`. |
| `PML_SKIA_OUT`        | —       | Path to pre-built skia output (needs `skia.lib`). Default `G:/Project/skia/out/llvm.x64.debug`. |

> **Skia is consumed as a pre-built static library.** The top-level
> `CMakeLists.txt` imports `skia.lib` + `skcms.lib` + (sksg, skshaper, svg,
> skottie, skresources, jsonreader, expat, bentleyottmann) — no Skia is
> compiled by us. Override the paths if your build lives elsewhere.

### Third-party layout

Deps prefer a local checkout at `third_party/<name>/`. If that's missing,
the top-level `pml_third_party()` helper issues a `FetchContent_Declare` with
`GIT_REPOSITORY` over **HTTPS** (so it works without SSH keys / vcpkg) and
clones into `third_party/<name>/` for next time.

| Dep          | Path in third_party/   | Status              |
|--------------|------------------------|---------------------|
| nlohmann-json| `json/`                | OK (header-only)    |
| googletest   | `googletest/`          | OK                  |
| libpng       | `libpng/`              | OK                  |
| freetype     | `freetype/`            | OK (unused for now; cloned only if `PML_BUILD_FREETYPE=ON`) |
| giflib       | `third_party/giflib/`  | auto-cloned if missing |
| cairo        | `third_party/cairo/`   | auto-cloned if `PML_BUILD_CAIRO=ON` |

---

## 3. Python vs C++ — semantic ground truth

When you implement or fix a feature, **read `../pml/<file>.py` first**. The
C++ port aims for byte-identical / pixel-identical behaviour. Notable
port-time deltas are listed below — add new ones as they appear.

### Implemented identically to Python
- Lexer / Parser / Expander — tokenize, S-expression parse, macro expansion
- Evaluator — 20 special forms: `if/cond/and/or/define/lambda/begin/set!/let/let*/do/quote/quasiquote/define-macro/defmacro/import/provide/macroexpand/assert/gensym`
- Environment — define / lookup / set! / extend (parent chain)
- AffineTransform — identity, translate, rotate, scale, shear, compose, inverse, apply
- GraphicObject — immutable, atomic id counter, `with_*` mutators
- Canvas — `_canvas` / `_sprite_canvas` / `add` / global singleton
- Easing — 12 functions
- Module system — `import` / `provide` with circular-dep detection
- Skeleton — Joint, SkeletonTemplate, SkeletonInstance, FABRIK + CCD
- Palette — predefined + `define-palette` / `palette-ref`
- Style — predefined cel/pixel/flat + `define-style` / `use-style` / `resolve_style`
- PMLRuntime — `execute` / `execute_file` / `render_sprite` / `validate` / `list_components` / `preview_params`
- CLI — file / REPL / --json / --watch (poll fallback on Windows) / -o
- MCP — JSON-RPC over stdio with `Content-Length: N\r\n\r\n` framing
- render / render-set / render-spritesheet — Skia backend (when `PML_BUILD_SKIA=ON`) + GIF backend
- Animation timeline — `animate`, `play`, `stop`, `pause`, `seek`, `animation-state`, `every-frame`, `parallel`, `sequence`
- Transform builtins — `translate`, `rotate`, `scale`, `shear`, `compose`, `matrix-inverse`, `matrix-apply`, `matrix?`
- Canvas / shape builtins — `canvas`, `sprite-canvas`, `add`, `circle`, `rect`, `ellipse`, `line`, `polygon`, `path`, `text`, `image`, `group`
- Sprite components — `character`, `body`, `eyes`, `hair`, `head`, `mouth`, `outfit`, `weapon`, `potion`, `chest`, `generic-item`, `button`, `panel`, `health-bar`, `icon`, `tile`, `decoration`, `background`
- Higher-order list functions — `map`, `filter`, `reduce`
- Hash tables — `make-hash`, `hash-ref`, `hash-set!`, `hash-delete!`, `hash-keys`, `hash-values`, `hash?`
- Vectors — `make-vector`, `vector-ref`, `vector-set!`, `vector-length`, `vector->list`, `list->vector`, `vector?`
- Hygienic macros — `defmacro` with automatic variable renaming to avoid capture
- Module introspection — `module-available?`, `module-list`, `module-exports`
- Skin binding — `bind-skin` with skeleton-driven transform merge into animation frames

### Implemented differently (intentional ports)
- **Value representation**: `std::variant` instead of Python's dynamic types.
  See `src/pml/core/types.h` for the full list of alternatives.
- **Errors**: `std::expected<T, PMLException>` instead of exceptions. Use
  `pml::type_error("...")` / `pml::syntax_error("...")` factories from
  `src/pml/core/error.h`.
- **Macro-expansion depth**: file-local `inline int g_macro_depth` in
  `evaluator.cpp` (not in header).
- **Builtins accept kwargs**: a single `BuiltinProcedure` with
  `accepts_kwargs=true` is dispatched as positional-then-keyword pairs. The
  same helper (`parse_kwargs` / `kw_string` / `kw_int`) lives in
  `src/pml/graphics/render.cpp` — **re-use it from new files**.
- **Transform namespace**: `AffineTransform` lives in `namespace pml` (defined
  in `src/pml/graphics/transform.h`), matching the forward declaration in
  `types.h`. Prefer plain `AffineTransform` inside `namespace pml`; use
  `pml::AffineTransform` only at namespace scope.
- **3D graphics**: C++ port adds native 3D primitives (`cube3d`, `cuboid3d`,
  `rounded-cuboid3d`, `cone3d`, `plane3d`, `sphere3d`), per-face PML 2D material
  mapping, 3D transforms (`rotate-x`/`y`/`z`, `translate3d`, `scale3d`), and a
  perspective/orthographic camera (`camera`). Python reference has no 3D support.

### NOT yet ported — known gaps
- Windows watch mode — CLI `--watch` now uses `FindFirstChangeNotification` on
  Windows, but does not yet handle recursive directory watching or file renames.
- **CJK text rendering** — Skia backend `draw_text` uses `drawSimpleText` with a
  single resolved typeface (system default → Arial → Microsoft YaHei). It does
  **not** perform font fallback, so CJK characters (Chinese, Japanese, Korean)
  render as blank or tofu. Fixing this requires per-character font fallback via
  `SkTextBlobBuilder` or `SkShaper`. The `text` builtin only reliably renders
  ASCII / Latin characters.

---

## 4. Code conventions

- **C++23**: `std::expected`, `std::optional`, `std::variant`, fold expressions,
  `std::numbers::pi_v<double>`, designated initialisers, deducing `this`.
- **Headers**: `#pragma once` + minimal `#include`s. Forward-declare in
  headers; include in .cpp.
- **Namespacing**: everything lives in `namespace pml { ... }`. The single
  exception is `AffineTransform` (global) — see §3.
- **Errors**: factory functions in `core/error.h` return
  `PMLException`. Return them via `std::unexpected(...)` from
  `Result<T>`-returning functions. **Don't use `throw` for control flow.**
- **Builtins**: register via `env->define(name, Value(std::make_shared<
  BuiltinProcedure>(name, fn, accepts_kwargs)))`. See
  `src/pml/evaluator/builtins.cpp` for the `def` lambda.
- **Value → string**: use `value_to_string(v)` from `core/types.h` for
  JSON-friendly output. `value_to_opt_string(v)` returns the string when
  the variant alternative is `std::string`, else `std::nullopt`.
- **kwargs**: re-use `parse_kwargs` / `kw_string` / `kw_int` from
  `graphics/render.cpp`. Do **not** duplicate the parser in new builtins.
- **Singletons**: `_current_canvas`, `g_timeline`, `StyleRegistry::instance()`,
  `PaletteManager::instance()`. Each has a `reset()` helper for tests.
- **No exceptions for control flow** in builtin implementations — return
  `Result<T>`.
- **Comments**: explain the "why", not the "what". Use the box-drawing
  separator style already in headers (`// ═══...`).

---

## 5. Adding a new feature

1. **Read `../pml/<matching_file>.py` first.** This is the spec.
2. Search `src/pml/` for the closest existing analogue (e.g. adding
   `register_transform_builtins`? copy the layout from
   `sprites/style.cpp` + `style.h`).
3. Add registration in `PMLRuntime::init_global_env()` in `src/pml/api/api.cpp`
   if it's a top-level builtin.
4. Add the new file to its `CMakeLists.txt` (e.g. `src/pml/evaluator/CMakeLists.txt`).
5. Add a smoke test case in `tests/builtins_smoke.cpp`.
6. Build: `cmake --build --preset debug`.
7. Verify: `.\build\debug\bin\Debug\pml-builtins-smoke.exe`.
8. If semantics differ from Python, add a row to §3 of this file.

---

## 6. Known build / environment issues

### `third_party/giflib/` is missing
The top-level `CMakeLists.txt` issues a `FetchContent_Declare` (HTTPS) for
giflib if `third_party/giflib/CMakeLists.txt` is absent. First configure
will clone it; subsequent builds reuse the local copy. Disable gif export
with `PML_BUILD_GIF=OFF` to skip it entirely (the smoke test still passes).

### Skia
Skia is **not** cloned by CMake — it's consumed pre-built. Set
`PML_SKIA_DIR` (headers at `<dir>/include`) and `PML_SKIA_OUT` (must
contain `skia.lib` + `skcms.lib`) to the paths you compiled into. The
default in `CMakePresets.json` points at `G:/Project/skia` and
`G:/Project/skia/out/llvm.x64.{debug,release}`.

### Toolchain
- Compiles with **Visual Studio 18 2026** (not 17). Presets are set to
  `"Visual Studio 18 2026"` — change in `CMakePresets.json` if needed.
- C++23 (`/std:c++latest` equivalent). `std::expected`, `<numbers>` etc.
  required.
- Single-config Debug + multi-config both work via presets.

### Things to NOT do
- **Do not** add a top-level `vcpkg.json` — we use CMake `FetchContent`
  (HTTPS) + local `third_party/`. No vcpkg dependency anywhere.
- **Do not** add vcpkg-style `find_package(VCPKG)` tooling or
  `CMAKE_TOOLCHAIN_FILE` pointing at vcpkg.
- **Do not** silently change Python behaviour — pixel-identical is the goal
  (fonts may differ; document any unavoidable differences in this file).
- **Do not** use `throw` for builtins — return `Result<T>` instead.
- **Do not** call `register_*` functions in CLI/MCP before the matching
  registration is implemented in C++ (PMLRuntime will link-fail).

---

## 7. Verification commands (cheat sheet)

```powershell
# Configure
cmake --preset debug

# Build everything
cmake --build --preset debug

# Build a single target
cmake --build --preset debug --target pml_graphics
cmake --build --preset debug --target pml-builtins-smoke

# Rebuild after CMakeLists change
cmake --build --preset debug --target rebuild_cache

# Run smoke tests
.\build\debug\tests\Debug\pml-builtins-smoke.exe

# Render test (needs PML_BUILD_RENDERING=ON and giflib)
.\build\debug\bin\Debug\pml.exe ..\examples\hello.pml -o .\out
```

---

## 8. Where to look for things

| You want to…                            | Look in… |
|-----------------------------------------|----------|
| Add a special form                      | `src/pml/evaluator/evaluator.cpp` (function-local `SPECIAL_FORMS` map) |
| Add a primitive (`circle`, `rect`, …)   | `src/pml/graphics/builtins_graphics.cpp` (create); add `register_graphics(env)` to PMLRuntime |
| Add a transform builtin                 | `src/pml/graphics/transform_builtins.cpp` (create); expose `register_transform_builtins` |
| Add a canvas / sprite-canvas builtin    | `src/pml/evaluator/canvas_builtins.cpp` (create); expose `register_canvas_builtins` |
| Add a 3D primitive / transform / camera | `src/pml/graphics3d/builtins_3d.cpp` + `primitive_factory.cpp`; expose `register_3d_builtins` |
| Add an animation builtin                | `src/pml/animation/timeline.cpp` (`register_timeline_builtins` already declared) |
| Add a new sprite component              | `src/pml/sprites/components/<name>.cpp/h` (port from `../pml/sprites/components/<name>.py`) |
| Add a render backend                    | New subdir under `src/pml/backend/`; subclass `RenderBackend`; self-register with `BackendRegistry` |
| Add a new error code                    | `src/pml/core/error.h` (`ErrorCode` enum) + factory function |
| Change the CLI                          | `src/pml/cli/main.cpp`; CLI flags in `CLIOptions` struct |
| Add an MCP tool                         | `src/pml/mcp/mcp_server.cpp` |

---

## 9. Quick comparison: PMLRuntime initialisation

```python
# Python (pml/repl.py:create_global_env)
env = Environment()
register_all(env)                            # arithmetic, list, string, IO, predicates
register_transforms(env)                     # translate, rotate, scale, compose, …
register_graphics(env)                       # primitives + canvas + render
register_sprites(env)                        # style, palette, components
register_animation(env)                      # animate, play, stop, parallel, sequence, …
register_skeleton(env)                       # defskeleton, IK, bind-skin
# + map, filter, reduce, gensym
```

```cpp
// C++ (src/pml/api/api.cpp:PMLRuntime::init_global_env)
register_builtins(m_env);             // arithmetic, list, string, IO, predicates, map/filter/reduce, gensym ✓
register_render(m_env);               // render, render-set, render-spritesheet ✓
register_skeleton(m_env);             // defskeleton, instantiate-skeleton, joint-position ✓
register_ik(m_env);                   // ik-solve ✓
register_style(m_env);                // define-style, use-style ✓
register_palette(m_env);              // define-palette, palette-ref ✓
register_components(m_env);           // sprite semantic components ✓
register_timeline_builtins(m_env);    // animate, play, stop, pause, seek, animation-state, every-frame, parallel, sequence ✓
register_skin_binding(m_env);         // bind-skin ✓
register_backend_builtins(m_env);     // list-backends, set-backend!, backend-capabilities ✓
register_shader_builtins(m_env);      // shader, apply-shader! (SkSL via SkRuntimeEffect) ✓
register_transform_builtins(m_env);   // translate, rotate, scale, shear, compose, matrix-inverse, matrix-apply, matrix? ✓
register_canvas_builtins(m_env);      // canvas, sprite-canvas, add, circle, rect, ..., group, color builtins ✓
register_3d_builtins(m_env);          // cube3d, cuboid3d, rounded-cuboid3d, cone3d, plane3d, sphere3d, rotate-x/y/z, translate3d, scale3d, camera ✓
// (module loading via special forms; no register_module_builtins)
```

Anything marked ⚠ is registered but not fully functional.

---

## 10. Don't trust the plan blindly

The plan at `.omo/plans/pml-cpp-refactor.md` is a roadmap, not a contract.
Several tasks it marks as `[x]` are *partially* done (declared, not wired);
a few are intentionally skipped. Always verify by reading the actual
source files in `src/pml/<module>/` before assuming a task is complete.

The authoritative truth about what works is:
1. `pml-builtins-smoke.exe` exit code 0
2. A `.pml` file that runs the Python version successfully also runs in
   the C++ version with the same observable behaviour
3. `BEHAVIOR_DIFFERENCES.md` (not yet written — see §3 todo list)
