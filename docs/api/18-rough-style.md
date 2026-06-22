# 18 — Rough-Style (Hand-Drawn / Sketchy Rendering)

Rough-style adds **hand-drawn / sketchy perturbation** to PML's vector shapes.
Every line and curve gets random jitter, multiple overlapping strokes, and
optional fill patterns (hachure, zigzag, cross-hatch, dots, dashed) —
inspired by [rough.js](https://roughjs.com/).

## Design Principles

| Principle | Detail |
|-----------|--------|
| **Per-shape kwargs only** | No global `roughness!` / `fill-style!` functions. Pass `:roughness`, `:fill-style` as kwargs to each shape call. |
| **Zero impact when absent** | `roughness=0` and `fill-style="solid"` (both defaults) → original exact Skia path, identical to pre-rough behaviour. |
| **Style integration** | `define-style` can carry roughness fields; per-shape kwargs override the style. |
| **Deterministic** | `:seed` controls the PRNG; same seed → same perturbation. |

## Kwargs Reference

All kwargs are optional; defaults produce exact (non-rough) rendering.

| Kwarg | Type | Default | Description |
|-------|------|---------|-------------|
| `:roughness` | number | `0` | Stroke perturbation intensity. `0` = exact. Typical range `1-5`. |
| `:bowing` | number | `1.0` | Mid-point bowing displacement. Higher = more curved strokes. |
| `:seed` | integer | `0` | PRNG seed for deterministic output. Same seed = same perturbation. |
| `:fill-style` | string | `"solid"` | Fill pattern. See [Fill Patterns](#fill-patterns) below. |

### Fill Patterns

| Value | Description |
|-------|-------------|
| `"solid"` | Exact Skia solid fill (default, no perturbation). |
| `"hachure"` | Parallel scanlines at ~49° angle. |
| `"zigzag"` | Zigzag lines along hachure direction. |
| `"cross-hatch"` | Hachure + rotated 90° for a grid. |
| `"dots"` | Dots placed along hachure scanlines. |
| `"dashed"` | Dashed segments along hachure scanlines. |

### Supported Shapes

All 5 basic shapes support rough-style kwargs:

| Shape | Complexity of Perturbation |
|-------|---------------------------|
| `circle` | Ellipse perturbation → Catmull-Rom curve |
| `rect` | 4-point polygon → double-line perturbation |
| `ellipse` | Sampled ellipse → Catmull-Rom curve |
| `line` | Single segment → double bezier curve |
| `polygon` | Vertex-connected → double-line perturbation |

**`text`, `path`, `image`, `group`** do not support roughness.

## Examples

### Basic rough stroke

```scheme
(circle 50 50 30 :fill "#e74c3c" :stroke "#c0392b"
                 :stroke-width 2 :roughness 2)
```

### Rough stroke + pattern fill

```scheme
(rect 10 10 80 80 :fill "#3498db" :stroke "#2980b9"
      :stroke-width 2 :roughness 2 :fill-style "hachure")
```

### Exact stroke + pattern fill only

```scheme
; roughness=0 (or omit) → stroke stays exact
(ellipse 50 50 40 30 :fill "#2ecc71" :stroke "#27ae60"
                      :stroke-width 2 :roughness 0
                      :fill-style "cross-hatch")
```

### Deterministic output via seed

```scheme
; These two circles are pixel-identical
(circle 20 20 40 :fill "#e74c3c" :roughness 3 :seed 42)
(circle 20 20 40 :fill "#e74c3c" :roughness 3 :seed 42)
```

### Via define-style

```scheme
(define-style 'hand-drawn
  :roughness 2 :bowing 1.5 :seed 0 :fill-style "hachure")

; Use it on any shape
(circle 50 50 30 :fill "#e74c3c" :style 'hand-drawn)

; Per-shape kwargs override style
(circle 50 50 30 :fill "#e74c3c" :style 'hand-drawn
        :roughness 1)  ; overrides style's roughness=2
```

### Animated rough shapes

```scheme
(define base (circle 100 100 60 :fill "#e74c3c" :stroke "#c0392b"
                     :stroke-width 2 :roughness 3 :seed 7
                     :fill-style "hachure"))
(animate base :duration 2.0 :loops true
  (parallel
    (every-frame (lambda (t) (translate-object base (* t 80) (* t 40))) 1.0)
    (every-frame (lambda (t) (rotate-object base (* t 180))) 1.0)))
(render "rough_anim.gif" :format "GIF" :fps 12)
```

## Implementation Details

- **Perturbation algorithm**: Ported from rough.js v4 `renderer.ts`. Each line
  segment becomes a cubic bezier with random control-point displacement,
  drawn twice (primary + overlay). Longer segments get less relative
  perturbation via `roughnessGain`.
- **Fill engine**: Scanline hachure ported from the `hachure-fill` npm package
  (active-edge-table algorithm). Patterns are rendered as rough line ops
  through the same `rough_double_line` pipeline.
- **No JS dependency**: Pure C++23 implementation using rough.js algorithms
  only as reference. No WASM / V8 / JavaScript runtime involved.
- **Skia integration**: Rough ops → `SkPathBuilder` → `drawPath`. The rough
  path is drawn instead of the exact Skia shape when `rough_` prefix is set
  on the GraphicObject's `shape_type`.

## Files

| Path | Purpose |
|------|---------|
| `src/pml/graphics/rough.h` | Type definitions (RoughStyleParams, RoughRandom, RoughOp) + function declarations |
| `src/pml/graphics/rough.cpp` | Line/ellipse/curve perturbation algorithms |
| `src/pml/graphics/rough_filler.h` | Fill pattern dispatcher (inline) |
| `src/pml/graphics/rough_filler.cpp` | Scanline hachure engine + 6 filler implementations |
| `src/pml/evaluator/shape_builtins.cpp` | `resolve_rough_params()` + rough metadata injection |
| `src/pml/sprites/style.{h,cpp}` | Roughness fields on StyleDescriptor |
| `src/pml/backend/skia/skia_backend_draw.cpp` | `draw_rough_*` functions + `rough_` prefix dispatcher |
