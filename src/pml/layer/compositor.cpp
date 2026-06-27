#include "compositor.h"

#include <format>

#include "pml/graphics/objects.h"
#include "pml/graphics/transform.h"

namespace pml {

namespace {

[[nodiscard]] AffineTransform layer_transform(const LayerProperties& props) {
    // The layer offset is applied when compositing the rendered layer surface
    // onto the canvas; user-supplied transforms are applied in local space.
    return props.transform.value_or(AffineTransform::identity());
}

[[nodiscard]] GraphicObject wrap_leaf(
    const std::shared_ptr<GraphicObject>& leaf,
    const LayerProperties& props) {
    GraphicObject group(ShapeType::Group);
    group.children.push_back(*leaf);
    group.transform = layer_transform(props);
    return group;
}

[[nodiscard]] uint32_t color_to_rgba(Color c) {
    return c.rgba;
}

}

Compositor::Compositor(RenderBackend& backend) : m_backend(backend) {}

Result<std::unique_ptr<Surface>> Compositor::render(const Composition& comp) {
    if (!m_backend.supports_layer_compositing()) {
        return std::unexpected(blend_mode_error(
            {}, "backend does not support layer compositing"));
    }

    auto canvas = m_backend.create_surface(
        comp.canvas_size().width, comp.canvas_size().height,
        color_to_rgba(comp.background()));
    if (!canvas) {
        return std::unexpected(resource_error("failed to create composition surface"));
    }

    for (const auto& layer_ptr : comp.layers()) {
        if (!layer_ptr || !layer_ptr->properties().visible) continue;
        auto layer_surface = render_layer(
            *layer_ptr,
            comp.canvas_size().width,
            comp.canvas_size().height,
            color_to_rgba(Color::Transparent()));
        if (!layer_surface) return std::unexpected(layer_surface.error());

        auto result = composite_onto(
            *canvas, **layer_surface,
            layer_ptr->properties(),
            comp.canvas_size().width, comp.canvas_size().height);
        if (!result) return std::unexpected(result.error());
    }

    return canvas;
}

Result<std::unique_ptr<Surface>> Compositor::render_layer(
    const Layer& layer, int width, int height, uint32_t bg_color) {
    Result<std::unique_ptr<Surface>> surface_result;

    if (layer.is_leaf()) {
        GraphicObject wrapped = wrap_leaf(layer.leaf(), layer.properties());
        surface_result = m_backend.draw_to_new_surface(wrapped, width, height, bg_color);
    } else {
        auto group_surface = m_backend.create_surface(width, height, bg_color);
        if (!group_surface) {
            return std::unexpected(resource_error("failed to create group surface"));
        }

        for (const auto& child_ptr : layer.children()) {
            if (!child_ptr || !child_ptr->properties().visible) continue;
            auto child_surface = render_layer(
                *child_ptr, width, height,
                color_to_rgba(Color::Transparent()));
            if (!child_surface) return std::unexpected(child_surface.error());

            auto result = composite_onto(
                *group_surface, **child_surface,
                child_ptr->properties(), width, height);
            if (!result) return std::unexpected(result.error());
        }
        surface_result = std::move(group_surface);
    }

    if (!surface_result) return surface_result;

    for (const auto& filter : layer.properties().filters) {
        if (!filter) continue;
        auto fr = filter->apply(m_backend, **surface_result);
        if (!fr) return std::unexpected(fr.error());
    }

    if (layer.properties().mask) {
        auto mask_surface = m_backend.draw_to_new_surface(
            *layer.properties().mask, width, height,
            color_to_rgba(Color::Transparent()));
        if (!mask_surface) return std::unexpected(mask_surface.error());

        auto mask_result = m_backend.apply_mask(**surface_result, **mask_surface);
        if (!mask_result) return std::unexpected(mask_result.error());
    }

    return surface_result;
}

Result<void> Compositor::composite_onto(
    Surface& dst, Surface& src,
    const LayerProperties& props,
    int canvas_width, int canvas_height) {
    int x = static_cast<int>(props.offset.x);
    int y = static_cast<int>(props.offset.y);

    if (props.blend != BlendMode::Normal && !m_backend.supports_blend_mode()) {
        return std::unexpected(blend_mode_error(
            {}, std::format("blend mode '{}' not supported by backend",
                            blend_mode_to_keyword(props.blend))));
    }

    if (props.opacity < 1.0f - 1e-6f || props.blend != BlendMode::Normal) {
        return m_backend.composite_with_blend(dst, src, x, y, props.blend, props.opacity);
    }
    return m_backend.composite(dst, src, x, y);
}

} // namespace pml
