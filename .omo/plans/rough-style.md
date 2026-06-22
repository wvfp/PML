# Rough-Style (Hand-Drawn) Rendering for PML

**Objective**: Add rough.js-compatible hand-drawn sketch style to PML shapes.
**Reference**: rough.js v4.x from `rough-stuff/rough` (cloned to `G:\AppData\temp\roughjs`).
**Status**: Plan

---

## 1. Architecture Overview

The approach is **geometry-level perturbation** — exactly what rough.js does. The perfect geometric paths from existing shapes (circle, rect, ellipse, line, polygon) are decomposed into short bezier segments with random offsets applied to control points, creating a hand-drawn appearance.

```
PML shape builtin (circle 50 50 30 :roughness 2)
  ↓
shape_builtins.cpp recognizes :roughness kwarg
  ↓
Creates GraphicObject with shape_type="rough_circle"
  ↓ stores rough params in metadata: {roughness, fill_style, seed, bowing, ...}
  ↓
skia_backend_draw.cpp dispatches to draw_rough_circle()
  ↓
draw_rough_circle() creates perturbed SkPath using the same algorithm as rough.js
  ↓ renders via SkCanvas::drawPath()
```

Key design decision: **Perturbation happens at render time** (in the Skia backend), NOT at GraphicObject creation time. Rationale:
- GraphicObject remains immutable and deterministic
- Perturbation uses a seeded PRNG → same seed always produces same "roughness"
- Animation doesn't regenerate perturbations each frame
- The `render` method can optionally de-rough for high-quality output

---

## 2. PML API Surface

Roughness is configured through two mechanisms only. There are no global
mutators like `roughness!` or `fill-style!`.

### 2.1 Per-shape kwargs

All shape builtins (`circle`, `rect`, `ellipse`, `line`, `polygon`) accept
roughness kwargs directly:

| Kwarg | Type | Default | Description |
|-------|------|---------|-------------|
| `:roughness` | float | 0 | 0=perfect geometry, 1=rough.js default, 2=very sketchy |
| `:fill-style` | keyword | `'solid` | Fill pattern: `'hachure`, `'solid`, `'cross-hatch`, `'zigzag`, `'dots`, `'dashed` |
| `:bowing` | float | 1 | Bowing amount (line curvature exaggeration) |
| `:seed` | int | random | PRNG seed for reproducible roughness |

`:roughness` controls **stroke** perturbation. `:fill-style` controls **fill**
rendering independently. A shape can have a rough stroke with a solid fill
(`:roughness 2 :fill-style 'solid`), or a perfect stroke with a rough hachure
fill (`:roughness 0 :fill-style 'hachure`).

### 2.2 Via define-style

The existing `define-style` system gains roughness-aware fields:

```scheme
(define-style 'sketchy :roughness 2 :fill-style 'cross-hatch :bowing 1.5)
(circle 50 50 30 :style 'sketchy)
```

When a shape has `:style 'name` and the named style includes roughness fields,
those values are applied to the shape. See the Style system docs for
`define-style` syntax.

### 2.3 Precedence

Per-shape kwargs override style-sourced values:

```scheme
(define-style 'sketchy :roughness 2)
(circle 50 50 30 :style 'sketchy :roughness 1)  ;; roughness=1 wins
```

If neither `:roughness` nor a style with roughness is present, the shape
renders with perfect geometry (the existing code path).

### Default values map to rough.js defaults

| Option | rough.js default | PML default |
|--------|-----------------|-------------|
| roughness | 1 | 0 (opt-in) |
| bowing | 1 | 1 |
| maxRandomnessOffset | 2 | 2 |
| fillStyle | 'hachure' | 'solid' (opt-in) |
| fillWeight | strokeWidth/2 | strokeWidth/2 |
| hachureAngle | -41 | -41 |
| hachureGap | strokeWidth*4 | strokeWidth*4 |
| disableMultiStroke | false | false |

---

## 3. File Changes

### New files

| File | Lines | Purpose |
|------|-------|---------|
| `src/pml/graphics/rough.h` | ~50 | RoughStyleParams struct + RoughRandom (seeded PRNG) |
| `src/pml/graphics/rough.cpp` | ~400 | Perturbation algorithms: line/bezier/ellipse/polygon roughing, fill pattern generation |
| `src/pml/graphics/rough_filler.h` | ~30 | Filler interface: `HachureFiller`, `ZigZagFiller`, `HatchFiller`, `DotFiller`, `DashedFiller` |
| `src/pml/graphics/rough_filler.cpp` | ~300 | Each filler's `fillPolygons()` + scanline hachure engine |

### Modified files

| File | Changes |
|------|---------|
| `src/pml/evaluator/shape_builtins.cpp` | All shape functions resolve roughness from `:style` first, then overlay explicit `:roughness`/`:fill-style`/`:bowing`/`:seed` kwargs; store in metadata |
| `src/pml/sprites/style.cpp` | `define-style` gains optional `:roughness`, `:fill-style`, `:bowing`, `:seed` fields stored in Style metadata |
| `src/pml/backend/skia/skia_backend_draw.cpp` | Add `draw_rough_path()` dispatcher branch; add `draw_rough_circle/rect/ellipse/line/polygon` |
| `src/pml/backend/skia/skia_backend_internal.h` | Add `draw_rough_*` declarations; add `configure_rough_paint()` helper |
| `src/pml/graphics/CMakeLists.txt` | Add `rough.cpp` + `rough_filler.cpp` |

---

## 4. Core Algorithms (Ported from rough.js)

### 4.1 Seeded PRNG (`RoughRandom`)

```
RoughRandom(seed):
  next() -> ((2^31-1) & (seed = 48271 * seed)) / 2^31
```

Identical to rough.js `math.ts:L5-L18`. C++23 `<random>` wrapper that seeds a `minstd_rand1`.

### 4.2 Line perturbation (`_doubleLine` + `_line`)

The heart of rough.js. A single straight line segment is converted to two overlapping bezier curves:

```
Input: P1(x1,y1), P2(x2,y2), options

1. Calculate length-based roughness gain:
   - length < 200: gain = 1
   - length > 500: gain = 0.4
   - else: gain = -0.0016668 * length + 1.233334

2. Calculate randomness offsets:
   offset = roughness * maxRandomnessOffset
   if offset^2 * 100 > length^2: offset = length / 10
   halfOffset = offset / 2

3. Calculate mid-point divergence (bowing):
   divergePoint = 0.2 + random() * 0.2  (20-40% along segment)
   midDispX = bowing * maxRandomnessOffset * (y2 - y1) / 200
   midDispY = bowing * maxRandomnessOffset * (x1 - x2) / 200
   Add random offset to midDisp

4. Generate two bezier curves (first=primary, second=multi-stroke overlay):
   Primary:  move->(P1 + offset)  bcurveTo->(mid+offset)  bcurveTo->(P2 + offset)
   Overlay:  move->(P1 + halfOffset)  bcurveTo->(mid+halfOffset)  bcurveTo->(P2 + halfOffset)
```

Rendered as two overlapping cubic beziers. This is the entire `_line()` function in `renderer.ts:L292-L361`.

### 4.3 Ellipse perturbation (`_computeEllipsePoints`)

The ellipse is sampled at `curveStepCount` (default 9) angular increments, but each sampled point receives:
- Radial offset (`rx`/`ry` perturbation based on `curveFitting`)
- Angular offset (the starting angle is jittered)
- Position offset (each point gets random x/y displacement)

```cpp
// Pseudocode:
auto [allPoints, corePoints] = compute_rough_ellipse(cx, cy, rx, ry, o);

std::vector<Point> points;
double radOffset = randomOffset(0.5) - PI/2;
points.push_back(offsetPoint(cx + 0.9*rx*cos(radOffset - inc), cy + 0.9*ry*sin(radOffset - inc)));
for (angle = radOffset; angle < 2*PI + radOffset; angle += increment) {
    points.push_back(offsetPoint(cx + rx*cos(angle), cy + ry*sin(angle)));
}
// Add overlapping start for seamless closure
points.push_back(offsetPoint(cx + rx*cos(radOffset + 2*PI + overlap*0.5), ...));
// Then run through _curve() (catmull-rom style spline)
```

### 4.4 Curve fitting (`_curve`)

Converts a set of points to bezier curves using Catmull-Rom to cubic bezier conversion. The `curveTightness` parameter controls how tightly the curves follow the points.

```cpp
b[0] = points[i];
b[1] = points[i] + tightness * (points[i+1] - points[i-1]) / 6;
b[2] = points[i+1] + tightness * (points[i] - points[i+2]) / 6;
b[3] = points[i+1];
// Output: bcurveTo(b[1], b[2], b[3])
```

### 4.5 Fill patterns

All fillers operate by generating fill lines within a polygon boundary using a scanline algorithm.

**Scanline Hachure** (from `hachure-fill` npm package → we implement inline):
```
Input: polygon points, gap, angle
1. Rotate polygon by -angle
2. For each scanline Y at gap intervals:
   - Find intersections with polygon edges
   - Sort intersections by X
   - Pair them: (x0,y) → (x1,y) for even-odd pairs
3. Rotate lines back by +angle
4. Return as hatched lines
```

**HachureFiller**: scanline lines + `doubleLineOps` (two overlapping paths per line)

**HatchFiller**: hachure at `angle` + hachure at `angle + 90`

**ZigZagFiller**: hachure lines with zigzag offset:
```
For each hachure line:
  split into two parallel lines offset by gap/2
```

**DotFiller**: hachure intersections → small ellipses at evenly spaced positions

**DashedFiller**: hachure lines → broken into dashes

**SolidFill**: simple polygon fill (no perturbation, just `SkCanvas::drawPath` with the exact polygon)

---

## 5. Implementation Steps

### Step 1: `rough.h` + `rough.cpp` — Core perturbation

- `struct RoughStyleParams` — stores parsed kwargs
  ```cpp
  struct RoughStyleParams {
      double roughness{0.0};
      double bowing{1.0};
      int seed{0};
      double max_randomness_offset{2.0};
      double curve_fitting{0.95};
      double curve_tightness{0.0};
      int curve_step_count{9};
      bool disable_multi_stroke{false};
      bool disable_multi_stroke_fill{false};
      bool preserve_vertices{false};
      double fill_shape_roughness_gain{0.8};
      std::string fill_style{"solid"};
  };
  ```

- `class RoughRandom` — seeded PRNG (minstd_rand1)
- `double rand_offset(double x, RoughRandom& rng, double roughness, double gain=1)` — returns random in [-x, x]
- `double rand_offset_range(double min, double max, RoughRandom& rng, double roughness)` — returns random in [min, max]

- `std::vector<Op> double_line(double x1, double y1, double x2, double y2, RoughStyleParams&, RoughRandom&)`
- `std::vector<Op> curve(std::vector<Point>& pts, RoughStyleParams&, RoughRandom&)`
- `std::vector<Point> compute_rough_ellipse_points(double cx, double cy, double rx, double ry, RoughStyleParams&, RoughRandom&)`

### Step 2: `rough_filler.cpp` — Fill patterns

- `struct Point { double x, y; }` + line intersection math
- `std::vector<std::pair<Point,Point>> scanline_hachure(const std::vector<Point>& polygon, double gap, double angle)`
- `class HachureFiller { fillPolygons(polygons, params) -> OpSet }`
- `class ZigZagFiller : HachureFiller`
- `class HatchFiller : HachureFiller`
- `class DotFiller : PatternFiller`
- `class DashedFiller : PatternFiller`

### Step 3: Modify `shape_builtins.cpp` + `style.cpp`

Each shape function (`builtin_circle`, `builtin_rect`, `builtin_ellipse`, `builtin_line`, `builtin_polygon`):

1. Resolve roughness params from `:style` first (if the named style has
   roughness fields), then overlay explicit kwargs (which win in case of
   conflict).
2. If resolved `roughness > 0`: set shape type prefix `"rough_"`, store
   `RoughStyleParams` in metadata (stroke will be perturbed).
3. If resolved `roughness = 0` but `fill-style != 'solid`: store fill-style in
   metadata only (fill uses rough pattern, stroke stays perfect). Shape type
   remains `"circle"` etc.
4. If both roughness=0 and fill-style='solid (or absent): existing behavior
   unchanged — no rough code path is entered.

`define-style` in `src/pml/sprites/style.cpp` gains a metadata map for optional
roughness fields alongside existing style properties.

Example:
```cpp
// Step 1: resolve from style (if any)
double roughness = 0.0;
std::string fill_style = "solid";
double bowing = 1.0;
int seed = 0;

if (auto style_name = kw_string(kwargs, "style", ""); !style_name.empty()) {
    auto& reg = StyleRegistry::instance();
    if (auto* style = reg.get(style_name)) {
        roughness = kw_double(style->metadata, "roughness", 0.0);
        fill_style = kw_string(style->metadata, "fill_style", "solid");
        bowing = kw_double(style->metadata, "bowing", 1.0);
        seed = static_cast<int>(kw_double(style->metadata, "seed", 0));
    }
}
// Step 2: overlay explicit kwargs (win over style)
roughness = kw_double(kwargs, "roughness", roughness);
fill_style = kw_string(kwargs, "fill-style", fill_style);
bowing = kw_double(kwargs, "bowing", bowing);
seed = static_cast<int>(kw_double(kwargs, "seed", static_cast<double>(seed)));

std::string shape = "circle";
bool rough_stroke = roughness > 0.0;
bool rough_fill = fill_style != "solid";

if (rough_stroke) {
    shape = "rough_circle";
    obj->metadata["roughness"] = Value(roughness);
    obj->metadata["bowing"] = Value(bowing);
    obj->metadata["seed"] = Value(static_cast<double>(seed));
}
if (rough_fill) {
    obj->metadata["fill_style"] = Value(std::string(fill_style));
}
```

### Step 4: Modify `skia_backend_draw.cpp`

- Add draw functions for each rough shape:
  - `draw_rough_circle(SkCanvas*, GraphicObject&, RoughStyleParams, RoughRandom&)`
  - `draw_rough_rect(SkCanvas*, GraphicObject&, RoughStyleParams, RoughRandom&)`
  - `draw_rough_ellipse(SkCanvas*, GraphicObject&, RoughStyleParams, RoughRandom&)`
  - `draw_rough_line(SkCanvas*, GraphicObject&, RoughStyleParams, RoughRandom&)`
  - `draw_rough_polygon(SkCanvas*, GraphicObject&, RoughStyleParams, RoughRandom&)`

- Each function checks two independent flags from metadata:
  1. **rough_stroke** (roughness > 0 in metadata):
     a. Generate perturbed line/curve points using rough algorithms
     b. Build perturbed `SkPath`
  2. **Else**: build exact `SkPath` (same as current code)
  3. **rough_fill** (fill_style != "solid" in metadata):
     a. If set: render fill pattern (hachure/zigzag/etc.) as additional paths
        on top of or behind the stroke path
     b. If not set: render normal solid fill via `SkCanvas::drawPath`
  4. Apply stroke (perturbed or exact, depending on rough_stroke)
  5. `canvas->drawPath(path, paint)`

- The dispatcher in `draw_object()` checks shape_type prefix, but note that a
  `"circle"` with rough_fill uses `draw_exact_circle` for stroke and then
  renders fill patterns on top:
  ```cpp
  if (obj.shape_type == "rough_circle")  return draw_rough_circle(canvas, obj, ...);
  else if (obj.shape_type == "circle")   return draw_exact_circle(canvas, obj, ...);
  // rough_fill-only shapes still pass through draw_exact_circle
  // which checks metadata for fill-style and renders patterns if needed
  ```

### Step 5: Build system + tests

- Add `rough.cpp` and `rough_filler.cpp` to `src/pml/graphics/CMakeLists.txt`
- Add smoke tests in `tests/builtins_smoke.cpp`:
  - Test each shape with roughness=0 → identical to normal shape
  - Test each shape with roughness>0 → output differs (pixel comparison)
  - Test that same seed produces identical output
  - Test fill-styles: hachure, zigzag, cross-hatch, dots, dashed
  - Test `(render (circle 50 50 30 :roughness 1 :fill-style 'hachure) "test.png")`

---

## 6. Rough.js Code Mapping

| rough.js file | PML C++ equivalent | Notes |
|---|---|---|
| `src/core.ts` | `rough.h` → `RoughStyleParams` | Types and options struct |
| `src/math.ts` | `rough.h` → `RoughRandom` | Seeded PRNG (minstd_rand1) |
| `src/geometry.ts` | `rough.h` → `struct Point`, line math | Point math utilities |
| `src/renderer.ts` (lines 256-361) | `rough.cpp` → `_line`, `_double_line` | Line perturbation core |
| `src/renderer.ts` (lines 363-424) | `rough.cpp` → `_curve` | Catmull-rom → bezier |
| `src/renderer.ts` (lines 426-484) | `rough.cpp` → `compute_rough_ellipse` | Ellipse point generation |
| `src/renderer.ts` (lines 486-508) | `rough.cpp` → `_arc` | Arc perturbation |
| `src/fillers/scan-line-hachure.ts` | `rough_filler.cpp` → `scanline_hachure()` | Inline port of `hachure-fill` |
| `src/fillers/hachure-filler.ts` | `rough_filler.cpp` → `HachureFiller` | |
| `src/fillers/zigzag-filler.ts` | `rough_filler.cpp` → `ZigZagFiller` | |
| `src/fillers/hatch-filler.ts` | `rough_filler.cpp` → `HatchFiller` | |
| `src/fillers/dot-filler.ts` | `rough_filler.cpp` → `DotFiller` | |
| `src/fillers/dashed-filler.ts` | `rough_filler.cpp` → `DashedFiller` | |
| `src/generator.ts` | `skia_backend_draw.cpp` (orchestration) | Shape creation + fill/stroke composing |

---

## 7. Reuse Strategy

Direct port of rough.js algorithms, NOT a JS binding:

| Component | Strategy |
|-----------|----------|
| Seeded PRNG | minstd_rand1 (C++11 `<random>`) — simpler than rough.js's hand-rolled LCG |
| `_line()` perturbation | Direct port of `renderer.ts:L292-L361` |
| `_curve()` Catmull-Rom | Direct port of `renderer.ts:L391-L424` |
| `_computeEllipsePoints()` | Direct port of `renderer.ts:L426-L484` |
| Scanline hachure | Port of `hachure-fill` npm package (~150 lines of elegant JS) |
| Fill pattern classes | Port of `fillers/*.ts` — each is ~30-50 lines |
| `linearPath`, `rectangle`, `arc` | Direct port of `renderer.ts:L25-L54` + `L123-L161` |
| `svgPath` | NOT porting initially — PML has its own `path` builtin |

---

## 8. OpSet → SkPath Translation

rough.js uses an intermediate representation (`OpSet`) comprising `move`, `bcurveTo`, `lineTo` ops. C++ can skip this and directly construct `SkPath`:

```cpp
SkPath path;
path.moveTo(x1, y1);
path.cubicTo(cx1, cy1, cx2, cy2, x2, y2);  // bcurveTo
path.lineTo(x, y);
```

The `SkPath` is both faster to construct and directly renderable.

---

## 9. Risk Assessment

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| **roughness=0 (or absent) changes rendering behavior** | **None** | roughness=0 means the shape_type stays `"circle"` (not `"rough_circle"`). The existing Skia `draw_circle` path is entered identically — no new code path runs. Zero risk. |
| **Roughness leaks / inherits across shapes** | None | The `:roughness` kwarg is stored per-GraphicObject in metadata. It is never inherited, merged, or propagated. Each shape's roughness is purely its own. |
| **Fill pattern performance** (scanlines for complex polys) | Medium | Cache per seed+polygon hash; skip for tiny areas |
| **Rendering not pixel-identical to rough.js** | Low | Same math, same defaults; tiny diffs from floating-point rounding |
| **CJK text + rough** | None | Text is never roughened |

---

## 10. Test Plan

1. **Determinism**: Same `:seed` produces identical renderings
2. **Opt-out**: `:roughness 0` → pixel-identical to no rough arg
3. **Fill styles**: All 6 fill styles produce visible pattern
4. **Shape coverage**: circle, rect, ellipse, line, polygon with roughness
5. **Regression**: 134/134 existing smoke tests still pass
6. **Visual**: Manual inspection of render outputs confirms sketchy appearance

---

## 11. Effort Estimate

| Step | Files | Estimated LOC | Time |
|------|-------|--------------|------|
| rough.h (types, PRNG) | 1 new | 80 | 30min |
| rough.cpp (line/ellipse/curve perturbation) | 1 new | 300 | 2h |
| rough_filler.cpp (fill patterns) | 1 new | 350 | 2h |
| shape_builtins.cpp modification | 1 mod | +50 | 30min |
| skia_backend_draw.cpp modification | 1 mod | +300 | 2h |
| CMakeLists.txt + smoke tests | 2 mod | +80 | 30min |
| **Total** | 7 files | ~1160 | **~8h** |

