#include "layer.h"

#include <utility>

namespace pml {

std::optional<Anchor> anchor_from_keyword(std::string_view name) {
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

Layer Layer::with_anchor(Anchor anchor) const {
    Layer copy = *this;
    copy.m_props.anchor = anchor;
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

Layer Layer::with_filter(std::shared_ptr<ImageFilter> filter) const {
    Layer copy = *this;
    copy.m_props.filters.clear();
    if (filter) copy.m_props.filters.push_back(std::move(filter));
    return copy;
}

Layer Layer::with_filters(std::vector<std::shared_ptr<ImageFilter>> filters) const {
    Layer copy = *this;
    copy.m_props.filters = std::move(filters);
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
