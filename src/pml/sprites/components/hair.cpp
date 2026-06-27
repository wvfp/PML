// ==========================================================================================================================================================================================================================================═
// PML Hair Component — Implementation
// ==========================================================================================================================================================================================================================================═

#include "hair.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace pml {

namespace {

using HairParts = std::vector<GraphicObject>;

constexpr double k_head_w = 56.0;

[[nodiscard]] Value poly(const std::vector<double>& coords) {
    std::vector<Value> values;
    values.reserve(coords.size());
    for (double c : coords) {
        values.emplace_back(c);
    }
    return make_list_value(std::move(values));
}

HairParts bangs(const std::string& color) {
    HairParts parts;
    double bw = k_head_w * 0.85;
    double bh = 14.0;
    parts.emplace_back(
        ShapeType::Rect,
        Params{{ParamKey::x, -bw / 2.0}, {ParamKey::y, -k_head_w / 2.0 - 4.0}, {ParamKey::w, bw}, {ParamKey::h, bh}},
        color,
        "#1a1a1a",
        1.5);
    double strand_spacing = bw / 6.0;
    for (int i = 0; i < 5; ++i) {
        double sx = -bw / 2.0 + strand_spacing * (i + 1);
        parts.emplace_back(
            ShapeType::Line,
            Params{{ParamKey::x1, sx}, {ParamKey::y1, -k_head_w / 2.0 - 4.0}, {ParamKey::x2, sx + 3.0}, {ParamKey::y2, -k_head_w / 2.0 - 4.0 + bh}},
            std::nullopt,
            "#1a1a1a",
            0.8);
    }
    return parts;
}

HairParts create_short(const std::string& color, double /*length*/, bool with_bangs, bool /*highlights*/) {
    HairParts parts;
    double cap_h = 16.0;
    parts.emplace_back(
        ShapeType::Ellipse,
        Params{{ParamKey::cx, 0.0}, {ParamKey::cy, -k_head_w / 2.0 + 4.0}, {ParamKey::rx, k_head_w / 2.0 + 4.0}, {ParamKey::ry, cap_h}},
        color,
        "#1a1a1a",
        1.5);
    for (double dx : {-k_head_w / 2.0 - 2.0, k_head_w / 2.0 + 2.0}) {
        parts.emplace_back(
            ShapeType::Ellipse,
            Params{{ParamKey::cx, dx}, {ParamKey::cy, -k_head_w / 4.0}, {ParamKey::rx, 6.0}, {ParamKey::ry, 12.0}},
            color,
            "#1a1a1a",
            1.0);
    }
    if (with_bangs) {
        auto b = bangs(color);
        parts.insert(parts.end(), b.begin(), b.end());
    }
    return parts;
}

HairParts create_medium(const std::string& color, double length, bool with_bangs, bool /*highlights*/) {
    HairParts parts;
    parts.emplace_back(
        ShapeType::Ellipse,
        Params{{ParamKey::cx, 0.0}, {ParamKey::cy, -k_head_w / 2.0 + 4.0}, {ParamKey::rx, k_head_w / 2.0 + 5.0}, {ParamKey::ry, 18.0}},
        color,
        "#1a1a1a",
        1.5);
    double side_h = std::min(length * 0.6, 40.0);
    for (double dx : {-k_head_w / 2.0 - 4.0, k_head_w / 2.0 + 4.0}) {
        parts.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, dx - 8.0}, {ParamKey::y, -k_head_w / 4.0}, {ParamKey::w, 16.0}, {ParamKey::h, side_h}},
            color,
            "#1a1a1a",
            1.0);
    }
    if (with_bangs) {
        auto b = bangs(color);
        parts.insert(parts.end(), b.begin(), b.end());
    }
    return parts;
}

HairParts create_long(const std::string& color, double length, bool with_bangs, bool /*highlights*/) {
    HairParts parts;
    parts.emplace_back(
        ShapeType::Ellipse,
        Params{{ParamKey::cx, 0.0}, {ParamKey::cy, -k_head_w / 2.0 + 4.0}, {ParamKey::rx, k_head_w / 2.0 + 6.0}, {ParamKey::ry, 20.0}},
        color,
        "#1a1a1a",
        1.5);
    double side_h = std::min(length * 0.8, 80.0);
    for (double dx : {-k_head_w / 2.0 - 5.0, k_head_w / 2.0 + 5.0}) {
        parts.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, dx - 10.0}, {ParamKey::y, -k_head_w / 4.0}, {ParamKey::w, 20.0}, {ParamKey::h, side_h}},
            color,
            "#1a1a1a",
            1.0);
    }
    parts.emplace_back(
        ShapeType::Rect,
        Params{{ParamKey::x, -k_head_w / 2.0 - 4.0}, {ParamKey::y, -k_head_w / 2.0 + 10.0}, {ParamKey::w, k_head_w + 8.0}, {ParamKey::h, length * 0.7}},
        color,
        "#1a1a1a",
        1.0);
    if (with_bangs) {
        auto b = bangs(color);
        parts.insert(parts.end(), b.begin(), b.end());
    }
    return parts;
}

HairParts create_ponytail(const std::string& color, double length, bool with_bangs, bool highlights) {
    HairParts parts = create_short(color, length, with_bangs, highlights);
    double tail_w = 16.0;
    double tail_h = std::min(length * 0.7, 60.0);
    parts.emplace_back(
        ShapeType::Rect,
        Params{{ParamKey::x, -tail_w / 2.0}, {ParamKey::y, -k_head_w / 2.0 - 8.0}, {ParamKey::w, tail_w}, {ParamKey::h, tail_h}},
        color,
        "#1a1a1a",
        1.5);
    parts.emplace_back(
        ShapeType::Ellipse,
        Params{{ParamKey::cx, 0.0}, {ParamKey::cy, -k_head_w / 2.0 - 8.0}, {ParamKey::rx, tail_w / 2.0 + 2.0}, {ParamKey::ry, 4.0}},
        "#c0392b",
        "#1a1a1a",
        1.0);
    return parts;
}

HairParts create_twintails(const std::string& color, double length, bool with_bangs, bool highlights) {
    HairParts parts = create_short(color, length, with_bangs, highlights);
    double tail_h = std::min(length * 0.6, 50.0);
    for (double dx : {-k_head_w / 2.0 - 8.0, k_head_w / 2.0 + 8.0}) {
        parts.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, dx - 7.0}, {ParamKey::y, -k_head_w / 4.0}, {ParamKey::w, 14.0}, {ParamKey::h, tail_h}},
            color,
            "#1a1a1a",
            1.5);
        parts.emplace_back(
            ShapeType::Ellipse,
            Params{{ParamKey::cx, dx}, {ParamKey::cy, -k_head_w / 4.0}, {ParamKey::rx, 9.0}, {ParamKey::ry, 4.0}},
            "#e74c3c",
            "#1a1a1a",
            1.0);
    }
    return parts;
}

HairParts create_spiky(const std::string& color, double /*length*/, bool with_bangs, bool /*highlights*/) {
    HairParts parts;
    double hw = k_head_w / 2.0 + 6.0;
    int spike_count = 7;
    for (int i = 0; i < spike_count; ++i) {
        double angle = -3.14159265358979 + (3.14159265358979 * i / (spike_count - 1));
        double bx = hw * std::cos(angle) * 0.9;
        double by = hw * std::sin(angle) * 0.9 - 4.0;
        double tx = hw * std::cos(angle) * 1.5;
        double ty = hw * std::sin(angle) * 1.5 - 4.0;
        double perp_x = -std::sin(angle) * 6.0;
        double perp_y = std::cos(angle) * 6.0;
        parts.emplace_back(
            ShapeType::Polygon,
            Params{
                {ParamKey::points,
                 poly({bx + perp_x, by + perp_y,
                       tx, ty,
                       bx - perp_x, by - perp_y})}},
            color,
            "#1a1a1a",
            1.5);
    }
    if (with_bangs) {
        auto b = bangs(color);
        parts.insert(parts.end(), b.begin(), b.end());
    }
    return parts;
}

HairParts create_bob(const std::string& color, double length, bool with_bangs, bool /*highlights*/) {
    HairParts parts;
    double cap_h = 20.0;
    parts.emplace_back(
        ShapeType::Ellipse,
        Params{{ParamKey::cx, 0.0}, {ParamKey::cy, -k_head_w / 2.0 + 4.0}, {ParamKey::rx, k_head_w / 2.0 + 6.0}, {ParamKey::ry, cap_h}},
        color,
        "#1a1a1a",
        1.5);
    double bob_h = std::min(length * 0.4, 28.0);
    for (double dx : {-k_head_w / 2.0 - 6.0, k_head_w / 2.0 + 6.0}) {
        parts.emplace_back(
            ShapeType::Ellipse,
            Params{{ParamKey::cx, dx}, {ParamKey::cy, 0.0}, {ParamKey::rx, 10.0}, {ParamKey::ry, bob_h}},
            color,
            "#1a1a1a",
            1.0);
    }
    if (with_bangs) {
        auto b = bangs(color);
        parts.insert(parts.end(), b.begin(), b.end());
    }
    return parts;
}

HairParts create_bald(const std::string& /*color*/, double /*length*/, bool /*bangs*/, bool /*highlights*/) {
    HairParts parts;
    parts.emplace_back(
        ShapeType::Ellipse,
        Params{{ParamKey::cx, -6.0}, {ParamKey::cy, -k_head_w / 2.0 + 8.0}, {ParamKey::rx, 8.0}, {ParamKey::ry, 5.0}},
        "#FFFFFF");
    return parts;
}

using HairMaker = HairParts(*)(const std::string&, double, bool, bool);

const std::unordered_map<std::string, HairMaker> k_hair_makers = {
    {"short", create_short},
    {"medium", create_medium},
    {"long", create_long},
    {"ponytail", create_ponytail},
    {"twintails", create_twintails},
    {"spiky", create_spiky},
    {"bob", create_bob},
    {"braid", create_long},
    {"bun", create_short},
    {"mohawk", create_spiky},
    {"bald", create_bald},
};

}  // anonymous namespace

// ==========================================================================================================================================================================================================================================═
// Schema
// ==========================================================================================================================================================================================================================================═

ParamSchema hair_schema() {
    return ParamSchema()
        .enum_("style",
               {"short", "medium", "long", "ponytail", "twintails", "spiky",
                "bob", "braid", "bun", "mohawk", "bald"},
               "medium")
        .color("color", "#2c2c2c")
        .number("length", 60.0, 10.0, 200.0)
        .boolean("bangs", true)
        .boolean("highlights", false);
}

// ==========================================================================================================================================================================================================================================═
// Factory
// ==========================================================================================================================================================================================================================================═

std::shared_ptr<GraphicObject> create_hair(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(hair_schema(), kwargs);

    std::string style = *p["style"].as_string();
    std::string color = *p["color"].as_string();
    double length = p["length"].double_val();
    bool bangs_flag = p["bangs"].bool_val();
    bool highlights = p["highlights"].bool_val();

    HairMaker maker = create_medium;
    auto it = k_hair_makers.find(style);
    if (it != k_hair_makers.end()) {
        maker = it->second;
    }

    HairParts parts = maker(color, length, bangs_flag, highlights);

    return std::make_shared<GraphicObject>(
        ShapeType::Group,
        Params{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(parts),
        std::unordered_map<std::string, Value>{
            {"component", std::string("hair")},
            {"style", style},
            {"color", color}});
}

}  // namespace pml
