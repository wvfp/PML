// ==========================================================================================================================================================================================================================================═
// PML UI Widget Components — Implementation
// ==========================================================================================================================================================================================================================================═

#include "ui_widgets.h"

#include "pml/core/types.h"
#include "pml/graphics/objects.h"
#include "pml/sprites/validator.h"

#include <cmath>
#include <format>
#include <numbers>
#include <unordered_map>
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

[[nodiscard]] int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return 0;
}

[[nodiscard]] std::string shift_color(const std::string& hex_color, int amount) {
    if (hex_color.empty() || hex_color[0] != '#') return hex_color;
    std::string h = hex_color.substr(1);
    if (h.size() == 3) {
        h = std::string{h[0], h[0], h[1], h[1], h[2], h[2]};
    }
    if (h.size() != 6) return hex_color;

    auto component = [&](int offset) {
        int v = hex_char_to_int(h[offset]) * 16 + hex_char_to_int(h[offset + 1]);
        v += amount;
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        return v;
    };

    int r = component(0);
    int g = component(2);
    int b = component(4);
    return std::format("#{:02x}{:02x}{:02x}", r, g, b);
}

[[nodiscard]] std::string label_string(const Value& v) {
    if (const auto* s = v.as_string()) return *s;
    if (const auto* sym = v.as_symbol()) return sym->name;
    return value_to_string(v);
}

}  // anonymous namespace

// ==========================================================================================================================================================================================================================================═
// Schemas
// ==========================================================================================================================================================================================================================================═

ParamSchema button_schema() {
    return ParamSchema()
        .any_type("label", std::string("Button"))
        .number("width", 120.0, 40.0, 600.0)
        .number("height", 40.0, 20.0, 200.0)
        .enum_("style", {"rounded", "sharp", "pixel", "ornate"}, "rounded")
        .enum_("state", {"normal", "hover", "pressed", "disabled"}, "normal")
        .color("color", "#3498db")
        .color("text-color", "#FFFFFF");
}

ParamSchema panel_schema() {
    return ParamSchema()
        .number("width", 200.0, 60.0, 800.0)
        .number("height", 120.0, 40.0, 600.0)
        .enum_("style", {"simple", "ornate", "pixel", "glass"}, "simple")
        .any_type("title", std::string(""))
        .color("color", "#2c3e50")
        .number("border-width", 2.0, 0.0, 10.0);
}

ParamSchema health_bar_schema() {
    return ParamSchema()
        .number("value", 0.75, 0.0, 1.0)
        .number("width", 150.0, 40.0, 400.0)
        .number("height", 16.0, 6.0, 60.0)
        .color("color", "#e74c3c")
        .color("bg-color", "#2c3e50")
        .enum_("style", {"flat", "segmented", "gradient"}, "flat");
}

ParamSchema icon_schema() {
    return ParamSchema()
        .enum_("type", {"heart", "star", "coin", "gem", "shield", "skull"}, "heart")
        .number("size", 24.0, 8.0, 128.0)
        .color("color", "#e74c3c")
        .enum_("style", {"flat", "pixel", "detailed"}, "flat");
}

// ==========================================================================================================================================================================================================================================═
// Button
// ==========================================================================================================================================================================================================================================═

std::shared_ptr<GraphicObject> create_button(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(button_schema(), kwargs);

    double w = p["width"].double_val();
    double h = p["height"].double_val();
    std::string state = *p["state"].as_string();
    std::string base_color = *p["color"].as_string();
    std::string text_color = *p["text-color"].as_string();
    std::string style = *p["style"].as_string();

    int shift = 0;
    if (state == "hover") shift = 20;
    else if (state == "pressed") shift = -30;
    else if (state == "disabled") shift = -60;
    std::string color = shift_color(base_color, shift);

    std::vector<GraphicObject> children;

    if (style == "rounded") {
        double r = std::min(h / 3.0, 10.0);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -w / 2.0 + r}, {ParamKey::y, -h / 2.0}, {ParamKey::w, w - 2.0 * r}, {ParamKey::h, h}},
            color,
            "#1a1a1a",
            2.0);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -w / 2.0}, {ParamKey::y, -h / 2.0 + r}, {ParamKey::w, w}, {ParamKey::h, h - 2.0 * r}},
            color,
            std::nullopt,
            0.0);
        for (auto [cx, cy] : std::vector<std::pair<double, double>>{
                 {-w / 2.0 + r, -h / 2.0 + r},
                 {w / 2.0 - r, -h / 2.0 + r},
                 {-w / 2.0 + r, h / 2.0 - r},
                 {w / 2.0 - r, h / 2.0 - r}}) {
            children.emplace_back(
                ShapeType::Ellipse,
                Params{{ParamKey::cx, cx}, {ParamKey::cy, cy}, {ParamKey::rx, r}, {ParamKey::ry, r}},
                color,
                std::nullopt,
                0.0);
        }
    } else if (style == "pixel") {
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -w / 2.0}, {ParamKey::y, -h / 2.0}, {ParamKey::w, w}, {ParamKey::h, h}},
            color,
            "#000000",
            3.0);
        children.emplace_back(
            ShapeType::Line,
            Params{{ParamKey::x1, -w / 2.0 + 2.0}, {ParamKey::y1, -h / 2.0 + 2.0}, {ParamKey::x2, w / 2.0 - 2.0}, {ParamKey::y2, -h / 2.0 + 2.0}},
            std::nullopt,
            "#FFFFFF60",
            2.0);
    } else if (style == "ornate") {
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -w / 2.0}, {ParamKey::y, -h / 2.0}, {ParamKey::w, w}, {ParamKey::h, h}},
            color,
            "#d4ac0f",
            3.0);
        for (auto [cx, cy] : std::vector<std::pair<double, double>>{
                 {-w / 2.0, -h / 2.0},
                 {w / 2.0, -h / 2.0},
                 {-w / 2.0, h / 2.0},
                 {w / 2.0, h / 2.0}}) {
            children.emplace_back(
                ShapeType::Ellipse,
                Params{{ParamKey::cx, cx}, {ParamKey::cy, cy}, {ParamKey::rx, 4.0}, {ParamKey::ry, 4.0}},
                "#d4ac0f",
                std::nullopt,
                0.0);
        }
    } else {
        // sharp
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -w / 2.0}, {ParamKey::y, -h / 2.0}, {ParamKey::w, w}, {ParamKey::h, h}},
            color,
            "#1a1a1a",
            2.0);
    }

    std::string label = label_string(p["label"]);
    if (!label.empty()) {
        double x = -static_cast<double>(label.size()) * 3.5;
        children.emplace_back(
            ShapeType::Text,
            Params{{ParamKey::text, label}, {ParamKey::x, x}, {ParamKey::y, 4.0}, {ParamKey::font_size, static_cast<int64_t>(h * 0.4)}},
            text_color,
            std::nullopt,
            0.0);
    }

    return std::make_shared<GraphicObject>(
        ShapeType::Group,
        Params{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(children),
        std::unordered_map<std::string, Value>{{"component", std::string("button")}, {"state", state}});
}

// ==========================================================================================================================================================================================================================================═
// Panel
// ==========================================================================================================================================================================================================================================═

std::shared_ptr<GraphicObject> create_panel(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(panel_schema(), kwargs);

    double w = p["width"].double_val();
    double h = p["height"].double_val();
    double bw = p["border-width"].double_val();
    std::string style = *p["style"].as_string();
    std::string bg = *p["color"].as_string();
    std::string border_c = style == "ornate" ? "#d4ac0f" : "#1a1a1a";

    if (style == "glass") {
        bg += "80";  // semi-transparent
    }

    std::vector<GraphicObject> children;

    children.emplace_back(
        ShapeType::Rect,
        Params{{ParamKey::x, -w / 2.0}, {ParamKey::y, -h / 2.0}, {ParamKey::w, w}, {ParamKey::h, h}},
        bg,
        border_c,
        bw);

    std::string title = label_string(p["title"]);
    if (!title.empty()) {
        const double title_h = 20.0;
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -w / 2.0}, {ParamKey::y, -h / 2.0}, {ParamKey::w, w}, {ParamKey::h, title_h}},
            shift_color(bg, 20),
            border_c,
            1.0);
        children.emplace_back(
            ShapeType::Text,
            Params{{ParamKey::text, title}, {ParamKey::x, -w / 2.0 + 8.0}, {ParamKey::y, -h / 2.0 + 14.0}, {ParamKey::font_size, static_cast<int64_t>(12)}},
            "#FFFFFF",
            std::nullopt,
            0.0);
    }

    if (style == "ornate") {
        double corner_r = 6.0;
        for (auto [cx, cy] : std::vector<std::pair<double, double>>{
                 {-w / 2.0, -h / 2.0},
                 {w / 2.0, -h / 2.0},
                 {-w / 2.0, h / 2.0},
                 {w / 2.0, h / 2.0}}) {
            children.emplace_back(
                ShapeType::Ellipse,
                Params{{ParamKey::cx, cx}, {ParamKey::cy, cy}, {ParamKey::rx, corner_r}, {ParamKey::ry, corner_r}},
                "#d4ac0f",
                "#1a1a1a",
                1.0);
        }
    }

    return std::make_shared<GraphicObject>(
        ShapeType::Group,
        Params{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(children),
        std::unordered_map<std::string, Value>{{"component", std::string("panel")}, {"style", style}});
}

// ==========================================================================================================================================================================================================================================═
// Health Bar
// ==========================================================================================================================================================================================================================================═

std::shared_ptr<GraphicObject> create_health_bar(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(health_bar_schema(), kwargs);

    double w = p["width"].double_val();
    double h = p["height"].double_val();
    double value = p["value"].double_val();
    std::string bar_color = *p["color"].as_string();
    std::string bg_color = *p["bg-color"].as_string();
    std::string bar_style = *p["style"].as_string();

    std::vector<GraphicObject> children;

    children.emplace_back(
        ShapeType::Rect,
        Params{{ParamKey::x, -w / 2.0}, {ParamKey::y, -h / 2.0}, {ParamKey::w, w}, {ParamKey::h, h}},
        bg_color,
        "#1a1a1a",
        1.5);

    double fill_w = std::max(0.0, (w - 4.0) * value);

    if (bar_style == "segmented") {
        const int seg_count = 10;
        double seg_w = (w - 4.0) / seg_count;
        int filled_segs = static_cast<int>(seg_count * value);
        for (int i = 0; i < filled_segs; ++i) {
            children.emplace_back(
                ShapeType::Rect,
                Params{{ParamKey::x, -w / 2.0 + 2.0 + i * seg_w}, {ParamKey::y, -h / 2.0 + 2.0}, {ParamKey::w, seg_w - 1.0}, {ParamKey::h, h - 4.0}},
                bar_color,
                std::nullopt,
                0.0);
        }
    } else if (bar_style == "gradient") {
        double mid_w = fill_w / 2.0;
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -w / 2.0 + 2.0}, {ParamKey::y, -h / 2.0 + 2.0}, {ParamKey::w, mid_w}, {ParamKey::h, h - 4.0}},
            shift_color(bar_color, 30),
            std::nullopt,
            0.0);
        if (fill_w > mid_w) {
            children.emplace_back(
                ShapeType::Rect,
                Params{{ParamKey::x, -w / 2.0 + 2.0 + mid_w}, {ParamKey::y, -h / 2.0 + 2.0}, {ParamKey::w, fill_w - mid_w}, {ParamKey::h, h - 4.0}},
                bar_color,
                std::nullopt,
                0.0);
        }
    } else {
        // flat
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -w / 2.0 + 2.0}, {ParamKey::y, -h / 2.0 + 2.0}, {ParamKey::w, fill_w}, {ParamKey::h, h - 4.0}},
            bar_color,
            std::nullopt,
            0.0);
    }

    return std::make_shared<GraphicObject>(
        ShapeType::Group,
        Params{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(children),
        std::unordered_map<std::string, Value>{{"component", std::string("health-bar")}, {"value", value}});
}

// ==========================================================================================================================================================================================================================================═
// Icon
// ==========================================================================================================================================================================================================================================═

std::shared_ptr<GraphicObject> create_icon(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(icon_schema(), kwargs);

    std::string itype = *p["type"].as_string();
    double s = p["size"].double_val() / 24.0;
    std::string color = *p["color"].as_string();

    std::vector<GraphicObject> children;

    if (itype == "heart") {
        double r = 7.0 * s;
        children.emplace_back(
            ShapeType::Ellipse,
            Params{{ParamKey::cx, -5.0 * s}, {ParamKey::cy, -3.0 * s}, {ParamKey::rx, r}, {ParamKey::ry, r}},
            color,
            "#1a1a1a",
            1.5);
        children.emplace_back(
            ShapeType::Ellipse,
            Params{{ParamKey::cx, 5.0 * s}, {ParamKey::cy, -3.0 * s}, {ParamKey::rx, r}, {ParamKey::ry, r}},
            color,
            "#1a1a1a",
            1.5);
        children.emplace_back(
            ShapeType::Polygon,
            Params{{ParamKey::points, make_points({-12.0 * s, 0.0, 12.0 * s, 0.0, 0.0, 14.0 * s})}},
            color,
            "#1a1a1a",
            1.5);
    } else if (itype == "star") {
        std::vector<double> points;
        points.reserve(20);
        for (int i = 0; i < 10; ++i) {
            double angle = std::numbers::pi_v<double> / 2.0 + i * std::numbers::pi_v<double> / 5.0;
            double r = (i % 2 == 0) ? 12.0 * s : 5.0 * s;
            points.push_back(r * std::cos(angle));
            points.push_back(-r * std::sin(angle));
        }
        children.emplace_back(
            ShapeType::Polygon,
            Params{{ParamKey::points, make_points(points)}},
            color,
            "#1a1a1a",
            1.5);
    } else if (itype == "coin") {
        children.emplace_back(
            ShapeType::Ellipse,
            Params{{ParamKey::cx, 0.0}, {ParamKey::cy, 0.0}, {ParamKey::rx, 10.0 * s}, {ParamKey::ry, 10.0 * s}},
            color,
            "#1a1a1a",
            2.0);
        children.emplace_back(
            ShapeType::Ellipse,
            Params{{ParamKey::cx, 0.0}, {ParamKey::cy, 0.0}, {ParamKey::rx, 7.0 * s}, {ParamKey::ry, 7.0 * s}},
            std::nullopt,
            shift_color(color, -40),
            1.0);
    } else if (itype == "gem") {
        children.emplace_back(
            ShapeType::Polygon,
            Params{{ParamKey::points, make_points({
                0.0, -12.0 * s,
                10.0 * s, -4.0 * s,
                8.0 * s, 8.0 * s,
                -8.0 * s, 8.0 * s,
                -10.0 * s, -4.0 * s})}},
            color,
            "#1a1a1a",
            1.5);
        children.emplace_back(
            ShapeType::Line,
            Params{{ParamKey::x1, -4.0 * s}, {ParamKey::y1, -6.0 * s}, {ParamKey::x2, 2.0 * s}, {ParamKey::y2, -2.0 * s}},
            std::nullopt,
            "#FFFFFF80",
            2.0);
    } else if (itype == "shield") {
        children.emplace_back(
            ShapeType::Polygon,
            Params{{ParamKey::points, make_points({
                0.0, -12.0 * s,
                12.0 * s, -6.0 * s,
                10.0 * s, 8.0 * s,
                0.0, 14.0 * s,
                -10.0 * s, 8.0 * s,
                -12.0 * s, -6.0 * s})}},
            color,
            "#1a1a1a",
            2.0);
        children.emplace_back(
            ShapeType::Line,
            Params{{ParamKey::x1, 0.0}, {ParamKey::y1, -10.0 * s}, {ParamKey::x2, 0.0}, {ParamKey::y2, 12.0 * s}},
            std::nullopt,
            "#1a1a1a",
            1.5);
    } else if (itype == "skull") {
        children.emplace_back(
            ShapeType::Ellipse,
            Params{{ParamKey::cx, 0.0}, {ParamKey::cy, -4.0 * s}, {ParamKey::rx, 10.0 * s}, {ParamKey::ry, 10.0 * s}},
            color,
            "#1a1a1a",
            2.0);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -6.0 * s}, {ParamKey::y, 4.0 * s}, {ParamKey::w, 12.0 * s}, {ParamKey::h, 6.0 * s}},
            color,
            "#1a1a1a",
            1.5);
        for (double dx : {-4.0 * s, 4.0 * s}) {
            children.emplace_back(
                ShapeType::Ellipse,
                Params{{ParamKey::cx, dx}, {ParamKey::cy, -5.0 * s}, {ParamKey::rx, 3.0 * s}, {ParamKey::ry, 3.0 * s}},
                "#1a1a1a",
                std::nullopt,
                0.0);
        }
    }

    return std::make_shared<GraphicObject>(
        ShapeType::Group,
        Params{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(children),
        std::unordered_map<std::string, Value>{{"component", std::string("icon")}, {"type", itype}});
}

}  // namespace pml
