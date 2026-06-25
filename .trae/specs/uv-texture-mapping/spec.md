# UV Texture Mapping System for PML Spec

## Why

PML 当前缺少将过程化纹理（由 PML 代码定义）映射到 2D 形状上的能力。用户希望:
- 文件 A 定义过程化纹理（如星空，使用 PML 形状而非纯色）
- 文件 B 定义任意闭合形状（path、polygon、circle 等）
- 文件 C 引用 A 和 B，将 A 的纹理通过 UV 映射应用到 B 的形状上

当前实现缺口：`(define-texture ...)`、`(texture-map ...)` 不存在；没有 UV 投影系统（Planar/Harmonic/Explicit）；没有图形对象三角化支持；没有基于 LRU 的纹理烘焙缓存。

## What Changes

- **新增** `Box::Kind::Texture` 一等值类型及其 `TextureBox` 结构（含 stable_id 用于缓存键）
- **新增** ParamKey 扩展：`uv`、`uv_mode`、`wrap_x`、`wrap_y`、`filter`、`uv_vertices`、`edge_blend`
- **新增** 共轭梯度（CG）求解器（基于 `cg_solver.h`）用于 Harmonic UV 投影
- **新增** 耳剪法（ear-clipping）三角化模块（替代不可用的 poly2tri）
- **新增** UV 投影模块：Planar / Harmonic / Explicit 三种模式
- **新增** 纹理烘焙管道：`GraphicObject → SkSurface → SkImage`（带 LRU 缓存）
- **新增** Skia 渲染端的 `draw_textured_shape()`（使用 `SkVertices::MakeCopy` + `drawVertices`）
- **新增** PML 内置函数：`define-texture`、`texture?`、`texture-width`、`texture-height`、`texture-id`、`texture-map`
- **修改** `skia_backend_draw.cpp::draw_object()` 在存在 `ParamKey::uv` 时路由到纹理绘制路径
- **修改** `api.cpp::PMLRuntime::init_global_env()` 注册新内置函数组
- **新增** CMakeLists 集成（核心、图形、评估器、skia 后端）
- **新增** 烟雾测试用例与示例 `.pml` 文件

## Impact

- Affected specs: 核心类型、图形渲染、内置函数注册、模块系统
- Affected code:
  - `src/pml/core/types.h`、`src/pml/core/types.cpp`（Box::Kind::Texture）
  - `src/pml/core/texture.h`、`src/pml/core/texture_cache.h/cpp`（新增）
  - `src/pml/graphics/params.h`（新增 ParamKey 7 项）
  - `src/pml/graphics/cg_solver.h`（新增）
  - `src/pml/graphics/triangulation.h/cpp`（新增）
  - `src/pml/graphics/uv_solver.h/cpp`（新增）
  - `src/pml/graphics/texture_bake.h/cpp`（新增）
  - `src/pml/evaluator/texture_builtins.h/cpp`（新增）
  - `src/pml/evaluator/texture_map_builtins.h/cpp`（新增）
  - `src/pml/backend/skia/textured_draw.h/cpp`（新增）
  - `src/pml/backend/skia/skia_backend_draw.cpp`（修改 draw_object）
  - `src/pml/api/api.cpp`（注册新内置）
  - `src/pml/*/CMakeLists.txt`（集成新源文件）
  - `tests/builtins_smoke.cpp`（新增用例）
  - `examples/texture-*.pml`（新增示例）

## ADDED Requirements

### Requirement: Texture as First-Class Value
The system SHALL provide `Box::Kind::Texture` as a first-class PML value, encapsulated by `TextureBox { stable_id, go, width, height, wrap_x, wrap_y, filter }`, with stable_id auto-assigned atomically and never reused.

#### Scenario: Texture value passes through functions
- **WHEN** a user creates a texture via `define-texture` and binds it to a symbol
- **THEN** the symbol holds a value of kind Texture
- **AND** `(texture? sym)` returns `#t`

### Requirement: define-texture Builtin
The system SHALL provide `(define-texture name (width height) body)` as a builtin that creates a `TextureBox` from the body GraphicObject and binds it in the current environment.

#### Scenario: Define and inspect a basic texture
- **WHEN** user runs `(define-texture tex (64 64) (rect 0 0 64 64 :fill "#f00"))`
- **THEN** `tex` is bound, `(texture? tex)` is `#t`, `(texture-width tex)` is `64`, `(texture-height tex)` is `64`

#### Scenario: Invalid dimensions
- **WHEN** user provides non-positive or non-numeric width/height
- **THEN** the system SHALL raise a type error

### Requirement: texture-map Builtin
The system SHALL provide `(texture-map shape :source tex ...)` as a kwarg-accepting builtin that sets UV parameters on the shape's GraphicObject and returns it. Supported kwargs:
- `:source` (required) — texture value
- `:mode` — `:planar` | `:harmonic` | `:explicit` (default `:harmonic`)
- `:wrap-x`, `:wrap-y` — `:clamp` | `:repeat` | `:mirror` (default `:clamp`)
- `:filter` — `:linear` | `:nearest` (default `:linear`)
- `:uv-vertices` — list of (x y) pairs (default `()`)
- `:edge-blend` — pixels (default `0`)

#### Scenario: Apply texture with harmonic UV
- **WHEN** user runs `(texture-map (rect 0 0 100 100) :source tex :mode :harmonic)`
- **THEN** the returned GraphicObject has `ParamKey::uv`, `uv_mode=1`, and the texture box stored

#### Scenario: Missing source
- **WHEN** user runs `(texture-map (rect 0 0 100 100))` without `:source`
- **THEN** the system SHALL raise a type error

### Requirement: Planar UV Projection
The system SHALL compute Planar UV coordinates by mapping the shape's AABB to `[0,1]²`. All vertices at the same point SHALL map to `(0,0)`. Zero-range axes SHALL be normalized to `1.0`.

### Requirement: Harmonic UV Projection
The system SHALL compute Harmonic UV by solving the Laplace equation with the CG solver. Boundary vertices SHALL be fixed to unit-square perimeter via arc-length parameterization. Cotangent weights SHALL be the default; degenerate triangles SHALL fall back to uniform weight `w_ij = 1`.

#### Scenario: Harmonic UV on a simple polygon
- **WHEN** user maps a texture to a 5-vertex polygon with `:mode :harmonic`
- **THEN** boundary UVs lie on the `[0,1]²` perimeter
- **AND** no UVs contain NaN/inf values

### Requirement: Explicit UV Projection
The system SHALL use user-provided per-vertex UVs when `:uv-vertices` is set. Extra vertices (Steiner points) SHALL fall back to Planar UV. The system SHALL return `valid=false` if more user UVs are provided than mesh vertices.

### Requirement: Triangulation Module
The system SHALL triangulate simple polygon contours using ear-clipping. The module SHALL support `polygon`, `path`, `rect`, `ellipse`, `circle` shape types and return `TriangulatedMesh { vertices, indices, contour_map }`. CCW winding SHALL be enforced. Empty/colinear contours SHALL return errors rather than crash.

### Requirement: Texture Bake Pipeline
The system SHALL rasterize a `TextureBox`'s source GraphicObject into a `shared_ptr<SkImage>` via the active Skia backend's `draw_to_new_surface`. Results SHALL be cached in `TextureCache` (LRU, keyed by stable_id, default budget 64 MiB). A cache lookup SHALL return the same `shared_ptr` for the same stable_id. Eviction SHALL drop the LRU entry first.

### Requirement: Skia Textured Draw Path
The system SHALL dispatch shapes with `ParamKey::uv` set to `draw_textured_shape()`, which triangulates, computes UVs, bakes the texture, builds `SkVertices::MakeCopy(...)`, and renders via `canvas->drawVertices()` with a shader from the baked image. Stroke rendering SHALL remain a separate solid-color pass on top of the textured fill.

#### Scenario: Render textured rect produces a textured output
- **WHEN** user renders a textured rect via `(render (texture-map (rect ...) :source tex))`
- **THEN** the output image shows the texture mapped onto the rect

#### Scenario: Non-textured shapes render unchanged
- **WHEN** a shape has no `ParamKey::uv`
- **THEN** the existing draw path is used with no regression

### Requirement: Module System Integration
The system SHALL allow textures to pass through `provide`/`import` as first-class Values without modification, since they are stored in `Box::Kind::Texture`. Serialization (if any) SHALL handle the Texture case.

## MODIFIED Requirements

### Requirement: ParamKey Capacity
**Before**: ParamKey enum had 24 entries; `static_assert(count <= 32)` allowed 8 free slots.
**After**: ParamKey enum has 31 entries (added `uv`, `uv_mode`, `wrap_x`, `wrap_y`, `filter`, `uv_vertices`, `edge_blend`); 1 free slot remains. The 32-bit mask is unchanged.

### Requirement: Builtin Registration Order
**Before**: PMLRuntime registered 19 builtin groups.
**After**: PMLRuntime additionally registers `register_texture_builtins(m_env)` and `register_texture_map_builtins(m_env)` after `register_canvas_builtins` and before `register_3d_builtins`.

### Requirement: draw_object Dispatch
**Before**: `draw_object()` dispatched only to per-shape fill/stroke paths.
**After**: `draw_object()` first checks `obj.has_param(ParamKey::uv)`; if true, routes to `draw_textured_shape()`. Stroke is then drawn on top via the existing solid-color stroke function. Non-textured shapes are unaffected.

## REMOVED Requirements

(None — this is a purely additive feature.)

---

## Guardrails (Must NOT Have)

- **NO** LSCM projection
- **NO** Cage deformation
- **NO** File-level texture convention
- **NO** Eigen dependency (CG is self-implemented)
- **NO** Textured stroke (solid-color stroke only)
- **NO** UV animation support
- **NO** Rough-style rendering with textures
- **NO** Multi-layer texture editor/palette system
