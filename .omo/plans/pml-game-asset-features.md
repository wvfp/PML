# PML 游戏素材增强 — Tilemap + 预烘焙 + SkSL 多纹理

## TL;DR

> **Quick Summary**: 为 PML 扩展三条管线，使游戏美术师能够用 PML 代码定义和渲染多层 tilemap、导出多通道精灵素材（albedo/normal/specular），并通过 SkSL 多纹理着色器实现动态光照——全部通过 PML 内置函数控制，0 行运行时 C++ 代码。
>
> **Deliverables**:
> - `define-tileset` / `make-tilemap` / `tilemap-set!` / `render-tilemap` PML builtins + C++ 后端
> - 正交 tilemap（drawAtlas 批渲染）+ 等距 tilemap（drawVertices 深序渲染）
> - `render-channels` builtin：albedo + normal（替换着色法）+ specular（灰阶）
> - `bind-textures` builtin + SkSL `uniform shader` child 纹理支持
>
> **Estimated Effort**: Large (5+ waves, ~14 implementation tasks + 4 review tasks)
> **Parallel Execution**: YES — 4 waves + 1 FINAL wave of 4 parallel reviews
> **Critical Path**: T1 (RenderBackend) → T6 (Tilemap builtins) → T7/T8 (Tilemap render) → Wave 3 integration → F1-F4

---

## Context

### Original Request
用户想要扩展 PML 在 2D 游戏素材制作方面的能力，特别是 tilemap 批量渲染、多通道（法线/高光）预烘焙、以及 SkSL 着色器多纹理支持。

### Interview Summary
**Key Discussions**:
- 三个方向全部做：Tilemap + 预烘焙 render-channels + SkSL 多纹理
- PML API 风格：矩阵式（define-tileset → make-tilemap → tilemap-set! → render-tilemap）
- 正交 tilemap：drawAtlas 批渲染（需要预栅格化瓦片类型到纹理图集）
- 等距 tilemap：drawVertices + Painter's Algorithm 深度排序
- render-channels 法线：替换着色法（fill color → 法线向量 RGB）
- render-channels 高光：灰阶输出
- 测试策略：TDD（先写 smoke test 再实现）
- Scope：无碰撞层、无物理、无路径寻路、无材料系统

**Research Findings**:
- `skia_backend_internal.h:lookup_shader()` 用 `makeShader(SkData::MakeEmpty(), {}, nullptr)`，children 数组为空——`uniform shader` 不工作，这是 SkSL 部分的关键修复点
- `create_tile()` 用 RNG（variant index 为种子）生成瓦片细节——确定的，每个 tile type 的 GraphicObject 模板可缓存
- RenderBackend 已有 `create_noise_shader()` / `create_shader_with_uniforms()` 两个虚方法，它们的默认实现返回错误——新增 `create_shader_with_children()` 遵循相同模式
- `BackendCap uint8_t` 8 bits 全用满——新增 tilemap/ channels 能力位需要展开到 `uint16_t`
- 不存在 drawAtlas 用法——全新建
- 不存在 tilemap 数据结构——全新建
- 不存在 render-channels 特性——全新建

### Metis Review
**Identified Gaps** (addressed):
- drawAtlas 需要预栅格化 tile type 到纹理图集，改变了渲染架构 → 接受，在计划中明确第一阶段为预栅格化
- render-channels 中法线贴图生成方式不确定 → 替换着色法（fill → normal vector）
- BackendCap 位不够 → uint8_t → uint16_t 扩展，向后兼容
- `uniform shader` children 空数组是核心技术瓶颈 → T1 修复
- 各特性间的依赖图清晰

---

## Work Objectives

### Core Objective
为 PML 扩展三条管线，使游戏美术师能够用 PML 代码定义和渲染多层 tilemap、导出多通道精灵素材（albedo/normal/specular），并通过 SkSL 多纹理着色器实现动态光照——全部通过 PML 内置函数控制，0 行运行时 C++ 代码。

### Concrete Deliverables
- **Tilemap system**: `define-tileset`, `make-tilemap`, `tilemap-set!`, `render-tilemap` builtins + C++ Tileset/Tilemap 数据结构 + 正交 drawAtlas 渲染 + 等距 drawVertices 渲染
- **Render channels**: `render-channels` builtin + 三通道导出（albedo/normal/specular）+ 文件输出
- **SkSL 多纹理**: `bind-textures` builtin + Skia `uniform shader` child 纹理支持

### Definition of Done
- [ ] `.\build\debug\tests\Debug\pml-builtins-smoke.exe` exits with code 0 (134 existing + ~20 new tests)
- [ ] All new PML examples run produce identical output to comparable Python version

### Must Have
- 134 现有 smoke tests 不破坏
- 正交 + 等距 tilemap 渲染都支持
- render-channels 输出 albedo + normal + specular 三通道
- SkSL 多纹理 bind-textures 的 builtin + child shader 正常工作
- build 通过，所有测试通过

### Must NOT Have (Guardrails)
- 不引入碰撞层、物理引擎、路径寻路
- 不引入材质/材料系统
- 不引入 HDR / 延迟渲染
- 不修改现有 builtins 注册签名
- 不重构 existing graphic pipeline internals（仅扩展）

---

## Verification Strategy (MANDATORY)

> **ZERO HUMAN INTERVENTION** — ALL verification is agent-executed. No exceptions.

### Test Decision
- **Infrastructure exists**: YES (GTest + builtins_smoke.cpp, 134 tests)
- **Automated tests**: TDD
- **Framework**: GTest (builtins_smoke.cpp CHECK/CHECK_ERROR macros)
- **TDD workflow**: Each task writes failing smoke test(s) first, then implements to pass

### QA Policy
Every task MUST include agent-executed QA scenarios.
Evidence saved to `.omo/evidence/task-{N}-{scenario-slug}.{ext}`.

- **C++ builtins**: Bash (run `pml-builtins-smoke.exe` check exit code + `pml.exe file.pml --json`)
- **Rendering output**: Playwright or Bash (compare output image dimensions/metadata)
- **Unit tests**: Bash (run `pml-builtins-smoke.exe` check individual test group)

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Foundation — start immediately, 5 tasks in parallel):
├── T1: RenderBackend — add create_shader_with_children() + BackendCap → uint16_t [quick]
├── T2: Tileset data structure + manager [quick]
├── T3: Tilemap data structure (flat int array + multi-layer + projection) [quick]
├── T4: Pre-rasterization utility (render GraphicObject → SkImage for drawAtlas) [quick]
└── T5: Smoke test stubs for all three features [quick]

Wave 2 (Tilemap — 4 tasks in parallel):
├── T6: define-tileset + make-tilemap + tilemap-set! builtins (depends: T2, T3, T5) [deep]
├── T7: Orthogonal tilemap render via drawAtlas (depends: T4, T6) [unspecified-high]
├── T8: Isometric tilemap render via drawVertices (depends: T6) [unspecified-high]
└── T9: Tilemap smoke tests (depends: T7, T8) [quick]

Wave 3 (Render channels — 3 tasks in parallel):
├── T10: render-channels — albedo + specular (depends: T5) [unspecified-high]
├── T11: render-channels — normal (replacement shading) (depends: T5) [unspecified-high]
└── T12: Render-channels smoke tests (depends: T10, T11) [quick]

Wave 4 (SkSL multi-texture — 2 tasks in parallel):
├── T13: bind-textures builtin + Skia child shader support (depends: T1, T5) [deep]
└── T14: Multi-texture smoke tests (depends: T13) [quick]

Wave FINAL (After ALL tasks — 4 parallel reviews):
├── F1: Plan compliance audit (oracle)
├── F2: Code quality review (unspecified-high)
├── F3: Real manual QA (unspecified-high)
└── F4: Scope fidelity check (deep)
   → Present consolidated results → Get explicit user okay
```

### Dependency Matrix

```
T1: -       → T13
T2: -       → T6
T3: -       → T6
T4: -       → T7
T5: -       → T6, T10, T11, T13
T6: T2,T3,T5,T9 → T7,T8
T7: T4,T6   → T9
T8: T6      → T9
T9: T7,T8   → -
T10: T5     → T12
T11: T5     → T12
T12: T10,T11 → -
T13: T1,T5  → T14
T14: T13    → -
F1-F4: ALL  → user okay
```

### Agent Dispatch Summary

- **Wave 1 (5 tasks)**: T1→`quick`, T2→`quick`, T3→`quick`, T4→`quick`, T5→`quick`
- **Wave 2 (4 tasks)**: T6→`deep`, T7→`unspecified-high`, T8→`unspecified-high`, T9→`quick`
- **Wave 3 (3 tasks)**: T10→`unspecified-high`, T11→`unspecified-high`, T12→`quick`
- **Wave 4 (2 tasks)**: T13→`deep`, T14→`quick`
- **FINAL (4 tasks)**: F1→`oracle`, F2→`unspecified-high`, F3→`unspecified-high`, F4→`deep`

---

## TODOs

- [x] 1. RenderBackend — add create_shader_with_children() + BackendCap → uint16_t

  **What to do**:
  - Add `virtual Result<Value> create_shader_with_children(const std::string& src, const std::vector<SkImageInfo>& childDescs, const std::vector<Value>& uniforms) = 0;` to `RenderBackend` ABC in `backend.h`
  - Add default error-return override in `null_backend.cpp` (returns `pml::runtime_error("...")`)
  - Add `create_shader_with_children` override in `skia_backend.cpp`:
    - 编译 SkSL source code (reuse `compile_shader()` logic)
    - `SkRuntimeEffect::MakeForShader(src, options)` → 这次传入 `options.fAllowUniform = true`
    - 为每个 child descriptor 创建 `SkRuntimeEffect::ChildPtr` (用 `SkImages::DeferredFromEncodedData` 或 `SkSurface::makeImageSnapshot`)
    - `effect->makeShader(uniform_data, children, nullptr)` — **把 children 数组传进去**
    - 缓存已编译 effect 和 uniform metadata
  - Expand `BackendCap` from `uint8_t` to `uint16_t` in `backend.h`
  - Add `BackendCap::Tilemap = 0x0100` and `BackendCap::RenderChannels = 0x0200`
  - All existing capability bits shift unchanged (0x01 through 0x80 stay same)

  **Must NOT do**:
  - Do NOT change existing `create_shader_with_uniforms()` signature
  - Do NOT break Null backend — it must return error, not crash

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Single-file ABC change + single-file backend override; well-defined scope with clear existing pattern to follow
  - **Skills**: (none needed — pure C++23, well-understood pattern)

  **Acceptance Criteria**:
  - [ ] `pml-builtins-smoke.exe` exits 0 (all 134 existing tests pass)
  - [ ] NullBackend::create_shader_with_children returns runtime_error with meaningful message
  - [ ] `BackendCap::Tilemap` exists and has value 0x0100

  **QA Scenarios**:
  ```
  Scenario: BackendCap Tilemap value
    Tool: Bash
    Preconditions: Build succeeded
    Steps:
      1. Search file `src/pml/backend/backend.h` for "Tilemap"
    Expected Result: `BackendCap::Tilemap = 0x0100` is present
    Failure Indicators: Not found or wrong value
    Evidence: .omo/evidence/task-1-backend-cap.txt

  Scenario: Null backend returns error
    Tool: Bash
    Preconditions: Built smoke test
    Steps:
      1. Run `.\build\debug\tests\Debug\pml-builtins-smoke.exe`
    Expected Result: Exit code 0 (all 134 existing tests pass unchanged)
    Failure Indicators: Exit code != 0
    Evidence: .omo/evidence/task-1-null-backend.txt
  ```

  **Evidence to Capture**:
  - [ ] `task-1-backend-cap.txt` — grep result
  - [ ] `task-1-null-backend.txt` — test exit code

  **Commit**: YES
  - Message: `feat(backend): add create_shader_with_children() virtual + BackendCap→uint16_t`
  - Files: `src/pml/backend/backend.h`, `src/pml/backend/null_backend.cpp`, `src/pml/backend/skia/skia_backend.cpp`, `src/pml/backend/skia/skia_backend_internal.h`

- [x] 2. Tileset data structure + manager

  **What to do**:
  - Create `src/pml/graphics/tileset.h` and `tileset.cpp`:
    - `struct TileType { int id; std::string name; GraphicObject graphic; std::optional<GraphicObject> detail; }`
    - `class TilesetManager` (singleton via `instance()` like `StyleRegistry`):
      - `register_tileset(const std::string& name, std::vector<TileType> tiles, int tile_size)`
      - `lookup_tileset(const std::string& name) → const Tileset*`
      - `Tileset` struct: `std::vector<TileType> tile_types`, `int tile_size`, `std::string name`
      - `get_tile_graphic(tileset_name, tile_id) → const GraphicObject*` (returns nullptr if not found)
    - Follow the `StyleRegistry` / `PaletteManager` singleton pattern
    - `void reset()` for test isolation
  - Add `#include "tileset.h"` to relevant files

  **Must NOT do**:
  - No PML builtins in this task — data layer only
  - No rendering code

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Standard data class + singleton manager; follows established StyleRegistry pattern exactly
  - **Skills**: (none needed)

  **Acceptance Criteria**:
  - [ ] Build succeeds with new .h/.cpp files
  - [ ] `pml-builtins-smoke.exe` exits 0 (tests don't use it yet, but compilation must not break)

  **QA Scenarios**:
  ```
  Scenario: TilesetManager singleton compiles
    Tool: Bash
    Preconditions: CMake configured
    Steps:
      1. `cmake --build --preset debug --target pml_graphics 2>&1 | tail -20`
    Expected Result: Build succeeds, no undefined symbols
    Failure Indicators: Linker errors or compilation errors
    Evidence: .omo/evidence/task-2-build.txt
  ```

  **Evidence to Capture**:
  - [ ] `task-2-build.txt` — build log tail

  **Commit**: YES (groups with T3)
  - Message: `feat(graphics): add Tileset data structure + TilesetManager singleton`

- [x] 3. Tilemap data structure (flat int array + multi-layer + projection)

  **What to do**:
  - Create `src/pml/graphics/tilemap.h` and `tilemap.cpp`:
    - `struct TilemapLayer { std::vector<int> tiles; int cols; int rows; bool visible; }`
    - `struct Tilemap { std::string tileset_name; int cols; int rows; int tile_size; std::vector<TilemapLayer> layers; Projection projection; }`
    - `enum class Projection { Orthogonal, Isometric }`
    - Helper: `tile_at(const TilemapLayer& layer, int col, int row) → int` (0=empty, bounds check→0)
    - Helper: `set_tile(TilemapLayer& layer, int col, int row, int tile_id)`
    - Helper: `add_layer(Tilemap& tm, int cols, int rows, bool visible = true)`
    - `TilemapManager` singleton: `create_tilemap(name, ...)`, `lookup_tilemap(name)`
    - `void reset()` for test isolation

  **Must NOT do**:
  - No rendering code
  - No PML builtins

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Straightforward data structures; no algorithms or graphics

  **Acceptance Criteria**:
  - [ ] Build succeeds
  - [ ] `pml-builtins-smoke.exe` exits 0

  **QA Scenarios**:
  ```
  Scenario: Tilemap compiles and links
    Tool: Bash
    Preconditions: CMake configured
    Steps:
      1. `cmake --build --preset debug --target pml_graphics 2>&1 | tail -20`
    Expected Result: Build succeeds
    Evidence: .omo/evidence/task-3-build.txt
  ```

  **Evidence to Capture**:
  - [ ] `task-3-build.txt`

  **Commit**: YES (groups with T2)
  - Message: `feat(graphics): add Tilemap data structure + TilemapLayer + Projection enum`

- [x] 4. Pre-rasterization utility: GraphicObject → SkImage for drawAtlas

  **What to do**:
  - Add a helper in `src/pml/backend/skia/skia_backend_draw.cpp` or a new `skia_backend_atlas.cpp`:
    - `SkImage* rasterize_graphic_to_image(const GraphicObject& go, int width, int height, SkColorType ct = kN32_SkColorType)`
    - Internally: create temp `SkSurface` (width×height), clear transparent, `draw_object(surface->getCanvas(), go, ...)`, return `surface->makeImageSnapshot()`
    - Handle groups: recurse into children
  - Create `struct RenderAtlas { std::unordered_map<int, sk_sp<SkImage>> tiles; int tile_size; }`
  - Helper: `build_tile_atlas(const Tileset& ts, const std::unordered_set<int>& used_ids) → RenderAtlas`
  - Handle `SkiaBackend` surface creation — reuse existing `create_surface()` or surface factory helper
  - Handle edge case: `nullptr` GraphicObject → transparent tile
  - Handle edge case: detail overlays (composite base + detail into same tile image)

  **Must NOT do**:
  - No drawAtlas draw calls here — just the pre-rasterization step
  - No tilemap traversal

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Self-contained utility; pattern exists in existing SkSurface usage in compositor
  - **Skills**: (none needed — follows existing skia surface patterns)

  **Acceptance Criteria**:
  - [ ] Build succeeds
  - [ ] `rasterize_graphic_to_image` creates valid SkImage with correct dimensions

  **QA Scenarios**:
  ```
  Scenario: Pre-rasterization compiles
    Tool: Bash
    Preconditions: Build setup
    Steps:
      1. `cmake --build --preset debug --target pml_graphics 2>&1 | tail -20`
    Expected Result: Build succeeds
    Evidence: .omo/evidence/task-4-build.txt
  ```

  **Evidence to Capture**:
  - [ ] `task-4-build.txt`

  **Commit**: NO (groups with T7)
  - Message: included in T7 commit

- [x] 5. Smoke test stubs for all three features

  **What to do**:
  - Add test groups to `tests/builtins_smoke.cpp`:
    - `// Test group: Tilemap basics`
    - `// Test group: Render channels (albedo/normal/specular)`
    - `// Test group: Multi-texture shaders`
  - Each group starts with 1-2 CHECK_ERROR tests verifying that unimplemented builtins return meaningful errors:
    - `CHECK_ERROR(execute("(render-tilemap 'nonexistent 'nonexistent)"), "not found")`
    - `CHECK_ERROR(execute("(define-tileset 'test :tile-size 32 :tiles '((0 none)))"), "not implemented")`
    - `CHECK_ERROR(execute("(render-channels ...)"), "not implemented")`
    - `CHECK_ERROR(execute("(bind-textures ...)"), "not implemented")`
  - These tests will FAIL at this stage (TDD red phase) — they define the expected API before implementation
  - Mark with comment: `// RED phase — expected to fail until builtin is implemented`
  - Add `#include` for any needed headers (probably none — execute() is the main interface)
  - Run test suite to confirm these 6+ tests fail (red)
  - Later tasks will make them pass (green)

  **Must NOT do**:
  - Do NOT implement the builtins here — just test stubs
  - Error messages must anticipate actual unimplemented state

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Simple CHECK_ERROR test addition; existing pattern to follow
  - **Skills**: (none needed)

  **Acceptance Criteria**:
  - [ ] All 134 existing tests still pass
  - [ ] The new 6 tests produce CHECK_ERROR failures (expected — red phase)
  - [ ] When builtins are implemented, tests automatically pass

  **QA Scenarios**:
  ```
  Scenario: Smoke test stubs exist
    Tool: Bash
    Preconditions: Built
    Steps:
      1. `.\build\debug\tests\Debug\pml-builtins-smoke.exe`
    Expected Result: Reports tests, new stubs show as "CHECK_ERROR passed" (unimplemented → error expected)
    Failure Indicators: Any existing test fails
    Evidence: .omo/evidence/task-5-smoke.txt
  ```

  **Evidence to Capture**:
  - [ ] `task-5-smoke.txt` — test output

  **Commit**: YES
  - Message: `test(builtins): add smoke test stubs for tilemap, render-channels, bind-textures`
  - Files: `tests/builtins_smoke.cpp`
  - Pre-commit: `.\build\debug\tests\Debug\pml-builtins-smoke.exe` (must emit but not crash)

- [x] 6. define-tileset + make-tilemap + tilemap-set! builtins

  **What to do**:
  - Create `src/pml/evaluator/tilemap_builtins.cpp` + `tilemap_builtins.h`
  - Register via `void register_tilemap_builtins(Environment* env)` in header
  - Implement:
    - `(define-tileset name :tile-size N :tiles '((id name [graphic-expr] [detail-expr]) ...))`:
      - Evaluate graphic-expr for each tile type (expects GraphicObject or compatible)
      - Evaluate detail-expr (optional overlay)
      - Store via `TilesetManager::instance().register_tileset(name, types, tile_size)`
      - Return the tileset name for chaining
    - `(make-tilemap tileset-name cols rows [:projection 'orthogonal|'isometric] [:layers N])`:
      - Lookup tileset, create Tilemap with N layers (default 2: ground + decoration)
      - Store via `TilemapManager`
      - Return tilemap name for chaining
    - `(tilemap-set! tilemap-name layer col row tile-id)`:
      - Lookup tilemap, set tile at position
      - Validate: layer in range, col/row in bounds, tile-id exists in tileset (or 0=empty)
      - Return #t
  - Must follow existing builtin registration pattern (see `graphics/render.cpp` `def()` lambda)
  - Error handling: runtime_error for invalid names, out of bounds, etc.
  - Follow the `kwargs` helper pattern from `graphics/render.cpp` (`parse_kwargs`, `kw_int`, `kw_string`)

  **Must NOT do**:
  - No rendering in this task — just data management
  - No `render-tilemap` yet (T7/T8)

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Requires understanding of existing builtin registration patterns, kwargs parsing, error handling; need to correctly integrate with TilesetManager/TilemapManager singletons
  - **Skills**: (none needed — codebase pattern knowledge required)

  **Parallelization**:
  - **Can Run In Parallel**: NO (blocked by T2, T3, T5)
  - **Parallel Group**: Wave 2
  - **Blocks**: T7, T8
  - **Blocked By**: T2 (Tileset data), T3 (Tilemap data), T5 (test stubs)

  **References**:
  - `src/pml/graphics/render.cpp:def()` — Builtin registration pattern with kwargs
  - `src/pml/sprites/style.cpp` — `StyleRegistry` singleton pattern
  - `src/pml/sprites/scene_elements.cpp:create_tile()` — TileColors/GraphicObject creation for tileset templates
  - `src/pml/evaluator/shader_builtins.cpp` — Another builtins file with separate .h/.cpp

  **Acceptance Criteria**:
  - [ ] Smoke test for `(define-tileset ...)` passes (was CHECK_ERROR → now returns name)
  - [ ] Smoke test for `(make-tilemap ...)` passes
  - [ ] Smoke test for `(tilemap-set! ...)` passes
  - [ ] All 134 existing tests still pass

  **QA Scenarios**:
  ```
  Scenario: define-tileset works
    Tool: Bash
    Preconditions: Built
    Steps:
      1. `.\build\debug\tests\Debug\pml-builtins-smoke.exe`
    Expected Result: Tilemap basic stubs now pass (CHECK_ERROR → CHECK)
    Failure Indicators: Any test failure
    Evidence: .omo/evidence/task-6-tilemap-builtins.txt

  Scenario: Error handling
    Tool: Bash
    Preconditions: Built
    Steps:
      1. Add a minimal .pml test: write `(tilemap-set! 'nonexistent 0 0 1)` and run with `.\build\debug\bin\Debug\pml.exe`
    Expected Result: Meaningful error message (not a crash)
    Evidence: .omo/evidence/task-6-error.txt
  ```

  **Evidence to Capture**:
  - [ ] `task-6-tilemap-builtins.txt`
  - [ ] `task-6-error.txt`

  **Commit**: YES
  - Message: `feat(evaluator): add define-tileset, make-tilemap, tilemap-set! builtins`
  - Files: `src/pml/evaluator/tilemap_builtins.cpp`, `src/pml/evaluator/tilemap_builtins.h`, `src/pml/evaluator/CMakeLists.txt`, `src/pml/api/api.cpp`
  - Pre-commit: `.\build\debug\tests\Debug\pml-builtins-smoke.exe`

- [x] 7. Orthogonal tilemap render via drawAtlas

  **What to do**:
  - Implement `(render-tilemap tilemap-name [:projection 'orthogonal] [:output path] [:layer N|:layers 'all])` builtin:
    - Orthogonal projection path (default):
      - Lookup tilemap + tileset
      - Call `build_tile_atlas()` (T4) to get RenderAtlas with SkImage per tile type
      - Traverse tilemap grid: for each non-empty tile, collect (SkImage, dst_rect) pair
      - Call `SkCanvas::drawAtlas(atlas_images, xforms, rects, paints, ...)` for batch draw
      - Apply tilemap transform (position, scale via :transform kwarg or graphic context)
      - Save result to output file via `save_image()` or skia surface export
    - Handle layer parameter: single layer or composite all layers
    - Handle empty tiles with SkBlendMode::kClear or skip
    - Handle multiple layers: render ground first, then decorations on top (depth sorting: layer 0 = bottom)
  - Add registration in `tilemap_builtins.cpp`
  - Handle edge cases: empty tilemap (no non-zero tiles), single-rows, single-columns

  **Must NOT do**:
  - Do NOT handle isometric projection (T8)
  - Do NOT add collision or spatial queries

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: Requires Skia drawAtlas API usage (new to codebase) + tilemap rendering pipeline assembly
  - **Skills**: (none needed — Skia API knowledge required)

  **References**:
  - `src/pml/backend/skia/skia_backend_draw.cpp:draw_object()` — Existing SkCanvas draw pattern
  - `SkCanvas::drawAtlas(const sk_sp<SkImage> atlas[], const SkRSXform xform[], const SkRect tex[], const SkColor colors[], int count, SkBlendMode mode, ...)` — Skia API for batch texture draw
  - `src/pml/backend/skia/skia_backend.cpp:save_image()` — Output image saving pattern
  - `src/pml/graphics/render.cpp:render()` — Full builtin with output file saving

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with T8)
  - **Blocks**: T9
  - **Blocked By**: T4, T6

  **Acceptance Criteria**:
  - [ ] `(render-tilemap 'test :projection 'orthogonal :output 'out.png)` produces a valid PNG
  - [ ] All existing tests pass

  **QA Scenarios**:
  ```
  Scenario: Orthogonal tilemap renders
    Tool: Bash
    Preconditions: Build, a minimal .pml test file
    Steps:
      1. Write test.pml: (define-tileset 'terrain :tile-size 32 :tiles '((1 grass (rect 0 0 32 32 :fill "green"))))
      2. (define tm (make-tilemap 'terrain 5 5 :projection 'orthogonal))
      3. (tilemap-set! tm 0 2 2 1)
      4. (render-tilemap tm :output 'test_tilemap.png)
      5. Check file exists and has correct dimensions (5*32 x 5*32 = 160x160)
    Expected Result: test_tilemap.png is 160x160 pixels, non-transparent at tile (2,2)
    Evidence: .omo/evidence/task-7-ortho.png
  ```

  **Evidence to Capture**:
  - [ ] `task-7-ortho.png` — rendered output screenshot

  **Commit**: YES (groups with T4, T8)
  - Message: `feat(tilemap): add orthogonal tilemap render via drawAtlas`

- [x] 8. Isometric tilemap render via drawVertices

  **What to do**:
  - Implement isometric projection path in `(render-tilemap ... :projection 'isometric)`:
    - Calculate isometric tile dimensions: tile_width = cols * tile_size, tile_height = rows * (tile_size/2)
    - For each non-empty tile at (col, row), compute 3-face diamond vertex positions:
      - Top face: standard diamond shape (4 vertices)
      - Left face: parallelogram for left side
      - Right face: parallelogram for right side
    - Build vertex array per tile face with `SkPoint[]` + `SkColor[]`
    - Call `SkCanvas::drawVertices(SkVertices::MakeCopy(...), SkBlendMode, ...)` for each 3-face tile
    - Depth sorting: Painter's Algorithm — sort tiles by (row + col), render back-to-front
    - Multiple layers: render layer 0 ground tiles first (back-to-front), then layer 1 decorations
    - Texture mapping: use tile atlas SkImage to texture-map onto diamond vertices
    - Handle tile size parameter: support different tile sizes (16, 32, 64)
  - Add to existing registration in `tilemap_builtins.cpp`
  - Handle edge cases: empty tilemap, partial tiles at edges
  - Can reference existing `src/pml/graphics3d/mesh_draw.cpp:draw_mesh_skia()` for drawVertices usage pattern

  **Must NOT do**:
  - Do NOT add a material system
  - Do NOT add 3D camera support for isometric (tilemap should use its own orthographic isometric projection)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: Requires understanding of isometric projection math + Skia drawVertices API + Painter's Algorithm depth sorting
  - **Skills**: (none needed)

  **References**:
  - `src/pml/graphics3d/mesh_draw.cpp:draw_mesh_skia()` — Existing drawVertices usage pattern with SkVertices
  - `SkCanvas::drawVertices(sk_sp<SkVertices>, SkBlendMode, const SkPaint&)` — Skia API reference
  - `examples/09-projects/07-11_isometric_tilemap/` — Existing PML isometric tilemap examples (semantics reference)

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with T7)
  - **Blocks**: T9
  - **Blocked By**: T6

  **Acceptance Criteria**:
  - [ ] `(render-tilemap 'test :projection 'isometric :output 'out.png)` produces a valid PNG
  - [ ] Isometric tiles render with correct diamond positioning
  - [ ] Depth ordering correct (back tiles render before front tiles)
  - [ ] All existing tests pass

  **QA Scenarios**:
  ```
  Scenario: Isometric tilemap renders
    Tool: Bash
    Preconditions: Build, a minimal .pml test file
    Steps:
      1. Write test_iso.pml: (define-tileset 'terrain :tile-size 64 :tiles '((1 grass (polygon 0 32 32 0 64 32 32 64 :fill "#4a7"))) )
      2. (define tm (make-tilemap 'terrain 3 3 :projection 'isometric))
      3. (tilemap-set! tm 0 1 1 1)
      4. (render-tilemap tm :output 'test_isometric.png)
      5. Check file exists
    Expected Result: test_isometric.png is a rendered isometric diamond tile
    Evidence: .omo/evidence/task-8-iso.png
  ```

  **Evidence to Capture**:
  - [ ] `task-8-iso.png` — rendered isometric output

  **Commit**: YES (groups with T7)
  - Message: `feat(tilemap): add isometric tilemap render via drawVertices`

- [x] 9. Tilemap smoke tests

  **What to do**:
  - Add full smoke tests to `tests/builtins_smoke.cpp`:
    - `CHECK(execute("(define-tileset 'terrain :tile-size 32 :tiles '((1 grass)) :graphics '((1 (rect 0 0 32 32 :fill \"green\"))))"))` — verify tileset creation
    - `CHECK(execute("(define tm (make-tilemap 'terrain 5 5 :projection 'orthogonal :layers 2))"))` — verify tilemap creation
    - `CHECK(execute("(tilemap-set! tm 0 2 2 1)"))` — verify tile setting
    - `CHECK(execute("(render-tilemap tm :output 'test_tm.png)"))` — verify rendering (file exists + non-zero)
    - Isometric: similar flow with `:projection 'isometric`
    - Error cases: `(tilemap-set! 'nonexistent 0 0 1)` → runtime_error
    - Edge cases: tilemap with zero layers, tilemap with all empty tiles, out-of-bounds set!
  - Convert the stubs from T5 from CHECK_ERROR to CHECK as builtins now exist
  - These tests are the "GREEN" phase — they must all pass

  **Must NOT do**:
  - Do NOT add render-channels or multi-texture tests here (those go in T12, T14)
  - Do NOT add image pixel-level comparison (file existence is sufficient for smoke tests)

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Test additions following existing CHECK/CHECK_ERROR pattern
  - **Skills**: (none needed)

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on T7, T8)
  - **Parallel Group**: Sequential after T7+T8
  - **Blocks**: (none)
  - **Blocked By**: T7, T8

  **Acceptance Criteria**:
  - [ ] All tilemap smoke tests pass
  - [ ] All 134 existing tests still pass
  - [ ] `pml-builtins-smoke.exe` exits 0

  **QA Scenarios**:
  ```
  Scenario: Tilemap smoke tests pass
    Tool: Bash
    Preconditions: Built
    Steps:
      1. `.\build\debug\tests\Debug\pml-builtins-smoke.exe`
    Expected Result: All tests pass, exit code 0
    Failure Indicators: Any test failure
    Evidence: .omo/evidence/task-9-smoke.txt
  ```

  **Evidence to Capture**:
  - [ ] `task-9-smoke.txt` — test output

  **Commit**: YES
  - Message: `test(tilemap): add tilemap smoke tests (tileset, tilemap, set!, render orthogonal + isometric)`
  - Files: `tests/builtins_smoke.cpp`
  - Pre-commit: `.\build\debug\tests\Debug\pml-builtins-smoke.exe`


- [x] 10. render-channels builtin — albedo + specular

  **What to do**:
  - Create `src/pml/evaluator/render_channels_builtins.cpp` + `render_channels_builtins.h`
  - Register via `void register_render_channels(Environment* env)` in header
  - Implement `(render-channels sprite-name :output path [:channels '(albedo specular normal)] [:width W] [:height H])`:
    - For each requested channel:
      - **Albedo**: Render the sprite GraphicObject normally (current render behavior), save as PNG
      - **Specular**: Render same sprite but output as grayscale intensity:
        - Traverse GraphicObject tree, replace all fill colors with their luminance (gray = 0.299R + 0.587G + 0.114B)
        - Use Skia grayscale color filter or directly set SkPaint color to gray value
        - Output file naming: `{base}_{channel}.png` (e.g., `sword_albedo.png`, `sword_specular.png`)
    - Handle graphic context (transform, position) — reuse existing render pipeline
    - Reuse `SkSurface`/`SkCanvas` creation pattern from `render()` in `render.cpp`
    - Reuse compositor layer infrastructure if sprite has layer annotations
  - Add registration call in `PMLRuntime::init_global_env()` in `api.cpp`
  - Handle error: invalid sprite name, missing output path, unknown channel name

  **Must NOT do**:
  - No normal channel in this task (T11)
  - Do NOT modify existing `render` builtin

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: Requires understanding of SkSurface/SkCanvas creation, GraphicObject tree traversal, and render pipeline; moderate complexity
  - **Skills**: (none needed)

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 3 (with T11)
  - **Blocks**: T12
  - **Blocked By**: T5 (test stubs)

  **References**:
  - `src/pml/graphics/render.cpp:render()` — Full render builtin with surface creation, file output
  - `src/pml/layer/compositor.cpp` — Layer composition (reusable for multi-channel)
  - `src/pml/backend/skia/skia_backend_draw.cpp:draw_object()` — SkCanvas draw dispatch
  - `src/pml/backend/skia/skia_backend.cpp:save_image()` — PNG output pattern

  **Acceptance Criteria**:
  - [ ] `(render-channels 'sprite :output 'test :channels '(albedo specular))` produces `test_albedo.png` + `test_specular.png`
  - [ ] All existing tests pass
  - [ ] Error on unknown channel name returns meaningful message

  **QA Scenarios**:
  ```
  Scenario: Render albedo channel
    Tool: Bash
    Preconditions: Build, a minimal .pml test file with a defined sprite
    Steps:
      1. Write test.pml: (define sprite (circle 32 32 16 :fill "red" :stroke "black"))
      2. (render-channels sprite :output 'test_output :channels '(albedo specular))
      3. Check test_output_albedo.png exists and is > 0 bytes
      4. Check test_output_specular.png exists and is > 0 bytes
    Expected Result: Two PNG files created
    Evidence: .omo/evidence/task-10-channels.txt
  ```

  **Evidence to Capture**:
  - [ ] `task-10-channels.txt` — file existence + sizes
  - [ ] `test_output_albedo.png` — albedo output

  **Commit**: YES (groups with T11, T12)
  - Message: `feat(render): add render-channels builtin (albedo + specular)`

- [x] 11. render-channels builtin — normal channel (replacement shading)

  **What to do**:
  - Extend `(render-channels ... :channels '(normal))` in `render_channels_builtins.cpp`:
    - Normal channel implementation (替换着色法):
      - Traverse GraphicObject tree recursively
      - For each primitive (rect, circle, polygon, etc.), compute its surface normal:
        - 2D shapes always face camera → default normal = (0, 0, 1) → RGB(128, 128, 255)
        - Edges/lines: normal perpendicular to line direction
        - Gradients: approximate per-pixel normal from gradient direction
        - Since this is 2D sprite pre-baking, simple default approach: use the shape's fill color as normal direction hint
      - Replace fill/stroke color with normal vector RGB:
        - `normal_color = (nx * 127 + 128, ny * 127 + 128, nz * 127 + 128)`
        - Default for 2D shapes: RGB(128, 128, 255) (straight up Z-normal)
        - Optional user override via `:normals '((shape-id (1.0 0.0 0.0)) ...)` kwarg
      - Preserve alpha channel (transparency from original sprite)
      - Render to SkSurface with modified colors
      - Output as `{base}_normal.png`
    - Handle all GraphicObject types: groups (recurse), paths (per-vertex normals future)
    - Handle nested groups (recurse into children)
    - Set `SkPaint::setColor()` with the normal-mapped color before each draw call

  **Must NOT do**:
  - Do NOT add 3D mesh-derived normals (this is 2D only)
  - Do NOT modify existing GraphicObject structures — only transform colors at render time

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: Requires understanding of GraphicObject tree recursion + color transformation at render time
  - **Skills**: (none needed)

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 3 (with T10)
  - **Blocks**: T12
  - **Blocked By**: T5

  **References**:
  - `src/pml/graphics/objects.h` — GraphicObject types (shape, group, path, etc.)
  - `src/pml/graphics/render.cpp` — Existing render dispatch
  - `src/pml/backend/skia/skia_backend_draw.cpp:draw_object()` — How colors are applied per shape type

  **Acceptance Criteria**:
  - [ ] `(render-channels 'sprite :channels '(normal))` produces `{base}_normal.png`
  - [ ] Normal image has correct dimensions matching sprite
  - [ ] All existing tests pass

  **QA Scenarios**:
  ```
  Scenario: Normal channel renders
    Tool: Bash
    Preconditions: Build
    Steps:
      1. Write test_norm.pml: (define sprite (rect 0 0 32 32 :fill "red"))
      2. (render-channels sprite :output 'norm_test :channels '(normal))
      3. Check norm_test_normal.png exists
      4. Check dimensions: expected 32x32
    Expected Result: normal PNG file created with proper dimensions
    Evidence: .omo/evidence/task-11-normal.png
  ```

  **Evidence to Capture**:
  - [ ] `task-11-normal.png`

  **Commit**: YES (groups with T10, T12)
  - Message: `feat(render): add normal channel to render-channels (replacement shading)`

- [x] 12. Render-channels smoke tests

  **What to do**:
  - Add full smoke tests to `tests/builtins_smoke.cpp`:
    - `CHECK(execute("(render-channels (rect 0 0 16 16 :fill \"red\") :output 'rc_test :channels '(albedo specular normal))"))` — verify multi-channel output
    - `CHECK_ERROR(execute("(render-channels ... :channels '(unknown))"))` — error on invalid channel
    - `CHECK_ERROR(execute("(render-channels 'nonexistent :channels '(albedo))"))` — error on invalid sprite
    - Check file existence: `rc_test_albedo.png`, `rc_test_specular.png`, `rc_test_normal.png`
  - Convert T5 stubs from CHECK_ERROR to CHECK

  **Must NOT do**:
  - Do NOT add image pixel comparison (file existence + non-zero size is sufficient)

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Test additions following existing pattern

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Sequential after T10+T11
  - **Blocks**: (none)
  - **Blocked By**: T10, T11

  **Acceptance Criteria**:
  - [ ] All render-channels smoke tests pass
  - [ ] `pml-builtins-smoke.exe` exits 0

  **QA Scenarios**:
  ```
  Scenario: Render-channels smoke tests pass
    Tool: Bash
    Steps:
      1. `.\build\debug\tests\Debug\pml-builtins-smoke.exe`
    Expected Result: Exit code 0
    Evidence: .omo/evidence/task-12-smoke.txt
  ```

  **Evidence to Capture**:
  - [ ] `task-12-smoke.txt`

  **Commit**: YES (groups with T10, T11)
  - Message: `test(render): add render-channels smoke tests`
  - Pre-commit: `.\build\debug\tests\Debug\pml-builtins-smoke.exe`

- [x] 13. bind-textures builtin + Skia child shader support

  **What to do**:
  - Extend `shader_builtins.cpp` (or create `multi_texture_builtins.h/.cpp`):
    - Implement `(bind-textures shader-name :textures '((slot-name texture-expr) ...))`:
      - Evaluate each texture-expr to get an SkImage (from file load, sprite-canvas snapshot, or inline)
      - Call `SkRuntimeEffect::MakeForShader(src, options)` with children descriptors
      - For each `uniform shader child_name`, bind the corresponding SkImage as `SkRuntimeEffect::ChildPtr`
      - Return a new shader value that encapsulates the SkRuntimeEffect + children
    - Extend existing `skia_backend.cpp:compile_shader()` to accept optional children array
    - Modify `lookup_shader()` in `skia_backend_internal.h`:
      - **Critical fix**: change `makeShader(SkData::MakeEmpty(), {}, nullptr)` to pass actual children
      - Store children as `std::vector<sk_sp<SkShader>>` alongside compiled programs
    - Extend `create_shader_with_uniforms()` to pass through children if present
  - Handle error: invalid slot name (mismatch with SkSL `uniform shader` declarations)
  - Handle error: texture-expr that doesn't resolve to an image
  - Wire into `PMLRuntime::init_global_env()` if new file

  **Must NOT do**:
  - Do NOT break existing `shader` / `apply-shader!` / `apply-uniforms` / `noise-fractal` workflows
  - Do NOT modify the `uniform float` path (that code is working)

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Requires deep understanding of SkSL compilation pipeline, SkRuntimeEffect::ChildPtr API, and fixing critical bug in existing lookup_shader() children array
  - **Skills**: (none needed)

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Sequential
  - **Blocks**: T14
  - **Blocked By**: T1 (create_shader_with_children virtual), T5 (test stubs)

  **References**:
  - `src/pml/backend/skia/skia_backend_internal.h:lookup_shader()` — **The key fix target** `makeShader(SkData::MakeEmpty(), {}, nullptr)`
  - `src/pml/backend/skia/skia_backend.cpp:compile_shader()` — Existing compilation
  - `src/pml/backend/skia/skia_backend.cpp:create_shader_with_uniforms()` — Uniform flow to extend
  - `src/pml/evaluator/shader_builtins.cpp` — Existing shader builtins
  - Skia API: `SkRuntimeEffect::MakeForShader(SkString, Options)` — Options allows `fAllowUniform = true`
  - Skia API: `SkRuntimeEffect::makeShader(sk_sp<SkData> uniforms, sk_sp<SkShader> children[], size_t childCount, const SkMatrix* localMatrix)` — **The correct call**

  **Acceptance Criteria**:
  - [ ] `(bind-textures ...)` successfully binds an SkImage to a `uniform shader` in SkSL code
  - [ ] Existing shader workflow (noise, uniforms) still works
  - [ ] All 134 existing tests pass

  **QA Scenarios**:
  ```
  Scenario: Bind-textures creates multi-texture shader
    Tool: Bash
    Preconditions: Build, Skia backend enabled
    Steps:
      1. Write test_tex.pml: (define tex-shader (shader "
          uniform shader base;
          uniform shader overlay;
          half4 main(float2 xy) {
            half4 b = base.eval(xy);
            half4 o = overlay.eval(xy);
            return half4(mix(b.rgb, o.rgb, o.a), b.a + o.a*(1-b.a));
          }"))
      2. (bind-textures tex-shader :textures '((base (rect 0 0 64 64 :fill "red")) (overlay (circle 32 32 16 :fill "blue"))))
      3. Render with shader applied
    Expected Result: Renders composited red rect + blue circle
    Evidence: .omo/evidence/task-13-multitex.png

  Scenario: Existing shader still works
    Tool: Bash
    Steps:
      1. `.\build\debug\tests\Debug\pml-builtins-smoke.exe`
    Expected Result: Exit 0
    Evidence: .omo/evidence/task-13-existing.txt
  ```

  **Evidence to Capture**:
  - [ ] `task-13-multitex.png` — rendered multi-texture output
  - [ ] `task-13-existing.txt` — smoke test output

  **Commit**: YES (groups with T14)
  - Message: `feat(sksl): add bind-textures builtin + child shader support in Skia backend`
  - Files: `src/pml/evaluator/shader_builtins.cpp`, `src/pml/backend/skia/skia_backend_internal.h`, `src/pml/backend/skia/skia_backend.cpp`
  - Pre-commit: `.\build\debug\tests\Debug\pml-builtins-smoke.exe`

- [x] 14. Multi-texture smoke tests

  **What to do**:
  - Add smoke tests to `tests/builtins_smoke.cpp`:
    - `CHECK(execute("(define s (shader \"uniform shader tex; half4 main(float2 xy) { return tex.eval(xy); }\"))"))` — verify shader with child compiles
    - `CHECK(execute("(bind-textures s :textures '((tex (rect 0 0 16 16 :fill \"red\"))))"))` — verify bind
    - `CHECK_ERROR(execute("(bind-textures 'nonexistent :textures '((tex ...)))"))` — error on invalid shader
    - Render with multi-texture shader and verify output exists
  - Convert T5 stubs from CHECK_ERROR to CHECK

  **Must NOT do**:
  - Do NOT add complex shader tests (just verify the pipeline works)

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Standard smoke test additions

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Sequential after T13
  - **Blocks**: (none)
  - **Blocked By**: T13

  **Acceptance Criteria**:
  - [ ] All multi-texture smoke tests pass
  - [ ] `pml-builtins-smoke.exe` exits 0

  **QA Scenarios**:
  ```
  Scenario: Multi-texture smoke tests pass
    Tool: Bash
    Steps:
      1. `.\build\debug\tests\Debug\pml-builtins-smoke.exe`
    Expected Result: Exit code 0
    Evidence: .omo/evidence/task-14-smoke.txt
  ```

  **Evidence to Capture**:
  - [ ] `task-14-smoke.txt`

  **Commit**: YES (groups with T13)
  - Message: `test(sksl): add multi-texture shader smoke tests`
  - Pre-commit: `.\build\debug\tests\Debug\pml-builtins-smoke.exe`

---

## Final Verification Wave (MANDATORY — after ALL implementation tasks)

> 4 review agents run in PARALLEL. ALL must APPROVE. Present consolidated results to user and get explicit "okay" before completing.

- [x] F1. **Plan Compliance Audit** — `oracle`
  Read the plan end-to-end. For each "Must Have": verify implementation exists (read file, curl endpoint, run command). For each "Must NOT Have": search codebase for forbidden patterns — reject with file:line if found. Check evidence files exist in .omo/evidence/. Compare deliverables against plan.
  Output: `Must Have [N/N] | Must NOT Have [N/N] | Tasks [N/N] | VERDICT: APPROVE/REJECT`

- [x] F2. **Code Quality Review** — `unspecified-high`
  Run `cmake --build --preset debug` + `pml-builtins-smoke.exe`. Review all changed files for: `throw` in builtins (should use Result<T>), raw pointers without ownership docs, unused includes, commented-out code, AI slop (excessive comments, over-abstraction). Check new singletons have `reset()` for tests.
  Output: `Build [PASS/FAIL] | Tests [N pass/N fail] | Files [N clean/N issues] | VERDICT`

- [x] F3. **Real Manual QA** — `unspecified-high` (+ `playwright` skill if UI)
  Start from clean build. Execute EVERY QA scenario from EVERY task — follow exact steps, capture evidence. Test cross-task integration: define tileset → make tilemap → render-tilemap orthogonal + isometric → render-channels on tilemap output → bind-textures. Test edge cases: empty tilemap, out-of-bounds tilemap-set!, invalid channel names, invalid shader textures. Save to `.omo/evidence/final-qa/`.
  Output: `Scenarios [N/N pass] | Integration [N/N] | Edge Cases [N tested] | VERDICT`

- [x] F4. **Scope Fidelity Check** — `deep`
  For each task: read "What to do", read actual diff (git log/diff). Verify 1:1 — everything in spec was built (no missing), nothing beyond spec was built (no creep). Check "Must NOT do" compliance (no collision/physics/material system). Detect cross-task contamination: T10 touching tilemap files. Flag unaccounted changes.
  Output: `Tasks [N/N compliant] | Contamination [CLEAN/N issues] | Unaccounted [CLEAN/N files] | VERDICT`

---

## Commit Strategy

| Wave | Commits | Message Format |
|------|---------|---------------|
| Wave 1 | T1 alone, T2+T3 together, T4+T5 together | `feat(backend): ...`, `feat(graphics): ...`, `test(builtins): ...` |
| Wave 2 | T6 alone, T7+T4+T8 together, T9 alone | `feat(evaluator): ...`, `feat(tilemap): ...`, `test(tilemap): ...` |
| Wave 3 | T10+T11+T12 together | `feat(render): add render-channels ...` |
| Wave 4 | T13+T14 together | `feat(sksl): add bind-textures ...` |
| Final | F1-F4 (review only, no code commit) | — |

---

## Success Criteria

### Verification Commands
```bash
cmake --build --preset debug
.\build\debug\tests\Debug\pml-builtins-smoke.exe  # Expected: exit 0, all tests pass
```

### Final Checklist
- [ ] All tilemap builtins work: define-tileset, make-tilemap, tilemap-set!, render-tilemap
- [ ] Orthogonal tilemap renders via drawAtlas
- [ ] Isometric tilemap renders via drawVertices with correct depth ordering
- [ ] render-channels outputs albedo, normal, specular PNGs
- [ ] bind-textures binds SkImages to `uniform shader` in SkSL
- [ ] All 134 existing smoke tests still pass
- [ ] No "Must NOT Have" features introduced (collision, physics, material system)
- [ ] 0 new compiler warnings
- [ ] All QA evidence files present in `.omo/evidence/`

