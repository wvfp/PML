#include "composition.h"

#include <utility>

namespace pml {

Composition::Composition(std::string name, Size2D canvas_size, int fps)
    : m_name(std::move(name))
    , m_canvas_size(canvas_size)
    , m_fps(fps)
    , m_background(Color::Transparent()) {}

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
