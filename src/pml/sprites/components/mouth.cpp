// ==========================================================================================================================================================================================================================================═
// PML Mouth Component — Implementation
// ==========================================================================================================================================================================================================================================═

#include "mouth.h"

#include <vector>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// Schema
// ==========================================================================================================================================================================================================================================═

ParamSchema mouth_schema() {
    return ParamSchema()
        .enum_("style", {"neutral", "smile", "frown", "open", "cat", "fang"}, "neutral")
        .number("size", 1.0, 0.3, 3.0);
}

// ==========================================================================================================================================================================================================================================═
// Factory
// ==========================================================================================================================================================================================================================================═

std::shared_ptr<GraphicObject> create_mouth(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(mouth_schema(), kwargs);

    std::string style = *p["style"].as_string();
    double s = p["size"].double_val();
    double w = 12.0 * s;

    std::vector<GraphicObject> children;

    if (style == "neutral") {
        children.emplace_back(
            "line",
            Params{{ParamKey::x1, -w / 2.0}, {ParamKey::y1, 0.0}, {ParamKey::x2, w / 2.0}, {ParamKey::y2, 0.0}},
            std::nullopt,
            "#c0392b",
            2.0 * s);
    } else if (style == "smile") {
        children.emplace_back(
            "line",
            Params{{ParamKey::x1, -w / 2.0}, {ParamKey::y1, -2.0 * s}, {ParamKey::x2, 0.0}, {ParamKey::y2, 4.0 * s}},
            std::nullopt,
            "#c0392b",
            2.0 * s);
        children.emplace_back(
            "line",
            Params{{ParamKey::x1, 0.0}, {ParamKey::y1, 4.0 * s}, {ParamKey::x2, w / 2.0}, {ParamKey::y2, -2.0 * s}},
            std::nullopt,
            "#c0392b",
            2.0 * s);
    } else if (style == "frown") {
        children.emplace_back(
            "line",
            Params{{ParamKey::x1, -w / 2.0}, {ParamKey::y1, 4.0 * s}, {ParamKey::x2, 0.0}, {ParamKey::y2, -2.0 * s}},
            std::nullopt,
            "#c0392b",
            2.0 * s);
        children.emplace_back(
            "line",
            Params{{ParamKey::x1, 0.0}, {ParamKey::y1, -2.0 * s}, {ParamKey::x2, w / 2.0}, {ParamKey::y2, 4.0 * s}},
            std::nullopt,
            "#c0392b",
            2.0 * s);
    } else if (style == "open") {
        children.emplace_back(
            "ellipse",
            Params{{ParamKey::cx, 0.0}, {ParamKey::cy, 2.0 * s}, {ParamKey::rx, w / 2.0}, {ParamKey::ry, 6.0 * s}},
            "#3d1010",
            "#c0392b",
            1.5);
        children.emplace_back(
            "rect",
            Params{{ParamKey::x, -w / 3.0}, {ParamKey::y, -1.0 * s}, {ParamKey::w, w * 2.0 / 3.0}, {ParamKey::h, 3.0 * s}},
            "#FFFFFF");
    } else if (style == "cat") {
        double cat_w = 8.0 * s;
        for (double dx : {-cat_w, 0.0, cat_w}) {
            children.emplace_back(
                "ellipse",
                Params{{ParamKey::cx, dx}, {ParamKey::cy, 2.0 * s}, {ParamKey::rx, 4.0 * s}, {ParamKey::ry, 3.0 * s}},
                std::nullopt,
                "#c0392b",
                1.5);
        }
    } else if (style == "fang") {
        children.emplace_back(
            "line",
            Params{{ParamKey::x1, -w / 2.0}, {ParamKey::y1, -2.0 * s}, {ParamKey::x2, w / 2.0}, {ParamKey::y2, -2.0 * s}},
            std::nullopt,
            "#c0392b",
            2.0 * s);
        double fang_x = w / 3.0;
        children.emplace_back(
            "polygon",
            Params{
                {ParamKey::points,
                 make_list_value(std::vector<Value>{
                     fang_x - 3.0 * s, -2.0 * s,
                     fang_x + 3.0 * s, -2.0 * s,
                     fang_x, 6.0 * s})}},
            "#FFFFFF",
            "#c0392b",
            1.0);
    }

    return std::make_shared<GraphicObject>(
        "group",
        Params{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(children),
        std::unordered_map<std::string, Value>{
            {"component", std::string("mouth")},
            {"style", style}});
}

}  // namespace pml
