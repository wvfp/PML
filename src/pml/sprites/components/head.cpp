// ==========================================================================================================================================================================================================================================═
// PML Head Component — Implementation
// ==========================================================================================================================================================================================================================================═

#include "head.h"

#include "pml/graphics/objects.h"
#include "pml/sprites/validator.h"

namespace pml {

namespace {

[[nodiscard]] Value make_polygon_points(const std::vector<double>& coords) {
    std::vector<Value> values;
    values.reserve(coords.size());
    for (double c : coords) {
        values.emplace_back(c);
    }
    return make_list_value(std::move(values));
}

}  // anonymous namespace

// ==========================================================================================================================================================================================================================================═
// Schema
// ==========================================================================================================================================================================================================================================═

ParamSchema head_schema() {
    return ParamSchema()
        .enum_("shape", {"oval", "round", "square", "heart", "angular"}, "oval")
        .color("skin", "#fce4c8")
        .enum_("ears", {"normal", "pointed", "animal", "none"}, "normal");
}

// ==========================================================================================================================================================================================================================================═
// Factory
// ==========================================================================================================================================================================================================================================═

std::shared_ptr<GraphicObject> create_head(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(head_schema(), kwargs);

    std::string shape = *p["shape"].as_string();
    std::string skin = *p["skin"].as_string();
    std::string ears = *p["ears"].as_string();

    constexpr double head_w = 56.0;
    constexpr double head_h = 64.0;

    GraphicObject head(ShapeType::Ellipse);

    if (shape == "oval" || shape == "heart") {
        head = GraphicObject(
            ShapeType::Ellipse, Params{{ParamKey::cx, 0.0}, {ParamKey::cy, 0.0}, {ParamKey::rx, head_w / 2.0}, {ParamKey::ry, head_h / 2.0}},
            skin,
            "#1a1a1a",
            2.0);
    } else if (shape == "round") {
        head = GraphicObject(
            ShapeType::Ellipse, Params{{ParamKey::cx, 0.0}, {ParamKey::cy, 0.0}, {ParamKey::rx, head_w / 2.0}, {ParamKey::ry, head_w / 2.0}},
            skin,
            "#1a1a1a",
            2.0);
    } else {  // square, angular, fallback
        head = GraphicObject(
            ShapeType::Rect, Params{{ParamKey::x, -head_w / 2.0},
             {ParamKey::y, -head_h / 2.0},
             {ParamKey::w, head_w},
             {ParamKey::h, head_h}},
            skin,
            "#1a1a1a",
            2.0);
    }

    std::vector<GraphicObject> children = {head};

    // Ears
    if (ears == "normal") {
        double ear_r = 6.0;
        for (double dx : {-head_w / 2.0 - ear_r, head_w / 2.0 + ear_r}) {
            children.emplace_back(
                ShapeType::Ellipse,
                Params{
                    {ParamKey::cx, dx}, {ParamKey::cy, -4.0}, {ParamKey::rx, ear_r}, {ParamKey::ry, ear_r * 1.2}},
                skin,
                "#1a1a1a",
                1.5);
        }
    } else if (ears == "pointed") {
        for (const auto& [dx, flip] : std::vector<std::pair<double, double>>{
                 {-head_w / 2.0 - 2.0, -1.0},
                 {head_w / 2.0 + 2.0, 1.0}}) {
            children.emplace_back(
                ShapeType::Polygon,
                Params{
                    {ParamKey::points,
                     make_polygon_points({dx, -4.0,
                                          dx + 8.0 * flip, -30.0,
                                          dx + 3.0 * flip, -2.0})}},
                skin,
                "#1a1a1a",
                1.5);
        }
    } else if (ears == "animal") {
        for (const auto& [dx, flip] : std::vector<std::pair<double, double>>{
                 {-head_w / 2.0 + 4.0, -1.0},
                 {head_w / 2.0 - 4.0, 1.0}}) {
            children.emplace_back(
                ShapeType::Polygon,
                Params{
                    {ParamKey::points,
                     make_polygon_points({dx - 6.0 * flip, -head_h / 2.0 + 4.0,
                                          dx + 4.0 * flip, -head_h / 2.0 - 18.0,
                                          dx + 12.0 * flip, -head_h / 2.0 + 6.0})}},
                skin,
                "#1a1a1a",
                1.5);
        }
    }
    // ears == "none" → no ear shapes

    return std::make_shared<GraphicObject>(
        ShapeType::Group,
        Params{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(children),
        std::unordered_map<std::string, Value>{
            {"component", std::string("head")},
            {"shape", shape},
            {"skin", skin},
            {"head_width", head_w},
            {"head_height", head_h}});
}

}  // namespace pml
