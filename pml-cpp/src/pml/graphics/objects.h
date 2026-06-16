#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML GraphicObject — Immutable scene-graph element
// ───────────────────────────────────────────────────────────────────────────────
// Value-semantic struct representing any primitive shape (circle, rect, text,
// group, etc.). All "mutating" methods return a new instance, enabling safe
// animation interpolation.
// ═══════════════════════════════════════════════════════════════════════════════

#include "transform.h"
#include "types.h"

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

/// An immutable graphic element in the PML scene graph.
///
/// All primitives (circle, rect, etc.) produce GraphicObjects.  Modification
/// returns new objects, enabling safe sharing and animation tweening.
///
/// Shape-specific properties live in the string-keyed @p params map;
/// fill, stroke, and transform are first-class members for fast access.
/// Children enable hierarchical composition (groups, character parts).
struct GraphicObject {
    /// The kind of shape: "circle", "rect", "ellipse", "line", "polygon",
    /// "path", "text", "image", "group", etc.
    std::string shape_type;

    /// Shape-specific parameters (radius, width/height, text content, etc.).
    std::unordered_map<std::string, Value> params;

    /// Fill color string (e.g. "#ff0000", "transparent", or nullopt for none).
    std::optional<std::string> fill;

    /// Stroke color string (e.g. "#000000", or nullopt for none).
    std::optional<std::string> stroke;

    /// Stroke width in pixels.
    double stroke_width{1.0};

    /// Local affine transform applied before rendering.
    /// Note: defined in global namespace as `struct AffineTransform` via
    /// `transform.h` — we refer to it with `::` prefix to avoid ambiguity
    /// with the forward declaration in `types.h`.
    ::AffineTransform transform;

    /// Child objects (for groups, composite characters, etc.).
    std::vector<GraphicObject> children;

    /// Arbitrary metadata keyed by string (used by sprites, animation, etc.).
    std::unordered_map<std::string, Value> metadata;

    /// Monotonically increasing unique identifier (auto-generated).
    uint64_t id;

    // ── Constructor ────────────────────────────────────────────────────

    GraphicObject(
        std::string shape_type_,
        std::unordered_map<std::string, Value> params_ = {},
        std::optional<std::string> fill_ = std::nullopt,
        std::optional<std::string> stroke_ = std::nullopt,
        double stroke_width_ = 1.0,
        ::AffineTransform transform_ = ::AffineTransform::identity(),
        std::vector<GraphicObject> children_ = {},
        std::unordered_map<std::string, Value> metadata_ = {}
    );

    // ── Immutable "mutators" — each returns a new copy ───────────────

    /// Return a copy with a new transform applied.
    [[nodiscard]] GraphicObject with_transform(::AffineTransform t) const;

    /// Return a copy with updated fill color.
    [[nodiscard]] GraphicObject with_fill(std::string color) const;

    /// Return a copy with updated stroke color.
    [[nodiscard]] GraphicObject with_stroke(std::string color) const;

    /// Return a copy with one param entry added / updated.
    [[nodiscard]] GraphicObject with_param(const std::string& key, Value value) const;

    // ── Debugging ─────────────────────────────────────────────────────

    friend std::ostream& operator<<(std::ostream& os, const GraphicObject& obj);
};

}  // namespace pml
