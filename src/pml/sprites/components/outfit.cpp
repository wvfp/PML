// ═══════════════════════════════════════════════════════════════════════════════
// PML Outfit Component — Implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "outfit.h"

#include "pml/graphics/objects.h"
#include "pml/sprites/validator.h"

#include <vector>

namespace pml {

namespace {

[[nodiscard]] Value make_points(const std::vector<double>& coords) {
    std::vector<Value> values;
    values.reserve(coords.size());
    for (double c : coords) {
        values.emplace_back(c);
    }
    return std::make_shared<ValueList>(std::move(values));
}

[[nodiscard]] std::vector<GraphicObject> draw_top(
    const std::string& top,
    const std::string& color,
    const std::string& detail,
    double torso_w,
    double torso_h) {
    std::vector<GraphicObject> parts;

    if (top == "armor") {
        parts.emplace_back(
            "rect",
            Params{
                {ParamKey::x, -torso_w / 2.0 + 2.0},
                {ParamKey::y, 2.0},
                {ParamKey::w, torso_w - 4.0},
                {ParamKey::h, torso_h * 0.6}},
            "#7f8c8d",
            "#2c3e50",
            2.0);
        for (double dx : {-torso_w / 2.0 - 4.0, torso_w / 2.0 - 8.0}) {
            parts.emplace_back(
                "ellipse",
                Params{
                    {ParamKey::cx, dx + 6.0},
                    {ParamKey::cy, torso_h * 0.08},
                    {ParamKey::rx, 12.0},
                    {ParamKey::ry, 8.0}},
                "#95a5a6",
                "#2c3e50",
                1.5);
        }
    } else if (top == "robe") {
        parts.emplace_back(
            "rect",
            Params{
                {ParamKey::x, -torso_w / 2.0 - 4.0},
                {ParamKey::y, -4.0},
                {ParamKey::w, torso_w + 8.0},
                {ParamKey::h, torso_h + 8.0}},
            color,
            "#1a1a1a",
            2.0);
        parts.emplace_back(
            "rect",
            Params{
                {ParamKey::x, -torso_w / 2.0},
                {ParamKey::y, torso_h * 0.5},
                {ParamKey::w, torso_w},
                {ParamKey::h, 6.0}},
            "#8B4513",
            "#1a1a1a",
            1.0);
    } else if (top == "hoodie") {
        parts.emplace_back(
            "rect",
            Params{
                {ParamKey::x, -torso_w / 2.0 - 2.0},
                {ParamKey::y, 0.0},
                {ParamKey::w, torso_w + 4.0},
                {ParamKey::h, torso_h + 4.0}},
            color,
            "#1a1a1a",
            2.0);
        parts.emplace_back(
            "ellipse",
            Params{
                {ParamKey::cx, 0.0},
                {ParamKey::cy, -8.0},
                {ParamKey::rx, torso_w * 0.4},
                {ParamKey::ry, 14.0}},
            color,
            "#1a1a1a",
            1.5);
        parts.emplace_back(
            "rect",
            Params{
                {ParamKey::x, -torso_w * 0.3},
                {ParamKey::y, torso_h * 0.55},
                {ParamKey::w, torso_w * 0.6},
                {ParamKey::h, torso_h * 0.2}},
            std::nullopt,
            "#1a1a1a",
            1.0);
    } else if (top == "dress") {
        parts.emplace_back(
            "rect",
            Params{
                {ParamKey::x, -torso_w / 2.0 - 2.0},
                {ParamKey::y, 0.0},
                {ParamKey::w, torso_w + 4.0},
                {ParamKey::h, torso_h * 1.2}},
            color,
            "#1a1a1a",
            2.0);
    } else if (top == "suit") {
        parts.emplace_back(
            "rect",
            Params{
                {ParamKey::x, -torso_w / 2.0},
                {ParamKey::y, 0.0},
                {ParamKey::w, torso_w},
                {ParamKey::h, torso_h}},
            color,
            "#1a1a1a",
            2.0);
        parts.emplace_back(
            "line",
            Params{
                {ParamKey::x1, 0.0},
                {ParamKey::y1, 0.0},
                {ParamKey::x2, -torso_w * 0.3},
                {ParamKey::y2, torso_h * 0.35}},
            std::nullopt,
            "#1a1a1a",
            1.5);
        parts.emplace_back(
            "line",
            Params{
                {ParamKey::x1, 0.0},
                {ParamKey::y1, 0.0},
                {ParamKey::x2, torso_w * 0.3},
                {ParamKey::y2, torso_h * 0.35}},
            std::nullopt,
            "#1a1a1a",
            1.5);
        parts.emplace_back(
            "polygon",
            Params{{ParamKey::points, make_points({
                0.0, 2.0,
                -4.0, torso_h * 0.4,
                0.0, torso_h * 0.5,
                4.0, torso_h * 0.4})}},
            "#c0392b",
            "#1a1a1a",
            1.0);
    } else if (top == "sailor") {
        parts.emplace_back(
            "rect",
            Params{
                {ParamKey::x, -torso_w / 2.0},
                {ParamKey::y, 0.0},
                {ParamKey::w, torso_w},
                {ParamKey::h, torso_h}},
            "#FFFFFF",
            "#1a1a1a",
            2.0);
        parts.emplace_back(
            "polygon",
            Params{{ParamKey::points, make_points({
                -torso_w * 0.4, 0.0,
                0.0, torso_h * 0.3,
                torso_w * 0.4, 0.0})}},
            "#2c3e50",
            "#1a1a1a",
            1.5);
    } else if (top == "tank") {
        parts.emplace_back(
            "rect",
            Params{
                {ParamKey::x, -torso_w / 2.0 + 6.0},
                {ParamKey::y, 0.0},
                {ParamKey::w, torso_w - 12.0},
                {ParamKey::h, torso_h}},
            color,
            "#1a1a1a",
            2.0);
    } else {
        // t-shirt / jacket default
        parts.emplace_back(
            "rect",
            Params{
                {ParamKey::x, -torso_w / 2.0},
                {ParamKey::y, 0.0},
                {ParamKey::w, torso_w},
                {ParamKey::h, torso_h}},
            color,
            "#1a1a1a",
            2.0);
        if (top == "jacket") {
            parts.emplace_back(
                "line",
                Params{
                    {ParamKey::x1, 0.0},
                    {ParamKey::y1, 0.0},
                    {ParamKey::x2, 0.0},
                    {ParamKey::y2, torso_h}},
                std::nullopt,
                "#1a1a1a",
                1.5);
        }
    }

    if (detail == "striped") {
        for (int i = 0; i < 3; ++i) {
            double y = torso_h * 0.2 + i * torso_h * 0.25;
            parts.emplace_back(
                "line",
                Params{
                    {ParamKey::x1, -torso_w / 2.0 + 4.0},
                    {ParamKey::y1, y},
                    {ParamKey::x2, torso_w / 2.0 - 4.0},
                    {ParamKey::y2, y}},
                std::nullopt,
                "#FFFFFF80",
                2.0);
        }
    } else if (detail == "badge") {
        parts.emplace_back(
            "ellipse",
            Params{
                {ParamKey::cx, torso_w * 0.25},
                {ParamKey::cy, torso_h * 0.2},
                {ParamKey::rx, 6.0},
                {ParamKey::ry, 6.0}},
            "#e74c3c",
            "#1a1a1a",
            1.0);
    }

    return parts;
}

[[nodiscard]] std::vector<GraphicObject> draw_bottom(
    const std::string& bottom,
    const std::string& color,
    double torso_w,
    double torso_h) {
    std::vector<GraphicObject> parts;
    double leg_h = torso_h * 0.7;
    double leg_w = torso_w * 0.38;
    double leg_y = torso_h;

    if (bottom == "skirt") {
        parts.emplace_back(
            "polygon",
            Params{{ParamKey::points, make_points({
                -torso_w / 2.0, leg_y,
                torso_w / 2.0, leg_y,
                torso_w / 2.0 + 8.0, leg_y + leg_h * 0.6,
                -torso_w / 2.0 - 8.0, leg_y + leg_h * 0.6})}},
            color,
            "#1a1a1a",
            2.0);
    } else if (bottom == "long-skirt") {
        parts.emplace_back(
            "polygon",
            Params{{ParamKey::points, make_points({
                -torso_w / 2.0, leg_y,
                torso_w / 2.0, leg_y,
                torso_w / 2.0 + 12.0, leg_y + leg_h,
                -torso_w / 2.0 - 12.0, leg_y + leg_h})}},
            color,
            "#1a1a1a",
            2.0);
    } else if (bottom == "shorts") {
        for (double dx : {-leg_w / 2.0 - 2.0, leg_w / 2.0 + 2.0}) {
            parts.emplace_back(
                "rect",
                Params{
                    {ParamKey::x, dx - leg_w / 2.0},
                    {ParamKey::y, leg_y},
                    {ParamKey::w, leg_w},
                    {ParamKey::h, leg_h * 0.4}},
                color,
                "#1a1a1a",
                2.0);
        }
    } else if (bottom == "armor") {
        for (double dx : {-leg_w / 2.0 - 2.0, leg_w / 2.0 + 2.0}) {
            parts.emplace_back(
                "rect",
                Params{
                    {ParamKey::x, dx - leg_w / 2.0},
                    {ParamKey::y, leg_y},
                    {ParamKey::w, leg_w},
                    {ParamKey::h, leg_h}},
                "#7f8c8d",
                "#2c3e50",
                2.0);
        }
    } else {
        // pants
        for (double dx : {-leg_w / 2.0 - 2.0, leg_w / 2.0 + 2.0}) {
            parts.emplace_back(
                "rect",
                Params{
                    {ParamKey::x, dx - leg_w / 2.0},
                    {ParamKey::y, leg_y},
                    {ParamKey::w, leg_w},
                    {ParamKey::h, leg_h}},
                color,
                "#1a1a1a",
                2.0);
        }
    }

    return parts;
}

[[nodiscard]] std::vector<GraphicObject> draw_shoes(
    const std::string& shoes,
    double torso_w,
    double torso_h) {
    if (shoes == "none") {
        return {};
    }

    std::vector<GraphicObject> parts;
    double leg_h = torso_h * 0.7;
    double leg_y = torso_h + leg_h;
    double shoe_w = torso_w * 0.22;
    std::string shoe_color = "#2c2c2c";
    double shoe_h = 10.0;

    if (shoes == "boots") {
        shoe_h = 18.0;
        shoe_color = "#5D4037";
    } else if (shoes == "sneakers") {
        shoe_color = "#FFFFFF";
    } else if (shoes == "sandals") {
        shoe_color = "#8B4513";
        shoe_h = 6.0;
    } else if (shoes == "heels") {
        shoe_color = "#c0392b";
        shoe_h = 14.0;
    }

    for (double dx : {-torso_w * 0.2, torso_w * 0.2}) {
        parts.emplace_back(
            "rect",
            Params{
                {ParamKey::x, dx - shoe_w / 2.0},
                {ParamKey::y, leg_y - 2.0},
                {ParamKey::w, shoe_w},
                {ParamKey::h, shoe_h}},
            shoe_color,
            "#1a1a1a",
            1.5);
    }

    return parts;
}

}  // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// Schema
// ═══════════════════════════════════════════════════════════════════════════════

ParamSchema outfit_schema() {
    return ParamSchema()
        .enum_("top", {"t-shirt", "jacket", "hoodie", "dress", "armor", "robe", "suit", "tank", "sailor"}, "t-shirt")
        .enum_("bottom", {"pants", "skirt", "shorts", "long-skirt", "armor"}, "pants")
        .enum_("shoes", {"boots", "sneakers", "sandals", "heels", "none"}, "boots")
        .color("color-top", "#3498db")
        .color("color-bottom", "#2c3e50")
        .enum_("detail", {"plain", "striped", "pattern", "badge"}, "plain");
}

// ═══════════════════════════════════════════════════════════════════════════════
// Factory
// ═══════════════════════════════════════════════════════════════════════════════

std::shared_ptr<GraphicObject> create_outfit(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(outfit_schema(), kwargs);

    std::string top = *p["top"].as_string();
    std::string bottom = *p["bottom"].as_string();
    std::string shoes = *p["shoes"].as_string();
    std::string color_top = *p["color-top"].as_string();
    std::string color_bottom = *p["color-bottom"].as_string();
    std::string detail = *p["detail"].as_string();

    const double torso_w = 50.0;
    const double torso_h = 64.0;

    auto top_parts = draw_top(top, color_top, detail, torso_w, torso_h);
    auto bottom_parts = draw_bottom(bottom, color_bottom, torso_w, torso_h);
    auto shoe_parts = draw_shoes(shoes, torso_w, torso_h);

    std::vector<GraphicObject> all;
    all.reserve(top_parts.size() + bottom_parts.size() + shoe_parts.size());
    all.insert(all.end(), top_parts.begin(), top_parts.end());
    all.insert(all.end(), bottom_parts.begin(), bottom_parts.end());
    all.insert(all.end(), shoe_parts.begin(), shoe_parts.end());

    return std::make_shared<GraphicObject>(
        "group",
        Params{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(all),
        std::unordered_map<std::string, Value>{
            {"component", std::string("outfit")},
            {"top", top},
            {"bottom", bottom},
            {"shoes", shoes}});
}

}  // namespace pml
