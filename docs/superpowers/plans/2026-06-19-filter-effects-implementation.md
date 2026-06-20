# Filter / Effect / Adjustment (Sub-project B) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a first-class, extensible image-filter engine to PML with professional color adjustments, levels/curves/threshold/posterize, blur/convolution/sharpen/edge detection/emboss, and Photoshop-style layer styles, attachable to `Layer` via `:filter`.

**Architecture:** A new `pml/filter` module defines `ImageFilter` / `FilterChain` and `FilterBackend`. `RenderBackend` inherits `FilterBackend`; the Skia backend implements each primitive with `SkImageFilters` / `SkColorFilters`. Filters become a new `Value` alternative; `LayerProperties` stores a filter chain applied by the compositor after rasterising a layer.

**Tech Stack:** C++23, Skia prebuilt static library, `std::expected` for errors, existing `pml/backend`, `pml/layer`, `pml/core`.

---

## Task 1: Add filter error codes

**Files:**
- Modify: `src/pml/core/error.h`

- [ ] **Step 1: Add `FilterError` / `FilterNotSupported` to `ErrorCode` and factory functions**

```cpp
enum class ErrorCode {
    // ... existing ...
    FilterError,
    FilterNotSupported,
};

inline PMLException filter_error(const std::string& msg,
                                 std::optional<SourceLocation> loc = std::nullopt) {
    return PMLException(ErrorCode::FilterError, msg, loc);
}
inline PMLException filter_not_supported(const std::string& msg,
                                         std::optional<SourceLocation> loc = std::nullopt) {
    return PMLException(ErrorCode::FilterNotSupported, msg, loc);
}
```

- [ ] **Step 2: Build to check `pml_core`**

Run: `cmake --build --preset debug --target pml_core`
Expected: builds cleanly.

---

## Task 2: Extend `Value` to support `ImageFilter`

**Files:**
- Modify: `src/pml/core/types.h`
- Modify: `src/pml/core/types.cpp`

- [ ] **Step 1: Add forward declaration and `Box` kind**

In `src/pml/core/types.h`, after `class Composition;` add:

```cpp
class ImageFilter;           // pml/filter/image_filter.h
```

In `Box::Kind` enum add `ImageFilter`; in the variant add `std::shared_ptr<ImageFilter>`.

- [ ] **Step 2: Add Value constructor, predicate, accessor**

```cpp
Value(std::shared_ptr<ImageFilter> v);
[[nodiscard]] bool is_image_filter() const noexcept;
[[nodiscard]] const std::shared_ptr<ImageFilter>* as_image_filter() const noexcept;
```

- [ ] **Step 3: Implement in `types.cpp`**

Follow the existing pattern for `Layer` / `Composition`:

```cpp
Value::Value(std::shared_ptr<ImageFilter> v)
    : tag_(Tag::Object), box_(std::make_shared<Box>(Box{Box::Kind::ImageFilter, std::move(v)})) {}

bool Value::is_image_filter() const noexcept { return box_kind() == Box::Kind::ImageFilter; }
const std::shared_ptr<ImageFilter>* Value::as_image_filter() const noexcept {
    if (!box_ || box_->kind != Box::Kind::ImageFilter) return nullptr;
    return std::get_if<std::shared_ptr<ImageFilter>>(&box_->data);
}
```

Also update `value_to_string` to return `"#<filter>"`.

- [ ] **Step 4: Build `pml_core`**

Run: `cmake --build --preset debug --target pml_core`
Expected: builds cleanly.

---

## Task 3: Create `ImageFilter` and `FilterBackend` abstractions

**Files:**
- Create: `src/pml/filter/image_filter.h`
- Create: `src/pml/filter/filter_backend.h`

- [ ] **Step 1: Create `src/pml/filter/image_filter.h`**

```cpp
#pragma once

#include "pml/core/error.h"
#include "pml/backend/backend.h"

#include <memory>
#include <string>
#include <vector>

namespace pml {

class FilterBackend;

class ImageFilter {
public:
    virtual ~ImageFilter() = default;
    [[nodiscard]] virtual Result<void> apply(FilterBackend& backend, Surface& surface) const = 0;
    [[nodiscard]] virtual std::string name() const = 0;
};

class FilterChain final : public ImageFilter {
public:
    explicit FilterChain(std::vector<std::shared_ptr<ImageFilter>> filters);

    [[nodiscard]] Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;
    [[nodiscard]] const std::vector<std::shared_ptr<ImageFilter>>& filters() const noexcept;

private:
    std::vector<std::shared_ptr<ImageFilter>> m_filters;
};

} // namespace pml
```

- [ ] **Step 2: Create `src/pml/filter/filter_backend.h`**

```cpp
#pragma once

#include "pml/core/error.h"
#include "pml/backend/backend.h"

#include <array>
#include <cstdint>
#include <vector>

namespace pml {

enum class BlurType : uint8_t { Gaussian, Box, Motion };

struct ConvolutionKernel {
    int width{};
    int height{};
    std::vector<float> values; // row-major, size = width*height
    float offset{0.0f};
    float divisor{1.0f};
    int anchor_x{-1}; // -1 == center
    int anchor_y{-1};
};

class FilterBackend {
public:
    virtual ~FilterBackend() = default;

    virtual Result<void> apply_color_matrix(Surface&, const std::array<float,20>&) = 0;
    virtual Result<void> apply_blur(Surface&, float radius_x, float radius_y, BlurType) = 0;
    virtual Result<void> apply_convolution(Surface&, const ConvolutionKernel&) = 0;
    virtual Result<void> apply_color_table(Surface&,
        const std::array<uint8_t,256>& r,
        const std::array<uint8_t,256>& g,
        const std::array<uint8_t,256>& b,
        const std::array<uint8_t,256>& a) = 0;
    virtual Result<void> apply_offset(Surface&, float dx, float dy) = 0;

    virtual Result<void> apply_drop_shadow(Surface&, float dx, float dy,
        float blur_x, float blur_y, uint32_t color) = 0;
    virtual Result<void> apply_inner_shadow(Surface&, float dx, float dy,
        float blur, uint32_t color) = 0;
    virtual Result<void> apply_outer_glow(Surface&, float blur, uint32_t color) = 0;
    virtual Result<void> apply_inner_glow(Surface&, float blur, uint32_t color) = 0;
    virtual Result<void> apply_bevel_emboss(Surface&, float angle, float altitude,
        float blur, uint32_t highlight, uint32_t shadow) = 0;
};

} // namespace pml
```

- [ ] **Step 3: Implement `FilterChain` in `src/pml/filter/image_filter.cpp`**

```cpp
#include "image_filter.h"
#include "filter_backend.h"

namespace pml {

FilterChain::FilterChain(std::vector<std::shared_ptr<ImageFilter>> filters)
    : m_filters(std::move(filters)) {}

Result<void> FilterChain::apply(FilterBackend& backend, Surface& surface) const {
    for (const auto& f : m_filters) {
        if (!f) continue;
        auto r = f->apply(backend, surface);
        if (!r) return r;
    }
    return {};
}

std::string FilterChain::name() const {
    return "filter-chain";
}

const std::vector<std::shared_ptr<ImageFilter>>& FilterChain::filters() const noexcept {
    return m_filters;
}

} // namespace pml
```

- [ ] **Step 4: Create `src/pml/filter/CMakeLists.txt`**

```cmake
pml_add_library(pml_filter
    image_filter.cpp
    adjustments.cpp
    convolution.cpp
    layer_style.cpp
    filter_builtins.cpp
)
target_link_libraries(pml_filter PUBLIC pml_backend pml_graphics pml_core)
```

- [ ] **Step 5: Build `pml_filter`**

Run: `cmake --build --preset debug --target pml_filter`
Expected: builds cleanly.

---

## Task 4: Wire `FilterBackend` into `RenderBackend` and Null backend

**Files:**
- Modify: `src/pml/backend/backend.h`
- Modify: `src/pml/backend/null_backend.cpp`

- [ ] **Step 1: Make `RenderBackend` inherit `FilterBackend`**

In `src/pml/backend/backend.h`:

```cpp
#include "pml/filter/filter_backend.h"

class RenderBackend : public FilterBackend {
    // ... existing methods ...
};
```

- [ ] **Step 2: Add Null backend stubs**

In `src/pml/backend/null_backend.cpp`, add overrides that return `filter_not_supported`:

```cpp
Result<void> apply_color_matrix(Surface&, const std::array<float,20>&) override {
    return std::unexpected(filter_not_supported("null backend: color matrix not supported"));
}
Result<void> apply_blur(Surface&, float, float, BlurType) override {
    return std::unexpected(filter_not_supported("null backend: blur not supported"));
}
// ... etc for all FilterBackend methods ...
```

- [ ] **Step 3: Build `pml_backend`**

Run: `cmake --build --preset debug --target pml_backend`
Expected: builds cleanly.

---

## Task 5: Implement `FilterBackend` in the Skia backend

**Files:**
- Modify: `src/pml/backend/skia/skia_backend.cpp`

- [ ] **Step 1: Add helper to apply an `SkImageFilter` to a surface in-place**

```cpp
Result<void> apply_sk_image_filter(SkiaSurface& surf, sk_sp<SkImageFilter> filter) {
    if (!filter) return {};
    auto tmp = SkSurfaces::Raster(surf.bitmap.info());
    if (!tmp) return std::unexpected(filter_error("skia: failed to create filter scratch surface"));
    SkPaint paint;
    paint.setImageFilter(std::move(filter));
    tmp->getCanvas()->drawImage(surf.bitmap.asImage().get(), 0, 0, SkSamplingOptions(), &paint);
    surf.bitmap = tmp->makeImageSnapshot()->asImage()->?; // TODO: read carefully
    return {};
}
```

> Implementation note: because `SkiaSurface` owns `SkBitmap bitmap` and `sk_sp<SkSurface> surface`, the safest in-place path is:
> 1. Create a temporary `SkSurface` of the same size.
> 2. Draw `surf.bitmap.asImage()` into it with the image filter.
> 3. Copy the temporary bitmap's pixels back into `surf.bitmap` using `surf.bitmap.writePixels(tmp->peekPixels())` or by replacing the `SkSurface` wrapper.  Use `SkPixmap` + `memcpy` if necessary.

- [ ] **Step 2: Implement each `FilterBackend` method**

Use the Skia APIs listed in the design doc:

```cpp
Result<void> apply_color_matrix(Surface& s, const std::array<float,20>& m) override {
    auto* surf = dynamic_cast<SkiaSurface*>(&s);
    if (!surf) return std::unexpected(filter_error("skia: invalid surface"));
    sk_sp<SkColorFilter> cf = SkColorFilters::Matrix(m.data());
    sk_sp<SkImageFilter> imf = SkImageFilters::ColorFilter(std::move(cf), nullptr);
    return apply_sk_image_filter(*surf, std::move(imf));
}

Result<void> apply_blur(Surface& s, float rx, float ry, BlurType type) override {
    auto* surf = dynamic_cast<SkiaSurface*>(&s);
    if (!surf) return std::unexpected(filter_error("skia: invalid surface"));
    if (type == BlurType::Gaussian) {
        auto imf = SkImageFilters::Blur(rx, ry, SkTileMode::kDecal, nullptr);
        return apply_sk_image_filter(*surf, std::move(imf));
    }
    // Box / motion implemented via apply_convolution in Task 6.
    return std::unexpected(filter_not_supported("skia: blur type not supported"));
}

Result<void> apply_convolution(Surface& s, const ConvolutionKernel& k) override {
    auto* surf = dynamic_cast<SkiaSurface*>(&s);
    if (!surf) return std::unexpected(filter_error("skia: invalid surface"));
    SkISize size{k.width, k.height};
    SkScalar gain = k.divisor != 0.0f ? 1.0f / k.divisor : 1.0f;
    SkIPoint bias{static_cast<int32_t>(k.offset), 0};
    SkIPoint target{k.anchor_x >= 0 ? k.anchor_x : k.width/2,
                    k.anchor_y >= 0 ? k.anchor_y : k.height/2};
    std::vector<SkScalar> kernel(k.values.begin(), k.values.end());
    auto imf = SkImageFilters::MatrixConvolution(
        size, kernel.data(), gain, bias, target,
        SkTileMode::kDecal, false, nullptr);
    return apply_sk_image_filter(*surf, std::move(imf));
}

Result<void> apply_color_table(Surface& s,
    const std::array<uint8_t,256>& r,
    const std::array<uint8_t,256>& g,
    const std::array<uint8_t,256>& b,
    const std::array<uint8_t,256>& a) override {
    auto* surf = dynamic_cast<SkiaSurface*>(&s);
    if (!surf) return std::unexpected(filter_error("skia: invalid surface"));
    sk_sp<SkColorFilter> cf = SkColorFilters::Table(
        const_cast<uint8_t*>(a.data()),
        const_cast<uint8_t*>(r.data()),
        const_cast<uint8_t*>(g.data()),
        const_cast<uint8_t*>(b.data()));
    sk_sp<SkImageFilter> imf = SkImageFilters::ColorFilter(std::move(cf), nullptr);
    return apply_sk_image_filter(*surf, std::move(imf));
}

Result<void> apply_offset(Surface& s, float dx, float dy) override {
    auto* surf = dynamic_cast<SkiaSurface*>(&s);
    if (!surf) return std::unexpected(filter_error("skia: invalid surface"));
    auto imf = SkImageFilters::Offset(dx, dy, nullptr);
    return apply_sk_image_filter(*surf, std::move(imf));
}

Result<void> apply_drop_shadow(Surface& s, float dx, float dy,
    float blur_x, float blur_y, uint32_t color) override {
    auto* surf = dynamic_cast<SkiaSurface*>(&s);
    if (!surf) return std::unexpected(filter_error("skia: invalid surface"));
    auto imf = SkImageFilters::DropShadow(dx, dy, blur_x, blur_y, to_sk_color(color), nullptr);
    return apply_sk_image_filter(*surf, std::move(imf));
}

Result<void> apply_inner_shadow(Surface&, float, float, float, uint32_t) override {
    return std::unexpected(filter_not_supported("skia: inner-shadow not yet implemented"));
}
Result<void> apply_outer_glow(Surface&, float, uint32_t) override {
    return std::unexpected(filter_not_supported("skia: outer-glow not yet implemented"));
}
Result<void> apply_inner_glow(Surface&, float, uint32_t) override {
    return std::unexpected(filter_not_supported("skia: inner-glow not yet implemented"));
}
Result<void> apply_bevel_emboss(Surface&, float, float, float, uint32_t, uint32_t) override {
    return std::unexpected(filter_not_supported("skia: bevel-emboss not yet implemented"));
}
```

- [ ] **Step 3: Build Skia backend**

Run: `cmake --build --preset debug --target pml_skia_backend`
Expected: builds cleanly (some layer-style methods may intentionally return not-supported for the first pass).

---

## Task 6: Implement adjustment filters

**Files:**
- Create: `src/pml/filter/adjustments.h`
- Create: `src/pml/filter/adjustments.cpp`

- [ ] **Step 1: Define classes in `adjustments.h`**

```cpp
#pragma once
#include "image_filter.h"
#include "filter_backend.h"
#include <array>
#include <vector>

namespace pml {

struct ColorAdjustParams {
    double brightness = 0.0;
    double contrast   = 1.0;
    double saturation = 1.0;
    double hue        = 0.0;
    double exposure   = 0.0;
    double gamma      = 1.0;
    double temperature= 0.0;
    double tint       = 0.0;
    bool grayscale    = false;
    bool sepia        = false;
    bool invert       = false;
};

class ColorMatrixFilter final : public ImageFilter {
public:
    explicit ColorMatrixFilter(ColorAdjustParams p);
    Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    std::string name() const override;
private:
    ColorAdjustParams m_params;
};

class LevelsFilter final : public ImageFilter {
public:
    LevelsFilter(double in_low, double gamma, double in_high,
                 double out_low, double out_high);
    Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    std::string name() const override;
private:
    double m_in_low, m_gamma, m_in_high, m_out_low, m_out_high;
};

class CurvesFilter final : public ImageFilter {
public:
    // channel: 0=R,1=G,2=B,3=RGB
    CurvesFilter(int channel, std::vector<std::pair<uint8_t,uint8_t>> points);
    Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    std::string name() const override;
private:
    int m_channel;
    std::vector<std::pair<uint8_t,uint8_t>> m_points;
};

class ThresholdFilter final : public ImageFilter {
public:
    explicit ThresholdFilter(uint8_t value);
    Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    std::string name() const override;
private:
    uint8_t m_value;
};

class PosterizeFilter final : public ImageFilter {
public:
    explicit PosterizeFilter(int levels);
    Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    std::string name() const override;
private:
    int m_levels;
};

} // namespace pml
```

- [ ] **Step 2: Implement math helpers and apply methods**

`ColorMatrixFilter::apply` builds a 5x4 matrix by composing:
- brightness/contrast offsets and scales,
- saturation (RGB→luminance weights),
- hue rotation,
- exposure/gamma via matrix approximation (or defer to `apply_color_table` if needed),
- grayscale / sepia / invert final matrices.

Multiply the partial 5x4 matrices into one row-major `std::array<float,20>` and call `backend.apply_color_matrix(surface, matrix)`.

`LevelsFilter::apply` builds a 256-entry LUT and calls `apply_color_table`.

`CurvesFilter::apply` interpolates control points to a 256-entry LUT and calls `apply_color_table`.

`ThresholdFilter::apply` builds LUT where output = 0 if input < value else 255, then calls `apply_color_table`.

`PosterizeFilter::apply` builds LUT with `levels` discrete steps.

- [ ] **Step 3: Build `pml_filter`**

Run: `cmake --build --preset debug --target pml_filter`
Expected: builds cleanly.

---

## Task 7: Implement convolution filters

**Files:**
- Create: `src/pml/filter/convolution.h`
- Create: `src/pml/filter/convolution.cpp`

- [ ] **Step 1: Define classes**

```cpp
#pragma once
#include "image_filter.h"
#include "filter_backend.h"

namespace pml {

enum class BlurFilterType : uint8_t { Gaussian, Box, Motion };
enum class EdgeDetectType : uint8_t { Sobel, Laplacian };

class BlurFilter final : public ImageFilter {
public:
    BlurFilter(float radius, BlurFilterType type, float angle);
    Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    std::string name() const override;
private:
    float m_radius, m_angle;
    BlurFilterType m_type;
};

class SharpenFilter final : public ImageFilter {
public:
    SharpenFilter(float amount, float radius);
    Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    std::string name() const override;
private:
    float m_amount, m_radius;
};

class EdgeDetectFilter final : public ImageFilter {
public:
    explicit EdgeDetectFilter(EdgeDetectType type);
    Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    std::string name() const override;
private:
    EdgeDetectType m_type;
};

class EmbossFilter final : public ImageFilter {
public:
    explicit EmbossFilter(float direction);
    Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    std::string name() const override;
private:
    float m_direction;
};

class ConvolutionFilter final : public ImageFilter {
public:
    ConvolutionFilter(int w, int h, std::vector<float> values,
                      float offset, float divisor);
    Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    std::string name() const override;
private:
    ConvolutionKernel m_kernel;
};

} // namespace pml
```

- [ ] **Step 2: Implement apply methods**

`BlurFilter::apply`:
- Gaussian / motion → call `backend.apply_blur(...)`.
- Box blur → build a uniform `ConvolutionKernel` and call `apply_convolution`.

`SharpenFilter::apply` builds an unsharp-mask kernel.

`EdgeDetectFilter::apply` builds Sobel or Laplacian kernels.

`EmbossFilter::apply` builds a directional emboss kernel.

`ConvolutionFilter::apply` forwards the stored `ConvolutionKernel` to `backend.apply_convolution`.

- [ ] **Step 3: Build `pml_filter`**

Run: `cmake --build --preset debug --target pml_filter`
Expected: builds cleanly.

---

## Task 8: Implement layer-style filters

**Files:**
- Create: `src/pml/filter/layer_style.h`
- Create: `src/pml/filter/layer_style.cpp`

- [ ] **Step 1: Define classes**

```cpp
#pragma once
#include "image_filter.h"
#include "filter_backend.h"

namespace pml {

class DropShadowFilter final : public ImageFilter { /* dx, dy, blur, color */ };
class InnerShadowFilter final : public ImageFilter { /* dx, dy, blur, color */ };
class OuterGlowFilter final : public ImageFilter { /* blur, color */ };
class InnerGlowFilter final : public ImageFilter { /* blur, color */ };
class BevelEmbossFilter final : public ImageFilter { /* angle, altitude, blur, highlight, shadow */ };

} // namespace pml
```

- [ ] **Step 2: Implement `apply` methods**

For the first pass, layer styles may call the matching `FilterBackend` primitive.  If a primitive returns `filter_not_supported`, the filter returns that error.  Drop shadow is implemented immediately via `apply_drop_shadow`; the others can initially return `filter_not_supported` until Skia composition is added.

- [ ] **Step 3: Build `pml_filter`**

Run: `cmake --build --preset debug --target pml_filter`
Expected: builds cleanly.

---

## Task 9: Integrate filters into `Layer` and `Compositor`

**Files:**
- Modify: `src/pml/layer/layer.h`
- Modify: `src/pml/layer/layer.cpp`
- Modify: `src/pml/layer/compositor.cpp`
- Modify: `src/pml/layer/layer_builtins.cpp`

- [ ] **Step 1: Add filter storage to `LayerProperties`**

```cpp
struct LayerProperties {
    // ... existing fields ...
    std::vector<std::shared_ptr<ImageFilter>> filters;
};
```

- [ ] **Step 2: Add `Layer` mutators**

```cpp
Layer with_filter(std::shared_ptr<ImageFilter> filter) const;
Layer with_filters(std::vector<std::shared_ptr<ImageFilter>> filters) const;
```

Implement by copying `m_props` and replacing the `filters` vector.

- [ ] **Step 3: Apply filters in the compositor**

In `Compositor::render_layer`, after `surface_result` is valid and before the mask block:

```cpp
for (const auto& filter : layer.properties().filters) {
    if (!filter) continue;
    auto fr = filter->apply(m_backend, **surface_result);
    if (!fr) return std::unexpected(fr.error());
}
```

- [ ] **Step 4: Parse `:filter` in layer builtins**

Add a helper `parse_filters_kw` that accepts either a single ImageFilter value or a list of ImageFilter values and returns `std::vector<std::shared_ptr<ImageFilter>>`.

Use it in `builtin_make_layer` (after parsing other kwargs) and `builtin_layer_with`:

```cpp
if (auto filters = parse_filters_kw(kwargs, "filter")) {
    layer = layer.with_filters(std::move(*filters));
}
```

- [ ] **Step 5: Build `pml_layer`**

Run: `cmake --build --preset debug --target pml_layer`
Expected: builds cleanly.

---

## Task 10: Implement PML builtins

**Files:**
- Create: `src/pml/filter/filter_builtins.h`
- Create: `src/pml/filter/filter_builtins.cpp`
- Modify: `src/pml/api/api.cpp`

- [ ] **Step 1: Create filter builtins**

Implement one C++ function per builtin from the catalog (Task 3 of the design doc).  Use `parse_kwargs` / `kw_string` / `kw_int` helpers from `pml/graphics/render.cpp` where appropriate.  Build `ColorAdjustParams`, kernels, etc., and return `Value(std::make_shared<...Filter>(...))`.

Add a `filter?` predicate.

Register all functions in `register_filter_builtins(std::shared_ptr<Environment>)`.

- [ ] **Step 2: Register in `PMLRuntime::init_global_env()`**

In `src/pml/api/api.cpp`, after `register_layer_builtins`:

```cpp
register_filter_builtins(m_env);
```

Include `pml/filter/filter_builtins.h` at the top.

- [ ] **Step 3: Build `pml_api` and `pml`**

Run: `cmake --build --preset debug --target pml_api pml`
Expected: builds cleanly.

---

## Task 11: CMake wiring

**Files:**
- Create: `src/pml/filter/CMakeLists.txt` (already created in Task 3)
- Modify: `src/pml/CMakeLists.txt`
- Modify: `src/pml/layer/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Add `filter` subdirectory**

In `src/pml/CMakeLists.txt`, add:

```cmake
add_subdirectory(filter)
```

- [ ] **Step 2: Link `pml_layer` against `pml_filter`**

In `src/pml/layer/CMakeLists.txt`:

```cmake
target_link_libraries(pml_layer PUBLIC pml_filter)
```

- [ ] **Step 3: Link tests**

In `tests/CMakeLists.txt`, add `pml_filter` to the link libraries for `pml-tests`, `pml-layer-test`, and `pml-builtins-smoke`.

- [ ] **Step 4: Re-configure and build all**

Run:
```powershell
cmake --preset debug
cmake --build --preset debug
```
Expected: full project builds cleanly.

---

## Task 12: Add unit tests

**Files:**
- Create: `tests/test_filter.cpp`

- [ ] **Step 1: Create mock `FilterBackend`**

```cpp
class MockFilterBackend : public FilterBackend {
public:
    // Override all methods to record calls; return success.
    Result<void> apply_color_matrix(Surface&, const std::array<float,20>&) override {
        calls.push_back("color-matrix"); return {};
    }
    // ... etc ...
    std::vector<std::string> calls;
};
```

- [ ] **Step 2: Write tests**

```cpp
TEST(FilterTest, ColorMatrixFilterCallsBackend) {
    MockFilterBackend backend;
    ColorAdjustParams p{.brightness=0.1, .contrast=1.2};
    ColorMatrixFilter f(p);
    SkiaSurface surf(10,10,0); // or a minimal Surface subclass
    auto r = f.apply(backend, surf);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(backend.calls, std::vector<std::string>{"color-matrix"});
}

TEST(FilterTest, FilterChainAppliesInOrder) { /* ... */ }
TEST(FilterTest, LayerWithFilterStoresFilter) { /* ... */ }
```

- [ ] **Step 3: Build and run `pml-filter-test`**

Run:
```powershell
cmake --build --preset debug --target pml-filter-test
.\build\debug\tests\Debug\pml-filter-test.exe
```
Expected: all tests pass.

---

## Task 13: Add smoke tests

**Files:**
- Modify: `tests/builtins_smoke.cpp`

- [ ] **Step 1: Add filter smoke checks**

After the existing layer checks, add:

```cpp
CHECK("filter-blur", "(filter? (blur :radius 3.0))", "#t");
CHECK("filter-color-adjust", "(filter? (color-adjust :brightness 0.1))", "#t");
CHECK("filter-chain", "(filter? (filter-chain (color-adjust :invert #t) (blur :radius 1.0)))", "#t");
CHECK("layer-with-filter",
      "(let ((l (make-layer \"l\" (circle 0 0 10))))"
      "  (layer? (layer-with l :filter (blur :radius 1.0))))",
      "#t");
```

- [ ] **Step 2: Build and run smoke tests**

Run:
```powershell
cmake --build --preset debug --target pml-builtins-smoke
.\build\debug\tests\Debug\pml-builtins-smoke.exe
```
Expected: all pass.

---

## Task 14: End-to-end CLI test

- [ ] **Step 1: Create temporary sample `test_filter_e2e.pml`**

```pml
(set-backend! "skia")

(define comp (make-composition "filtered" 128 128 :bg "white"))

(define red-circle
  (make-layer "red" (circle 64 64 50 :fill "red")
              :filter (blur :radius 4.0)))

(define blue-dot
  (make-layer "blue" (circle 90 90 20 :fill "blue")
              :filter (color-adjust :brightness 0.2)))

(composition-add comp red-circle)
(composition-add comp blue-dot)
```

- [ ] **Step 2: Run CLI and verify output**

```powershell
.\build\debug\src\pml\cli\Debug\pml.exe g:\Project\PML\test_filter_e2e.pml -o g:\Project\PML\test_filter_out
```

Expected: `test_filter_out/filtered.png` is created and non-empty.

- [ ] **Step 3: Clean up temporary files**

Delete `test_filter_e2e.pml` and `test_filter_out/`.

---

## Task 15: Final full build and test suite

- [ ] **Step 1: Full debug build**

```powershell
cmake --build --preset debug
```

- [ ] **Step 2: Run all tests**

```powershell
.\build\debug\tests\Debug\pml-builtins-smoke.exe
.\build\debug\tests\Debug\pml-tests.exe
.\build\debug\tests\Debug\pml-layer-test.exe
.\build\debug\tests\Debug\pml-filter-test.exe
```

Expected: all tests pass.

- [ ] **Step 3: Summarize results**

Report new files, changed files, and test counts to the user.
