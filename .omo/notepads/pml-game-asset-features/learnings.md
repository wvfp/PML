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
