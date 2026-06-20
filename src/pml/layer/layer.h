#pragma once

#include "blend_mode.h"
#include "pml/graphics/graphic_object.h"
#include "pml/graphics/transform.h"
#include "pml/filter/image_filter.h"

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
    std::vector<std::shared_ptr<ImageFilter>> filters;
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
    Layer with_anchor(Anchor anchor) const;
    Layer with_transform(AffineTransform t) const;
    Layer with_opacity(float opacity) const;
    Layer with_blend(BlendMode mode) const;
    Layer with_mask(std::optional<GraphicObject> mask) const;
    Layer with_filter(std::shared_ptr<ImageFilter> filter) const;
    Layer with_filters(std::vector<std::shared_ptr<ImageFilter>> filters) const;
    Layer with_visibility(bool visible) const;
    Layer with_locked(bool locked) const;

private:
    LayerProperties m_props;
    std::variant<std::shared_ptr<GraphicObject>,
                 std::vector<std::shared_ptr<Layer>>> m_content;
};

} // namespace pml
