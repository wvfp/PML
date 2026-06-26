// ==========================================================================================================================================================================================================================================═
// PML Canvas — Implementation
// ==========================================================================================================================================================================================================================================═

#include "canvas.h"

#include "graphics_types.h"

namespace pml {

// ---- Canvas implementation ----------------------------------------------------------------------------------------------------

Canvas::Canvas(int width_,
               int height_,
               std::string bg_color_,
               std::string anchor_,
               int padding_,
               bool is_sprite_)
    : width(width_)
    , height(height_)
    , bg_color(std::move(bg_color_))
    , anchor(std::move(anchor_))
    , padding(padding_)
    , is_sprite(is_sprite_) {}

void Canvas::add(GraphicObject obj) {
    objects.push_back(std::move(obj));
}

// ---- Factory functions ------------------------------------------------------------------------------------------------------------

std::shared_ptr<Canvas>
_canvas(int w, int h, const std::unordered_map<std::string, Value>& kwargs) {
    std::string bg = "#FFFFFF";
    if (auto it = kwargs.find("bg"); it != kwargs.end()) {
        bg = value_to_string(it->second);
    }

    auto c = std::make_shared<Canvas>(w, h, std::move(bg));
    current_canvas_ref() = c;
    return c;
}

std::shared_ptr<Canvas>
_sprite_canvas(int w, int h, const std::unordered_map<std::string, Value>& kwargs) {
    // Background (default "transparent")
    std::string bg = "transparent";
    if (auto it = kwargs.find("bg"); it != kwargs.end()) {
        bg = value_to_string(it->second);
    }

    // Anchor (default Center-Bottom)
    std::string anchor = "center-bottom";
    if (auto it = kwargs.find("anchor"); it != kwargs.end()) {
        anchor = value_to_string(it->second);
    }

    // Padding (default 0)
    int padding = 0;
    if (auto it = kwargs.find("padding"); it != kwargs.end()) {
        const Value& v = it->second;
        if (v.is_int()) {
            padding = static_cast<int>(v.int_val());
        }
    }

    auto c = std::make_shared<Canvas>(w, h, std::move(bg), std::move(anchor), padding, true);
    current_canvas_ref() = c;
    return c;
}

} // namespace pml
