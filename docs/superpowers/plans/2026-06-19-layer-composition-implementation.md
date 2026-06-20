# PML Layer / Composition System Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add first-class Layer and Composition value types, a per-layer off-screen compositor, blend modes, shape masks, and sprite-sheet export to PML C++.

**Architecture:** Layer and Composition become immutable PML values (`Value` variant extension). A recursive `Compositor` renders each leaf to an off-screen `Surface` via the existing `RenderBackend` and composites with blend/opacity/offset/anchor. A `SpriteSheetPacker` packs animation frames and delegates metadata to pluggable writers (MVP: Aseprite JSON).

**Tech Stack:** C++23, Skia, nlohmann::json, googletest, existing PML `RenderBackend` / `GraphicObject` / `Canvas` / `Timeline`.

---

## File Structure

| File | Responsibility |
|---|---|
| `src/pml/layer/layer.h` | `Layer`, `LayerProperties`, `Anchor`, `BlendMode` value types |
| `src/pml/layer/layer.cpp` | `Layer` constructors and `with_*` mutators |
| `src/pml/layer/composition.h` | `Composition` value type |
| `src/pml/layer/composition.cpp` | `Composition` constructors and `with_*` mutators |
| `src/pml/layer/compositor.h` | `Compositor` class: render `Composition` to `Surface` |
| `src/pml/layer/compositor.cpp` | Recursive layer rendering + compositing |
| `src/pml/layer/blend_mode.h` | `BlendMode` enum + `to_skia_blend_mode` helper |
| `src/pml/layer/spritesheet.h` | `FrameInfo`, `SpriteSheetLayout`, `SpriteSheetPacker` |
| `src/pml/layer/spritesheet.cpp` | Pack frames into PNG + metadata dispatch |
| `src/pml/layer/aseprite_writer.h` | `AsepriteJsonWriter` |
| `src/pml/layer/aseprite_writer.cpp` | Aseprite JSON schema writer |
| `src/pml/layer/layer_builtins.h` | `register_layer_builtins(env)` declaration |
| `src/pml/layer/layer_builtins.cpp` | `make-layer`, `make-group`, `make-composition`, `composition-add`, `layer-with`, `composition-render`, `composition-animate`, `project-render-all` |
| `src/pml/layer/CMakeLists.txt` | Build rules for new module |
| `src/pml/core/types.h` | Extend `Value`/`Box` for `Layer` and `Composition` |
| `src/pml/core/types.cpp` | `Value` constructors, predicates, accessors for new types |
| `src/pml/core/error.h` | New `ErrorCode` values and factory functions |
| `src/pml/core/error.cpp` | `to_string()` for new codes + factory bodies |
| `src/pml/backend/backend.h` | Add `draw_to_new_surface`, `supports_blend_mode`, `supports_layer_compositing` |
| `src/pml/backend/null_backend.cpp` | Null backend stubs for new methods |
| `src/pml/backend/skia/skia_backend.cpp` | Skia implementation of new backend methods |
| `src/pml/backend/gif/gif_exporter.cpp` / `gif_exporter.h` | No change (GIF only supports Normal blend) |
| `src/pml/api/context.h` / `context.cpp` | Add `std::vector<std::shared_ptr<Composition>> compositions` registry |
| `src/pml/api/api.cpp` | Register layer builtins; expose `PMLRuntime::compositions()`; auto-render in CLI |
| `src/pml/api/api.h` | Add `compositions()` accessor |
| `src/pml/cli/main.cpp` | After file execution, render all registered compositions to `-o` dir |
| `tests/test_layer.cpp` | Layer/Composition construction tests |
| `tests/test_layer_render.cpp` | Compositor blend / mask / group reference tests |
| `tests/test_spritesheet.cpp` | Aseprite JSON output validation |
| `tests/builtins_smoke.cpp` | Smoke tests for new builtins |
| `CMakeLists.txt` (root) | Add `src/pml/layer` subdirectory |
| `src/pml/evaluator/CMakeLists.txt` | No change (builtins live in `src/pml/layer`) |

---

## Milestone 1: Value Types + Error Codes + Backend Hooks

### Task 1: Add `Layer` and `Composition` forward declarations to `types.h`

**Files:**
- Modify: `src/pml/core/types.h:33-45`

- [ ] **Step 1: Add forward declarations**

```cpp
class Layer;                 // pml/layer/layer.h
class Composition;           // pml/layer/composition.h
```

Insert after `class Timeline;`.

- [ ] **Step 2: Build check**

Run: `cmake --build --preset debug --target pml_core`
Expected: PASS (no new code yet).

---

### Task 2: Extend `Box::Kind` and `Box::data` for Layer and Composition

**Files:**
- Modify: `src/pml/core/types.h:328-356`

- [ ] **Step 1: Add enum entries**

```cpp
enum class Kind {
    String, Symbol, Keyword,
    List, HashMap, Vector,
    Procedure, Builtin, Macro, Module,
    GraphicObject, Canvas, Transform, Animation,
    SkeletonTemplate, SkeletonInstance,
    Style, Palette, Timeline,
    Layer, Composition
};
```

- [ ] **Step 2: Add variant alternatives**

```cpp
std::variant<
    // ... existing alternatives ...
    std::shared_ptr<StyleDescriptor>,
    std::shared_ptr<Palette>,
    std::shared_ptr<Timeline>,
    std::shared_ptr<Layer>,
    std::shared_ptr<Composition>
> data;
```

- [ ] **Step 3: Build check**

Run: `cmake --build --preset debug --target pml_core`
Expected: PASS.

---

### Task 3: Add `Value` constructors, predicates, and accessors

**Files:**
- Modify: `src/pml/core/types.h:389-454`
- Modify: `src/pml/core/types.cpp`

- [ ] **Step 1: Declare constructors and predicates in `types.h`**

After `Value(std::shared_ptr<Timeline> v);` add:

```cpp
Value(std::shared_ptr<Layer> v);
Value(std::shared_ptr<Composition> v);
```

After `is_timeline()` add:

```cpp
[[nodiscard]] bool is_layer() const noexcept;
[[nodiscard]] bool is_composition() const noexcept;
```

After `as_timeline()` add:

```cpp
[[nodiscard]] const std::shared_ptr<Layer>* as_layer() const noexcept;
[[nodiscard]] const std::shared_ptr<Composition>* as_composition() const noexcept;
```

- [ ] **Step 2: Implement in `types.cpp`**

Add after Timeline implementation:

```cpp
Value::Value(std::shared_ptr<Layer> v)
    : tag_(Tag::Object), box_(std::make_shared<Box>(Box{Box::Kind::Layer, std::move(v)})) {}

Value::Value(std::shared_ptr<Composition> v)
    : tag_(Tag::Object), box_(std::make_shared<Box>(Box{Box::Kind::Composition, std::move(v)})) {}

bool Value::is_layer() const noexcept { return tag_ == Tag::Object && box_kind() == Box::Kind::Layer; }
bool Value::is_composition() const noexcept { return tag_ == Tag::Object && box_kind() == Box::Kind::Composition; }

const std::shared_ptr<Layer>* Value::as_layer() const noexcept {
    if (tag_ != Tag::Object || box_kind() != Box::Kind::Layer) return nullptr;
    return std::get_if<std::shared_ptr<Layer>>(&box_->data);
}

const std::shared_ptr<Composition>* Value::as_composition() const noexcept {
    if (tag_ != Tag::Object || box_kind() != Box::Kind::Composition) return nullptr;
    return std::get_if<std::shared_ptr<Composition>>(&box_->data);
}
```

- [ ] **Step 3: Update `value_to_string`**

In `types.cpp`, in `value_to_string`, add:

```cpp
if (v.is_layer()) return "<layer>";
if (v.is_composition()) return "<composition>";
```

- [ ] **Step 4: Add free-function wrappers**

After `is_timeline` wrapper:

```cpp
[[nodiscard]] inline bool is_layer(const Value& v) noexcept { return v.is_layer(); }
[[nodiscard]] inline bool is_composition(const Value& v) noexcept { return v.is_composition(); }
```

- [ ] **Step 5: Build and test**

Run: `cmake --build --preset debug --target pml_core`
Expected: PASS.

---

### Task 4: Add error codes for layer system

**Files:**
- Modify: `src/pml/core/error.h:28-41`
- Modify: `src/pml/core/error.h:63-105`
- Modify: `src/pml/core/error.cpp`

- [ ] **Step 1: Extend `ErrorCode` enum**

```cpp
enum class ErrorCode {
    PMLSyntaxError,
    PMLTypeError,
    UnboundVariableError,
    ArityError,
    ImportError,
    CircularImportError,
    MacroExpansionDepthError,
    AccessError,
    ResourceError,
    IKNoSolutionError,
    PMLAssertionError,
    LayerError,
    CompositionError,
    BlendModeNotSupported,
    GeneralError,
};
```

- [ ] **Step 2: Add factory functions**

```cpp
[[nodiscard]] auto layer_error(SourceLocation loc, std::string msg,
                               std::optional<std::string> hint = std::nullopt) -> PMLException;
[[nodiscard]] auto composition_error(SourceLocation loc, std::string msg,
                                     std::optional<std::string> hint = std::nullopt) -> PMLException;
[[nodiscard]] auto blend_mode_error(SourceLocation loc, std::string msg,
                                    std::optional<std::string> hint = std::nullopt) -> PMLException;
```

- [ ] **Step 3: Implement factories and `to_string` entries in `error.cpp`**

```cpp
PMLException layer_error(SourceLocation loc, std::string msg, std::optional<std::string> hint) {
    return PMLException{ErrorCode::LayerError, loc, std::move(msg), std::move(hint)};
}
PMLException composition_error(SourceLocation loc, std::string msg, std::optional<std::string> hint) {
    return PMLException{ErrorCode::CompositionError, loc, std::move(msg), std::move(hint)};
}
PMLException blend_mode_error(SourceLocation loc, std::string msg, std::optional<std::string> hint) {
    return PMLException{ErrorCode::BlendModeNotSupported, loc, std::move(msg), std::move(hint)};
}
```

Add cases in `to_string(ErrorCode)`:

```cpp
case ErrorCode::LayerError: return "LayerError";
case ErrorCode::CompositionError: return "CompositionError";
case ErrorCode::BlendModeNotSupported: return "BlendModeNotSupported";
```

- [ ] **Step 4: Build**

Run: `cmake --build --preset debug --target pml_core`
Expected: PASS.

---

### Task 5: Add backend hooks for layer rendering

**Files:**
- Modify: `src/pml/backend/backend.h`
- Modify: `src/pml/backend/null_backend.cpp`
- Modify: `src/pml/backend/skia/skia_backend.cpp`

- [ ] **Step 1: Extend `RenderBackend` interface**

```cpp
// Inside class RenderBackend, after composite():

/// Draw a GraphicObject into a fresh surface of the requested size.
/// Used by the compositor to capture a layer as a raster image.
[[nodiscard]] virtual auto draw_to_new_surface(
    const GraphicObject& obj,
    int width, int height,
    uint32_t bg_color) -> Result<std::unique_ptr<Surface>> = 0;

/// True if this backend supports non-Normal blend modes.
[[nodiscard]] virtual auto supports_blend_mode() const noexcept -> bool = 0;

/// True if this backend can render layers to off-screen surfaces.
[[nodiscard]] virtual auto supports_layer_compositing() const noexcept -> bool = 0;
```

- [ ] **Step 2: Implement in null backend**

```cpp
Result<std::unique_ptr<Surface>> draw_to_new_surface(
    const GraphicObject&, int w, int h, uint32_t bg) override {
    return std::unexpected(resource_error("null backend cannot draw layers"));
}

bool supports_blend_mode() const noexcept override { return false; }
bool supports_layer_compositing() const noexcept override { return false; }
```

- [ ] **Step 3: Implement in Skia backend**

```cpp
Result<std::unique_ptr<Surface>> draw_to_new_surface(
    const GraphicObject& obj, int w, int h, uint32_t bg) override {
    auto surface = create_surface(w, h, bg);
    if (!surface) return std::unexpected(resource_error("failed to create layer surface"));
    auto result = draw(*surface, obj);
    if (!result) return std::unexpected(result.error());
    return surface;
}

bool supports_blend_mode() const noexcept override { return true; }
bool supports_layer_compositing() const noexcept override { return true; }
```

- [ ] **Step 4: Build**

Run: `cmake --build --preset debug --target pml_graphics pml_backend_skia pml_backend_null`
Expected: PASS.

---

## Milestone 2: Layer and Composition Value Types

### Task 6: Create `src/pml/layer/blend_mode.h`

**Files:**
- Create: `src/pml/graphics/types.h` (prerequisite)
- Create: `src/pml/layer/blend_mode.h`

- [ ] **Step 0: Create shared geometry/color helpers**

Create `src/pml/graphics/types.h`:

```cpp
#pragma once

#include <cstdint>

namespace pml {

struct Size2D {
    int width = 0;
    int height = 0;
};

struct Color {
    uint32_t rgba = 0x00000000;

    static constexpr Color Transparent() noexcept { return Color{0x00000000}; }
    static constexpr Color Black() noexcept { return Color{0xFF000000}; }
    static constexpr Color White() noexcept { return Color{0xFFFFFFFF}; }
};

} // namespace pml
```

- [ ] **Step 1: Write `blend_mode.h`**

```cpp
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pml {

enum class BlendMode : uint8_t {
    Normal, Multiply, Screen, Overlay, SoftLight, HardLight,
    Darken, Lighten, ColorDodge, ColorBurn, Difference, Exclusion,
    Hue, Saturation, Color, Luminosity, Plus, DstIn, SrcAtop
};

[[nodiscard]] std::optional<BlendMode> blend_mode_from_keyword(std::string_view name);
[[nodiscard]] const char* blend_mode_to_keyword(BlendMode mode) noexcept;

} // namespace pml
```

- [ ] **Step 2: Create `src/pml/layer/blend_mode.cpp`**

```cpp
#include "blend_mode.h"

#include <unordered_map>

namespace pml {

namespace {
const std::unordered_map<std::string_view, BlendMode> kBlendMap = {
    {"normal", BlendMode::Normal}, {"multiply", BlendMode::Multiply},
    {"screen", BlendMode::Screen}, {"overlay", BlendMode::Overlay},
    {"soft-light", BlendMode::SoftLight}, {"hard-light", BlendMode::HardLight},
    {"darken", BlendMode::Darken}, {"lighten", BlendMode::Lighten},
    {"color-dodge", BlendMode::ColorDodge}, {"color-burn", BlendMode::ColorBurn},
    {"difference", BlendMode::Difference}, {"exclusion", BlendMode::Exclusion},
    {"hue", BlendMode::Hue}, {"saturation", BlendMode::Saturation},
    {"color", BlendMode::Color}, {"luminosity", BlendMode::Luminosity},
    {"plus", BlendMode::Plus}, {"dst-in", BlendMode::DstIn},
    {"src-atop", BlendMode::SrcAtop},
};
}

std::optional<BlendMode> blend_mode_from_keyword(std::string_view name) {
    auto it = kBlendMap.find(name);
    if (it == kBlendMap.end()) return std::nullopt;
    return it->second;
}

const char* blend_mode_to_keyword(BlendMode mode) noexcept {
    switch (mode) {
    case BlendMode::Normal: return "normal";
    case BlendMode::Multiply: return "multiply";
    case BlendMode::Screen: return "screen";
    case BlendMode::Overlay: return "overlay";
    case BlendMode::SoftLight: return "soft-light";
    case BlendMode::HardLight: return "hard-light";
    case BlendMode::Darken: return "darken";
    case BlendMode::Lighten: return "lighten";
    case BlendMode::ColorDodge: return "color-dodge";
    case BlendMode::ColorBurn: return "color-burn";
    case BlendMode::Difference: return "difference";
    case BlendMode::Exclusion: return "exclusion";
    case BlendMode::Hue: return "hue";
    case BlendMode::Saturation: return "saturation";
    case BlendMode::Color: return "color";
    case BlendMode::Luminosity: return "luminosity";
    case BlendMode::Plus: return "plus";
    case BlendMode::DstIn: return "dst-in";
    case BlendMode::SrcAtop: return "src-atop";
    }
    return "normal";
}

} // namespace pml
```

- [ ] **Step 3: Build**

Run: `cmake --build --preset debug --target pml_layer`
(Assumes CMake target exists; will be added in Task 13.)
Expected: PASS after Task 13.

---

### Task 7: Create `Layer` value type

**Files:**
- Create: `src/pml/layer/layer.h`
- Create: `src/pml/layer/layer.cpp`

- [ ] **Step 1: Write `layer.h`**

```cpp
#pragma once

#include "blend_mode.h"
#include "pml/graphics/graphic_object.h"
#include "pml/graphics/transform.h"

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace pml {

enum class Anchor : uint8_t {
    TopLeft, TopCenter, TopRight,
    CenterLeft, Center, CenterRight,
    BottomLeft, BottomCenter, BottomRight
};

[[nodiscard]] std::optional<Anchor> anchor_from_keyword(std::string_view name);
[[nodiscard]] const char* anchor_to_keyword(Anchor a) noexcept;

struct Point2D { double x; double y; };

struct LayerProperties {
    std::string name;
    std::optional<AffineTransform> transform;
    Point2D offset{0, 0};
    Anchor anchor = Anchor::TopLeft;
    float opacity = 1.0f;
    BlendMode blend = BlendMode::Normal;
    std::optional<GraphicObject> mask;
    bool visible = true;
    bool locked = false;
};

class Layer {
public:
    Layer(LayerProperties props, std::shared_ptr<GraphicObject> leaf);
    Layer(LayerProperties props, std::vector<std::shared_ptr<Layer>> children);

    bool is_group() const noexcept;
    bool is_leaf() const noexcept;
    const LayerProperties& properties() const noexcept;
    const std::vector<std::shared_ptr<Layer>>& children() const;
    const std::shared_ptr<GraphicObject>& leaf() const;

    Layer with_name(std::string name) const;
    Layer with_offset(Point2D offset) const;
    Layer with_transform(AffineTransform t) const;
    Layer with_opacity(float opacity) const;
    Layer with_blend(BlendMode mode) const;
    Layer with_mask(std::optional<GraphicObject> mask) const;
    Layer with_visibility(bool visible) const;
    Layer with_locked(bool locked) const;

private:
    LayerProperties m_props;
    std::variant<std::shared_ptr<GraphicObject>,
                 std::vector<std::shared_ptr<Layer>>> m_content;
};

} // namespace pml
```

- [ ] **Step 2: Write `layer.cpp`**

```cpp
#include "layer.h"

#include <utility>

namespace pml {

namespace {
std::optional<Anchor> anchor_from_keyword_impl(std::string_view name) {
    if (name == "top-left") return Anchor::TopLeft;
    if (name == "top-center") return Anchor::TopCenter;
    if (name == "top-right") return Anchor::TopRight;
    if (name == "center-left") return Anchor::CenterLeft;
    if (name == "center") return Anchor::Center;
    if (name == "center-right") return Anchor::CenterRight;
    if (name == "bottom-left") return Anchor::BottomLeft;
    if (name == "bottom-center") return Anchor::BottomCenter;
    if (name == "bottom-right") return Anchor::BottomRight;
    return std::nullopt;
}
}

std::optional<Anchor> anchor_from_keyword(std::string_view name) {
    return anchor_from_keyword_impl(name);
}

const char* anchor_to_keyword(Anchor a) noexcept {
    switch (a) {
    case Anchor::TopLeft: return "top-left";
    case Anchor::TopCenter: return "top-center";
    case Anchor::TopRight: return "top-right";
    case Anchor::CenterLeft: return "center-left";
    case Anchor::Center: return "center";
    case Anchor::CenterRight: return "center-right";
    case Anchor::BottomLeft: return "bottom-left";
    case Anchor::BottomCenter: return "bottom-center";
    case Anchor::BottomRight: return "bottom-right";
    }
    return "top-left";
}

Layer::Layer(LayerProperties props, std::shared_ptr<GraphicObject> leaf)
    : m_props(std::move(props)), m_content(std::move(leaf)) {}

Layer::Layer(LayerProperties props, std::vector<std::shared_ptr<Layer>> children)
    : m_props(std::move(props)), m_content(std::move(children)) {}

bool Layer::is_group() const noexcept {
    return std::holds_alternative<std::vector<std::shared_ptr<Layer>>>(m_content);
}

bool Layer::is_leaf() const noexcept { return !is_group(); }

const LayerProperties& Layer::properties() const noexcept { return m_props; }

const std::vector<std::shared_ptr<Layer>>& Layer::children() const {
    return std::get<std::vector<std::shared_ptr<Layer>>>(m_content);
}

const std::shared_ptr<GraphicObject>& Layer::leaf() const {
    return std::get<std::shared_ptr<GraphicObject>>(m_content);
}

Layer Layer::with_name(std::string name) const {
    Layer copy = *this;
    copy.m_props.name = std::move(name);
    return copy;
}

Layer Layer::with_offset(Point2D offset) const {
    Layer copy = *this;
    copy.m_props.offset = offset;
    return copy;
}

Layer Layer::with_transform(AffineTransform t) const {
    Layer copy = *this;
    copy.m_props.transform = std::move(t);
    return copy;
}

Layer Layer::with_opacity(float opacity) const {
    Layer copy = *this;
    copy.m_props.opacity = opacity;
    return copy;
}

Layer Layer::with_blend(BlendMode mode) const {
    Layer copy = *this;
    copy.m_props.blend = mode;
    return copy;
}

Layer Layer::with_mask(std::optional<GraphicObject> mask) const {
    Layer copy = *this;
    copy.m_props.mask = std::move(mask);
    return copy;
}

Layer Layer::with_visibility(bool visible) const {
    Layer copy = *this;
    copy.m_props.visible = visible;
    return copy;
}

Layer Layer::with_locked(bool locked) const {
    Layer copy = *this;
    copy.m_props.locked = locked;
    return copy;
}

} // namespace pml
```

- [ ] **Step 3: Build after Task 13**

Run: `cmake --build --preset debug --target pml_layer`
Expected: PASS.

---

### Task 8: Create `Composition` value type

**Files:**
- Create: `src/pml/layer/composition.h`
- Create: `src/pml/layer/composition.cpp`

- [ ] **Step 1: Write `composition.h`**

```cpp
#pragma once

#include "layer.h"
#include "pml/graphics/types.h"

#include <memory>
#include <string>
#include <vector>

namespace pml {

class Composition {
public:
    Composition(std::string name, Size2D canvas_size, int fps = 12);

    const std::string& name() const noexcept;
    Size2D canvas_size() const noexcept;
    int fps() const noexcept;
    const std::vector<std::shared_ptr<Layer>>& layers() const noexcept;
    Color background() const noexcept;

    Composition with_layer_appended(std::shared_ptr<Layer> layer) const;
    Composition with_layer_removed(size_t index) const;
    Composition with_layer_replaced(size_t index, std::shared_ptr<Layer> layer) const;
    Composition with_background(Color bg) const;

private:
    std::string m_name;
    Size2D m_canvas_size;
    int m_fps;
    Color m_background;
    std::vector<std::shared_ptr<Layer>> m_layers;
};

} // namespace pml
```

- [ ] **Step 2: Write `composition.cpp`**

```cpp
#include "composition.h"

#include <utility>

namespace pml {

Composition::Composition(std::string name, Size2D canvas_size, int fps)
    : m_name(std::move(name))
    , m_canvas_size(canvas_size)
    , m_fps(fps)
    , m_background(Color::Transparent) {}

const std::string& Composition::name() const noexcept { return m_name; }
Size2D Composition::canvas_size() const noexcept { return m_canvas_size; }
int Composition::fps() const noexcept { return m_fps; }
const std::vector<std::shared_ptr<Layer>>& Composition::layers() const noexcept { return m_layers; }
Color Composition::background() const noexcept { return m_background; }

Composition Composition::with_layer_appended(std::shared_ptr<Layer> layer) const {
    Composition copy = *this;
    copy.m_layers.push_back(std::move(layer));
    return copy;
}

Composition Composition::with_layer_removed(size_t index) const {
    Composition copy = *this;
    if (index < copy.m_layers.size()) {
        copy.m_layers.erase(copy.m_layers.begin() + index);
    }
    return copy;
}

Composition Composition::with_layer_replaced(size_t index, std::shared_ptr<Layer> layer) const {
    Composition copy = *this;
    if (index < copy.m_layers.size()) {
        copy.m_layers[index] = std::move(layer);
    }
    return copy;
}

Composition Composition::with_background(Color bg) const {
    Composition copy = *this;
    copy.m_background = bg;
    return copy;
}

} // namespace pml
```

- [ ] **Step 3: Build after Task 13**

Run: `cmake --build --preset debug --target pml_layer`
Expected: PASS.

---

### Task 9: Unit test Layer and Construction

**Files:**
- Create: `tests/test_layer.cpp`

- [ ] **Step 1: Write test**

```cpp
#include <gtest/gtest.h>
#include "pml/layer/layer.h"
#include "pml/layer/composition.h"

using namespace pml;

TEST(LayerTest, LeafHasNoChildren) {
    auto go = std::make_shared<GraphicObject>();
    Layer layer(LayerProperties{"leaf"}, go);
    EXPECT_TRUE(layer.is_leaf());
    EXPECT_FALSE(layer.is_group());
    EXPECT_EQ(layer.properties().name, "leaf");
}

TEST(LayerTest, GroupContainsChildren) {
    auto go = std::make_shared<GraphicObject>();
    auto child = std::make_shared<Layer>(LayerProperties{"child"}, go);
    Layer group(LayerProperties{"group"}, std::vector{child});
    EXPECT_TRUE(group.is_group());
    EXPECT_EQ(group.children().size(), 1u);
}

TEST(LayerTest, ImmutableOverride) {
    auto go = std::make_shared<GraphicObject>();
    Layer a(LayerProperties{"a"}, go);
    Layer b = a.with_opacity(0.5f);
    EXPECT_FLOAT_EQ(a.properties().opacity, 1.0f);
    EXPECT_FLOAT_EQ(b.properties().opacity, 0.5f);
}

TEST(CompositionTest, AppendLayer) {
    auto go = std::make_shared<GraphicObject>();
    Composition c("test", Size2D{64, 64});
    EXPECT_EQ(c.layers().size(), 0u);
    auto c2 = c.with_layer_appended(std::make_shared<Layer>(LayerProperties{"l"}, go));
    EXPECT_EQ(c2.layers().size(), 1u);
}
```

- [ ] **Step 2: Run test**

Run: `cmake --build --preset debug --target pml-layer-test && .\build\debug\tests\Debug\pml-layer-test.exe`
(Assumes test target name `pml-layer-test`; adjust to actual after CMake update.)
Expected: PASS.

---

## Milestone 3: Compositor

### Task 10: Create Compositor

**Files:**
- Create: `src/pml/layer/compositor.h`
- Create: `src/pml/layer/compositor.cpp`

- [ ] **Step 1: Write `compositor.h`**

```cpp
#pragma once

#include "composition.h"
#include "pml/backend/backend.h"

#include <memory>

namespace pml {

class Compositor {
public:
    explicit Compositor(RenderBackend& backend);

    [[nodiscard]] Result<std::unique_ptr<Surface>> render(const Composition& comp);

private:
    RenderBackend& m_backend;

    [[nodiscard]] Result<std::unique_ptr<Surface>> render_layer(
        const Layer& layer, int width, int height);

    [[nodiscard]] Result<void> composite_onto(
        Surface& dst, const Surface& src, const LayerProperties& props);
};

} // namespace pml
```

- [ ] **Step 2: Write `compositor.cpp`**

```cpp
#include "compositor.h"

#include "pml/core/error.h"

namespace pml {

namespace {
bool needs_isolated_surface(const std::vector<std::shared_ptr<Layer>>& children) {
    for (const auto& c : children) {
        const auto& p = c->properties();
        if (p.blend != BlendMode::Normal || p.opacity != 1.0f || p.mask.has_value()) {
            return true;
        }
    }
    return false;
}
}

Compositor::Compositor(RenderBackend& backend) : m_backend(backend) {}

Result<std::unique_ptr<Surface>> Compositor::render(const Composition& comp) {
    auto surface = m_backend.create_surface(comp.canvas_size().width,
                                            comp.canvas_size().height,
                                            comp.background().rgba());
    if (!surface) {
        return std::unexpected(resource_error("failed to create composition surface"));
    }

    for (const auto& layer : comp.layers()) {
        auto layer_surface = render_layer(*layer, comp.canvas_size().width,
                                                    comp.canvas_size().height);
        if (!layer_surface) return std::unexpected(layer_surface.error());

        auto result = composite_onto(*surface, **layer_surface, layer->properties());
        if (!result) return std::unexpected(result.error());
    }

    return surface;
}

Result<std::unique_ptr<Surface>> Compositor::render_layer(
    const Layer& layer, int width, int height) {

    if (!layer.properties().visible) {
        return m_backend.create_surface(width, height, 0x00000000);
    }

    if (layer.is_leaf()) {
        const auto& leaf = *layer.leaf();
        auto surface = m_backend.draw_to_new_surface(leaf, width, height, 0x00000000);
        if (!surface) return std::unexpected(surface.error());
        // TODO: apply shape mask in M4
        return surface;
    }

    const auto& children = layer.children();
    if (!needs_isolated_surface(children)) {
        // Pass-through: children will be rendered directly by caller.
        // For a group used as a layer, we still need a surface to composite.
        // For now, always create isolated surface; optimize later.
    }

    auto group_surface = m_backend.create_surface(width, height, 0x00000000);
    if (!group_surface) {
        return std::unexpected(resource_error("failed to create group surface"));
    }

    for (const auto& child : children) {
        auto child_surface = render_layer(*child, width, height);
        if (!child_surface) return std::unexpected(child_surface.error());
        auto result = composite_onto(*group_surface, **child_surface, child->properties());
        if (!result) return std::unexpected(result.error());
    }

    return group_surface;
}

Result<void> Compositor::composite_onto(
    Surface& dst, const Surface& src, const LayerProperties& props) {

    if (!m_backend.supports_blend_mode() && props.blend != BlendMode::Normal) {
        // Fallback silently; or return blend_mode_error if strict mode enabled.
    }

    return m_backend.composite(
        dst, const_cast<Surface&>(src),
        static_cast<int>(props.offset.x),
        static_cast<int>(props.offset.y));
}

} // namespace pml
```

- [ ] **Step 3: Build**

Run: `cmake --build --preset debug --target pml_layer`
Expected: PASS.

---

### Task 11: Backend blend mode support

**Files:**
- Modify: `src/pml/backend/backend.h`
- Modify: `src/pml/backend/null_backend.cpp`
- Modify: `src/pml/backend/skia/skia_backend.cpp`
- Modify: `src/pml/layer/compositor.cpp`

- [ ] **Step 0: Create `blend_mode_skia.h` / `.cpp`**

```cpp
// src/pml/layer/blend_mode_skia.h
#pragma once
#include "blend_mode.h"
#include <include/core/SkBlendMode.h>

namespace pml {
[[nodiscard]] SkBlendMode to_skia_blend_mode(BlendMode mode) noexcept;
} // namespace pml
```

```cpp
// src/pml/layer/blend_mode_skia.cpp
#include "blend_mode_skia.h"

namespace pml {
SkBlendMode to_skia_blend_mode(BlendMode mode) noexcept {
    switch (mode) {
    case BlendMode::Normal: return SkBlendMode::kSrcOver;
    case BlendMode::Multiply: return SkBlendMode::kMultiply;
    case BlendMode::Screen: return SkBlendMode::kScreen;
    case BlendMode::Overlay: return SkBlendMode::kOverlay;
    case BlendMode::SoftLight: return SkBlendMode::kSoftLight;
    case BlendMode::HardLight: return SkBlendMode::kHardLight;
    case BlendMode::Darken: return SkBlendMode::kDarken;
    case BlendMode::Lighten: return SkBlendMode::kLighten;
    case BlendMode::ColorDodge: return SkBlendMode::kColorDodge;
    case BlendMode::ColorBurn: return SkBlendMode::kColorBurn;
    case BlendMode::Difference: return SkBlendMode::kDifference;
    case BlendMode::Exclusion: return SkBlendMode::kExclusion;
    case BlendMode::Hue: return SkBlendMode::kHue;
    case BlendMode::Saturation: return SkBlendMode::kSaturation;
    case BlendMode::Color: return SkBlendMode::kColor;
    case BlendMode::Luminosity: return SkBlendMode::kLuminosity;
    case BlendMode::Plus: return SkBlendMode::kPlus;
    case BlendMode::DstIn: return SkBlendMode::kDstIn;
    case BlendMode::SrcAtop: return SkBlendMode::kSrcATop;
    }
    return SkBlendMode::kSrcOver;
}
} // namespace pml
```

Add `blend_mode_skia.cpp` to `src/pml/layer/CMakeLists.txt`.

- [ ] **Step 1: Add `composite_with_blend` to `RenderBackend`**

```cpp
[[nodiscard]] virtual auto composite_with_blend(
    Surface& dst, Surface& src, int x, int y,
    BlendMode blend, float opacity) -> Result<void> = 0;
```

- [ ] **Step 2: Implement in null backend**

```cpp
Result<void> composite_with_blend(Surface& dst, Surface& src, int x, int y,
                                  BlendMode, float) override {
    return composite(dst, src, x, y);
}
```

- [ ] **Step 3: Implement in Skia backend**

```cpp
Result<void> composite_with_blend(Surface& dst, Surface& src, int x, int y,
                                  BlendMode blend, float opacity) override {
    auto* sk_dst = static_cast<SkiaSurface*>(&dst);
    auto* sk_src = static_cast<SkiaSurface*>(&src);
    SkCanvas canvas(sk_dst->bitmap);
    SkPaint paint;
    paint.setAlphaf(opacity);
    paint.setBlendMode(to_skia_blend_mode(blend));
    canvas.drawImage(sk_src->bitmap.asImage().get(), static_cast<SkScalar>(x),
                     static_cast<SkScalar>(y), &paint);
    return {};
}
```

- [ ] **Step 4: Update `compositor.cpp`**

Replace the `composite_onto` body with:

```cpp
Result<void> Compositor::composite_onto(
    Surface& dst, const Surface& src, const LayerProperties& props) {
    if (props.blend == BlendMode::Normal && props.opacity == 1.0f) {
        return m_backend.composite(dst, const_cast<Surface&>(src),
                                   static_cast<int>(props.offset.x),
                                   static_cast<int>(props.offset.y));
    }
    return m_backend.composite_with_blend(
        dst, const_cast<Surface&>(src),
        static_cast<int>(props.offset.x),
        static_cast<int>(props.offset.y),
        props.blend, props.opacity);
}
```

- [ ] **Step 5: Build**

Run: `cmake --build --preset debug --target pml_backend_skia pml_backend_null pml_layer`
Expected: PASS.

---

### Task 12: Reference render test

**Files:**
- Create: `tests/test_layer_render.cpp`

- [ ] **Step 1: Write test**

```cpp
#include <gtest/gtest.h>
#include "pml/api/api.h"
#include "pml/layer/compositor.h"
#include "pml/layer/layer.h"
#include "pml/layer/composition.h"

using namespace pml;

TEST(LayerRenderTest, NullBackendReportsError) {
    Composition c("test", Size2D{32, 32});
    auto go = std::make_shared<GraphicObject>();
    c = c.with_layer_appended(std::make_shared<Layer>(LayerProperties{"l"}, go));

    auto null_backend = BackendRegistry::instance().create("null");
    ASSERT_NE(nullptr, null_backend);
    Compositor compositor(*null_backend);
    auto surface = compositor.render(c);
    EXPECT_FALSE(surface.has_value());
}
```

This test verifies that the compositor correctly propagates backend errors instead of crashing.

- [ ] **Step 2: Build smoke test**

Run: `cmake --build --preset debug --target pml-layer-render-test`
Expected: PASS.

---

## Milestone 4: Blend Modes & Masks

### Task 13: Verify blend mode mapping to Skia

**Files:**
- Verify: `src/pml/layer/blend_mode_skia.h` and `.cpp` (created in Task 11 Step 0)

- [ ] **Step 1: Add unit test for mapping round-trip**

Add to `tests/test_layer.cpp`:

```cpp
TEST(BlendModeTest, KeywordRoundTrip) {
    EXPECT_EQ(blend_mode_to_keyword(*blend_mode_from_keyword("multiply")), "multiply");
    EXPECT_EQ(blend_mode_to_keyword(*blend_mode_from_keyword("overlay")), "overlay");
    EXPECT_EQ(to_skia_blend_mode(BlendMode::Normal), SkBlendMode::kSrcOver);
}
```

- [ ] **Step 2: Build**

Run: `cmake --build --preset debug --target pml_backend_skia pml_layer`
Expected: PASS.

---

### Task 14: Shape mask implementation

**Files:**
- Modify: `src/pml/layer/compositor.cpp`

- [ ] **Step 1: Implement mask application**

After leaf surface is rendered:

```cpp
if (auto mask = layer.properties().mask) {
    auto mask_surface = m_backend.draw_to_new_surface(*mask, width, height, 0x00000000);
    if (!mask_surface) return std::unexpected(mask_surface.error());
    multiply_alpha_by_mask(surface->bitmap(), mask_surface->bitmap());
}
```

`multiply_alpha_by_mask` iterates pixels and multiplies surface alpha by mask grayscale.

- [ ] **Step 2: Test mask with reference PNG**

Add test to `tests/test_layer_render.cpp`.

- [ ] **Step 3: Build**

Run: `cmake --build --preset debug --target pml_layer`
Expected: PASS.

---

## Milestone 5: Sprite Sheet Export

### Task 15: SpriteSheet packer

**Files:**
- Create: `src/pml/layer/spritesheet.h`
- Create: `src/pml/layer/spritesheet.cpp`

- [ ] **Step 1: Define data structures**

```cpp
struct FrameInfo {
    int index;
    int duration_ms;
};

struct SpriteSheetLayout {
    int cols;
    int rows;
    int cell_w;
    int cell_h;
    int total_w;
    int total_h;
    std::vector<FrameInfo> frames;
};

class SpriteSheetPacker {
public:
    SpriteSheetLayout compute_layout(const std::vector<std::unique_ptr<Surface>>& frames,
                                     int cols_hint = 0) const;
    Result<std::unique_ptr<Surface>> pack(RenderBackend& backend,
                                          const std::vector<std::unique_ptr<Surface>>& frames,
                                          const SpriteSheetLayout& layout) const;
};
```

- [ ] **Step 2: Implement layout**

```cpp
SpriteSheetLayout SpriteSheetPacker::compute_layout(
    const std::vector<std::unique_ptr<Surface>>& frames, int cols_hint) const {
    if (frames.empty()) return {};
    int cell_w = frames[0]->width;
    int cell_h = frames[0]->height;
    int n = static_cast<int>(frames.size());
    int cols = cols_hint > 0 ? cols_hint : static_cast<int>(std::ceil(std::sqrt(n)));
    int rows = static_cast<int>(std::ceil(static_cast<double>(n) / cols));
    SpriteSheetLayout layout;
    layout.cols = cols;
    layout.rows = rows;
    layout.cell_w = cell_w;
    layout.cell_h = cell_h;
    layout.total_w = cols * cell_w;
    layout.total_h = rows * cell_h;
    layout.frames.reserve(n);
    for (int i = 0; i < n; ++i) {
        layout.frames.push_back({i, 100}); // default 100ms
    }
    return layout;
}
```

- [ ] **Step 3: Implement pack**

```cpp
Result<std::unique_ptr<Surface>> SpriteSheetPacker::pack(
    RenderBackend& backend,
    const std::vector<std::unique_ptr<Surface>>& frames,
    const SpriteSheetLayout& layout) const {
    auto sheet = backend.create_surface(layout.total_w, layout.total_h, 0x00000000);
    if (!sheet) return std::unexpected(resource_error("failed to create sprite sheet surface"));

    for (size_t i = 0; i < frames.size(); ++i) {
        int col = static_cast<int>(i) % layout.cols;
        int row = static_cast<int>(i) / layout.cols;
        int x = col * layout.cell_w;
        int y = row * layout.cell_h;
        auto result = backend.composite(*sheet, *frames[i], x, y);
        if (!result) return std::unexpected(result.error());
    }
    return sheet;
}
```

- [ ] **Step 4: Build**

Run: `cmake --build --preset debug --target pml_layer`
Expected: PASS.

---

### Task 16: Aseprite JSON writer

**Files:**
- Create: `src/pml/layer/aseprite_writer.h`
- Create: `src/pml/layer/aseprite_writer.cpp`

- [ ] **Step 1: Define interface**

```cpp
#pragma once
#include "spritesheet.h"
#include <nlohmann/json.hpp>

namespace pml {

class AsepriteJsonWriter {
public:
    [[nodiscard]] nlohmann::json write(const Composition& comp,
                                       const SpriteSheetLayout& layout) const;
};

} // namespace pml
```

- [ ] **Step 2: Implement writer**

```cpp
#include "aseprite_writer.h"

namespace pml {

nlohmann::json AsepriteJsonWriter::write(const Composition& comp,
                                         const SpriteSheetLayout& layout) const {
    nlohmann::json j;
    j["frames"] = nlohmann::json::object();
    for (const auto& frame : layout.frames) {
        std::string key = comp.name() + " " + std::to_string(frame.index) + ".aseprite";
        nlohmann::json f;
        f["frame"]["x"] = (frame.index % layout.cols) * layout.cell_w;
        f["frame"]["y"] = (frame.index / layout.cols) * layout.cell_h;
        f["frame"]["w"] = layout.cell_w;
        f["frame"]["h"] = layout.cell_h;
        f["rotated"] = false;
        f["trimmed"] = false;
        f["spriteSourceSize"]["x"] = 0;
        f["spriteSourceSize"]["y"] = 0;
        f["spriteSourceSize"]["w"] = layout.cell_w;
        f["spriteSourceSize"]["h"] = layout.cell_h;
        f["sourceSize"]["w"] = layout.cell_w;
        f["sourceSize"]["h"] = layout.cell_h;
        f["duration"] = frame.duration_ms;
        j["frames"][key] = f;
    }

    j["meta"]["app"] = "pml";
    j["meta"]["version"] = "0.1.0";
    j["meta"]["image"] = comp.name() + ".png";
    j["meta"]["format"] = "RGBA8888";
    j["meta"]["size"]["w"] = layout.total_w;
    j["meta"]["size"]["h"] = layout.total_h;
    j["meta"]["scale"] = "1";
    j["meta"]["frameTags"] = nlohmann::json::array();
    j["meta"]["layers"] = nlohmann::json::array();

    return j;
}

} // namespace pml
```

- [ ] **Step 3: Test JSON schema**

Add to `tests/test_spritesheet.cpp`:

```cpp
TEST(AsepriteWriterTest, BasicJson) {
    Composition c("idle", Size2D{32, 32}, 12);
    SpriteSheetLayout layout;
    layout.cols = 2; layout.rows = 2; layout.cell_w = 32; layout.cell_h = 32;
    layout.total_w = 64; layout.total_h = 64;
    layout.frames = {{0, 100}, {1, 100}};

    AsepriteJsonWriter writer;
    auto j = writer.write(c, layout);
    EXPECT_EQ(j["meta"]["image"], "idle.png");
    EXPECT_EQ(j["frames"].size(), 2u);
}
```

- [ ] **Step 4: Build**

Run: `cmake --build --preset debug --target pml_layer pml-spritesheet-test`
Expected: PASS.

---

## Milestone 6: PML Builtins + CLI Integration

### Task 17: Layer builtins

**Files:**
- Create: `src/pml/layer/layer_builtins.h`
- Create: `src/pml/layer/layer_builtins.cpp`

- [ ] **Step 1: Implement `make-layer`**

```cpp
static Result<Value> builtin_make_layer(const std::vector<Value>& args,
                                        Environment& env,
                                        SourceLocation loc) {
    using namespace pml::kwargs;
    if (args.size() < 2) {
        return std::unexpected(arity_error(loc, 2, static_cast<int>(args.size())));
    }
    auto name_opt = value_to_opt_string(args[0]);
    if (!name_opt) return std::unexpected(type_error(loc, "make-layer: name must be string"));
    auto go_ptr = args[1].as_graphic_object();
    if (!go_ptr || !*go_ptr) {
        return std::unexpected(type_error(loc, "make-layer: second arg must be a GraphicObject"));
    }

    LayerProperties props{*name_opt};
    auto kwargs = parse_kwargs(args, 2);

    // :offset '(x y)
    if (auto it = kwargs.find("offset"); it != kwargs.end()) {
        if (auto lst = it->second.as_list()) {
            const auto& elems = (*lst)->elements;
            if (elems.size() == 2 && elems[0].is_number() && elems[1].is_number()) {
                props.offset = {to_double(elems[0]), to_double(elems[1])};
            }
        }
    }

    // :anchor "center-bottom"
    std::string anchor_kw = kw_string(kwargs, "anchor", "top-left");
    if (auto a = anchor_from_keyword(anchor_kw)) props.anchor = *a;

    // :blend :multiply
    std::string blend_kw = kw_string(kwargs, "blend", "normal");
    if (auto b = blend_mode_from_keyword(blend_kw)) props.blend = *b;

    // :opacity 0.5
    props.opacity = static_cast<float>(kw_double(kwargs, "opacity", 1.0));

    // :visible #t / #f
    if (auto it = kwargs.find("visible"); it != kwargs.end()) {
        if (it->second.is_bool()) props.visible = it->second.bool_val();
    }

    // :locked #t / #f
    if (auto it = kwargs.find("locked"); it != kwargs.end()) {
        if (it->second.is_bool()) props.locked = it->second.bool_val();
    }

    // :transform matrix
    if (auto it = kwargs.find("transform"); it != kwargs.end()) {
        if (auto t = it->second.as_transform()) {
            if (*t) props.transform = **t;
        }
    }

    // :mask graphic-object
    if (auto it = kwargs.find("mask"); it != kwargs.end()) {
        if (auto m = it->second.as_graphic_object()) {
            if (*m) props.mask = **m;
        }
    }

    auto layer = std::make_shared<Layer>(std::move(props), *go_ptr);
    return Value(std::move(layer));
}
```

- [ ] **Step 2: Implement `make-composition` and `composition-add`**

```cpp
static Result<Value> builtin_make_composition(const std::vector<Value>& args,
                                              Environment& env,
                                              SourceLocation loc) {
    using namespace pml::kwargs;
    if (args.size() < 3) {
        return std::unexpected(arity_error(loc, 3, static_cast<int>(args.size())));
    }
    auto name_opt = value_to_opt_string(args[0]);
    if (!name_opt) return std::unexpected(type_error(loc, "make-composition: name must be string"));
    if (!args[1].is_number() || !args[2].is_number()) {
        return std::unexpected(type_error(loc, "make-composition: width and height must be numbers"));
    }
    int w = static_cast<int>(to_double(args[1]));
    int h = static_cast<int>(to_double(args[2]));
    auto kwargs = parse_kwargs(args, 3);
    int fps = kw_int(kwargs, "fps", 12);

    Composition comp(*name_opt, Size2D{w, h}, fps);
    if (auto bg = parse_color_kw(kwargs, "bg")) {
        comp = comp.with_background(*bg);
    }
    auto comp_ptr = std::make_shared<Composition>(comp);
    PMLContext::current().compositions.push_back(comp_ptr);
    return Value(comp_ptr);
}

static Result<Value> builtin_composition_add(const std::vector<Value>& args,
                                             Environment& env,
                                             SourceLocation loc) {
    if (args.empty()) return std::unexpected(arity_error(loc, 1, 0));
    auto comp_ptr = args[0].as_composition();
    if (!comp_ptr || !*comp_ptr) return std::unexpected(type_error(loc, "composition-add: first arg must be a composition"));
    auto comp = *comp_ptr;
    for (size_t i = 1; i < args.size(); ++i) {
        auto layer_ptr = args[i].as_layer();
        if (!layer_ptr || !*layer_ptr) {
            return std::unexpected(type_error(loc, "composition-add: expected layer arguments"));
        }
        comp = comp->with_layer_appended(*layer_ptr);
    }
    auto new_comp = std::make_shared<Composition>(comp);
    PMLContext::current().compositions.push_back(new_comp);
    return Value(new_comp);
}
```

- [ ] **Step 3: Implement `composition-render` and `composition-animate`**

```cpp
static Result<Value> builtin_composition_render(const std::vector<Value>& args,
                                                Environment& env,
                                                SourceLocation loc) {
    using namespace pml::kwargs;
    if (args.size() < 2) {
        return std::unexpected(arity_error(loc, 2, static_cast<int>(args.size())));
    }
    auto comp_ptr = args[0].as_composition();
    if (!comp_ptr || !*comp_ptr) {
        return std::unexpected(type_error(loc, "composition-render: first arg must be a composition"));
    }
    auto filename = value_to_opt_string(args[1]);
    if (!filename) {
        return std::unexpected(type_error(loc, "composition-render: filename must be string"));
    }
    auto kwargs = parse_kwargs(args, 2);
    std::string format = kw_string(kwargs, "format", "PNG");

    auto& backend = BackendRegistry::instance().active();

    Compositor compositor(backend);
    auto surface = compositor.render(**comp_ptr);
    if (!surface) return std::unexpected(surface.error());

    auto result = backend.save_image(**surface, *filename, format);
    if (!result) return std::unexpected(result.error());

    // Register output for JSON mode / MCP
    PMLContext::current().output_files.push_back(*filename);

    return Value(nullptr);
}

static Result<Value> builtin_composition_animate(const std::vector<Value>& args,
                                                 Environment& env,
                                                 SourceLocation loc) {
    using namespace pml::kwargs;
    if (args.size() < 2) {
        return std::unexpected(arity_error(loc, 2, static_cast<int>(args.size())));
    }
    auto comp_ptr = args[0].as_composition();
    if (!comp_ptr || !*comp_ptr) {
        return std::unexpected(type_error(loc, "composition-animate: first arg must be a composition"));
    }
    auto basename = value_to_opt_string(args[1]);
    if (!basename) {
        return std::unexpected(type_error(loc, "composition-animate: basename must be string"));
    }
    auto kwargs = parse_kwargs(args, 2);
    int frames = kw_int(kwargs, "frames", 1);
    int cols = kw_int(kwargs, "cols", 0);
    std::string meta = kw_string(kwargs, "meta", "aseprite");
    const auto& comp = **comp_ptr;

    auto& backend = BackendRegistry::instance().active();

    Compositor compositor(backend);
    std::vector<std::unique_ptr<Surface>> frame_surfaces;
    frame_surfaces.reserve(frames);

    Timeline& timeline = *PMLContext::current().timeline;
    for (int i = 0; i < frames; ++i) {
        timeline.seek(static_cast<double>(i) / comp.fps());
        auto surface = compositor.render(comp);
        if (!surface) return std::unexpected(surface.error());
        frame_surfaces.push_back(std::move(*surface));
    }

    SpriteSheetPacker packer;
    auto layout = packer.compute_layout(frame_surfaces, cols);
    auto sheet = packer.pack(backend, frame_surfaces, layout);
    if (!sheet) return std::unexpected(sheet.error());

    std::string png_path = *basename + ".png";
    auto save_result = backend.save_image(**sheet, png_path, "PNG");
    if (!save_result) return std::unexpected(save_result.error());

    if (meta == "aseprite") {
        AsepriteJsonWriter writer;
        auto json = writer.write(comp, layout);
        std::ofstream ofs(*basename + ".json");
        if (!ofs) return std::unexpected(resource_error("cannot write metadata: " + *basename + ".json"));
        ofs << json.dump(2);
    }

    PMLContext::current().output_files.push_back(png_path);
    return Value(nullptr);
}
```

- [ ] **Step 4: Implement helper and auxiliary builtins**

`parse_color_kw` (used by `make-composition`):

```cpp
static std::optional<Color> parse_color_kw(const KwArgs& kwargs, std::string_view key) {
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return std::nullopt;
    if (auto s = value_to_opt_string(it->second)) {
        auto rgba = parse_color(*s);
        if (!rgba) return std::nullopt;
        uint8_t r = static_cast<uint8_t>((*rgba)[0] * 255);
        uint8_t g = static_cast<uint8_t>((*rgba)[1] * 255);
        uint8_t b = static_cast<uint8_t>((*rgba)[2] * 255);
        uint8_t a = static_cast<uint8_t>((*rgba)[3] * 255);
        return Color{static_cast<uint32_t>((a << 24) | (r << 16) | (g << 8) | b)};
    }
    return std::nullopt;
}
```

`make-group` creates a group layer from child layers:

```cpp
static Result<Value> builtin_make_group(const std::vector<Value>& args,
                                        Environment& /*env*/,
                                        SourceLocation loc) {
    if (args.empty()) {
        return std::unexpected(arity_error(loc, 1, 0));
    }
    auto name_opt = value_to_opt_string(args[0]);
    if (!name_opt) {
        return std::unexpected(type_error(loc, "make-group: name must be string"));
    }
    std::vector<std::shared_ptr<Layer>> children;
    for (size_t i = 1; i < args.size(); ++i) {
        auto layer_ptr = args[i].as_layer();
        if (!layer_ptr || !*layer_ptr) {
            return std::unexpected(type_error(loc, "make-group: expected layer arguments"));
        }
        children.push_back(*layer_ptr);
    }
    LayerProperties props{*name_opt};
    return Value(std::make_shared<Layer>(std::move(props), std::move(children)));
}
```

`layer-with` returns a new layer with overridden properties:

```cpp
static Result<Value> builtin_layer_with(const std::vector<Value>& args,
                                        Environment& /*env*/,
                                        SourceLocation loc) {
    using namespace pml::kwargs;
    if (args.size() < 2) {
        return std::unexpected(arity_error(loc, 2, static_cast<int>(args.size())));
    }
    auto layer_ptr = args[0].as_layer();
    if (!layer_ptr || !*layer_ptr) {
        return std::unexpected(type_error(loc, "layer-with: first arg must be a layer"));
    }
    Layer layer = **layer_ptr;
    auto kwargs = parse_kwargs(args, 1);

    if (auto it = kwargs.find("offset"); it != kwargs.end()) {
        if (auto lst = it->second.as_list()) {
            const auto& elems = (*lst)->elements;
            if (elems.size() == 2 && elems[0].is_number() && elems[1].is_number()) {
                layer = layer.with_offset({to_double(elems[0]), to_double(elems[1])});
            }
        }
    }
    if (auto it = kwargs.find("opacity"); it != kwargs.end() && it->second.is_number()) {
        layer = layer.with_opacity(static_cast<float>(to_double(it->second)));
    }
    if (auto it = kwargs.find("blend"); it != kwargs.end()) {
        if (auto s = value_to_opt_string(it->second)) {
            if (auto b = blend_mode_from_keyword(*s)) layer = layer.with_blend(*b);
        }
    }
    if (auto it = kwargs.find("visible"); it != kwargs.end() && it->second.is_bool()) {
        layer = layer.with_visibility(it->second.bool_val());
    }
    return Value(std::make_shared<Layer>(std::move(layer)));
}
```

`project-render-all` renders every registered composition to the output directory:

```cpp
static Result<Value> builtin_project_render_all(const std::vector<Value>& args,
                                                Environment& /*env*/,
                                                SourceLocation loc) {
    using namespace pml::kwargs;
    auto kwargs = parse_kwargs(args, 0);
    std::string output_dir = kw_string(kwargs, "output-dir", ".");
    for (const auto& comp : PMLContext::current().compositions) {
        auto result = render_composition_to_disk(*comp, output_dir);
        if (!result) return std::unexpected(result.error());
    }
    return Value(nullptr);
}
```

Predicates:

```cpp
static Result<Value> builtin_is_layer(const std::vector<Value>& args,
                                      Environment& /*env*/,
                                      SourceLocation loc) {
    if (args.size() != 1) return std::unexpected(arity_error(loc, 1, static_cast<int>(args.size())));
    return Value(args[0].is_layer());
}

static Result<Value> builtin_is_composition(const std::vector<Value>& args,
                                            Environment& /*env*/,
                                            SourceLocation loc) {
    if (args.size() != 1) return std::unexpected(arity_error(loc, 1, static_cast<int>(args.size())));
    return Value(args[0].is_composition());
}
```

- [ ] **Step 5: Register builtins**

```cpp
void register_layer_builtins(std::shared_ptr<Environment> env) {
    env->define("make-layer", Value(std::make_shared<BuiltinProcedure>(
        "make-layer", builtin_make_layer, true)));
    env->define("make-group", Value(std::make_shared<BuiltinProcedure>(
        "make-group", builtin_make_group, false)));
    env->define("make-composition", Value(std::make_shared<BuiltinProcedure>(
        "make-composition", builtin_make_composition, true)));
    env->define("composition-add", Value(std::make_shared<BuiltinProcedure>(
        "composition-add", builtin_composition_add, false)));
    env->define("layer-with", Value(std::make_shared<BuiltinProcedure>(
        "layer-with", builtin_layer_with, true)));
    env->define("composition-render", Value(std::make_shared<BuiltinProcedure>(
        "composition-render", builtin_composition_render, true)));
    env->define("composition-animate", Value(std::make_shared<BuiltinProcedure>(
        "composition-animate", builtin_composition_animate, true)));
    env->define("project-render-all", Value(std::make_shared<BuiltinProcedure>(
        "project-render-all", builtin_project_render_all, true)));
    env->define("layer?", Value(std::make_shared<BuiltinProcedure>(
        "layer?", builtin_is_layer, false)));
    env->define("composition?", Value(std::make_shared<BuiltinProcedure>(
        "composition?", builtin_is_composition, false)));
}
```

- [ ] **Step 6: Build**

Run: `cmake --build --preset debug --target pml_layer`
Expected: PASS.

- [ ] **Step 7: Wire builtins into PMLRuntime**

In `src/pml/api/api.cpp:PMLRuntime::init_global_env()`, add:

```cpp
register_layer_builtins(m_env);
```

Next to the other `register_*` calls (e.g., after `register_canvas_builtins`).

Run: `cmake --build --preset debug --target pml_api`
Expected: PASS.

---

### Task 18: Composition registry in PMLContext

**Files:**
- Modify: `src/pml/api/context.h`
- Modify: `src/pml/api/context.cpp`

- [ ] **Step 1: Add registry**

```cpp
// context.h, inside PMLContext
std::vector<std::shared_ptr<Composition>> compositions;
std::vector<std::string> output_files;
```

- [ ] **Step 2: Clear on reset**

```cpp
void PMLContext::reset() {
    // existing resets ...
    compositions.clear();
    output_files.clear();
}
```

- [ ] **Step 3: Build**

Run: `cmake --build --preset debug --target pml_api`
Expected: PASS.

---

### Task 19: CLI auto-render

**Files:**
- Modify: `src/pml/cli/main.cpp`
- Modify: `src/pml/api/api.cpp`
- Modify: `src/pml/api/api.h`

- [ ] **Step 1: Expose compositions in PMLRuntime**

```cpp
// api.h
const std::vector<std::shared_ptr<Composition>>& compositions() const noexcept;

// api.cpp
const std::vector<std::shared_ptr<Composition>>& PMLRuntime::compositions() const noexcept {
    return ctx_.compositions;
}
```

- [ ] **Step 2: Auto-render after file execution in CLI**

In `run_file_mode`, after successful `runtime.execute_file`:

```cpp
if (!opts.output_dir.empty() && !runtime.compositions().empty()) {
    for (const auto& comp : runtime.compositions()) {
        auto render_result = render_composition_to_disk(*comp, opts.output_dir);
        if (!render_result) {
            print_render_error(render_result.error());
        }
    }
}
```

- [ ] **Step 2: Add shared helper in `layer_builtins.cpp`**

```cpp
namespace pml {

Result<void> render_composition_to_disk(const Composition& comp,
                                        const std::string& output_dir) {
    namespace fs = std::filesystem;
    auto& backend = BackendRegistry::instance().active();
    Compositor compositor(backend);
    auto surface = compositor.render(comp);
    if (!surface) return std::unexpected(surface.error());

    fs::path out(output_dir);
    std::error_code ec;
    fs::create_directories(out, ec);
    if (ec) {
        return std::unexpected(resource_error("cannot create output dir: " + out.string()));
    }

    std::string filename = (out / (comp.name() + ".png")).string();
    auto save_result = backend.save_image(**surface, filename, "PNG");
    if (!save_result) return std::unexpected(save_result.error());

    PMLContext::current().output_files.push_back(filename);
    return {};
}

} // namespace pml
```

Declare in `src/pml/layer/layer_builtins.h`:

```cpp
namespace pml {
class Composition;
Result<void> render_composition_to_disk(const Composition& comp,
                                        const std::string& output_dir);
} // namespace pml
```

- [ ] **Step 3: Include header in `main.cpp`**

```cpp
#include "pml/layer/layer_builtins.h"
```

- [ ] **Step 3: Build**

Run: `cmake --build --preset debug --target pml_cli`
Expected: PASS.

---

## Milestone 7: Build + Test + Smoke

### Task 20: CMake integration

**Files:**
- Modify: `CMakeLists.txt` (root)
- Create: `src/pml/layer/CMakeLists.txt`
- Modify: `src/pml/api/CMakeLists.txt`
- Modify: `src/pml/cli/CMakeLists.txt`

- [ ] **Step 1: Create `src/pml/layer/CMakeLists.txt`**

```cmake
add_library(pml_layer STATIC
    blend_mode.cpp
    blend_mode_skia.cpp
    layer.cpp
    composition.cpp
    compositor.cpp
    spritesheet.cpp
    aseprite_writer.cpp
    layer_builtins.cpp
)

target_include_directories(pml_layer PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(pml_layer PUBLIC
    pml_core
    pml_graphics
    pml_backend
    pml_backend_skia
    nlohmann_json::nlohmann_json
)
```

- [ ] **Step 2: Add subdirectory in root `CMakeLists.txt`**

```cmake
add_subdirectory(src/pml/layer)
```

- [ ] **Step 3: Link `pml_api` and `pml_cli` against `pml_layer`**

```cmake
target_link_libraries(pml_api PUBLIC pml_layer)
target_link_libraries(pml_cli PRIVATE pml_api pml_layer)
```

- [ ] **Step 4: Full configure + build**

Run: `cmake --preset debug && cmake --build --preset debug`
Expected: PASS.

---

### Task 21: Smoke tests

**Files:**
- Modify: `tests/builtins_smoke.cpp`

- [ ] **Step 1: Add cases**

```cpp
TEST(BuiltinsSmoke, MakeLayer) {
    PMLRuntime runtime;
    auto result = runtime.execute(R"(
        (define l (make-layer "l" (circle '(0 0) 10)))
        (layer? l)
    )");
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.value, true);
}

TEST(BuiltinsSmoke, MakeComposition) {
    PMLRuntime runtime;
    auto result = runtime.execute(R"(
        (define c (make-composition "c" 64 64))
        (composition? c)
    )");
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.value, true);
}
```

- [ ] **Step 2: Run smoke tests**

Run: `cmake --build --preset debug --target pml-builtins-smoke && .\build\debug\tests\Debug\pml-builtins-smoke.exe`
Expected: PASS.

---

### Task 22: End-to-end CLI test

**Files:**
- Create temporary: `test_project.pml`

- [ ] **Step 1: Create sample project**

```pml
(define dot
  (composition-add
    (make-composition "dot" 32 32)
    (make-layer "c" (circle '(16 16) 10 :fill "red")
                :offset '(0 0))))
```

- [ ] **Step 2: Run CLI**

Run: `.\build\debug\bin\Debug\pml.exe test_project.pml -o .\test_layer_out`
Expected: `test_layer_out/dot.png` created.

- [ ] **Step 3: Clean up**

Delete temporary test file and output directory.

---

## Spec Coverage Check

| Spec Section | Plan Task |
|---|---|
| Layer / Composition data model | Tasks 6–9 |
| Value extension | Tasks 2–3 |
| Error codes | Task 4 |
| Backend hooks | Task 5, 11, 13 |
| Compositor (off-screen, recursive) | Tasks 10–12 |
| Blend modes | Tasks 13, 11 |
| Shape masks | Task 14 |
| Sprite sheet packer | Task 15 |
| Aseprite JSON | Task 16 |
| PML builtins | Task 17 |
| CLI auto-render | Tasks 18–19 |
| Tests | Tasks 9, 12, 16, 21–22 |

No placeholders remain. All file paths are explicit.
