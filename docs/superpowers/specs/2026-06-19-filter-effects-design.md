# Filter / Effect / Adjustment (Sub-project B) Design

**Goal:** Add professional, Photoshop-grade image filters, color adjustments and layer styles to PML, exposed as first-class built-ins and attachable to `Layer` via a chainable `:filter` property.

**Status:** Design approved — scope = extensible filter engine + color adjustments + levels/curves/threshold/posterize + blur/sharpen/convolution + layer styles (drop/inner shadow, outer/inner glow, bevel/emboss).

---

## 1. Architecture

We introduce a new `pml/filter` module between `pml/backend` and `pml/layer`.

```
pml/filter/
├── image_filter.h          # ImageFilter + FilterChain base classes
├── filter_backend.h        # FilterBackend interface (implemented by RenderBackend)
├── adjustments.h/.cpp      # ColorMatrix, Levels, Curves, Threshold, Posterize
├── convolution.h/.cpp      # Blur, Sharpen, EdgeDetect, Emboss, Custom kernel
├── layer_style.h/.cpp      # DropShadow, InnerShadow, OuterGlow, InnerGlow, BevelEmboss
└── filter_builtins.h/.cpp  # PML builtin registration
```

The `RenderBackend` interface is extended to inherit from `FilterBackend`.  The Skia backend implements every method with native `SkImageFilters` / `SkColorFilters`, giving GPU-ready quality.  The Null backend returns a `filter_not_supported` error for accelerated ops.

A filter is a first-class `Value` alternative, so users can define, compose and reuse filters:

```pml
(define photo-filter
  (filter-chain
    (color-adjust :brightness 0.1 :contrast 1.1 :saturation 0.9)
    (blur :radius 2.0)))

(make-layer "portrait" (circle 64 64 60 :fill "image.png")
            :filter photo-filter)
```

## 2. Core Abstractions

### 2.1 ImageFilter

```cpp
class ImageFilter {
public:
    virtual ~ImageFilter() = default;
    [[nodiscard]] virtual Result<void> apply(FilterBackend& backend, Surface& surface) const = 0;
    [[nodiscard]] virtual std::string name() const = 0;
};
```

`FilterChain` is an `ImageFilter` that owns a vector of child filters and applies them in order.

### 2.2 FilterBackend

`FilterBackend` lives in `pml/filter/filter_backend.h` and is inherited by `RenderBackend`.  It exposes the minimal set of accelerated primitives needed by the concrete filters:

```cpp
class FilterBackend {
public:
    virtual ~FilterBackend() = default;

    // 5x4 color matrix (row-major, RGBA offset + alpha scale/offset).
    virtual Result<void> apply_color_matrix(Surface&, const std::array<float,20>&) = 0;

    // Separable Gaussian / box / motion blur.
    virtual Result<void> apply_blur(Surface&, float radius_x, float radius_y,
                                    BlurType type) = 0;

    // Generic matrix convolution.
    virtual Result<void> apply_convolution(Surface&, const ConvolutionKernel&) = 0;

    // 1D lookup tables per channel (RGBA).
    virtual Result<void> apply_color_table(Surface&,
        const std::array<uint8_t,256>& r,
        const std::array<uint8_t,256>& g,
        const std::array<uint8_t,256>& b,
        const std::array<uint8_t,256>& a) = 0;

    // Offset / displacement.
    virtual Result<void> apply_offset(Surface&, float dx, float dy) = 0;

    // Layer-style primitives.
    virtual Result<void> apply_drop_shadow(Surface&, float dx, float dy,
        float blur_x, float blur_y, uint32_t color) = 0;
    virtual Result<void> apply_inner_shadow(Surface&, float dx, float dy,
        float blur, uint32_t color) = 0;
    virtual Result<void> apply_outer_glow(Surface&, float blur, uint32_t color) = 0;
    virtual Result<void> apply_inner_glow(Surface&, float blur, uint32_t color) = 0;
    virtual Result<void> apply_bevel_emboss(Surface&, float angle, float altitude,
        float blur, uint32_t highlight, uint32_t shadow) = 0;
};
```

### 2.3 Skia mapping

- `apply_color_matrix` → `SkImageFilters::ColorFilter(SkColorFilters::Matrix(...))`
- `apply_blur` (gaussian) → `SkImageFilters::Blur(...)`
- `apply_blur` (box/motion) → `SkImageFilters::MatrixConvolution` with uniform or directional kernel
- `apply_convolution` → `SkImageFilters::MatrixConvolution`
- `apply_color_table` → `SkColorFilters::Table(...)` wrapped in `SkImageFilters::ColorFilter`
- `apply_offset` → `SkImageFilters::Offset`
- `apply_drop_shadow` → `SkImageFilters::DropShadow` / `DropShadowOnly`
- `apply_inner_shadow/outer_glow/inner_glow/bevel_emboss` → composed `SkImageFilters` + alpha masks

## 3. Filter Catalog

| PML builtin | Returns | Notes |
|-------------|---------|-------|
| `(color-adjust [:brightness n] [:contrast n] [:saturation n] [:hue n] [:exposure n] [:gamma n] [:temperature n] [:tint n] [:grayscale bool] [:sepia bool] [:invert bool])` | ImageFilter | Builds a single 5x4 color matrix. |
| `(levels in-low in-gamma in-high out-low out-high)` | ImageFilter | Standard levels adjustment. |
| `(curves channel points)` | ImageFilter | `channel` = `r/g/b/rgb`; `points` = list of `(x y)` control points, interpolated to a 256-entry LUT. |
| `(threshold value)` | ImageFilter | Pixels above value become white, else black. |
| `(posterize levels)` | ImageFilter | Reduce to `levels` discrete values per channel. |
| `(blur :radius n [:type 'gaussian/box/motion] [:angle n])` | ImageFilter | Default gaussian. |
| `(sharpen [:amount n] [:radius n])` | ImageFilter | Unsharp-mask style kernel. |
| `(edge-detect [:type 'sobel/laplacian])` | ImageFilter | Convolution-based edge detection. |
| `(emboss [:direction n])` | ImageFilter | Emboss convolution kernel. |
| `(convolve kernel [:offset n] [:divisor n])` | ImageFilter | `kernel` is a flat list of weights; auto-size = sqrt(len). |
| `(drop-shadow dx dy blur color)` | ImageFilter | Layer-style drop shadow. |
| `(inner-shadow dx dy blur color)` | ImageFilter | Inner shadow. |
| `(outer-glow blur color)` | ImageFilter | Outer glow. |
| `(inner-glow blur color)` | ImageFilter | Inner glow. |
| `(bevel-emboss angle altitude blur highlight shadow)` | ImageFilter | Bevel/emboss. |
| `(filter-chain f1 f2 ...)` | ImageFilter | Sequential chain. |

## 4. Value / Layer / Compositor Integration

1. Add `ImageFilter` forward declaration to `pml/core/types.h`.
2. Add `ImageFilter` to `Box::Kind` and `Box::data` variant.
3. Add Value constructor / predicate / accessor.
4. Extend `LayerProperties` with `std::vector<std::shared_ptr<ImageFilter>> filters`.
5. Add `Layer::with_filter(ImageFilter)` / `with_filters(std::vector<...>)` methods.
6. Extend `make-layer` and `layer-with` to parse `:filter` (single filter or list of filters).
7. In `Compositor::render_layer`, after rendering the layer surface and before applying the mask, run the filter chain in order.

Filter application order:

```
render layer content → apply filter chain → apply mask → composite
```

This matches Photoshop: pixel filters affect the layer contents, then masks modulate transparency.

## 5. Error Handling

New error codes / factories in `pml/core/error.h`:

- `ErrorCode::FilterError` — generic filter failure.
- `ErrorCode::FilterNotSupported` — backend cannot run a given filter.
- Factory functions: `filter_error(...)`, `filter_not_supported(...)`.

A filter returns `Result<void>`; the compositor propagates errors through `Result<std::unique_ptr<Surface>>`.

## 6. Testing Strategy

- **Unit tests** in `tests/test_filter.cpp`:
  - Mock `FilterBackend` that records which primitives were invoked.
  - Verify `ColorMatrixFilter`, `BlurFilter`, `ConvolutionFilter`, `LevelsFilter`, `FilterChain` call the correct backend methods.
  - Verify `Layer::with_filter` stores filters and `Compositor::render_layer` applies them.
- **Smoke tests** in `tests/builtins_smoke.cpp`:
  - `(filter? (blur :radius 3.0))` → `#t`
  - `(layer? (layer-with l :filter (blur :radius 1.0)))` → `#t`
  - `(filter? (filter-chain (invert) (grayscale)))` → `#t`
- **End-to-end CLI**:
  - A sample PML that creates a composition with filtered layers and renders to `test_filter_out/filtered.png`.

## 7. File Layout

```
src/pml/filter/CMakeLists.txt
src/pml/filter/image_filter.h
src/pml/filter/filter_backend.h
src/pml/filter/adjustments.h
src/pml/filter/adjustments.cpp
src/pml/filter/convolution.h
src/pml/filter/convolution.cpp
src/pml/filter/layer_style.h
src/pml/filter/layer_style.cpp
src/pml/filter/filter_builtins.h
src/pml/filter/filter_builtins.cpp

tests/test_filter.cpp
```

## 8. Non-Goals

- Non-destructive adjustment layers (can be emulated with group layers + filters; explicit adjustment layer type is out of scope).
- CMYK / LAB color spaces (RGBA only).
- Content-aware fill / healing / cloning.
- 3D / lighting effects beyond basic bevel/emboss.

## 9. Dependencies

- `pml_filter` depends on `pml_backend` and `pml_graphics`.
- `pml_layer` depends on `pml_filter`.
- `pml_api` / CLI link `pml_filter` transitively.
