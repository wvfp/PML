# Draft: UV Texture Mapping System

## Requirements (confirmed)
- Complete UV texture mapping system for PML
- Texture source: `(define-texture name [args] body...)` 作为函数式纹理定义
- No file-level texture convention per Metis recommendation (ambiguous, use define-texture only)
- Shape mapping: Extend existing shapes via `:fill` parameter to accept texture values
- Plus `texture-map` as a high-level builtin function
- UV coordinate: Auto-flatten by default (Harmonic mapping), with manual override (explicit UV)
- Projection methods: Planar (trivial) + Harmonic (default) + Explicit (manual)
- LSCM and Cage DEFERRED (not in scope)
- Quality: Highest possible rendering quality, performance not a concern
- Multi-layer: Single texture with optional blend (NOT a full layer system)
- Open-source libraries for triangulation: poly2tri (BSD 3-Clause)
- Sparse solver: Self-implemented Conjugate Gradient (NOT Eigen)
- Texture as Box::Kind::Texture value (first-class PML value)
- UV coordinates in [0,1] range (matching Mesh3D)
- Texture overrides fill (not blends)
- No rough rendering with UV mapping (skipped)
- Deep integration with module system (texture passes through provide/import as Value)

## Technical Decisions
- ParamKey: 8 slots remain (32-bit mask, 24 used). Need uv, uv_mode, wrap_x, wrap_y, filter_mode
- Centralized function: shape_to_textured_mesh() — not per-shape modification
- New directory: src/pml/graphics/textures/ — 4 new files
- Skia backend: 1 new file (textured_draw.cpp) + modifications to draw_object()
- poly2tri via FetchContent (BSD 3-Clause), header-only
- Texture cache: LRU with texture stable ID as key

## Research Findings
- Skia SkPictureShader::Make() exists but we'll use SkSurface→SkImage→makeShader for explicit control
- 3D Mesh3D already has per-face UV + material rendering (draw_mesh3d_impl) — reference implementation
- bind-textures already renders GraphicObjects to offscreen surfaces
- poly2tri: https://github.com/greenm01/poly2tri (BSD 3-Clause)
- poly2tri requires convex boundary, holes supported
- Conjugate Gradient method for sparse symmetric positive definite systems

## Scope Boundaries
- INCLUDE: define-texture, texture-map, Harmonic UV, Planar UV, Explicit UV, poly2tri integration, module integration, texture cache, wrap modes, filter modes, texture cache
- INCLUDE: Shader-based multi-texture blend on single shape (via SkRuntimeEffect)
- EXCLUDE: LSCM projection (deferred)
- EXCLUDE: Cage deformation (deferred)
- EXCLUDE: File-level texture convention
- EXCLUDE: Multi-layer texture editor/palette
- EXCLUDE: Animation of UV coordinates
- EXCLUDE: rough-style rendering with textures

## Test Strategy
- Approach: Tests-after (add smoke test cases after implementation)
- Each task includes Agent-Executed QA Scenarios:
  - Render a .pml file using the new feature
  - Verify output image exists (file size > 0)
  - Visual verification via screenshot comparison where possible
- Framework: GTest (existing) + builtins_smoke.cpp pattern

## Stroke Behavior
- Stroke is independent of texture (solid color only)
- Texture only affects fill
- Stroke draws on top of textured fill
- :stroke and :stroke-width params work exactly as without texture
