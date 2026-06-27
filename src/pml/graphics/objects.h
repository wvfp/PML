#pragma once

// ==========================================================================================================================================================================================================================================═
// PML GraphicObject — Immutable scene-graph element
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Value-semantic struct representing any primitive shape (circle, rect, text,
// group, etc.). All "mutating" methods return a new instance, enabling safe
// animation interpolation.
// ==========================================================================================================================================================================================================================================═

#include "../layer/blend_mode.h"
#include "gradient.h"
#include "params.h"
#include "transform.h"
#include "graphics_types.h"

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace pml {

/// Shape type for GraphicObject — replaces string-based type dispatch.
enum class ShapeType : uint8_t {
    Circle,
    Rect,
    Ellipse,
    Line,
    Polygon,
    Path,
    Text,
    Image,
    Group,
    Mesh3D,
    RoughCircle,
    RoughRect,
    RoughEllipse,
    RoughLine,
    RoughPolygon,
    RoughPath,
};

/// Whether a ShapeType is a rough variant.
inline constexpr bool is_rough(ShapeType t) noexcept {
    return t >= ShapeType::RoughCircle;
}

/// Strip the rough prefix to get the base shape.
inline constexpr ShapeType base_shape(ShapeType t) noexcept {
    if (is_rough(t)) {
        return static_cast<ShapeType>(static_cast<uint8_t>(t) - static_cast<uint8_t>(ShapeType::RoughCircle));
    }
    return t;
}

/// An immutable graphic element in the PML scene graph.
///
/// All primitives (circle, rect, etc.) produce GraphicObjects.  Modification
/// returns new objects, enabling safe sharing and animation tweening.
///
/// Shape-specific properties live in the string-keyed @p params map;
/// fill, stroke, and transform are first-class members for fast access.
/// Children enable hierarchical composition (groups, character parts).
struct GraphicObject {
    /// The kind of shape (circle, rect, ellipse, line, polygon, path, text, image, group, etc.).
    ShapeType shape_type{ShapeType::Group};

    /// Shape-specific parameters (radius, width/height, text content, etc.).
    Params params;

    /// Fill color string (e.g. "#ff0000", "transparent", or nullopt for none).
    std::optional<std::string> fill;

    /// Gradient fill descriptor (takes precedence over fill when set).
    std::optional<Gradient> fill_gradient;

    /// Stroke color string (e.g. "#000000", or nullopt for none).
    std::optional<std::string> stroke;

    /// Stroke width in pixels.
    double stroke_width{1.0};

    /// Blend mode for compositing this shape (nullopt = Normal / SrcOver).
    std::optional<BlendMode> blend_mode;

    /// Stroke alignment relative to the shape boundary.
    /// "center" (default), "inside", or "outside".
    std::string stroke_align{"center"};

    /// Opacity multiplier 0.0–1.0 (1.0 = fully opaque).
    double opacity{1.0};

    /// Local affine transform applied before rendering (defined in
    /// `pml/graphics/transform.h`).
    AffineTransform transform;

    /// Child objects (for groups, composite characters, etc.).
    std::vector<GraphicObject> children;

    /// Arbitrary metadata keyed by string (used by sprites, animation, etc.).
    std::unordered_map<std::string, Value> metadata;

    /// Monotonically increasing unique identifier (auto-generated).
    /// Default-constructed objects use id == 0 (reserved, never issued).
    uint64_t id{};

    // ---- Constructors ----------------------------------------------------------------------------------------------------─

    GraphicObject() = default;
    GraphicObject(
        ShapeType shape_type_,
        Params params_ = {},
        std::optional<std::string> fill_ = std::nullopt,
        std::optional<std::string> stroke_ = std::nullopt,
        double stroke_width_ = 1.0,
        AffineTransform transform_ = AffineTransform::identity(),
        std::vector<GraphicObject> children_ = {},
        std::unordered_map<std::string, Value> metadata_ = {}
    );

    // ---- Immutable "mutators" — each returns a new copy ----------------------------─

    /// Return a copy with a new transform applied.
    [[nodiscard]] GraphicObject with_transform(AffineTransform t) const;

    /// Return a copy with updated fill color.
    [[nodiscard]] GraphicObject with_fill(std::string color) const;

    /// Return a copy with updated gradient fill.
    [[nodiscard]] GraphicObject with_fill_gradient(Gradient g) const;

    /// Return a copy with updated stroke color.
    [[nodiscard]] GraphicObject with_stroke(std::string color) const;

    /// Return a copy with one param entry added / updated.
    [[nodiscard]] GraphicObject with_param(const std::string& key, Value value) const;
    [[nodiscard]] GraphicObject with_param(ParamKey key, Value value) const;

    // ---- Debugging --------------------------------------------------------------------------------------------------------─

    friend std::ostream& operator<<(std::ostream& os, const GraphicObject& obj);
};

}  // namespace pml
