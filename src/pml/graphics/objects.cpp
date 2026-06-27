// ==========================================================================================================================================================================================================================================═
// PML GraphicObject — Implementation
// ==========================================================================================================================================================================================================================================═

#include "objects.h"

#include <atomic>

namespace pml {

// ---- ID generation ------------------------------------------------------------------------------------------------------------------------

namespace {
    std::atomic<uint64_t> s_next_graphic_object_id{1};
}

// ---- Constructor ----------------------------------------------------------------------------------------------------------------------------

GraphicObject::GraphicObject(
    ShapeType shape_type_,
    Params params_,
    std::optional<std::string> fill_,
    std::optional<std::string> stroke_,
    double stroke_width_,
    AffineTransform transform_,
    std::vector<GraphicObject> children_,
    std::unordered_map<std::string, Value> metadata_
)
    : shape_type(shape_type_)
    , params(std::move(params_))
    , fill(std::move(fill_))
    , stroke(std::move(stroke_))
    , stroke_width(stroke_width_)
    , transform(transform_)
    , children(std::move(children_))
    , metadata(std::move(metadata_))
    , id(s_next_graphic_object_id++)
{
}

// ---- Immutable "mutators" --------------------------------------------------------------------------------------------------------

GraphicObject GraphicObject::with_transform(AffineTransform t) const
{
    GraphicObject result = *this;
    result.transform = t;
    result.id = this->id;  // preserve id so animations can find the object
    return result;
}

GraphicObject GraphicObject::with_fill(std::string color) const
{
    GraphicObject result = *this;
    result.fill = std::move(color);
    return result;
}

GraphicObject GraphicObject::with_fill_gradient(Gradient g) const
{
    GraphicObject result = *this;
    result.fill_gradient = std::move(g);
    result.fill = std::nullopt;  // gradient takes precedence
    return result;
}

GraphicObject GraphicObject::with_stroke(std::string color) const
{
    GraphicObject result = *this;
    result.stroke = std::move(color);
    return result;
}

GraphicObject GraphicObject::with_param(const std::string& key, Value value) const
{
    GraphicObject result = *this;
    result.params.set(key, std::move(value));
    return result;
}

GraphicObject GraphicObject::with_param(ParamKey key, Value value) const
{
    GraphicObject result = *this;
    result.params.set(key, std::move(value));
    return result;
}

// ── to_string(ShapeType) ──────────────────────────────────────────────────────

std::string to_string(ShapeType type) {
    switch (type) {
    case ShapeType::Circle:        return "circle";
    case ShapeType::Rect:          return "rect";
    case ShapeType::Ellipse:       return "ellipse";
    case ShapeType::Line:          return "line";
    case ShapeType::Polygon:       return "polygon";
    case ShapeType::Path:          return "path";
    case ShapeType::Text:          return "text";
    case ShapeType::Image:         return "image";
    case ShapeType::Group:         return "group";
    case ShapeType::Mesh3D:        return "mesh3d";
    case ShapeType::RoughCircle:   return "rough_circle";
    case ShapeType::RoughRect:     return "rough_rect";
    case ShapeType::RoughEllipse:  return "rough_ellipse";
    case ShapeType::RoughLine:     return "rough_line";
    case ShapeType::RoughPolygon:  return "rough_polygon";
    case ShapeType::RoughPath:     return "rough_path";
    }
    return "unknown";
}

// ── Streaming ────────────────────────────────────────────────────────────────

std::ostream& operator<<(std::ostream& os, const GraphicObject& obj)
{
    os << "<GraphicObject " << to_string(obj.shape_type);
    if (obj.fill.has_value()) {
        os << " fill=" << *obj.fill;
    }
    if (obj.stroke.has_value()) {
        os << " stroke=" << *obj.stroke;
    }
    if (obj.blend_mode.has_value()) {
        os << " blend=" << blend_mode_to_keyword(*obj.blend_mode);
    }
    if (obj.stroke_align != "center") {
        os << " stroke-align=" << obj.stroke_align;
    }
    if (!obj.children.empty()) {
        os << " children=" << obj.children.size();
    }
    os << " id=" << obj.id << ">";
    return os;
}

}  // namespace pml
