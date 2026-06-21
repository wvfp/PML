## Learnings: PML Game Asset Features

### Codebase Patterns
- Singleton pattern: StyleRegistry::instance(), PaletteManager::instance() �� use instance() + reset() for test isolation
- Builtin registration: def(name, lambda) in graphics/render.cpp
- Kwargs parsing: parse_kwargs / kw_string / kw_int from graphics/render.cpp �� reuse, don't duplicate
- RenderBackend virtuals with default error: create_noise_shader(), create_shader_with_uniforms()
- Smoke tests: CHECK / CHECK_ERROR macros in builtins_smoke.cpp
- GraphicObject tree: recursive structure, immutable with_* mutators
- CMake: each subdir gets own CMakeLists.txt
- lookup_shader(): makeShader(SkData::MakeEmpty(), {}, nullptr) �� children array is EMPTY, fix needed
- BackendCap uint8_t has all 8 bits used �� expand to uint16_t
- BackendCap expanded to uint16_t per decision 2026-06-21
- create_shader_with_children() added to RenderBackend ABC + all 3 backends (null, skia)
- Skia SkRuntimeEffect::Options does NOT have fAllowUniform field in this version (m100+ era), uniforms are always allowed
- compile_shader uses SkRuntimeEffect::MakeForShader(SkString) with default Options{}
- create_shader_with_children uses SkRuntimeEffect::ChildPtr with makeShader(SkSpan<ChildPtr>) overload
- makeShader has two overloads: (SkData*, sk_sp<SkShader>[], size_t, SkMatrix*) and (SkData*, SkSpan<ChildPtr>, SkMatrix*)

### Wave 1 — Tileset + Tilemap (data layer)

**Files created:**
- `src/pml/graphics/tileset.h` — TileType, Tileset, TilesetManager
- `src/pml/graphics/tileset.cpp` — TilesetManager singleton

**Pre-existing files (found in repo):**
- `src/pml/graphics/tilemap.h` — TilemapLayer, Tilemap, TilemapManager, Projection enum, helpers
- `src/pml/graphics/tilemap.cpp` — TilemapManager singleton

**PMLContext integration:**
- `context.h`: forward-declares `TilemapManager` and `TilesetManager`, owns `std::unique_ptr<TilemapManager> tilemaps` and `std::unique_ptr<TilesetManager> tilesets`
- `context.cpp`: includes both headers, resets both unique_ptrs in `PMLContext::reset()`

**Singleton pattern:**
- Both TilesetManager and TilemapManager follow PaletteManager pattern: `instance()` delegates to `PMLContext::current()`, lazy-creates via `std::make_unique`

**Tileset API:**
- `register_tileset(name, tiles, tile_size)` — stores vector of TileType
- `lookup_tileset(name)` — returns const Tileset* or nullptr
- `get_tile_graphic(tileset_name, tile_id)` — returns const GraphicObject* or nullptr
- `reset()` — clears all registered tilesets for test isolation

**CMake:**
- Both `tilemap.cpp` and `tileset.cpp` in `src/pml/graphics/CMakeLists.txt`
- Smoke tests: 262/262 pass (tilemap/tileset builtins correctly show UnboundVariableError)

### Wave 2 — Tilemap builtins (define-tileset, make-tilemap, tilemap-set!)

**Files created:**
- `src/pml/evaluator/tilemap_builtins.h` — declares `register_tilemap_builtins(env)`
- `src/pml/evaluator/tilemap_builtins.cpp` — implements 3 builtins

**Key implementation details:**
- Uses `def()` lambda pattern matching `shader_builtins.cpp` style (not `render.cpp`'s separate variable pattern)
- `value_to_expr()` helper converts a runtime Value back to an AST Expr for evaluating quoted graphic expressions via `eval_to_value()`
- `define-tileset`: positional name + kwargs `:tile-size` (default 32) and `:tiles` (list of `(id name [graphic [detail]])` entries)
- `make-tilemap`: positional tileset-name, cols, rows + kwargs `:projection` ('orthogonal|'isometric, default 'orthogonal) and `:layers` (default 1)
- `tilemap-set!`: 5 positional args (tilemap-name, layer, col, row, tile-id), returns #t
- Kwargs parsing uses `pml::kwargs::parse_kwargs` / `kw_int` / `value_to_opt_string`
- Error handling: `arity_error`, `type_error` (no `throw` — `Result<T>` + `std::unexpected`)
- **Gotcha**: `value_to_opt_string`, `parse_kwargs`, `kw_int` are in `pml::kwargs::` namespace — need `using pml::kwargs::value_to_opt_string;` etc.
- **Gotcha**: `eval_to_value` needs `env.shared_from_this()` — requires `evaluator.h` include

**Files modified:**
- `src/pml/evaluator/CMakeLists.txt` — added `tilemap_builtins.cpp`
- `src/pml/api/api.cpp` — added `#include "pml/evaluator/tilemap_builtins.h"` and `register_tilemap_builtins(m_env)` call
- `tests/builtins_smoke.cpp` — added include + registration + converted 3 CHECK_ERRORs to CHECKs (render-tilemap stays CHECK_ERROR for T7/T8)

**Test results:**
- 262/262 pass (3 tilemap tests converted from CHECK_ERROR to CHECK)
