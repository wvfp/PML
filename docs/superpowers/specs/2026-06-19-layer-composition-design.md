# PML Layer / Composition System Design

> Version: 0.1  
> Date: 2026-06-19  
> Scope: Sub-project A — Layer / Composition system for game-art pipelines.  
> Depends on: existing PML C++23 Skia backend, `GraphicObject`, `Canvas`, animation timeline.

## 1. Goals

- Let a PML project consist of multiple `.pml` files, each treated as a **reusable layer or component**.
- Compose layers into **compositions** (static or animated), which are the output units for sprite sheets.
- Provide z-order, per-layer transform, opacity, blend mode, anchor, and shape mask.
- Keep PML code first-class: layers and compositions are immutable PML values.
- Output multi-format sprite sheet metadata (MVP: Aseprite JSON).

## 2. Non-Goals

- This spec does **not** cover Photoshop-like image adjustments (curves, levels, HSL) — that is sub-project B.
- It does **not** introduce a GUI editor or a separate project manifest language.
- It does not support vector masks, gradient masks, or clipping masks in the MVP; only shape masks.

## 3. Architecture Decisions

| Dimension | Decision |
|---|---|
| Use case | Game art (characters, equipment, scenes) |
| Layer ↔ file | Recursive: a `.pml` can expose one layer, multiple layers, or nested groups |
| Render model | Per-layer off-screen bitmap + SkCanvas compositing (PSD-like) |
| Animation | Each layer content can be animated; composition outputs sprite sheets |
| API style | Declarative top-level `(composition ...)` + imperative `(make-composition)` / `(add-layer! ...)` |
| Output formats | Multi-format, MVP implements Aseprite JSON |
| Coordinate model | Dual local/world: layer has local origin + `offset` in parent, `anchor` for transform pivot |
| Override semantics | Caller can override any layer property when importing a component |
| Output granularity | One project file can define multiple compositions; each renders independently |
| Implementation path | Layer / Composition as first-class PML value types |

## 4. Data Model

### 4.1 Layer

```cpp
namespace pml {

enum class BlendMode : uint8_t {
    Normal, Multiply, Screen, Overlay, SoftLight, HardLight,
    Darken, Lighten, ColorDodge, ColorBurn, Difference, Exclusion,
    Hue, Saturation, Color, Luminosity, Plus, DstIn, SrcAtop
};

enum class Anchor : uint8_t {
    TopLeft, TopCenter, TopRight,
    CenterLeft, Center, CenterRight,
    BottomLeft, BottomCenter, BottomRight,
    Custom
};

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

    // Immutable mutators
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

### 4.2 Composition

```cpp
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

### 4.3 Value Extension

In `src/pml/core/types.h`, add to the `Value` variant:

```cpp
struct LayerValue { std::shared_ptr<Layer> ptr; };
struct CompositionValue { std::shared_ptr<Composition> ptr; };
```

Add helpers:

```cpp
bool is_layer(const Value& v) noexcept;
bool is_composition(const Value& v) noexcept;
const LayerValue* as_layer(const Value& v) noexcept;
const CompositionValue* as_composition(const Value& v) noexcept;
```

## 5. PML API

### 5.1 Declarative top-level

```pml
(composition "elf-idle"
  :canvas '(64 64)
  :fps 12
  :bg "transparent"

  (layer "back-hair" head/back-hair
         :anchor "center-bottom"
         :offset '(32 60))

  (layer "body" body/idle
         :anchor "center-bottom"
         :offset '(32 64))

  (layer "head" head/base
         :anchor "center-bottom"
         :offset '(32 40))

  (layer "weapon" weapon/idle
         :anchor "center-bottom"
         :offset '(48 44)
         :blend :multiply
         :opacity 0.9))
```

### 5.2 Imperative building

```pml
(composition-add (make-composition "elf-walk" 64 64 :fps 8)
  (make-layer "body" body/walk
              :anchor "center-bottom"
              :offset '(32 64))
  (make-layer "head" head/base
              :anchor "center-bottom"
              :offset '(32 40)))
```

### 5.3 Builtin summary

| Builtin | Signature |
|---|---|
| `make-layer` | `(make-layer name object [:anchor ...] [:offset '(x y)] [:blend :normal] [:opacity 1.0] [:mask obj] [:visible t] [:transform mat] [:locked f])` |
| `make-group` | `(make-group name child-layer ... [:offset ...] [:opacity ...] ...)` |
| `layer?` | `(layer? x)` |
| `composition?` | `(composition? x)` |
| `layer-properties` | `(layer-properties layer)` → plist |
| `layer-with` | `(layer-with layer :opacity 0.5 ...)` |
| `make-composition` | `(make-composition name w h [:fps n] [:bg color])` |
| `composition-add` | `(composition-add composition layer)` — returns a new Composition; original is unchanged |
| `composition->layers` | `(composition->layers composition)` |
| `composition-render` | `(composition-render comp "out.png" [:format "png"] [:meta "aseprite"])` |
| `composition-animate` | `(composition-animate comp "out" [:frames 8] [:cols auto] [:format "spritesheet"] [:meta "aseprite"])` |
| `project-render-all` | `(project-render-all)` |

## 6. Rendering & Compositing

### 6.1 High-level flow

```cpp
Surface Compositor::render(const Composition& comp, RenderBackend& backend) {
    auto surface = backend.create_surface(comp.canvas_size().w,
                                          comp.canvas_size().h);
    fill_background(*surface, comp.background());

    for (const auto& layer : comp.layers()) {
        SkBitmap bitmap = render_layer(*layer, backend, comp.canvas_size());
        composite_onto(surface->bitmap(), bitmap, layer->properties());
    }

    return surface;
}
```

### 6.2 Recursive layer rendering

```cpp
SkBitmap render_layer(const Layer& layer, RenderBackend& backend, Size2D canvas_size) {
    if (!layer.properties().visible) return empty_bitmap();

    if (layer.is_leaf()) {
        Rect bounds = compute_transformed_bounds(layer.properties(), canvas_size);
        SkBitmap bmp = backend.render_object_to_bitmap(*layer.leaf(), bounds, Color::Transparent);
        if (auto mask = layer.properties().mask) {
            apply_mask(bmp, *mask, bounds, backend);
        }
        return bmp;
    }

    // group: render children into a temporary surface and return as bitmap
    SkBitmap group_bmp = create_transparent(canvas_size);
    for (const auto& child : layer.children()) {
        SkBitmap child_bmp = render_layer(*child, backend, canvas_size);
        composite_onto(group_bmp, child_bmp, child->properties());
    }
    return group_bmp;
}
```

### 6.3 Pass-through optimization

If a group contains only `Normal` blend children with opacity 1.0 and no mask, skip the group off-screen surface and draw children directly onto the parent surface.

### 6.4 Backend interface additions

In `src/pml/backend/backend.h`:

```cpp
class RenderBackend {
public:
    virtual ~RenderBackend() = default;

    // existing
    virtual std::unique_ptr<Surface> create_surface(int w, int h) = 0;
    virtual void render_to_surface(const GraphicObject& obj, Surface& surface) = 0;

    // new
    virtual SkBitmap render_object_to_bitmap(const GraphicObject& obj,
                                             const Rect& bounds,
                                             const Color& bg) = 0;
    virtual bool supports_blend_mode(BlendMode mode) const noexcept = 0;
    virtual bool supports_offscreen_rendering() const noexcept = 0;
};
```

Skia backend implements all blend modes. GIF/null backends implement only `Normal` and report capabilities accordingly.

## 7. Blend Modes

- PML keyword: `:blend :multiply`
- Internal `BlendMode` enum maps to `SkBlendMode`.
- GIF/null backend: non-Normal blend modes fall back to `Normal` with a warning.

## 8. Masks

MVP supports shape masks:

```pml
(layer "head" head/base
  :mask (ellipse '(32 32) 30 25))
```

Implementation: render mask shape into a grayscale alpha map, multiply alpha channel of layer bitmap.

Out of scope for MVP: vector masks, gradient masks, inverted masks, clipping masks.

## 9. Animation / Sprite Sheet Output

### 9.1 `composition-animate`

```pml
(composition-animate comp "elf-idle"
  :frames 8
  :cols 4
  :format "spritesheet"
  :meta "aseprite")
```

Steps:
1. For each frame index `i`:
   - `Timeline::seek(i / fps)`
   - `render(comp)`
2. Pack frames into a single PNG.
3. Write metadata file via pluggable `SpriteSheetMetadataWriter`.

### 9.2 Metadata writer interface

```cpp
class SpriteSheetMetadataWriter {
public:
    virtual ~SpriteSheetMetadataWriter() = default;
    virtual std::string extension() const = 0;
    virtual nlohmann::json write(const Composition& comp,
                                 const std::vector<FrameInfo>& frames,
                                 const SpriteSheetLayout& layout) = 0;
};
```

MVP implement `AsepriteJsonWriter`. Future: `TexturePackerWriter`, `GodotAtlasWriter`.

## 10. Import / Component Semantics

A `.pml` component file:

```pml
;; parts/head.pml
(define head-base (group (circle ...) (ellipse ...)))
(define back-hair (polygon ...))
(provide head-base back-hair)
```

Project file:

```pml
(import "parts/head.pml" as head)
(layer "head" head/head-base :offset '(32 40))
```

Any property of the layer can be overridden by the caller at import/use time.

## 11. CLI Behavior

When a `.pml` file contains one or more `composition` forms, executing:

```powershell
pml project.pml -o ./out
```

automatically renders every composition defined at top level into `./out/`:

```text
out/
  elf-idle.png
  elf-idle.json
  elf-walk.png
  elf-walk.json
```

If the file contains no compositions, behavior remains unchanged (backward compatible).

## 12. Error Handling

Add to `src/pml/core/error.h`:

```cpp
LayerError,
CompositionError,
BlendModeNotSupported
```

All functions return `Result<T>` / `std::expected`. No exceptions for control flow.

## 13. Testing

- `tests/test_layer.cpp`: Layer/Composition construction, property overrides, z-order.
- `tests/test_layer_render.cpp`: pixel-diff reference PNGs for blend modes, masks, group nesting.
- `tests/builtins_smoke.cpp`: add 10–15 new cases for new builtins.
- `tests/test_spritesheet.cpp`: Aseprite JSON schema validation.

## 14. Milestones

| Milestone | Deliverable |
|---|---|
| M1 | Layer / Composition value types; Value variant extension; builtin registration |
| M2 | Compositor off-screen rendering; Normal blend; basic composition render |
| M3 | BlendMode enum + Skia mapping; pass-through optimization |
| M4 | Shape mask support |
| M5 | `composition-animate`; Aseprite JSON output |
| M6 | CLI auto-render all compositions; tests; docs |

## 15. Open Questions (to resolve during implementation)

1. Should `Composition` be tracked globally like `current_canvas`, or passed explicitly?  
   Recommendation: explicit first-class value; avoid another global singleton.
2. Should `render_object_to_bitmap` accept a `Surface` return instead of `SkBitmap` to stay backend-agnostic?  
   Recommendation: return backend-agnostic `Surface`, let compositor read pixels through abstract accessor.
3. Exact Aseprite JSON schema subset to support.  
   Recommendation: `frames`, `meta.size`, `meta.frameTags`, `meta.layers`.
