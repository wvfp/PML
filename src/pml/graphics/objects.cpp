// ═══════════════════════════════════════════════════════════════════════════════
// PML GraphicObject — Implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "objects.h"

#include <atomic>

namespace pml {

// ── ID generation ────────────────────────────────────────────────────────────

namespace {
    std::atomic<uint64_t> s_next_graphic_object_id{1};
}

// ── Constructor ──────────────────────────────────────────────────────────────

GraphicObject::GraphicObject(
    std::string shape_type_,
    Params params_,
    std::optional<std::string> fill_,
    std::optional<std::string> stroke_,
    double stroke_width_,
    AffineTransform transform_,
    std::vector<GraphicObject> children_,
    std::unordered_map<std::string, Value> metadata_
)
    : shape_type(std::move(shape_type_))
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

// ── Immutable "mutators" ────────────────────────────────────────────────────

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

// ── Streaming ────────────────────────────────────────────────────────────────

std::ostream& operator<<(std::ostream& os, const GraphicObject& obj)
{
    os << "<GraphicObject " << obj.shape_type;
    if (obj.fill.has_value()) {
        os << " fill=" << *obj.fill;
    }
    if (obj.stroke.has_value()) {
        os << " stroke=" << *obj.stroke;
    }
    if (!obj.children.empty()) {
        os << " children=" << obj.children.size();
    }
    os << " id=" << obj.id << ">";
    return os;
}

}  // namespace pml
