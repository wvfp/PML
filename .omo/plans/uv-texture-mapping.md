# UV Texture Mapping System for PML

## TL;DR

> **Quick Summary**: Add a complete UV texture mapping system to PML, allowing procedural textures (defined via PML code) to be mapped onto 2D shapes with high-quality Harmonic UV projection, explicit UV control, and full module system integration.
>
> **Deliverables**:
> - `define-texture` builtin — define procedural textures as PML values
> - `texture-map` builtin — UV-map a texture onto any shape
> - Harmonic UV auto-projection for 2D shapes (quality default)
> - Planar UV and Explicit UV as alternatives
> - poly2tri-based shape triangulation (constraint Delaunay)
> - Self-implemented Conjugate Gradient solver (no Eigen dependency)
> - Texture caching system (LRU by stable ID)
> - Multi-texture blend via SkSL shader composition
> - Module integration (textures pass through import/provide as Values)
> - Smoke tests for all new builtins
>
> **Estimated Effort**: Large (~2400 lines)
> **Parallel Execution**: YES — 4 waves
> **Critical Path**: poly2tri integration → ParamKey expansion → Texture Value type → Central dispatcher → all builtins

---

## Context

### Original Request
The user wants a PML feature where:
- File A defines a procedural texture (e.g., a starry sky using PML shapes, not just a solid color)
- File B defines a shape (any closed shape: path, polygon, circle, etc.)
- File C references both and UV-maps the texture from A onto the shape from B

### Interview Summary
**Key Discussions**:
- Texture source: `define-texture` (function-based lambda) as the sole mechanism. No file-level convention.
- Shape mapping: `texture-map` builtin as high-level API + extension of `:fill` parameter on existing shapes
- UV coordinate: Auto-flatten (Harmonic mapping) by default, with `:uv-vertices` for manual override
- Projection methods: Planar (trivial) + Harmonic (default high quality) + Explicit (manual control). LSCM and Cage DEFERRED.
- Texture is a first-class Value (`Box::Kind::Texture`), passes through module system like any other value
- Stroke is independent of texture (solid color stroke drawn on top of textured fill)
- No rough-style rendering with textures (skipped)
- Highest rendering quality, performance not a concern
- Reuse open-source libraries: poly2tri for triangulation (BSD 3-Clause), self-implemented CG solver (not Eigen)

**Research Findings**:
- Skia has no built-in 2D triangulation → need poly2tri (BSD 3-Clause, header-only, simple API)
- Skia's `drawVertices()` with `SkVertices::MakeCopy(positions, uvs)` provides perspective-correct texture mapping — proven by existing `draw_mesh3d_impl()`
- `bind-textures` already demonstrates the pattern: GraphicObject → offscreen SkSurface → SkImage → SkShader
- Existing 3D Mesh3D::Face already has per-face UV + material GraphicObject — the 2D system mirrors this
- ParamKey enum has 8 free slots (32-bit mask, 24 used) — sufficient for uv, uv_mode, wrap_x, wrap_y, filter_mode
- Box::Kind enum is open (23 members currently) — adding `Texture` is clean

### Metis Review
**Identified Gaps** (addressed):
- **Test strategy**: Tests-after + Agent QA scenarios per task (each task renders a .pml and verifies output)
- **Stroke behavior**: Solid-color stroke independent of texture (texture affects fill only)
- **ParamKey capacity**: Only 8 slots remain, confirmed sufficient for 5 needed params
- **Library choice**: poly2tri > libtess2 (simpler API, BSD 3-Clause, header-only)
- **Solver choice**: Self-implemented CG (not Eigen — Eigen is massive overkill for <500 vertices)
- **Projection scope**: Planar + Harmonic + Explicit only. LSCM and Cage DEFERRED.

---

## Work Objectives

### Core Objective
Add a UV texture mapping system that lets users define PML-based procedural textures and map them onto 2D shapes with high-quality automatic UV projection, full module integration, and multiple projection methods (Planar, Harmonic, Explicit).

### Concrete Deliverables
- `core/texture.h/cpp` — Texture value type (`Box::Kind::Texture`), lazy bake cache
- `graphics/triangulation.h/cpp` — poly2tri integration, shape → triangle mesh
- `graphics/uv_solver.h/cpp` — Planar/Harmonic/Explicit UV solvers + CG solver
- `evaluator/texture_builtins.h/cpp` — `define-texture`, `texture?`, `texture-width`, `texture-height`
- `evaluator/texture_map_builtins.h/cpp` — `texture-map`, multi-texture blend
- `backend/skia/textured_draw.h/cpp` — Skia backend: bake texture, build SkVertices, draw
- Modified `graphics/params.h` — 5 new ParamKey entries
- Modified `core/types.h` — Box::Kind::Texture
- Modified `backend/skia/skia_backend_draw.cpp` — dispatch to textured draw path
- Modified `api/api.cpp` — register new builtin groups
- CMakeLists — integrate poly2tri, new texture library
- `tests/builtins_smoke.cpp` — smoke test cases

### Definition of Done
- [ ] `(define-texture ...)` successfully creates a Texture value
- [ ] `(texture-map (polygon ...) :source tex)` renders texture on shape with Harmonic UV
- [ ] `(polygon ... :fill tex)` works as shorthand for texture-map
- [ ] `:wrap-x/:wrap-y/:filter` control texture sampling
- [ ] Multi-texture blend via Shader-based composition
- [ ] Textures pass through modules via import/provide
- [ ] All builtins smoke-test pass
- [ ] Example .pml files render correctly

### Must Have
- Texture as `Box::Kind::Texture` first-class value
- Harmonic UV projection as default (auto, high quality)
- Planar UV projection (trivial AABB)
- Explicit UV via `:uv-vertices` parameter
- poly2tri-based triangulation for non-trivial shapes
- Texture caching (LRU by stable ID)
- Module integration (provide/import of textures)
- Wrap modes (clamp, repeat, mirror)
- Filter modes (linear, nearest, anisotropic)
- Multi-texture blend via SkSL `uniform shader` composition

### Must NOT Have (Guardrails)
- **NO** LSCM projection (deferred)
- **NO** Cage deformation (deferred)
- **NO** File-level texture convention (use define-texture only)
- **NO** Multi-layer texture editor/palette system
- **NO** UV animation support
- **NO** Eigen library dependency (self-implemented CG solver instead)
- **NO** Rough-style rendering with textures (skipped)
- **NO** Textured stroke (solid-color stroke only)

---

## Verification Strategy

> **ZERO HUMAN INTERVENTION** — ALL verification is agent-executed. No exceptions.
> Acceptance criteria requiring "user manually tests/confirms" are FORBIDDEN.

### Test Decision
- **Infrastructure exists**: YES (GTest + builtins_smoke.cpp pattern)
- **Automated tests**: Tests-after (add smoke test cases after implementation)
- **Framework**: GTest (`tests/builtins_smoke.cpp`, ~134 existing cases)
- **Pattern**: Each new builtin gets a smoke test: execute .pml string, verify output image exists and is non-empty

### QA Policy
Every task MUST include agent-executed QA scenarios (see TODO template below).
Evidence saved to `.omo/evidence/task-{N}-{scenario-slug}.{ext}`.

- **Texture builtins**: Write .pml file → execute with `pml.exe` → verify output image exists
- **Module integration**: Write multi-file test → execute main file → verify texture renders
- **Skia backend**: Build and run `pml-builtins-smoke.exe` → verify 0 failures

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Foundation — independent, all parallel):
├── Task 1: poly2tri integration [quick]
├── Task 2: ParamKey expansion [quick]
├── Task 3: Box::Kind::Texture value type [quick]
├── Task 4: CG solver for Harmonic UV [unspecified-high]
├── Task 5: Texture cache [quick]
└── Task 6: CMakeLists for new texture library [quick]

Wave 2 (Core algorithms — depend on Wave 1):
├── Task 7: Triangulation module (poly2tri wrapper) [quick]
├── Task 8: UV projection: Planar [quick]
├── Task 9: UV projection: Harmonic [deep]
├── Task 10: UV projection: Explicit [quick]
└── Task 11: Texture bake (GraphicObject → offscreen SkSurface → SkImage) [unspecified-high]

Wave 3 (PML builtins + Skia backend — depend on Wave 2):
├── Task 12: define-texture builtin [deep]
├── Task 13: texture-map builtin [deep]
├── Task 14: Multi-texture blend (SkSL composition) [unspecified-high]
├── Task 15: Skia backend textured draw [deep]
└── Task 16: draw_object() integration for textured path [unspecified-high]

Wave 4 (Integration + tests — depend on Wave 3):
├── Task 17: Module system integration (provide/import textures) [quick]
├── Task 18: api.cpp registration [quick]
├── Task 19: Smoke tests [quick]
├── Task 20: Example .pml files + visual QA [writing]
└── Task F1-F4: Final verification [parallel review]

Critical Path: Task 6 → Task 7 → Task 11 → Task 13 → Task 15 → Task 19 → F1-F4
Parallel Speedup: ~65% faster than sequential
Max Concurrent: 6 (Waves 1 & 2)
```

### Dependency Matrix
- **1-6**: - - 7-11
- **7**: 1, 2 - 11, 13, 15
- **8**: 2, 4 - 9, 10, 13
- **9**: 4, 7, 8 - 13, 15
- **10**: 2, 8 - 13, 15
- **11**: 3, 5 - 12, 13, 15
- **12**: 3, 11 - 17, 18
- **13**: 7, 9, 10, 11 - 14, 15, 17
- **14**: 11, 13 - 16, 17, 19
- **15**: 7, 9, 11 - 16, 19
- **16**: 2, 15 - 17, 19
- **17**: 12, 13, 16 - 18, 19
- **18**: 12, 13, 16, 17 - 19
- **19**: 14, 15, 16, 17, 18 - 20
- **20**: 19 - F1-F4

### Agent Dispatch Summary
- **Wave 1** (6 tasks): T1-T3,T5,T6 → `quick`, T4 → `unspecified-high`
- **Wave 2** (5 tasks): T7,T8,T10 → `quick`, T9 → `deep`, T11 → `unspecified-high`
- **Wave 3** (5 tasks): T12,T13,T15 → `deep`, T14,T16 → `unspecified-high`
- **Wave 4** (5 tasks): T17,T18,T19 → `quick`, T20 → `writing`
- **FINAL** (4 tasks): F1 → `oracle`, F2 → `unspecified-high`, F3 → `unspecified-high`, F4 → `deep`

---

## TODOs

- [x] 1. **Integrate poly2tri via CMake FetchContent**

  **What to do**:
  - Add poly2tri to the project using the existing `pml_third_party()` pattern in the top-level `CMakeLists.txt`
  - poly2tri is a header-only library; no compilation needed beyond adding include path
  - Verify the include works by adding a quick `#include <poly2tri.h>` test to the build
  - Target file: `CMakeLists.txt` (top-level)
  - Repository: `https://github.com/greenm01/poly2tri` (BSD 3-Clause)
  - Alternative if poly2tri fails: prepare `mapbox-ear-cut` (ISC) as fallback

  **Must NOT do**:
  - Do NOT use vcpkg or manual clone scripts
  - Do NOT add poly2tri as a compiled target (header-only only)

  **Recommended Agent Profile**:
  > **Category**: `quick`
  > Reason: CMake integration is procedural; no algorithmic complexity.
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 2,3,4,5,6)
  - **Blocks**: Tasks 7, 11, 13, 15
  - **Blocked By**: None

  **References**:
  - `CMakeLists.txt:top` — existing `pml_third_party(giflib ...)` pattern to copy
  - `CMakeLists.txt:add_library(pml_core)` — see how header-only deps are handled
  - poly2tri docs: `https://github.com/greenm01/poly2tri` — API reference

  **Acceptance Criteria**:
  - [ ] CMake configure succeeds after adding poly2tri
  - [ ] `#include <poly2tri.h>` compiles in a test file
  - [ ] `pml-builtins-smoke.exe` still passes
  - [ ] poly2tri directory exists at `third_party/poly2tri/`

  **QA Scenarios**:
  ```
  Scenario: poly2tri header inclusion
    Tool: Bash
    Preconditions: CMake configure completed
    Steps:
      1. cmake --build --preset debug --target pml_core 2>&1
    Expected Result: Build succeeds, no include errors
    Evidence: .omo/evidence/task-1-poly2tri-include.log

  Scenario: poly2tri smoke test (triangulate a simple polygon)
    Tool: Bash
    Preconditions: Include verified
    Steps:
      1. Build the pml-builtins-smoke target
      2. Run .\build\debug\tests\Debug\pml-builtins-smoke.exe
    Expected Result: All tests pass (no regression)
    Evidence: .omo/evidence/task-1-smoke-pass.log
  ```

  **Evidence to Capture**:
  - [ ] Build log with successful include
  - [ ] Smoke test output (exit 0)

  **Commit**: YES
  - Message: `build(third_party): integrate poly2tri for shape triangulation`
  - Files: `CMakeLists.txt`

- [x] 2. **Expand ParamKey enum with UV-related parameters**

  **What to do**:
  - Open `src/pml/graphics/params.h` and add these new ParamKey entries:
    - `uv` (GraphicObject → owns a TextureBox)
    - `uv_mode` (planar = 0, harmonic = 1, explicit = 2)
    - `wrap_x` (clamp = 0, repeat = 1, mirror = 2)
    - `wrap_y` (clamp = 0, repeat = 1, mirror = 2)
    - `filter` (linear = 0, nearest = 1, anisotropic = 2)
    - `uv_vertices` (vector of 2D positions for explicit UV)
    - `edge_blend` (edge blend radius in pixels, default 0 = off)
  - Verify the `static_assert(count <= 32)` still holds (24 used + 7 new = 31 ≤ 32 ✓)
  - Update any switch/case that enumerates all ParamKey values if needed
  - Update `param_key_to_string()` or equivalent debug function

  **Must NOT do**:
  - Do NOT add ParamKey entries that are not in the Must Have list
  - Do NOT modify the enum's underlying type (int32_t)

  **Recommended Agent Profile**:
  > **Category**: `quick`
  > Reason: Enum extension is purely mechanical
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1,3,4,5,6)
  - **Blocks**: Tasks 7, 8, 9, 10, 13, 16
  - **Blocked By**: None

  **References**:
  - `src/pml/graphics/params.h` — the file to edit
  - `src/pml/core/types.h:ParamKey` — current enum with 24 members
  - `src/pml/graphics/objects.h` — GraphicObject struct using ParamKey

  **Acceptance Criteria**:
  - [ ] ParamKey has 31 entries (was 24, added 7)
  - [ ] `static_assert(count <= 32)` compiles
  - [ ] Project builds without warnings

  **QA Scenarios**:
  ```
  Scenario: Compile-time static_assert
    Tool: Bash
    Preconditions: Edit applied
    Steps:
      1. cmake --build --preset debug --target pml_graphics 2>&1
    Expected Result: No static_assert failures
    Evidence: .omo/evidence/task-2-params-build.log
  ```

  **Evidence to Capture**:
  - [ ] Build log showing successful compilation

  **Commit**: YES (with Task 3)
  - Message: `feat(graphics): add ParamKey entries for UV texture mapping`
  - Files: `src/pml/graphics/params.h`

- [x] 3. **Add Box::Kind::Texture value type**

  **What to do**:
  - In `src/pml/core/types.h`: Add `Texture` to the `Box::Kind` enum (as #25)
  - Create `src/pml/core/texture.h` — declare the `TextureBox` class:
    ```cpp
    struct TextureBox {
      size_t stable_id;        // for cache key
      mutable size_t cache_key_hint; // for LRU cache
      std::shared_ptr<GraphicObject> go; // the procedural content
      // Optional parameters captured at define-time
      int width;               // baked texture width
      int height;              // baked texture height
      WrapMode wrap_x, wrap_y; // default clamp
      FilterMode filter;       // default linear
      TextureBox(std::shared_ptr<GraphicObject> g, int w, int h);
      TextureBox duplicate() const;
    };
    ```
  - Extend `Box` struct with `std::shared_ptr<TextureBox> texture` member
  - Add `Value(std::shared_ptr<TextureBox>)` constructor if not generic enough
  - Update `value_to_string()` to handle `Box::Kind::Texture`
  - Update any exhaustiveness checks on Box::Kind that would fail

  **Must NOT do**:
  - Do NOT add methods that require Skia headers in core/

  **Recommended Agent Profile**:
  > **Category**: `quick`
  > Reason: Pure data type extension; straightforward additions
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1,2,4,5,6)
  - **Blocks**: Tasks 11, 12, 13
  - **Blocked By**: None

  **References**:
  - `src/pml/core/types.h:Box` — existing Box struct with `std::shared_ptr<GraphicObject>` member pattern
  - `src/pml/core/types.h:Box::Kind` — enum to extend
  - `src/pml/core/types.h:value_to_string()` — switch to update
  - `src/pml/graphics/objects.h` — GraphicObject definition (for the `go` member)

  **Acceptance Criteria**:
  - [ ] `Box::Kind::Texture` compiles as a valid enum value
  - [ ] `Value v = Value(std::make_shared<TextureBox>(go, 512, 512));` compiles
  - [ ] `value_to_string(v)` returns a reasonable representation (not "unknown")
  - [ ] Project builds without warnings

  **QA Scenarios**:
  ```
  Scenario: Texture value construction
    Tool: Bash
    Preconditions: Edit applied
    Steps:
      1. cmake --build --preset debug --target pml_core 2>&1
    Expected Result: Build succeeds
    Evidence: .omo/evidence/task-3-texture-type-build.log
  ```

  **Evidence to Capture**:
  - [ ] Build log

  **Commit**: YES (with Task 2)
  - Message: `feat(core): add Box::Kind::Texture value type`
  - Files: `src/pml/core/types.h`, `src/pml/core/texture.h`

- [x] 4. **Implement Conjugate Gradient solver for Harmonic UV**

  **What to do**:
  - Create `src/pml/graphics/cg_solver.h` (header only)
  - Implement a sparse Conjugate Gradient solver for symmetric positive definite systems:
    ```cpp
    namespace pml {
    // Sparse matrix in CSR format
    struct SparseMatrix {
      std::vector<double> values;
      std::vector<int> col_indices;
      std::vector<int> row_ptr;   // size = n+1, row_ptr[n] = nnz
      int n;                       // matrix dimension
    };

    struct CGSolver {
      // Solve Ax = b with optional initial guess x0
      // Returns {x, converged, iterations}
      struct Result {
        std::vector<double> x;
        bool converged;
        int iterations;
      };
      Result solve(const SparseMatrix& A, const std::vector<double>& b,
                   const std::vector<double>* x0 = nullptr,
                   double tolerance = 1e-10, int max_iterations = 1000);
    };
    }
    ```
  - Implement: residual, direction update, alpha/beta computation, convergence check
  - Pure math, no dependencies beyond `<vector>`, `<cmath>`, `<algorithm>`
  - The matrix for Harmonic UV is a Poisson system: same CG works for both x and y coordinates (same Laplacian, different boundary conditions)
  - Keep it under 200 lines — this is a standard CG, not a multi-grid solver

  **Must NOT do**:
  - Do NOT add Eigen dependency
  - Do NOT implement preconditioner (texture UV at ~500 vertices doesn't need it)
  - Do NOT add GPU acceleration

  **Recommended Agent Profile**:
  > **Category**: `unspecified-high`
  > Reason: Numerical algorithm requiring mathematical correctness; moderate complexity
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1,2,3,5,6)
  - **Blocks**: Tasks 9 (Harmonic UV solver depends on this)
  - **Blocked By**: None

  **References**:
  - Shewchuk, "An Introduction to the Conjugate Gradient Method Without the Agonizing Pain" — definitive educational reference
  - `src/pml/graphics3d/math3d.h` — existing math utilities (Vec3, Mat4) for style reference
  - CG for Laplacian systems: typical convergence in O(N^1/2) iterations for 2D Poisson

  **Acceptance Criteria**:
  - [ ] CG solver compiles as header-only
  - [ ] Solves a 5-point Laplacian system (2D grid) to tolerance
  - [ ] Convergence in ≤1000 iterations for 500x500 system
  - [ ] Explicit test: `CG::solve(A, b) == expected_x` for known system

  **QA Scenarios**:
  ```
  Scenario: CG solver correctness
    Tool: Bash
    Preconditions: Header created, include test in a .cpp
    Steps:
      1. Build --target pml_graphics
      2. (Quick unit test via smoke tester would call the solver)
    Expected Result: Build succeeds
    Evidence: .omo/evidence/task-4-cg-build.log

  Scenario: Known system solve
    Tool: Bash
    Preconditions: A small test program
    Steps:
      1. Compile and run a tiny CG test
      2. Solve 3x3 diagonal system
    Expected Result: x = [1, 1, 1] for A = diag(2, 3, 4), b = [2, 3, 4]
    Evidence: .omo/evidence/task-4-cg-test.log
  ```

  **Evidence to Capture**:
  - [ ] Build log
  - [ ] Correctness test log

  **Commit**: YES
  - Message: `feat(graphics): add sparse CG solver for Harmonic UV projection`
  - Files: `src/pml/graphics/cg_solver.h`

- [x] 5. **Implement Texture Cache (LRU by stable ID)**

  **What to do**:
  - In `src/pml/core/texture.h/cpp` (or create `texture_cache.h/cpp`):
    ```cpp
    struct TextureCacheEntry {
      std::shared_ptr<SkImage> baked_image;
      std::chrono::steady_clock::time_point last_access;
    };

    class TextureCache {
    public:
      static TextureCache& instance();
      std::shared_ptr<SkImage> lookup(size_t stable_id) const;
      void insert(size_t stable_id, std::shared_ptr<SkImage> image);
      void evict(size_t stable_id);
      void clear();
      void set_max_size(size_t max_bytes);

    private:
      std::unordered_map<size_t, TextureCacheEntry> m_cache;
      std::vector<size_t> m_lru_order;
      size_t m_max_bytes = 64 * 1024 * 1024; // 64 MB default
      // Track approximate byte usage
      size_t m_current_bytes = 0;
    };
    ```
  - LRU eviction on insert when over budget (evict oldest entries)
  - Thread-safe is not needed (PML is single-threaded)
  - Expose `reset()` for tests (matching `g_timeline`, `StyleRegistry::instance()` etc.)
  - Include this in `core/` because it's used by both backend and frontend

  **Must NOT do**:
  - Do NOT add Skia headers to the cache header — forward declare SkImage
  - Do NOT implement disk caching or persistence

  **Recommended Agent Profile**:
  > **Category**: `quick`
  > Reason: Standard data structure implementation
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1,2,3,4,6)
  - **Blocks**: Tasks 11, 13, 15
  - **Blocked By**: Task 3 (TextureBox with stable_id)

  **References**:
  - `src/pml/animation/timeline.cpp` — singleton pattern with `reset()` for tests
  - `src/pml/sprites/style.h:StyleRegistry::instance()` — singleton pattern to follow
  - `src/pml/backend/skia/skia_backend_draw.cpp` — code that will use the cache

  **Acceptance Criteria**:
  - [ ] TextureCache singleton compiles
  - [ ] insert/lookup works (cached image returned)
  - [ ] eviction works (insert beyond max → oldest evicted)
  - [ ] reset() clears all entries
  - [ ] Project builds

  **QA Scenarios**:
  ```
  Scenario: TextureCache basic CRUD
    Tool: Bash
    Preconditions: Build with a quick unit test
    Steps:
      1. Build --target pml_core
    Expected Result: Build succeeds
    Evidence: .omo/evidence/task-5-cache-build.log
  ```

  **Evidence to Capture**:
  - [ ] Build log

  **Commit**: YES (with Task 3)
  - Message: `feat(core): add TextureCache with LRU eviction`
  - Files: `src/pml/core/texture_cache.h`, `src/pml/core/texture_cache.cpp`

- [x] 6. **Update CMakeLists for new texture library and directories**

  **What to do**:
  - Create new CMake target or extend existing `pml_graphics` with:
    - `src/pml/core/texture.h`, `src/pml/core/texture_cache.h/cpp`
    - `src/pml/graphics/cg_solver.h`
    - `src/pml/graphics/triangulation.h/cpp`
    - `src/pml/graphics/uv_solver.h/cpp`
  - In `src/pml/graphics/CMakeLists.txt`: add new source files
  - In `src/pml/core/CMakeLists.txt`: add texture.h, texture_cache.h/cpp
  - In `src/pml/evaluator/CMakeLists.txt`: add texture_builtins.h/cpp, texture_map_builtins.h/cpp
  - In `src/pml/backend/CMakeLists.txt` or `src/pml/backend/skia/CMakeLists.txt`: add textured_draw.h/cpp
  - Ensure include directories for poly2tri are set (via the earlier FetchContent)
  - Ensure no ODR violations from header-only modules

  **Must NOT do**:
  - Do NOT create separate executable targets for each module
  - Do NOT add `vcpkg.json` or any vcpkg dependency

  **Recommended Agent Profile**:
  > **Category**: `quick`
  > Reason: CMake file management is mechanical
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1,2,3,4,5)
  - **Blocks**: ALL tasks in Wave 2, 3, 4
  - **Blocked By**: Task 1 (poly2tri include path)

  **References**:
  - `src/pml/graphics/CMakeLists.txt` — existing file to extend
  - `src/pml/core/CMakeLists.txt` — for texture module
  - `src/pml/evaluator/CMakeLists.txt` — for builtin registration
  - `src/pml/backend/skia/CMakeLists.txt` — for textured draw

  **Acceptance Criteria**:
  - [ ] CMake configure succeeds
  - [ ] All new source files included in build
  - [ ] No "unresolved external symbol" or missing include errors
  - [ ] Build succeeds end-to-end

  **QA Scenarios**:
  ```
  Scenario: Full build after CMake changes
    Tool: Bash
    Preconditions: All Wave 1 source files created
    Steps:
      1. cmake --preset debug
      2. cmake --build --preset debug 2>&1
    Expected Result: Build succeeds (warnings allowed but no errors)
    Evidence: .omo/evidence/task-6-full-build.log
  ```

  **Evidence to Capture**:
  - [ ] Configure log
  - [ ] Build log

  **Commit**: YES (with Tasks 1-5)
  - Message: `build: integrate texture mapping source files into CMake build`
  - Files: `src/pml/core/CMakeLists.txt`, `src/pml/graphics/CMakeLists.txt`, `src/pml/evaluator/CMakeLists.txt`, `src/pml/backend/skia/CMakeLists.txt`

---

## Wave 2 — Core Algorithms (depends on Wave 1)

- [ ] 7. **Implement Triangulation module (poly2tri wrapper)**

  **What to do**:
  - Create `src/pml/graphics/triangulation.h`:
    ```cpp
    namespace pml {
    struct TriangulatedMesh {
      std::vector<Vec2> vertices;      // [x, y]
      std::vector<uint32_t> indices;   // triangle indices (3 per triangle)
      // Optional: map from mesh vertex index back to shape contour vertex
      std::vector<int> contour_map;    // -1 for Steiner points
    };

    // Triangulate a polygon contour (outer boundary)
    // Optionally include hole contours
    // Returns triangulated mesh with CCW winding
    Result<TriangulatedMesh> triangulate_polygon(
      const std::vector<Vec2>& outer_contour,
      const std::vector<std::vector<Vec2>>& holes = {}
    );

    // Triangulate from a GraphicObject shape (auto-detect shape type)
    Result<TriangulatedMesh> triangulate_shape(
      const GraphicObject& obj
    );
    }
    ```
  - `triangulate_polygon()` wraps poly2tri:
    1. Convert outer_contour to `std::vector<p2t::Point*>`
    2. Add hole contours
    3. Call `p2t::CDT` → `Triangulate()` → `GetTriangles()`
    4. Extract vertices and triangle indices
    5. Handle Steiner points (poly2tri inserts them; map with -1 in contour_map)
  - `triangulate_shape()` dispatches based on shape type:
    - Polygon/Path: extract polyline → merge segments
    - Rect/Circle/Ellipse: generate contour points first
    - Rect/Line: return a simple 2-triangle mesh
  - Must handle degenerate cases: empty shape, single point, colinear points
  - Target: ~100-120 lines

  **Must NOT do**:
  - Do NOT implement own Delaunay triangulation — use poly2tri
  - Do NOT handle 3D vertices (2D only)
  - Do NOT modify shape data during triangulation (read-only)

  **Recommended Agent Profile**:
  > **Category**: `quick`
  > Reason: Straightforward library wrapper
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 2 (sequential with Tasks 8,9,10,11)
  - **Blocks**: Tasks 11, 13, 15
  - **Blocked By**: Task 1 (poly2tri include), Task 2 (ParamKey for shape extraction)

  **References**:
  - poly2tri API: `https://github.com/greenm01/poly2tri` — `CDT::Triangulate()`, `GetTriangles()`
  - `src/pml/graphics/objects.h` — GraphicObject types (polygon, path, rect, etc.)
  - `src/pml/graphics3d/mesh3d.h:Mesh3D` — existing mesh structure for reference on indices/vertices layout
  - `src/pml/graphics/transform.h:AffineTransform` — Vec2 type used for 2D points

  **Acceptance Criteria**:
  - [ ] `triangulate_polygon()` correctly triangulates a convex quad → 2 triangles (4→6 indices)
  - [ ] `triangulate_polygon()` correctly triangulates a concave polygon (e.g., L-shape)
  - [ ] `triangulate_polygon()` correctly handles a polygon with holes
  - [ ] `triangulate_shape()` handles polygon, path, rect, ellipse shapes
  - [ ] Degenerate case (empty/colinear) returns error (not crash)
  - [ ] All indices form valid triangles (non-zero area, CCW winding)
  - [ ] No memory leaks from poly2tri's `p2t::Point*` allocations

  **QA Scenarios**:
  ```
  Scenario: Triangulate a convex quadrilateral
    Tool: Bash
    Preconditions: Triangulation code compiled
    Steps:
      1. Call triangulate_polygon({(0,0), (100,0), (100,100), (0,100)})
    Expected Result: 2 triangles → 6 indices, no Steiner points
    Evidence: .omo/evidence/task-7-quad.log

  Scenario: Triangulate an L-shaped polygon
    Tool: Bash
    Preconditions: Triangulation code compiled
    Steps:
      1. Call triangulate_polygon with L-shape vertices
    Expected Result: Valid triangulation, all triangles inside contour
    Evidence: .omo/evidence/task-7-lshape.log

  Scenario: Triangulate a circle (30-point approximation)
    Tool: Bash
    Preconditions: Triangulation code compiled
    Steps:
      1. Generate 30 points on a circle → call triangulate
    Expected Result: Valid ~28 triangles (using Steiner points from chord ears)
    Evidence: .omo/evidence/task-7-circle.log
  ```

  **Evidence to Capture**:
  - [ ] Test output for all three scenarios
  - [ ] Each test shows vertex count, triangle count, validation

  **Commit**: YES
  - Message: `feat(graphics): add poly2tri-based triangulation module`
  - Files: `src/pml/graphics/triangulation.h`, `src/pml/graphics/triangulation.cpp`

- [ ] 8. **Implement Planar UV projection**

  **What to do**:
  - Create `src/pml/graphics/uv_solver.h` (shared header for all UV methods):
    ```cpp
    namespace pml {
    struct UVResult {
      std::vector<Vec2> uvs;  // UV coordinates per vertex, one-to-one with mesh vertices
      bool valid;
    };

    // Planar UV: simple axis-aligned projection
    // Maps shape's AABB to [0,1]²
    UVResult solve_planar_uv(
      const std::vector<Vec2>& mesh_vertices
    );

    // Harmonic UV: solves Laplace equation with cotangent weights
    // Boundary vertices fixed to polygon arc-length parameterization
    UVResult solve_harmonic_uv(
      const std::vector<Vec2>& mesh_vertices,
      const std::vector<uint32_t>& indices,
      const std::vector<int>& boundary_vertices
    );

    // Explicit UV: user-provided per-vertex UVs
    // Expects same count as mesh_vertices
    UVResult apply_explicit_uv(
      const std::vector<Vec2>& user_uvs,
      const std::vector<Vec2>& mesh_vertices
    );
    }
    ```
  - `solve_planar_uv()`:
    1. Compute bounding box of mesh_vertices (min/max x, y)
    2. For each vertex: `uv = ((x - min_x) / (max_x - min_x), (y - min_y) / (max_y - min_y))`
    3. Handle degenerate case (all same point → uvs = (0,0))

  **Must NOT do**:
  - Do NOT add LSCM or other complex projections
  - Do NOT modify vertices (read-only)

  **Recommended Agent Profile**:
  > **Category**: `quick`
  > Reason: Simple AABB → [0,1]² mapping
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 2 (sequential with 7,9,10,11)
  - **Blocks**: Task 9, 10, 13
  - **Blocked By**: Task 2 (ParamKey::uv_mode)

  **References**:
  - `src/pml/graphics3d/math3d.h` — Vec2 definition
  - Real-Time Rendering, 4th ed., §11.5 — planar projection basics

  **Acceptance Criteria**:
  - [ ] `solve_planar_uv({(0,0), (100,0), (100,100), (0,100)})` → `{(0,0), (1,0), (1,1), (0,1)}`
  - [ ] Handles off-origin polygons correctly (AABB-based, not assumption of (0,0) origin)
  - [ ] Handles empty input gracefully

  **QA Scenarios**:
  ```
  Scenario: Planar UV for a unit square
    Tool: Bash (via test program)
    Preconditions: uv_solver compiled
    Steps:
      1. Call solve_planar_uv on 4 corners of a square at origin
    Expected Result: UVs map from (0,0)-(1,1) exactly
    Evidence: .omo/evidence/task-8-planar-square.log

  Scenario: Planar UV for off-origin rect
    Tool: Bash
    Preconditions: uv_solver compiled
    Steps:
      1. Call solve_planar_uv on rect at (50,50)-(150,150)
    Expected Result: UVs also (0,0)-(1,1) (AABB normalized)
    Evidence: .omo/evidence/task-8-planar-offset.log
  ```

  **Evidence to Capture**:
  - [ ] Test output showing UV per vertex

  **Commit**: YES (with Task 9)
  - Message: `feat(graphics): add Planar and Harmonic UV projection solvers`
  - Files: `src/pml/graphics/uv_solver.h`, `src/pml/graphics/uv_solver.cpp`

- [ ] 9. **Implement Harmonic UV projection**

  **What to do**:
  - Add `solve_harmonic_uv()` to `src/pml/graphics/uv_solver.cpp`
  - Algorithm:
    1. **Identify boundary vertices**: vertices that lie on the polygon's boundary (not Steiner points). These are indexed by `contour_map != -1` from triangulation step.
    2. **Parameterize boundary to [0,1]²**: use chord-length arc-length parameterization:
       - Compute total boundary length
       - For each boundary vertex: t = distance_along_boundary / total_length
       - Map to circle: `(cos(2πt), sin(2πt))` for the boundary condition OR
       - Map to rect boundary: place boundary vertices on unit square perimeter proportional to arc length
    3. **Build Laplacian matrix** (cotangent weights):
       - For each interior vertex i, for each adjacent edge (i,j):
         - `w_ij = (cot α_ij + cot β_ij) / 2` where α_ij and β_ij are angles opposite edge (i,j) in the two incident triangles
       - Diagonal: `w_ii = -sum_{j != i} w_ij`
    4. **Solve** two linear systems using CG solver (Task 4):
       - `A * u = b_u` and `A * v = b_v`
       - Where boundary vertices' u,v are fixed (known from arc-length parameterization)
       - Move boundary terms to RHS for interior vertices
    5. **Handle degenerate triangles**: if cotangent weight is NaN (degenerate tri), fall back to uniform Laplacian weight of `w_ij = 1`
  - Expect ~80-100 lines of algorithm code
  - Must handle case where mesh has no interior vertices (all boundary) — just return boundary UVs

  **Must NOT do**:
  - Do NOT attempt LSCM (angle-preserving, more complex — deferred)
  - Do NOT use Eigen for sparse LU

  **Recommended Agent Profile**:
  > **Category**: `deep`
  > Reason: Numerical algorithm requiring careful implementation; cotangent weights, Laplacian construction, boundary conditions
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 2 (sequential with 7,8,10,11)
  - **Blocks**: Tasks 13, 15
  - **Blocked By**: Task 4 (CG solver), Task 7 (triangulation), Task 8 (boundary detection)

  **References**:
  - "Least Squares Conformal Maps for Automatic Texture Atlas Generation" (Lévy et al., 2002) — Harmonic basis is the same Laplacian without the conformal correction
  - Botsch et al., "Polygon Mesh Processing" §5.4 — cotangent Laplacian construction
  - Shewchuk's CG notes — solver integration
  - `src/pml/graphics3d/math3d.h` — Vec2 and math utilities
  - `src/pml/graphics/cg_solver.h` — the CG solver from Task 4

  **Acceptance Criteria**:
  - [ ] Harmonic UV of a convex polygon returns valid UVs (no overlap, in [0,1]² range, C0 continuous at boundary)
  - [ ] Boundary vertices map to the perimeter of [0,1]² unit square
  - [ ] Interior UVs are smooth (no sudden jumps between adjacent triangles)
  - [ ] Same mesh + different boundary fixing → different UVs
  - [ ] All-zero mesh (single point) returns (0,0) UVs
  - [ ] No NaN/inf in output UVs

  **QA Scenarios**:
  ```
  Scenario: Harmonic UV for convex pentagon
    Tool: Bash (via test program)
    Preconditions: Triangulated pentagon (5 boundary vertices, 0 interior)
    Steps:
      1. solve_harmonic_uv on 5-vertex mesh
    Expected Result: UVs on perimeter of [0,1]², no folding
    Evidence: .omo/evidence/task-9-harmonic-pentagon.log

  Scenario: Harmonic UV for shape with interior vertex
    Tool: Bash
    Preconditions: Triangulated hexagon with 1 interior Steiner point
    Steps:
      1. solve_harmonic_uv
    Expected Result: Interior UV is inside [0,1]², boundary on perimeter
    Evidence: .omo/evidence/task-9-harmonic-interior.log
  ```

  **Evidence to Capture**:
  - [ ] UV results (vertex → UV mapping)
  - [ ] Convergence info (CG iterations, residual)

  **Commit**: YES (with Task 8)
  - Message: (included in Task 8 commit)

- [ ] 10. **Implement Explicit UV projection**

  **What to do**:
  - Add `apply_explicit_uv()` to `src/pml/graphics/uv_solver.cpp`
  - Logic:
    1. If `user_uvs.size() == mesh_vertices.size()` → return user_uvs directly (1:1 mapping)
    2. If `user_uvs.size() < mesh_vertices.size()`:
       - The difference is Steiner points from triangulation
       - For each Steiner point vertex, interpolate UV from its barycentric coordinates in the original polygon
       - Or: compute UV as Planar fallback for non-user vertices
    3. Validate UV range: clamp to [0, 1] if `wrap == clamp`, otherwise allow `[-inf,inf]`
  - Error if user_uvs.size() > mesh_vertices.size()
  - This is the manual override path for expert users who provide their own UV layout

  **Must NOT do**:
  - Do NOT infer missing UVs via interpolation (only accept user-provided + Steiner fallback)

  **Recommended Agent Profile**:
  > **Category**: `quick`
  > Reason: Straightforward validation + passthrough
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 2 (sequential with 7,8,9,11)
  - **Blocks**: Tasks 13, 15
  - **Blocked By**: Task 2 (ParamKey::uv_vertices), Task 8 (UV utilities)

  **References**:
  - ParamKey::uv_vertices from Task 2 — parameter format for explicit UVs

  **Acceptance Criteria**:
  - [ ] 1:1 mapping returns exactly the input
  - [ ] Extra vertices (Steiner) get Planar-fallback UVs
  - [ ] Error on user_uvs > mesh_vertices (invalid)
  - [ ] Clamps values when wrap=clamp

  **QA Scenarios**:
  ```
  Scenario: Exact count
    Tool: Bash
    Steps: apply_explicit_uv({(0,0),(0.5,0.5),(1,1)}, 3 verts)
    Result: Returns exactly the 3 UVs
    Evidence: .omo/evidence/task-10-exact.log

  Scenario: Extra Steiner vertex
    Tool: Bash
    Steps: 3 user UVs, 4 mesh vertices
    Result: 3 return match, 4th is Planar fallback
    Evidence: .omo/evidence/task-10-steiner.log
  ```

  **Evidence to Capture**:
  - [ ] Test output

  **Commit**: YES (with Task 8)
  - Message: (included)

- [ ] 11. **Implement Texture Bake (GraphicObject → offscreen SkSurface → SkImage)**

  **What to do**:
  - Create `src/pml/graphics/texture_bake.h/cpp`:
    ```cpp
    namespace pml {
    // Bake a TextureBox into an SkImage
    // Uses the existing render-to-offscren-SkSurface pattern
    std::shared_ptr<SkImage> bake_texture(
      const std::shared_ptr<TextureBox>& texture,
      int width = 512, int height = 512
    );

    // Bake a GraphicObject directly into an SkImage
    std::shared_ptr<SkImage> bake_graphic_object(
      const GraphicObject& go,
      int width = 512, int height = 512
    );
    }
    ```
  - Implementation:
    1. Create offscreen `SkSurface` of requested size (or texture's default)
    2. Get existing SkCanvas from SkSurface
    3. Set up rendering: clear to transparent, apply any transforms
    4. Call the existing `draw_object()` pipeline to render the GraphicObject into the offscreen surface
    5. Extract SkImage via `surface->makeImageSnapshot()`
    6. Optionally generate mipmaps via Skia's `SkiaBackend::with_mipmaps()` if filter=anisotropic
    7. Insert into TextureCache (Task 5)
  - This reuses the _exact_ existing rendering pipeline — no SkPicture needed
  - Cache lookup: if `texture->stable_id` is in TextureCache, return cached SkImage
  - Cache miss: bake, insert, return
  - IMPORTANT: This file lives in `src/pml/graphics/` even though it uses Skia types (the Skia headers are already available via `pml_graphics` target's transitive includes)

  **Must NOT do**:
  - Do NOT duplicate the rendering pipeline — delegate to existing draw_object()
  - Do NOT call this from non-Skia paths (guarded by `#ifdef PML_BUILD_SKIA`)

  **Recommended Agent Profile**:
  > **Category**: `unspecified-high`
  > Reason: Requires understanding both Skia surface creation and PML rendering pipeline
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 2 (sequential with 7,8,9,10)
  - **Blocks**: Tasks 12, 13, 15
  - **Blocked By**: Task 3 (TextureBox), Task 5 (TextureCache), Task 6 (CMakeLists)

  **References**:
  - `src/pml/evaluator/multi_texture_builtins.cpp` — the existing `bind_textures()` function that does the exact offscreen SkSurface → SkImage pipeline. Copy this pattern.
  - `src/pml/backend/skia/skia_backend_draw.cpp:draw_object()` — the rendering pipeline to call
  - `src/pml/backend/skia/skia_backend_draw.cpp:draw_mesh3d_impl()` — shows how to render a GraphicObject as material then use it as shader
  - `SkSurface::makeImageSnapshot()` — Skia API

  **Acceptance Criteria**:
  - [ ] `bake_texture()` returns a non-null `SkImage` for a valid TextureBox
  - [ ] Baked image has the correct dimensions
  - [ ] Cache hit returns the same SkImage pointer (identity test)
  - [ ] Cache eviction triggers rebake on next access
  - [ ] Works with `#ifdef PML_BUILD_SKIA` guard
  - [ ] Build under non-Skia configs does not link Skia-specific code

  **QA Scenarios**:
  ```
  Scenario: Bake a simple filled rect texture
    Tool: Bash + build
    Preconditions: TextureBox with a red rect GraphicObject
    Steps:
      1. Call bake_texture(tex, 64, 64)
    Expected Result: Returns valid SkImage, 64x64, non-transparent pixels
    Evidence: .omo/evidence/task-11-bake.log

  Scenario: Cache hit
    Tool: Bash + build
    Preconditions: Same texture baked once
    Steps:
      1. bake_texture(tex) → img1
      2. bake_texture(tex) → img2
    Expected Result: img1 == img2 (same shared_ptr)
    Evidence: .omo/evidence/task-11-cache.log
  ```

  **Evidence to Capture**:
  - [ ] Build log
  - [ ] Cache identity test output

  **Commit**: YES
  - Message: `feat(graphics): add texture bake pipeline (GO → SkSurface → SkImage)`
  - Files: `src/pml/graphics/texture_bake.h`, `src/pml/graphics/texture_bake.cpp`

---

## Wave 3 — PML Builtins + Skia Backend (depends on Wave 2)

- [ ] 12. **Implement define-texture builtin**

  **What to do**:
  - Create `src/pml/evaluator/texture_builtins.h/cpp`:
    ```cpp
    void register_texture_builtins(std::shared_ptr<Environment> env);
    ```
  - Implement `define-texture` special form or builtin:
    ```scheme
    ;; Syntax option A (lambda-style, recommended):
    (define-texture name (width height)
      body ...)
    
    ;; Evaluate body in a temp canvas, capture result as texture
    ```
  - Registration:
    1. Parse arguments: name (symbol), params (width, height), body expressions
    2. Create an environment for the texture body evaluation
    3. Evaluate body, capturing the GraphicObject result
    4. Store as `Value(std::make_shared<TextureBox>(go, width, height))`
    5. Bind name in the calling environment
  - Additional builtins:
    - `(texture? val)` → `#t` if val is a Texture value
    - `(texture-width tex)` → width
    - `(texture-height tex)` → height
    - `(texture-id tex)` → stable_id (for debugging/cache inspection)
  - Follow the pattern of `define-palette` / `define-style` for syntactic structure
  - Follow `defmacro` or other define-* forms for how to capture and evaluate body expressions

  **Must NOT do**:
  - Do NOT execute the body eagerly at define-time (lazy, or evaluated once then cached? — evaluate once at define time, cache the GraphicObject)
  - Do NOT evaluate `body` in the global environment — use a child environment

  **Recommended Agent Profile**:
  > **Category**: `deep`
  > Reason: Builtin registration + PML evaluator integration; requires understanding special forms
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 3 (sequential with 13,14,15,16)
  - **Blocks**: Tasks 13, 17, 18
  - **Blocked By**: Task 3 (TextureBox), Task 6 (CMakeLists), Task 11 (texture bake)

  **References**:
  - `src/pml/evaluator/builtins.cpp` — main builtins registration file. Use `env->define(name, Value(std::make_shared<BuiltinProcedure>(...)))` pattern.
  - `src/pml/sprites/style.cpp:register_style()` — `define-palette` / `define-style` syntax: same keyword-argument approach.
  - `src/pml/evaluator/evaluator.cpp` — SpecialForm handling for `define-*` forms. Use `def_syntax` lambda.
  - `src/pml/core/types.h` — TextureBox definition from Task 3
  - `src/pml/graphics/texture_bake.h` — texture bake pipeline from Task 11

  **Acceptance Criteria**:
  - [ ] `(define-texture my-tex (64 64) (rect 0 0 64 64 :fill "#f00"))` evaluates without error
  - [ ] `my-tex` is bound and is a Texture value
  - [ ] `(texture? my-tex)` returns `#t`
  - [ ] `(texture-width my-tex)` returns `64`
  - [ ] `(texture-height my-tex)` returns `64`
  - [ ] Texture body with multiple shapes works (e.g., starry sky)
  - [ ] Error on invalid width/height (not positive integers)

  **QA Scenarios**:
  ```
  Scenario: Define and inspect a basic texture
    Tool: Bash (pml.exe REPL)
    Preconditions: Built executable with texture builtins
    Steps:
      1. echo '(define-texture tex (64 64) (rect 0 0 64 64 :fill "#f00")) (texture? tex) (texture-width tex)' | .\build\debug\bin\Debug\pml.exe
    Expected Result: #t, 64
    Evidence: .omo/evidence/task-12-basic-define.log

  Scenario: Texture with complex body
    Tool: Bash
    Preconditions: Same build
    Steps:
      1. Run a .pml file that defines a multi-shape texture (stars + moon)
    Expected Result: No errors, texture is defined
    Evidence: .omo/evidence/task-12-complex-define.log
  ```

  **Evidence to Capture**:
  - [ ] REPL output logs

  **Commit**: YES
  - Message: `feat(evaluator): add define-texture builtin and texture accessors`
  - Files: `src/pml/evaluator/texture_builtins.h`, `src/pml/evaluator/texture_builtins.cpp`

- [ ] 13. **Implement texture-map builtin**

  **What to do**:
  - Create `src/pml/evaluator/texture_map_builtins.h/cpp`:
    ```cpp
    void register_texture_map_builtins(std::shared_ptr<Environment> env);
    ```
  - Implement `texture-map` builtin (as a function, not special form):
    ```scheme
    ;; Map texture onto any shape:
    (texture-map <shape> :source <texture> [:mode :planar|:harmonic|:explicit]
                 [:wrap-x :clamp|:repeat|:mirror] [:wrap-y :clamp|:repeat|:mirror]
                 [:filter :linear|:nearest|:anisotropic]
                 [:uv-vertices <list-of-points>])
                 
    ;; Or shorthand via shape's :fill parameter:
    (polygon ... :fill (define-texture tex (64 64) ...))
    ```
  - Implementation flow:
    1. First argument is the shape (GraphicObject)
    2. Parse keyword arguments:
       - `:source` — a Texture value (required)
       - `:mode` — `:planar`, `:harmonic`, `:explicit` (default: `:harmonic`)
       - `:wrap-x`, `:wrap-y` — wrap modes (default: `:clamp`)
       - `:filter` — filter mode (default: `:linear`)
       - `:uv-vertices` — list of `(x y)` pairs for explicit mode
       - `:edge-blend` — edge blend radius in pixels (default: 0)
    3. Store ALL params onto the GraphicObject's param map under the ParamKey entries from Task 2
    4. Return the modified GraphicObject
  - The `:fill` shorthand path: in `make_rect()`, `make_polygon()`, etc., check if `:fill` argument is a Texture value. If so, set UV params and call `texture-map` logic internally.
  - Reuse `parse_kwargs` / `kw_string` / `kw_int` from `src/pml/graphics/render.cpp` (as documented in AGENTS.md §4)
  - The actual rendering is deferred to Skia backend (Task 15)

  **Must NOT do**:
  - Do NOT render during `texture-map` call — just set parameters for deferred rendering
  - Do NOT modify the shape's core GraphicObject type (it stays as e.g., `ShapeType::Polygon`)
  - Do NOT add UV animation support

  **Recommended Agent Profile**:
  > **Category**: `deep`
  > Reason: Builtin with keyword argument parsing, GraphicObject parameter manipulation
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 3 (sequential with 12,14,15,16)
  - **Blocks**: Tasks 14, 15, 17
  - **Blocked By**: Tasks 7 (triangulation), 9 (Harmonic UV), 10 (Explicit UV), 11 (texture bake)

  **References**:
  - `src/pml/evaluator/builtins.cpp` — `BuiltinProcedure` registration, `accepts_kwargs=true` pattern
  - `src/pml/graphics/render.cpp` — `parse_kwargs`, `kw_string`, `kw_int` function definitions
  - `src/pml/graphics/objects.h:GraphicObject` — `set_param()` / `params()` methods
  - `src/pml/graphics/params.h` — ParamKey entries from Task 2
  - `src/pml/sprites/components/` — example of keyword-based builtins

  **Acceptance Criteria**:
  - [ ] `(texture-map (polygon ...) :source tex)` returns a GraphicObject with UV params set
  - [ ] `(polygon ... :fill tex)` returns a GraphicObject with UV params set (shorthand works)
  - [ ] `:mode :harmonic` sets uv_mode to harmonic on the object
  - [ ] `:wrap-x :repeat` sets wrap_x to repeat
  - [ ] `:uv-vertices ((0 0) (1 0) (1 1) (0 1))` stores explicit UVs
  - [ ] Missing `:source` raises an error
  - [ ] Invalid `:mode` value raises an error

  **QA Scenarios**:
  ```
  Scenario: Basic texture-map call
    Tool: Bash (pml.exe)
    Preconditions: Build with texture_map builtins
    Steps:
      1. Run: '(define-texture tex (64 64) (rect 0 0 64 64 :fill "#f00")) (texture-map (polygon ((0 0) (100 0) (100 100) (0 100))) :source tex :mode :harmonic)'
    Expected Result: Returns a GraphicObject (no error), params show uv_mode=harmonic
    Evidence: .omo/evidence/task-13-basic-map.log

  Scenario: Shorthand via :fill
    Tool: Bash
    Preconditions: Same build
    Steps:
      1. Run: '(define-texture tex (64 64) (rect 0 0 64 64 :fill "#f00")) (polygon ((0 0) (100 0) (100 100) (0 100)) :fill tex)'
    Expected Result: Returns GraphicObject with texture params
    Evidence: .omo/evidence/task-13-shorthand.log

  Scenario: Missing source error
    Tool: Bash
    Preconditions: Same build
    Steps:
      1. Run: '(texture-map (rect 0 0 100 100))'
    Expected Result: Error (missing :source)
    Evidence: .omo/evidence/task-13-error.log
  ```

  **Evidence to Capture**:
  - [ ] REPL output logs

  **Commit**: YES
  - Message: `feat(evaluator): add texture-map builtin for UV mapping shapes`
  - Files: `src/pml/evaluator/texture_map_builtins.h`, `src/pml/evaluator/texture_map_builtins.cpp`

- [ ] 14. **Implement Multi-Texture Blend via SkSL Composition**

  **What to do**:
  - Add function to `src/pml/graphics/texture_blend.h/cpp`:
    ```cpp
    namespace pml {
    // Generate SkSL shader code that composes multiple textures
    // Supports per-layer blend mode and opacity
    struct TextureLayer {
      std::shared_ptr<SkShader> shader;
      BlendMode mode;   // default: AlphaBlend
      float opacity;    // 0.0 - 1.0
    };
    
    // Compose multiple texture layers into a single SkShader
    sk_sp<SkShader> compose_texture_layers(
      const std::vector<TextureLayer>& layers,
      const SkMatrix& local_matrix = SkMatrix::I()
    );
    }
    ```
  - Design:
    - The multi-texture case is for: texture fill + optional background fill + optional stroke
    - Most common: single texture → override fill. This is the majority case and should be trivial.
    - Multi-blend case: Skia `SkShader::MakeBlend()` can combine up to 2 shaders;
      for >2 layers, compose incrementally with SkRuntimeEffect
    - Generate SkSL at runtime:
      ```glsl
      uniform shader layer0;
      uniform shader layer1;
      uniform float opacity1;
      half4 main(float2 uv) {
        half4 c0 = layer0.eval(uv);
        half4 c1 = layer1.eval(uv);
        return c0 * (1 - c1.a * opacity1) + c1 * c1.a * opacity1;
      }
      ```
    - For a single texture layer (most common), just return `texture_shader` directly with no SkSL overhead
  - Use `SkRuntimeEffect::MakeForShader()` for dynamic multi-blend when needed
  - For edge blending: In SkSL, multiply by `smoothstep(0, edge_width, min(u, v, 1-u, 1-v))`

  **Must NOT do**:
  - Do NOT generate per-fragment SkSL for every single texture case (optimize for 1-layer path)
  - Do NOT implement a full layer editor system

  **Recommended Agent Profile**:
  > **Category**: `unspecified-high`
  > Reason: SkSL shader generation requires careful correctness with Skia API
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 3 (sequential with 12,13,15,16)
  - **Blocks**: Tasks 16, 17, 19
  - **Blocked By**: Task 11 (texture bake → SkShader), Task 13 (texture-map params)

  **References**:
  - Skia `SkShader::MakeBlend()` — for 2-layer composition
  - `SkRuntimeEffect::MakeForShader()` — for >2 layers
  - `src/pml/backend/skia/skia_backend_draw.cpp` — existing Skia usage patterns
  - `src/pml/evaluator/shader_builtins.cpp` — SkSL shader compilation pattern

  **Acceptance Criteria**:
  - [ ] Single texture returns the shader directly (zero SkSL overhead)
  - [ ] Two textures compose correctly with alpha blend
  - [ ] Edge blending works (pixels near UV boundary fade to transparent)
  - [ ] No shader compilation errors
  - [ ] Multi-layer (>2) works with SkRuntimeEffect fallback
  - [ ] All blend modes work (at minimum: normal, multiply, screen, overlay)

  **QA Scenarios**:
  ```
  Scenario: Single texture passthrough
    Tool: Bash
    Steps: compose_texture_layers([one shader])
    Result: Returns the same shader pointer
    Evidence: .omo/evidence/task-14-single.log

  Scenario: Two texture blend
    Tool: Bash
    Steps: compose_texture_layers([red_shader, blue_shader, mode=multiply])
    Result: Returns a valid SkShader, renders blended output
    Evidence: .omo/evidence/task-14-blend.log
  ```

  **Evidence to Capture**:
  - [ ] Build log
  - [ ] Shader compilation output

  **Commit**: YES (with Task 15)
  - Message: `feat(graphics): add multi-texture SkSL blend composition`
  - Files: `src/pml/graphics/texture_blend.h`, `src/pml/graphics/texture_blend.cpp`

- [ ] 15. **Implement Skia Backend Textured Draw**

  **What to do**:
  - Create `src/pml/backend/skia/textured_draw.h/cpp`:
    ```cpp
    namespace pml {

    // Draw a textured shape using Skia's drawVertices
    // This is the core rendering path for UV texture mapping
    bool draw_textured_shape(
      SkCanvas* canvas,
      const GraphicObject& obj,
      const std::shared_ptr<TextureBox>& texture_box,
      const std::vector<Vec2>& texture_uvs,
      const UVResult& uv_result,
      const SkPaint& debug_paint = {}
    );

    // Central dispatcher: given a GraphicObject that has texture params,
    // compute UVs, triangulate, bake texture, and render
    void render_textured_object(
      SkCanvas* canvas,
      const GraphicObject& obj
    );

    }
    ```
  - `render_textured_object()` — central logic:
    1. Read texture params from GraphicObject (via ParamKey entries)
    2. Retrieve TextureBox from params
    3. Bake texture via `bake_texture()` (Task 11) → get `SkImage`
    4. Create `SkShader` from image: `image->makeShader(wrap_x, wrap_y, filter, local_matrix)`
    5. Apply multi-texture blend if needed (Task 14)
    6. Triangulate shape via `triangulate_shape()` (Task 7)
    7. Compute UVs via one of: `solve_planar_uv`, `solve_harmonic_uv`, `apply_explicit_uv` (Tasks 8-10)
    8. Build `SkVertices::MakeCopy(SkVertices::kTriangles_VertexMode, ...)` with positions + UVs + optional colors
    9. Draw: `canvas->drawVertices(vertices, SkBlendMode::kModulate, shader)`
  - **Quality details**:
    - Use `kHighFilterQuality` for anisotropic filtering when `filter == anisotropic`
    - Build SkImage with mipmaps for `kLinear_MipmapLinear_FilterMode`
    - Apply `SkMatrix::MakeScale(1.0/tex_width, 1.0/tex_height)` as local matrix for proper texel-to-pixel mapping
    - For edge blending: pass edge_blend radius to `compose_texture_layers()`
  - This is the MOST CRITICAL file — it's where all the pieces come together

  **Must NOT do**:
  - Do NOT draw textures on rough backends (guard with `#ifdef PML_BUILD_SKIA`)
  - Do NOT modify SkVertices after construction (use `MakeCopy`)
  - Do NOT add per-vertex colors (use shader-based coloring)

  **Recommended Agent Profile**:
  > **Category**: `deep`
  > Reason: Central rendering pipeline where all subsystems converge; most complex single file
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 3 (sequential with 12,13,14,16)
  - **Blocks**: Task 16, 19
  - **Blocked By**: Tasks 7 (triangulation), 9 (Harmonic UV), 11 (texture bake), 13 (texture-map params)

  **References**:
  - `src/pml/backend/skia/skia_backend_draw.cpp:draw_mesh3d_impl()` (line ~1345-1481) — THE PRIMARY REFERENCE. It already does `SkVertices::MakeCopy(positions, uvs, indices)` and draws with `drawVertices()`. Copy this exact pattern.
  - `src/pml/backend/skia/skia_backend_draw.cpp:draw_object()` — the main dispatch loop where `render_textured_object()` will be called from
  - Skia `SkVertices::MakeCopy()` docs
  - Skia `SkShader::MakeFromImage()` / `image->makeShader()` docs

  **Acceptance Criteria**:
  - [ ] `render_textured_object(canvas, obj)` draws onto the canvas when obj has texture params
  - [ ] Harmonic UV renders a persistent, correctly oriented texture (not mirrored/flipped)
  - [ ] Planar UV works (simple mapping)
  - [ ] Explicit UV works (user-provided UVs)
  - [ ] Wrap modes work: clamp (edge pixels repeat), repeat (tile), mirror (reflect)
  - [ ] Filter modes work: linear (smooth), nearest (pixelated), anisotropic (sharp at angles)
  - [ ] Edge blending works (feathered edges)
  - [ ] Non-textured shapes draw identically to before (no regression)
  - [ ] Transparent background in texture renders as transparent on the shape

  **QA Scenarios**:
  ```
  Scenario: Render a simple rect with harmonic UV
    Tool: Bash (pml.exe + output check)
    Preconditions: Build with textured draw
    Steps:
      1. Write test.pml: define-texture + texture-map + render-set → output file
      2. Run pml.exe test.pml -o out
    Expected Result: Output image exists, texture is rendered on shape
    Evidence: .omo/evidence/task-15-render-harmonic.png

  Scenario: Repeat wrap mode
    Tool: Bash
    Preconditions: Same build
    Steps:
      1. Write test.pml with :wrap-x :repeat
      2. Run pml.exe test.pml -o out
    Expected Result: Texture tiles horizontally
    Evidence: .omo/evidence/task-15-repeat.png

  Scenario: Explicit UV with uv-vertices
    Tool: Bash
    Preconditions: Same build
    Steps:
      1. Write test.pml with :uv-vertices
      2. Run pml.exe test.pml -o out
    Expected Result: Texture is distorted according to custom UVs
    Evidence: .omo/evidence/task-15-explicit.png
  ```

  **Evidence to Capture**:
  - [ ] Output images (PNG)
  - [ ] Build log

  **Commit**: YES
  - Message: `feat(backend): add Skia textured draw pipeline with Vertices+shader`
  - Files: `src/pml/backend/skia/textured_draw.h`, `src/pml/backend/skia/textured_draw.cpp`

- [ ] 16. **Integrate textured draw into draw_object() dispatch**

  **What to do**:
  - In `src/pml/backend/skia/skia_backend_draw.cpp`, modify `draw_object()` (or the per-shape switch/case):
  - Early check: if a GraphicObject has `param(ParamKey::uv)` set (i.e., has texture params):
    1. Skip the existing per-shape draw calls (fill, stroke)
    2. Route to `render_textured_object(canvas, obj)`
    3. After `render_textured_object()`, draw stroke if `:stroke` is set
  - If the object has no texture params, use the existing draw path unchanged
  - The dispatch logic:
    ```cpp
    if (obj.has_param(ParamKey::uv)) {
      // Textured draw path
      render_textured_object(canvas, obj);
      if (obj.stroke_color && obj.stroke_width > 0) {
        // Draw stroke on top (solid color)
        draw_stroke(canvas, obj); // existing stroke function
      }
      return;
    }
    // Existing draw path below...
    ```
  - This is a small but critical integration point

  **Must NOT do**:
  - Do NOT modify the existing draw path semantics for non-textured shapes
  - Do NOT add special cases per shape type — the textured path is uniform

  **Recommended Agent Profile**:
  > **Category**: `unspecified-high`
  > Reason: Requires understanding the existing draw_object() dispatch to integrate correctly
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 3 (sequential with 12,13,14,15)
  - **Blocks**: Tasks 17, 19
  - **Blocked By**: Task 2 (ParamKey), Task 15 (textured_draw)

  **References**:
  - `src/pml/backend/skia/skia_backend_draw.cpp:draw_object()` — the dispatch function to modify
  - Find the switch or if-else chain that dispatches on shape type

  **Acceptance Criteria**:
  - [ ] Objects with texture params render via textured draw path
  - [ ] Objects without texture params render exactly as before
  - [ ] Stroke renders on top of textured fill
  - [ ] No code duplication (textured path is separate, not mixed into each shape case)

  **QA Scenarios**:
  ```
  Scenario: Textured rect renders
    Tool: Bash
    Steps: Write .pml with textured rect + render-set → check output
    Result: Output shows textured rect (not solid fill)
    Evidence: .omo/evidence/task-16-integrated.png

  Scenario: Non-textured rect unchanged
    Tool: Bash
    Steps: Write .pml with plain rect + render-set (no texture)
    Result: Output exactly matches pre-texture baseline
    Evidence: .omo/evidence/task-16-baseline.png
  ```

  **Evidence to Capture**:
  - [ ] Output images

  **Commit**: YES
  - Message: `feat(backend): integrate textured draw dispatch into draw_object()`
  - Files: `src/pml/backend/skia/skia_backend_draw.cpp`

---

## Wave 4 — Integration + Tests (depends on Wave 3)

- [ ] 17. **Module system integration (provide/import of textures)**

  **What to do**:
  - Textures are `Box::Kind::Texture` values — they already pass through the module system automatically as arbitrary Values
  - Verify and fix any issues:
    1. In `src/pml/module/module.cpp` (or equivalent): ensure Texture values serialize/deserialize if the module system does serialization
    2. If module system uses a `known_kinds` or serialization dispatch: add Texture case
    3. Test: `(provide my-tex)` and `(import "a.pml")` → `my-tex` available in importing module
    4. Test: texture value retains its GraphicObject content across module boundaries
  - PML's module system is based on Environment extend — provide/import work by symbol binding. Textures are just Values; if the module system can pass strings, numbers, GOs, it can pass textures. The only potential gap is serialization (if modules save/load). Verify and fix.
  - Add `module-available?` and `module-list` for texture-only modules if needed

  **Must NOT do**:
  - Do NOT change the module system architecture
  - Do NOT add texture-specific serialization unless necessary

  **Recommended Agent Profile**:
  > **Category**: `quick`
  > Reason: Textures are already Values; only serialization edge-cases need work
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 4 (sequential with 18,19,20)
  - **Blocks**: Tasks 18, 19
  - **Blocked By**: Tasks 12 (define-texture), 13 (texture-map), 16 (draw_object integration)

  **References**:
  - `src/pml/module/module.cpp` — module system implementation
  - `src/pml/types.h:value_to_string()` — serialization functions
  - `src/pml/api/api.cpp` — PMLRuntime for module loading

  **Acceptance Criteria**:
  - [ ] Texture defined in one file, imported in another → texture value available
  - [ ] `(provide tex)` in module A, `(import "A.pml")` → `tex` bound in importing env
  - [ ] Texture's GraphicObject content is identical after import
  - [ ] No serialization errors

  **QA Scenarios**:
  ```
  Scenario: Module export/import of texture
    Tool: Bash (pml.exe)
    Preconditions: Build complete
    Steps:
      1. Write a.pml: (define-texture t (64 64) (rect 0 0 64 64 :fill "#f00")) (provide t)
      2. Write b.pml: (import "a.pml") (texture? t)
      3. Run pml.exe b.pml
    Expected Result: #t (texture exists in importing module)
    Evidence: .omo/evidence/task-17-module.log

  Scenario: Use imported texture in rendering
    Tool: Bash
    Steps:
      1. Write c.pml as above
      2. Write d.pml: (import "a.pml") (render (texture-map (rect 0 0 256 256) :source t))
    Expected Result: Renders textured rect using imported texture
    Evidence: .omo/evidence/task-17-import-usage.png
  ```

  **Evidence to Capture**:
  - [ ] Module test output
  - [ ] Rendered output image

  **Commit**: YES (with Task 18)
  - Message: `feat(api): integrate texture values with module system`
  - Files: `src/pml/module/module.cpp`, `src/pml/core/types.h`

- [ ] 18. **Register new builtin groups in PMLRuntime**

  **What to do**:
  - In `src/pml/api/api.cpp`, add registration calls to `PMLRuntime::init_global_env()`:
    ```cpp
    // After register_canvas_builtins(m_env);
    register_texture_builtins(m_env);         // define-texture, texture?, etc.
    register_texture_map_builtins(m_env);     // texture-map
    ```
  - Registration order matters: texture builtins must be registered before any test file that uses them
  - Add the includes:
    ```cpp
    #include "evaluator/texture_builtins.h"
    #include "evaluator/texture_map_builtins.h"
    ```
  - Verify the registration functions are declared in their headers with the correct signature
  - Verify no linker errors: `register_texture_builtins` and `register_texture_map_builtins` must be defined exactly once

  **Must NOT do**:
  - Do NOT change the order of existing registrations
  - Do NOT include non-existent registration functions

  **Recommended Agent Profile**:
  > **Category**: `quick`
  > Reason: Simple registration calls; mechanical task
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 4 (sequential with 17,19,20)
  - **Blocks**: Task 19
  - **Blocked By**: Tasks 12, 13, 16, 17

  **References**:
  - `src/pml/api/api.cpp` — `init_global_env()` method
  - `src/pml/evaluator/texture_builtins.h` — declaration from Task 12
  - `src/pml/evaluator/texture_map_builtins.h` — declaration from Task 13
  - `src/pml/graphics/CMakeLists.txt` — ensure texture_builtins.cpp and texture_map_builtins.cpp are compiled

  **Acceptance Criteria**:
  - [ ] `pml.exe --version` (or any execution) works without symbol errors
  - [ ] `(define-texture ...)` is available as a built-in
  - [ ] `(texture-map ...)` is available as a built-in
  - [ ] `pml-builtins-smoke.exe` passes

  **QA Scenarios**:
  ```
  Scenario: Builtins available in REPL
    Tool: Bash (pml.exe)
    Preconditions: Build complete with registrations
    Steps:
      1. echo '(define-texture t (64 64) (rect 0 0 64 64 :fill "#f00"))' | .\build\debug\bin\Debug\pml.exe
    Expected Result: Evaluates without "undefined" errors
    Evidence: .omo/evidence/task-18-registration.log
  ```

  **Evidence to Capture**:
  - [ ] REPL test output

  **Commit**: YES (with Task 17)
  - Message: `feat(api): register texture and texture-map builtins in PMLRuntime`
  - Files: `src/pml/api/api.cpp`

- [ ] 19. **Add smoke tests for texture builtins**

  **What to do**:
  - In `tests/builtins_smoke.cpp`, add test cases (following existing pattern):
    ```cpp
    // --- Texture mapping ---
    TEST_CASE("define-texture creates a valid texture value") {
      auto env = create_test_env();
      eval("(define-texture tex (64 64) (rect 0 0 64 64 :fill \"#f00\"))");
      CHECK(eval("(texture? tex)") == "#t");
      CHECK(eval("(texture-width tex)") == "64");
    }

    TEST_CASE("texture-map sets texture params on shape") {
      auto env = create_test_env();
      eval("(define-texture tex (64 64) (rect 0 0 64 64 :fill \"#f00\"))");
      auto result = eval("(texture-map (rect 0 0 100 100) :source tex :mode :planar)");
      // Verify object has texture params (check through param accessor)
      CHECK(result.find("texture") != string::npos); // or better assertion
    }

    TEST_CASE("render textured shape produces output") {
      // Full integration test: define, map, render, check file
      auto result = run_pml(R"(
        (define-texture tex (64 64)
          (rect 0 0 64 64 :fill "#ff0000"))
        (render-to-file (texture-map (rect 0 0 256 256) :source tex)
          "test-textured-output.png")
      )");
      CHECK(file_exists("test-textured-output.png"));
      CHECK(file_size("test-textured-output.png") > 0);
    }

    TEST_CASE("texture wrap modes work") {
      // Test all three wrap modes produce different output
    }

    TEST_CASE("explicit texture-map with uv-vertices") {
      // Test explicit UV path
    }

    TEST_CASE("harmonic UV produces smooth mapping on complex shape") {
      // Test harmonic UV on polygon
    }

    TEST_CASE("texture cache works across multiple renders") {
      // Verify second render hits cache (render time should be faster)
    }

    TEST_CASE("texture module export/import") {
      // Multi-file test
    }

    TEST_CASE("textured shape with stroke renders both") {
      // Test stroke + textured fill
    }

    TEST_CASE("non-textured shapes render identically with and without texture system code") {
      // Regression test
    }
    ```
  - Follow the exact test pattern from existing cases in `builtins_smoke.cpp`
  - Each test must be self-contained (no test-to-test dependency)
  - Target: 10-12 new smoke test cases

  **Must NOT do**:
  - Do NOT add tests that require human visual inspection
  - Do NOT add tests that depend on specific file paths or system state

  **Recommended Agent Profile**:
  > **Category**: `quick`
  > Reason: Follow existing test patterns; mechanical addition
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 4 (sequential with 17,18,20)
  - **Blocks**: Task 20
  - **Blocked By**: Tasks 14, 15, 16, 17, 18

  **References**:
  - `tests/builtins_smoke.cpp` — existing test file (134 cases) — copy the exact `TEST_CASE`, `CHECK`, `eval` patterns
  - `tests/builtins_smoke.cpp:top` — includes, test infrastructure macros

  **Acceptance Criteria**:
  - [ ] All new smoke tests compile
  - [ ] `pml-builtins-smoke.exe` passes with all new tests (0 failures)
  - [ ] Existing tests still pass (no regression)
  - [ ] Each test checks actual output (not just "no error")

  **QA Scenarios**:
  ```
  Scenario: Smoke tests pass
    Tool: Bash
    Preconditions: Build complete
    Steps:
      1. .\build\debug\tests\Debug\pml-builtins-smoke.exe
    Expected Result: All tests pass, exit code 0
    Evidence: .omo/evidence/task-19-smoke-pass.log
  ```

  **Evidence to Capture**:
  - [ ] Smoke test output

  **Commit**: YES
  - Message: `test: add smoke tests for texture mapping builtins`
  - Files: `tests/builtins_smoke.cpp`

- [ ] 20. **Create example .pml files and visual verification**

  **What to do**:
  - Create example files in `examples/`:
    - `examples/texture-basic.pml` — simplest case: define texture → map onto rect
    - `examples/texture-starry-sky.pml` — user's original scenario:
      - Complex texture (stars, gradient sky, moon)
      - Map onto a cloud shape (complex polygon)
    - `examples/texture-repeat.pml` — demonstrates wrap modes
    - `examples/texture-explicit.pml` — demonstrates explicit UV with distortion
    - `examples/texture-module/` — multi-file example:
      - `texture-module/lib.pml` — defines and provides a texture
      - `texture-module/main.pml` — imports and uses it
  - Each example should include comments explaining the feature
  - Run each example to verify correct rendering
  - Save rendered output as reference images

  **Must NOT do**:
  - Do NOT overwrite existing example files (add new ones)

  **Recommended Agent Profile**:
  > **Category**: `writing`
  > Reason: Documentation-style example creation with PML code
  > **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 4 (sequential after 19)
  - **Blocks**: None (final example)
  - **Blocked By**: Task 19 (smoke tests pass)

  **References**:
  - `examples/` — existing example files for style/format reference
  - User's original scenario: a.pml (starry sky texture) → b.pml (cloud shape) → c.pml (uv-map composition)

  **Acceptance Criteria**:
  - [ ] All example .pml files run without errors
  - [ ] Each example produces a non-empty output image
  - [ ] Example module pattern works (multi-file)
  - [ ] Examples are self-documenting with comments

  **QA Scenarios**:
  ```
  Scenario: Basic texture example renders
    Tool: Bash (pml.exe)
    Preconditions: Build complete
    Steps:
      1. .\build\debug\bin\Debug\pml.exe examples/texture-basic.pml -o out/basic
    Expected Result: out/basic.png exists, non-zero size
    Evidence: .omo/evidence/task-20-basic.png

  Scenario: Starry sky on cloud shape
    Tool: Bash
    Steps:
      1. .\build\debug\bin\Debug\pml.exe examples/texture-starry-sky.pml -o out/starry
    Expected Result: out/starry.png exists, shows sky texture on cloud
    Evidence: .omo/evidence/task-20-starry.png
  ```

  **Evidence to Capture**:
  - [ ] Example output images
  - [ ] Example run logs

  **Commit**: YES
  - Message: `docs: add UV texture mapping examples`
  - Files: `examples/texture-basic.pml`, `examples/texture-starry-sky.pml`, `examples/texture-repeat.pml`, `examples/texture-explicit.pml`, `examples/texture-module/lib.pml`, `examples/texture-module/main.pml`

---

## Final Verification Wave

> 4 review agents run in PARALLEL. ALL must APPROVE. Present consolidated results to user and get explicit "okay" before completing.
>
> **Do NOT auto-proceed after verification. Wait for user's explicit approval before marking work complete.**
> **Never mark F1-F4 as checked before getting user's okay.** Rejection or user feedback → fix → re-run → present again → wait for okay.

- [ ] F1. **Plan Compliance Audit** — `oracle`
  Read the plan end-to-end. For each "Must Have": verify implementation exists (read file, read source, check header declarations, check builtin registration). For each "Must NOT Have": search codebase for forbidden patterns (LSCM, Eigen, Cage, file-level texture convention) — reject with file:line if found. Check evidence files exist in `.omo/evidence/`. Compare deliverables against plan objectives.
  Output: `Must Have [N/N] | Must NOT Have [N/N] | Tasks [N/N] | VERDICT: APPROVE/REJECT`

- [ ] F2. **Code Quality Review** — `unspecified-high`
  Build: `cmake --build --preset debug`. Run smoke tests: `.\build\debug\tests\Debug\pml-builtins-smoke.exe`. Review all changed files for: `as any`/`@ts-ignore` (N/A for C++), empty catches, `std::cout` in prod code, commented-out code, unused includes. Check AI slop: excessive comments, over-abstraction, generic names (`data`/`result`/`item`/`temp`). Check CG solver for numeric stability.
  Output: `Build [PASS/FAIL] | Lint [PASS/FAIL] | Tests [N pass/N fail] | Files [N clean/N issues] | VERDICT`

- [ ] F3. **Real Manual QA** — `unspecified-high` (+ `playwright` if UI, not needed here — use bash)
  Start from clean state. Execute EVERY QA scenario from EVERY task — follow exact steps, capture evidence. Test cross-task integration: define-texture → texture-map → render → check output image. Test edge cases: empty texture, invalid UV range, missing source, non-polygon shape. Save to `.omo/evidence/final-qa/`.
  Output: `Scenarios [N/N pass] | Integration [N/N] | Edge Cases [N tested] | VERDICT`

- [ ] F4. **Scope Fidelity Check** — `deep`
  For each task: read "What to do", read actual diff (git log/diff). Verify 1:1 — everything in spec was built (no missing), nothing beyond spec was built (no creep). Check "Must NOT do" compliance: no LSCM, no Eigen, no Cage, no file-level texture convention, no textured stroke. Detect cross-task contamination: Task N touching Task M's files. Flag unaccounted changes.
  Output: `Tasks [N/N compliant] | Contamination [CLEAN/N issues] | Unaccounted [CLEAN/N files] | VERDICT`

---

## Commit Strategy

| # | Type | Message | Files |
|---|------|---------|-------|
| 1 | `build` | `build(third_party): integrate poly2tri for shape triangulation` | `CMakeLists.txt` |
| 2 | `feat` | `feat(graphics): add ParamKey entries for UV texture mapping` | `src/pml/graphics/params.h` |
| 3 | `feat` | `feat(core): add Box::Kind::Texture value type and cache` | `src/pml/core/types.h`, `src/pml/core/texture.h`, `src/pml/core/texture_cache.h/cpp` |
| 4 | `feat` | `feat(graphics): add sparse CG solver for Harmonic UV projection` | `src/pml/graphics/cg_solver.h` |
| 5 | `build` | `build: integrate texture mapping source files into CMake build` | `src/pml/.../CMakeLists.txt` (all) |
| 6 | `feat` | `feat(graphics): add poly2tri-based triangulation module` | `src/pml/graphics/triangulation.h/cpp` |
| 7 | `feat` | `feat(graphics): add Planar and Harmonic UV projection solvers` | `src/pml/graphics/uv_solver.h/cpp` |
| 8 | `feat` | `feat(graphics): add texture bake pipeline (GO → SkSurface → SkImage)` | `src/pml/graphics/texture_bake.h/cpp` |
| 9 | `feat` | `feat(evaluator): add define-texture builtin and texture accessors` | `src/pml/evaluator/texture_builtins.h/cpp` |
| 10 | `feat` | `feat(evaluator): add texture-map builtin for UV mapping shapes` | `src/pml/evaluator/texture_map_builtins.h/cpp` |
| 11 | `feat` | `feat(graphics): add multi-texture SkSL blend composition` | `src/pml/graphics/texture_blend.h/cpp` |
| 12 | `feat` | `feat(backend): add Skia textured draw pipeline` | `src/pml/backend/skia/textured_draw.h/cpp` |
| 13 | `feat` | `feat(backend): integrate textured draw dispatch into draw_object()` | `src/pml/backend/skia/skia_backend_draw.cpp` |
| 14 | `feat` | `feat(api): integrate texture values with module system + registration` | `src/pml/module/module.cpp`, `src/pml/api/api.cpp` |
| 15 | `test` | `test: add smoke tests for texture mapping builtins` | `tests/builtins_smoke.cpp` |
| 16 | `docs` | `docs: add UV texture mapping examples` | `examples/texture-*.pml` |

---

## Success Criteria

### Verification Commands
```bash
# Build
cmake --preset debug
cmake --build --preset debug

# Smoke tests
.\build\debug\tests\Debug\pml-builtins-smoke.exe

# Example rendering
.\build\debug\bin\Debug\pml.exe examples/texture-basic.pml -o out/basic
.\build\debug\bin\Debug\pml.exe examples/texture-starry-sky.pml -o out/starry
.\build\debug\bin\Debug\pml.exe examples/texture-repeat.pml -o out/repeat
.\build\debug\bin\Debug\pml.exe examples/texture-explicit.pml -o out/explicit
```

### Final Checklist
- [ ] All "Must Have" items implemented and verified
- [ ] All "Must NOT Have" items absent from codebase
- [ ] All smoke tests pass (0 failures, exit code 0)
- [ ] All examples render correctly to non-empty output images
- [ ] No regression in existing builtins (pre-texture examples still work)
- [ ] Non-textured shapes render identically (pixel-level where possible)
- [ ] `define-texture` works in multi-file module scenarios
- [ ] All QA scenario evidence files exist in `.omo/evidence/`
