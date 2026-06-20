#pragma once

#include "composition.h"
#include "pml/backend/backend.h"
#include "pml/core/error.h"

namespace pml {

class Compositor {
public:
    explicit Compositor(RenderBackend& backend);

    [[nodiscard]] Result<std::unique_ptr<Surface>> render(const Composition& comp);
    [[nodiscard]] Result<std::unique_ptr<Surface>> render_layer(
        const Layer& layer, int width, int height, uint32_t bg_color);

private:
    [[nodiscard]] Result<void> composite_onto(
        Surface& dst, Surface& src,
        const LayerProperties& props, int canvas_width, int canvas_height);

    RenderBackend& m_backend;
};

} // namespace pml
