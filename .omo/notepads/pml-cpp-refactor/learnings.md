# Learnings - PML C++ Refactor

Initialized: 2026-06-16

## Session Context
- C++23 rewrite of PML (Python Lisp-style DSL for code-to-image)
- Skia GPU/shader backend, BackendRegistry for pluggability
- CMake + vcpkg build system
- Shader via SkRuntimeEffect (direct GLSL embedding)
- GIF via giflib, GPU optional with CPU fallback

## T6: Stdlib embedding
- Created `tools/embed_stdlib.py` — reads all 13 `.pml` files from `stdlib/` recursively, generates `src/pml/core/embedded_stdlib.h`
- Header has: `constexpr const char stdlib_xxx[N]` byte arrays, `StdlibEntry` struct, `stdlib_entries[]` table, `find_stdlib(const char*)` lookup
- Deterministic (sorted by path), binary-safe (xxd-style hex escapes), C++23 compatible
- Output: `pml-cpp/src/pml/core/embedded_stdlib.h` (10655 bytes across 13 files)
- Script: `python tools/embed_stdlib.py`

## T14 — Cairo Render Backend (2026-06-16)

### Files Created
- `pml-cpp/src/pml/backend/cairo/cairo_backend.h` — CairoBackend(CairoSurface) + headers
- `pml-cpp/src/pml/backend/cairo/cairo_backend.cpp` — Full implementation + SVG path parser
- `pml-cpp/src/pml/backend/cairo/CMakeLists.txt` — pml_backend_cairo target

### Files Modified
- `pml-cpp/src/pml/backend/CMakeLists.txt` — added add_subdirectory(cairo)
- `pml-cpp/src/pml/graphics/transform.h` — added to_cairo_matrix() declaration + <cairo.h>
- `pml-cpp/src/pml/graphics/transform.cpp` — added to_cairo_matrix() implementation

### Design Decisions
- `CairoSurface` extends `Surface` base struct, wraps cairo_surface_t* + cairo_t*
- Shape dispatch via string comparison on `shape_type`, matching Python PML names
- Transform applied via `cairo_transform()` before path building
- Fill/stroke pattern: build path → fill_preserve → stroke (when both)
- Ellipse: unit circle scaled/translated via cairo_scale/cairo_translate
- SVG path: simplified parser (M/L/H/V/Z only, absolute coords)
- Registration: static init via BackendRegistry::register_backend("cairo", ...)
- Parse_color (from color_helpers.h) returns ARGB uint32 → converted to Cairo RGBA doubles
- `to_cairo_matrix()` via `cairo_matrix_init(&m, a, b, c, d, e, f)` — direct mapping

### Key Config
- `pml_backend_cairo` links `pml_backend pml_graphics ${CAIRO_LIBRARIES} ${PNG_LIBRARIES}`
- Cairo is always compiled (required dep, unlike optional Skia)
- BackendCap: RasterCPU | FontRendering | AlphaComposite | LoadPNG

## T5 — AffineTransform (2026-06-16)

### Files Created
- `pml-cpp/src/pml/graphics/transform.h` — header with full struct
- `pml-cpp/src/pml/graphics/transform.cpp` — Skia integration only

### Design Decisions
- Struct with 6 public `double` fields (a,b,c,d,e,f), matching Python's frozen dataclass
- All methods `constexpr` where possible (C++23: std::cos/sin constexpr via P0533R9)
- `[[nodiscard]]` on all accessors/operations
- `rotate()` takes degrees (not radians like Python); converts internally
- `inverse()` returns `std::optional<AffineTransform>` instead of raising; nullopt on singular
- `to_skmatrix()` only definition in .cpp via forward-declared `class SkMatrix` in header
- Operator overloads `operator*` for compose and apply convenience
- Skia dependency isolated to .cpp only — header is clean

## T7 — GraphicObject + Canvas (2026-06-16)

### Files Created
- `pml-cpp/src/pml/graphics/objects.h` — `GraphicObject` struct definition
- `pml-cpp/src/pml/graphics/objects.cpp` — constructor, immutable mutators, streaming
- `pml-cpp/src/pml/graphics/canvas.h` — `Canvas` struct, global current canvas, factory decls
- `pml-cpp/src/pml/graphics/canvas.cpp` — add(), _canvas(), _sprite_canvas(), get_current_canvas()

### Design Decisions
- Namespace: `GraphicObject` and `Canvas` defined in `pml::` (matching forward decls in `types.h`)
- `::AffineTransform` qualified with global scope prefix in `objects.h` to avoid ambiguity with the forward declaration `pml::AffineTransform` in `types.h` (transform.h defines it in global namespace)
- `fill`/`stroke` use `std::optional<std::string>` to match Python's `None` semantics
- ID auto-generation via file-local `static std::atomic<uint64_t>` in objects.cpp (not in header)
- `children` as `std::vector<GraphicObject>` — value semantics for full copyability
- `GraphicObject` itself is copyable (compiler-generated copy ctor/assignment) — all members are value types
- `Canvas::add()` takes `GraphicObject` by value (moved into vector) — no raw pointers
- Factory functions `_canvas()` and `_sprite_canvas()` return `shared_ptr<Canvas>` matching Python pattern
- `g_current_canvas` is `extern shared_ptr<Canvas>` — mirrors Python's `_current_canvas[0]`
- `params` and `metadata` typed as `std::unordered_map<std::string, Value>` using core runtime Value type
- CMake: added `objects.cpp canvas.cpp` to `pml_graphics` library sources

### Quirks
- `transform.h` defines `AffineTransform` in global namespace (not `pml::`), while `types.h` forward-declares it inside `pml::`. This causes two separate types. Workaround: use `::AffineTransform` in `objects.h` member declarations, or wrap transform.h in `pml::` namespace.

## T11 — Backend ABC + Registry + NullBackend (2026-06-16)

### Files Created
- `src/pml/backend/capabilities.h` — `BackendCap` enum (bitmask flags: RasterCPU, GPUAccel, Shaders, VectorOutput, AnimationGIF, FontRendering, LoadPNG), `|`/`&`/`|=`/`&=`/`~` operators, `has_capability()` helper, `BackendInfo` struct
- `src/pml/backend/backend.h` — `RenderBackend` ABC, `Surface` base struct (width, height, virtual dtor), pure virtual methods matching spec: `info()`, `create_surface()`, `draw()`, `save_image()`, `save_animation()`, `compile_shader()`
- `src/pml/backend/registry.h` — `BackendRegistry` singleton with `add()`, `create()`, `create_best()`, `available()`, `set_active()`, `active()`, static `register_backend()` convenience
- `src/pml/backend/registry.cpp` — full implementation, no NullBackend embedded (separate file now)
- `src/pml/backend/null_backend.cpp` — NullBackend with call counters (draw_count, create_count, save_count, shader_count, anim_count), call history vector, static registration

### Design Decisions
- `BackendCap` is `enum class BackendCap : uint8_t` with explicit bitmask operators — avoids `enum` namespace pollution while allowing `|`/`&`/`~` syntax
- `Surface` is a base struct (not purely forward-declared) with virtual destructor — needed so `unique_ptr<Surface>` can be returned from factory and properly destroyed by callers
- `Surface` is non-copyable, movable — backends own their own pixel data
- All backend methods use `Result<T>` (std::expected) for error propagation — no exceptions
- `create_surface()` takes `uint32_t bg_color` (0xRRGGBBAA) — matches color_helpers.h ARGB convention but numeric, not string
- `draw()` takes `Surface&` and `const GraphicObject&` — reference semantics, forward-declared GraphicObject
- `save_animation()` takes `const std::vector<Surface*>& frames` — non-owning pointers
- `compile_shader()` returns `Result<uint64_t>` — opaque backend-specific handle
- BackendRegistry stores factories (`std::function`), not instances — backends created on demand by `create()`, `create_best()`, or `set_active()`
- `set_active()` returns `bool` (false if name unknown)
- `active()` lazily creates best backend if none set
- NullBackend in separate file with static registration via `[[maybe_unused]] static bool` — always compiled
- include path: `${CMAKE_CURRENT_SOURCE_DIR}/../..` (resolves to `pml-cpp/src/`) so `#include "pml/core/error.h"` works
- CMake: `pml_backend` links `pml_core` + `pml_graphics` PUBLIC; no Cairo/Skia dependency
- Root `CMakeLists.txt`: added `add_subdirectory(src/pml/backend)` unconditionally (before the optional skia subdirectory)

### Lessons
- Existing code had `RenderSurface` (abstract class) vs new `Surface` (struct) — rewrote all backend files to match new unified design
- Old `registry.cpp` embedded NullBackend and used `shared_ptr<RenderSurface>` + string bg_color + void returns — replaced with new interface
- Capabilities enum and operators should be in a separate header (`capabilities.h`) so they can be included independently of the ABC
- `std::unique_ptr<Surface>` needs complete type at destruction — defining Surface with virtual dtor in header solves this cleanly

## T? — Color Helpers (2026-06-16)

### Files Created
- `src/pml/backend/color_helpers.h` — header with `parse_color()`, `apply_color()`
- `src/pml/backend/color_helpers.cpp` — implementation

### Design Decisions
- `parse_color(const std::string&) → std::optional<uint32_t>` — ARGB format (A in high 8 bits)
- Named colors: 18 entries (transparent, none, black, white, gray, grey, red, green, blue, yellow, cyan, magenta, orange, purple, pink, brown, navy, coral)
- Hex: `#RGB` (digit duplication), `#RRGGBB` (alpha=F), `#RRGGBBAA`
- `apply_color(void*, uint32_t, offsets)` writes channel bytes at given offsets — generic, no struct dependency
- `apply_color(void*, string, offsets)` → parse + apply, returns bool success
- No Skia/Cairo dependency in header — pure C++23
- Added to `pml_graphics` CMake target (`src/pml/graphics/CMakeLists.txt`)
- Include path `../backend` added to `pml_graphics` for `#include "color_helpers.h"`

## T13 — Render Dispatch (2026-06-16)

### Files Created / Modified
- `src/pml/graphics/render.h` — public API: `parse_svg_path()`, `render()`, `render_set()`, `render_spritesheet()`, `register_render()`, `PathCommand` struct
- `src/pml/graphics/render.cpp` — full implementation (~660 lines)
- `src/pml/backend/backend.h` — added `composite(Surface&, Surface&, int, int)` pure virtual to ABC
- `src/pml/backend/null_backend.cpp` — added `composite()` override + `composite_count_`

### Design Decisions
- `render()`: resolves current canvas (or fallback), detects format from extension, calls `BackendRegistry::instance().active()`, creates surface via `backend.create_surface()` with parsed ARGB bg color, auto-centers sprite canvas content at (w/2, h*0.45), draws each object, saves, writes sidecar .meta.json for sprite canvases
- `render_set()`: renders content at each scale factor (e.g. 1x, 2x, 4x), filenames `name@Nx.ext`
- `render_spritesheet()`: grid layout via `backend.composite()`, sidecar .spritesheet.json with frame metadata
- `register_render()`: binds 3 BuiltinProcedure instances (render, render-set, render-spritesheet) all with `accepts_kwargs=true` — kwargs parsed as alternating `Keyword→Value` pairs after positional args
- SVG path parsing: pure function `parse_svg_path(string) → Result<vector<PathCommand>>` — no Cairo dependency, manual `strtod`-based number tokenizer, supports all standard commands (M/L/H/V/C/S/Q/T/A/Z) with implicit repeats

### CMake Structure
- Added `render.cpp` to `pml_graphics` sources
- `pml_graphics` links `pml_backend` (for BackendRegistry + RenderBackend impl)
- `pml_backend` links `pml_core` only (no circular dep) — GraphicObject is forward-declared
- Added `src/pml/backend/CMakeLists.txt` → registered `color_helpers.cpp`
- Added `add_subdirectory(src/pml/backend)` to root CMakeLists.txt
- Include paths: `../backend` (registry.h, backend.h), `../..` (for `pml/core/error.h` from backend.h), `../evaluator` (environment.h)

### Quirks
- `BackendRegistry::active()` is a non-static member — call via `BackendRegistry::instance().active()`
- `create_surface()` returns `unique_ptr<Surface>` — draw/save take `Surface&` (dereference with `*surface`)
- `color_helpers.cpp` was not in any CMakeLists — added to pml_backend
- `pml_backend` originally linked `pml_graphics` creating circular dep — removed: only forward declarations needed
- Had to restore `backend.h` (was emptied by failed `git show` redirect) — reconstructed from build error output

## T16 — Evaluator (Special Forms, Function Application) — 2026-06-16

### Files Created
- `src/pml/evaluator/evaluator.h` — header with `evaluate()`, `evaluate_arguments()`, `apply_function()`, `is_truthy()`, all 20 special form declarations, `expand_macro()`, `expand_quasiquote()`
- `src/pml/evaluator/evaluator.cpp` — full implementation (1475 lines, matching Python evaluator.py)

### Files Modified
- `src/pml/evaluator/CMakeLists.txt` — added `evaluator.cpp` to `pml_evaluator` sources
- `src/pml/core/types.cpp` — removed `Procedure::call()` stub (implementation moved to evaluator.cpp)
- Removed `src/pml/evaluator/stub.cpp` (now unused, replaced by evaluator.cpp)

### Design Decisions
- `evaluate()` takes `shared_ptr<Environment>` (not `Environment&`) — avoids circular dependency when `Procedure::call()` in evaluator.cpp needs to call `evaluate()`. A convenience overload accepting `Environment&` calls `shared_from_this()` internally.
- `Procedure::call()` defined in evaluator.cpp (not types.cpp) — breaks circular dependency between `pml_core` and `pml_evaluator`. The declaration remains in `types.h`, definition is in `evaluator.cpp` which has access to `evaluate()`.
- `Procedure::body` is a single `Expr` (not `vector<Expr>` like Python) — multiple body expressions are wrapped in `(begin ...)` form by `make_body_from_vector()`. This preserves semantic equivalence without changing types.h.
- Rest parameters use the "." convention: `(lambda (x . rest) ...)` → params = [x, ".", rest]. `apply_function` detects "." in params and handles binding automatically.
- `expand_quasiquote` returns `Expr` (not `Value`) — embeds evaluated results via `(quote ...)` wrapping to keep the result as an AST node.
- Special forms dispatch via function-local static `unordered_map<string, SpecialForm>` — avoids static initialization order issues.
- Macro expansion depth tracking uses `inline int g_macro_depth` — module-level global matching Python's `_macro_depth`.
- `value_to_expr()` helper converts `Value` → `Expr` — needed for quasiquote to embed evaluated values back into the expression tree.

### Implementation Notes
- 20 top-level special forms in dispatch table + unquote/unquote-splicing handled inside `expand_quasiquote`
- `eval_import` stores `nil` instead of a `Module` object — Module is only forward-declared (not defined yet in this task). TODO for T19 (module_loader).
- `eval_macroexpand` currently just returns the expression quoted — full expansion through Expander requires Expander to be wired up.
- `make_body_from_vector()` wraps multiple body expressions in `(begin ...)` so a single `Procedure::body` Expr can represent the full body.
- `expr_to_value()` converts AST Expr → runtime Value for self-evaluating expressions (quote returns values, not ASTs).
- `is_truthy()`: Only `#f`, `0`, `nil`, and empty list are falsy — everything else is truthy.
- GCC 13 `if constexpr` in `std::visit` generic lambdas can fail to fully discard branches — restructured `expand_quasiquote` to use explicit `is_list()` check instead.

## T24 — FABRIK IK Solver (2026-06-16)

### Files Created
- `src/pml/skeleton/ik_fabrik.h` — `solve_fabrik()` declaration + `detail::normalize()` and `detail::positions_to_angles()` helpers
- `src/pml/skeleton/ik_fabrik.cpp` — Full FABRIK implementation (backward pass, forward pass, convergence check, edge cases)

### Files Modified
- `src/pml/skeleton/CMakeLists.txt` — added `ik_fabrik.cpp` to `pml_skeleton` sources

### Design Decisions
- Free function `solve_fabrik()` (not a SkeletonInstance method) matching Python reference
- Uses `SkeletonInstance::chain_positions(end_idx)` and `bone_lengths(end_idx)` via int index (not string)
- End effector lookup via `skeleton->tmpl->joint_index()` — returns -1 if not found → return false
- `_normalize` → `detail::normalize()`: returns {0,0} for vectors < 1e-10
- `_positions_to_angles` → `detail::positions_to_angles()`: iterates from root, computes `atan2` bone angle → relative joint angle → normalize to [-pi, pi] → clamp via `set_angle()` → cumulative update using clamped value from `angles[i]`
- Single-joint chain (n==1) treated as always converged after max iterations (only 1 DOF)
- Unreachable target: stretch chain toward target direction, return false
- `std::hypot()` for distance (matches Python `math.hypot`)
- `std::numbers::pi_v<double>` from `<numbers>` for pi constant (C++20+, available in C++23)
- Same GCC parameter-passing notes as existing ik_ccd.cpp (benign, same pattern)

## T33 — Stdlib Runtime Loader (2026-06-16)

### Files Created
- `src/pml/module/embedded_stdlib.h` — declares `load_embedded_stdlib(shared_ptr<Environment>)`
- `src/pml/module/embedded_stdlib.cpp` — iterates `stdlib_entries[]`, lexes/parses/expands/evaluates each into global env

### Files Modified
- `src/pml/module/CMakeLists.txt` — added `embedded_stdlib.cpp` to `pml_module` sources

### Design Decisions
- Each stdlib module is lexed, parsed, macro-expanded, and evaluated directly into the provided global environment (no separate module scope — the `.pml` files use `(define ...)` and `(provide ...)` which work at global scope)
- On per-module failure, prints a warning to stderr with the module name and error details, then continues to the next module (graceful degradation)
- Module name extracted via `fs::path(entry.path).stem()` — e.g. `"math.pml"` → `"math"`, `"sprites/body.pml"` → `"body"`
- Uses `#include "../core/embedded_stdlib.h"` relative path in `.cpp` to access the auto-generated data (avoids name collision with the module's own `embedded_stdlib.h` header)
- The core's `embedded_stdlib.h` (auto-generated by `embed_stdlib.py`) is NOT modified
- No new dependencies added to `pml_module` — reuses existing `pml_core`/`pml_frontend`/`pml_evaluator` links
- Full build and all 118 builtins smoke tests pass

## T30 — PMLRuntime API Facade (2026-06-16)

### Files Created
- `src/pml/api/api.h` — `PMLRuntime` class, `RenderResult` struct, `serialize_value()`, `error_to_dict()`, `generate_hint()`
- `src/pml/api/api.cpp` — Full implementation
- `src/pml/api/CMakeLists.txt` — `pml_api` CMake target

### Files Modified
- `CMakeLists.txt` — added `add_subdirectory(src/pml/api)` after module and before CLI

### Design Decisions
- `PMLRuntime::init_global_env()` calls 6 registration functions that exist: `register_builtins`, `register_render`, `register_skeleton`, `register_ik`, `register_style`, `register_palette`
- Skipped (with comments): `register_timeline_builtins` (timeline.cpp exists but not compiled into pml_animation yet), `register_transform_builtins` and `register_canvas_builtins` (not yet implemented in C++ port)
- Execute pipeline: `Lexer::tokenize()` → `Parser::parse()` → `Expander::expand_all()` → `evaluate()` per expression, checking each Result<T> for errors along the way
- `serialize_value()` uses `value_to_string()` as fallback for complex types (Canvas, AffineTransform, etc.) — matches Python's `repr(val)` pattern
- Module case in `serialize_value()` returns generic `"<module>"` string since Module is only forward-declared in types.h (accessing `->name` would require full definition)
- `execute_file()` reads via `std::ifstream` and delegates to `execute()`
- `execute_pml()` returns JSON object matching Python's `RenderResult.to_dict()` structure
- `error_to_dict()` matches Python's `_error_to_dict()`: type (ErrorCode string), message, optional line/column/filename, hint
- `generate_hint()` covers all 10 PML error types with LLM-friendly repair hints
- CMake: `pml_api` links `pml_core` PUBLIC (for types exposed in header), all other subsystems PRIVATE
- Include dir: `${CMAKE_CURRENT_SOURCE_DIR}/..` resolves to `src/pml/` for `pml/`-prefixed includes

### Key Quirks
- `transform.h` defines `AffineTransform` in global namespace (not `pml::`) — causes benign GCC notes about parameter passing when included transitively
- `register_timeline_builtins` is declared in `animation/timeline.h` and implemented in `animation/timeline.cpp`, but `timeline.cpp` is not added to `animation/CMakeLists.txt` — linking will fail if called
- No `register_module_builtins` exists — module loading is handled via special forms in the evaluator (eval_import/eval_provide)
