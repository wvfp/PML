#pragma once

#include "layer.h"
#include "pml/graphics/graphics_types.h"

#include <memory>
#include <string>
#include <vector>

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
