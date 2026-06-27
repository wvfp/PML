// ==========================================================================================================================================================================================================================================═
// PML Item Components — Implementation
// ==========================================================================================================================================================================================================================================═

#include "items.h"

#include "pml/graphics/objects.h"
#include "pml/sprites/validator.h"

#include <cmath>
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

const std::unordered_map<std::string, std::string> k_material_colors = {
    {"iron", "#7f8c8d"},
    {"steel", "#bdc3c7"},
    {"gold", "#f1c40f"},
    {"crystal", "#85c1e9"},
    {"wood", "#8B4513"},
    {"bone", "#f5f5dc"}};

const std::unordered_map<std::string, std::string> k_element_colors = {
    {"fire", "#e74c3c"},
    {"ice", "#3498db"},
    {"lightning", "#f1c40f"},
    {"holy", "#f9e79f"},
    {"dark", "#6c3483"}};

const std::unordered_map<std::string, std::string> k_potion_colors = {
    {"health", "#e74c3c"},
    {"mana", "#3498db"},
    {"buff", "#f1c40f"},
    {"poison", "#27ae60"},
    {"bomb", "#e67e22"}};

const std::unordered_map<std::string, double> k_size_scale = {
    {"small", 0.7},
    {"medium", 1.0},
    {"large", 1.4},
    {"legendary", 1.8}};

[[nodiscard]] double get_size_scale(const std::string& size) {
    auto it = k_size_scale.find(size);
    return it != k_size_scale.end() ? it->second : 1.0;
}

[[nodiscard]] std::string get_material_color(const std::string& material) {
    auto it = k_material_colors.find(material);
    return it != k_material_colors.end() ? it->second : "#bdc3c7";
}

[[nodiscard]] std::string get_element_color(const std::string& element) {
    auto it = k_element_colors.find(element);
    return it != k_element_colors.end() ? it->second : "#85c1e9";
}

[[nodiscard]] std::string get_potion_color(const std::string& type) {
    auto it = k_potion_colors.find(type);
    return it != k_potion_colors.end() ? it->second : "#e74c3c";
}

[[nodiscard]] std::vector<GraphicObject> element_glow(
    const std::string& element, double cx, double cy, double r) {
    if (element == "none") return {};
    auto it = k_element_colors.find(element);
    if (it == k_element_colors.end()) return {};
    return {GraphicObject(
        ShapeType::Ellipse,
        Params{{ParamKey::cx, cx}, {ParamKey::cy, cy}, {ParamKey::rx, r}, {ParamKey::ry, r}},
        it->second + "40",
        it->second,
        1.0)};
}

[[nodiscard]] std::vector<GraphicObject> ornament_deco(
    const std::string& ornament, double x, double y) {
    if (ornament == "plain") return {};
    if (ornament == "gem") {
        return {GraphicObject(
            ShapeType::Ellipse,
            Params{{ParamKey::cx, x}, {ParamKey::cy, y}, {ParamKey::rx, 4.0}, {ParamKey::ry, 4.0}},
            "#e74c3c",
            "#1a1a1a",
            1.0)};
    }
    if (ornament == "rune") {
        return {GraphicObject(
            ShapeType::Line,
            Params{{ParamKey::x1, x - 3.0}, {ParamKey::y1, y - 3.0}, {ParamKey::x2, x + 3.0}, {ParamKey::y2, y + 3.0}},
            std::nullopt,
            "#85c1e9",
            1.5)};
    }
    if (ornament == "engraved") {
        return {GraphicObject(
            ShapeType::Line,
            Params{{ParamKey::x1, x - 4.0}, {ParamKey::y1, y}, {ParamKey::x2, x + 4.0}, {ParamKey::y2, y}},
            std::nullopt,
            "#1a1a1a",
            0.8)};
    }
    return {};
}

}  // anonymous namespace

// ==========================================================================================================================================================================================================================================═
// Schemas
// ==========================================================================================================================================================================================================================================═

ParamSchema weapon_schema() {
    return ParamSchema()
        .enum_("type", {"sword", "axe", "bow", "staff", "dagger", "spear", "gun"}, "sword")
        .enum_("size", {"small", "medium", "large", "legendary"}, "medium")
        .enum_("material", {"iron", "steel", "gold", "crystal", "wood", "bone"}, "steel")
        .enum_("element", {"none", "fire", "ice", "lightning", "holy", "dark"}, "none")
        .enum_("ornament", {"plain", "gem", "rune", "engraved"}, "plain");
}

ParamSchema potion_schema() {
    return ParamSchema()
        .enum_("type", {"health", "mana", "buff", "poison", "bomb"}, "health")
        .enum_("container", {"bottle", "flask", "vial", "jar"}, "bottle")
        .color("color", "#e74c3c")
        .enum_("size", {"small", "medium", "large"}, "medium");
}

ParamSchema chest_schema() {
    return ParamSchema()
        .enum_("type", {"wooden", "iron", "gold", "crystal"}, "wooden")
        .enum_("state", {"closed", "open", "locked"}, "closed")
        .enum_("size", {"small", "medium", "large"}, "medium");
}

ParamSchema generic_item_schema() {
    return ParamSchema()
        .any_type("name", std::string("item"))
        .enum_("base-shape", {"circle", "rect", "diamond", "custom"}, "circle")
        .color("color", "#95a5a6")
        .any_type("detail", nullptr)
        .boolean("outline", true);
}

// ==========================================================================================================================================================================================================================================═
// Weapon
// ==========================================================================================================================================================================================================================================═

std::shared_ptr<GraphicObject> create_weapon(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(weapon_schema(), kwargs);

    std::string wtype = *p["type"].as_string();
    double s = get_size_scale(*p["size"].as_string());
    std::string mat_color = get_material_color(*p["material"].as_string());
    std::string element = *p["element"].as_string();
    std::string ornament = *p["ornament"].as_string();

    std::vector<GraphicObject> children;

    if (wtype == "sword") {
        double blade_h = 60.0 * s;
        children.emplace_back(
            ShapeType::Polygon,
            Params{{ParamKey::points, make_points({0.0, 0.0, -5.0 * s, -blade_h, 5.0 * s, -blade_h})}},
            mat_color,
            "#1a1a1a",
            1.5);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -12.0 * s}, {ParamKey::y, 0.0}, {ParamKey::w, 24.0 * s}, {ParamKey::h, 4.0 * s}},
            "#8B4513",
            "#1a1a1a",
            1.0);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -3.0 * s}, {ParamKey::y, 4.0 * s}, {ParamKey::w, 6.0 * s}, {ParamKey::h, 16.0 * s}},
            "#5D4037",
            "#1a1a1a",
            1.0);
        auto decos = ornament_deco(ornament, 0.0, -blade_h * 0.5);
        children.insert(children.end(), decos.begin(), decos.end());
    } else if (wtype == "axe") {
        double handle_h = 50.0 * s;
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -2.0 * s}, {ParamKey::y, 0.0}, {ParamKey::w, 4.0 * s}, {ParamKey::h, handle_h}},
            "#8B4513",
            "#1a1a1a",
            1.0);
        children.emplace_back(
            ShapeType::Polygon,
            Params{{ParamKey::points, make_points({
                2.0 * s, 4.0 * s,
                20.0 * s, -8.0 * s,
                20.0 * s, 12.0 * s})}},
            mat_color,
            "#1a1a1a",
            1.5);
    } else if (wtype == "bow") {
        double bow_h = 50.0 * s;
        children.emplace_back(
            ShapeType::Line,
            Params{{ParamKey::x1, 0.0}, {ParamKey::y1, -bow_h / 2.0}, {ParamKey::x2, 8.0 * s}, {ParamKey::y2, 0.0}},
            std::nullopt,
            mat_color,
            3.0 * s);
        children.emplace_back(
            ShapeType::Line,
            Params{{ParamKey::x1, 8.0 * s}, {ParamKey::y1, 0.0}, {ParamKey::x2, 0.0}, {ParamKey::y2, bow_h / 2.0}},
            std::nullopt,
            mat_color,
            3.0 * s);
        children.emplace_back(
            ShapeType::Line,
            Params{{ParamKey::x1, 0.0}, {ParamKey::y1, -bow_h / 2.0}, {ParamKey::x2, 0.0}, {ParamKey::y2, bow_h / 2.0}},
            std::nullopt,
            "#f5f5dc",
            1.0);
    } else if (wtype == "staff") {
        double staff_h = 80.0 * s;
        std::string material = *p["material"].as_string();
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -2.0 * s}, {ParamKey::y, -staff_h}, {ParamKey::w, 4.0 * s}, {ParamKey::h, staff_h}},
            material == "wood" ? mat_color : "#5D4037",
            "#1a1a1a",
            1.5);
        children.emplace_back(
            ShapeType::Ellipse,
            Params{{ParamKey::cx, 0.0}, {ParamKey::cy, -staff_h - 8.0 * s}, {ParamKey::rx, 8.0 * s}, {ParamKey::ry, 8.0 * s}},
            get_element_color(element),
            "#1a1a1a",
            1.5);
        auto glow = element_glow(element, 0.0, -staff_h - 8.0 * s, 12.0 * s);
        children.insert(children.end(), glow.begin(), glow.end());
    } else if (wtype == "dagger") {
        double blade_h = 30.0 * s;
        children.emplace_back(
            ShapeType::Polygon,
            Params{{ParamKey::points, make_points({0.0, 0.0, -4.0 * s, -blade_h, 4.0 * s, -blade_h})}},
            mat_color,
            "#1a1a1a",
            1.5);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -8.0 * s}, {ParamKey::y, 0.0}, {ParamKey::w, 16.0 * s}, {ParamKey::h, 3.0 * s}},
            "#8B4513",
            "#1a1a1a",
            1.0);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -2.0 * s}, {ParamKey::y, 3.0 * s}, {ParamKey::w, 4.0 * s}, {ParamKey::h, 10.0 * s}},
            "#5D4037",
            "#1a1a1a",
            1.0);
    } else if (wtype == "spear") {
        double shaft_h = 80.0 * s;
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -2.0 * s}, {ParamKey::y, -shaft_h}, {ParamKey::w, 4.0 * s}, {ParamKey::h, shaft_h}},
            "#8B4513",
            "#1a1a1a",
            1.0);
        children.emplace_back(
            ShapeType::Polygon,
            Params{{ParamKey::points, make_points({
                0.0, -shaft_h - 16.0 * s,
                -5.0 * s, -shaft_h,
                5.0 * s, -shaft_h})}},
            mat_color,
            "#1a1a1a",
            1.5);
    } else if (wtype == "gun") {
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, 0.0}, {ParamKey::y, -4.0 * s}, {ParamKey::w, 30.0 * s}, {ParamKey::h, 8.0 * s}},
            mat_color,
            "#1a1a1a",
            1.5);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, 2.0 * s}, {ParamKey::y, 4.0 * s}, {ParamKey::w, 8.0 * s}, {ParamKey::h, 16.0 * s}},
            "#5D4037",
            "#1a1a1a",
            1.0);
    }

    auto glow = element_glow(element, 0.0, -20.0 * s, 20.0 * s);
    children.insert(children.end(), glow.begin(), glow.end());

    return std::make_shared<GraphicObject>(
        ShapeType::Group,
        Params{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(children),
        std::unordered_map<std::string, Value>{{"component", std::string("weapon")}, {"type", wtype}, {"size", *p["size"].as_string()}});
}

// ==========================================================================================================================================================================================================================================═
// Potion
// ==========================================================================================================================================================================================================================================═

std::shared_ptr<GraphicObject> create_potion(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(potion_schema(), kwargs);

    std::string type = *p["type"].as_string();
    std::string container = *p["container"].as_string();
    double s = get_size_scale(*p["size"].as_string());
    std::string liquid_color = get_potion_color(type);
    if (type != "health" && type != "mana" && type != "buff" && type != "poison" && type != "bomb") {
        liquid_color = *p["color"].as_string();
    }

    std::vector<GraphicObject> children;

    if (container == "bottle") {
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -10.0 * s}, {ParamKey::y, -20.0 * s}, {ParamKey::w, 20.0 * s}, {ParamKey::h, 24.0 * s}},
            "#d5e8f0",
            "#1a1a1a",
            1.5);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -8.0 * s}, {ParamKey::y, -14.0 * s}, {ParamKey::w, 16.0 * s}, {ParamKey::h, 16.0 * s}},
            liquid_color,
            std::nullopt,
            0.0);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -4.0 * s}, {ParamKey::y, -28.0 * s}, {ParamKey::w, 8.0 * s}, {ParamKey::h, 10.0 * s}},
            "#d5e8f0",
            "#1a1a1a",
            1.0);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -5.0 * s}, {ParamKey::y, -32.0 * s}, {ParamKey::w, 10.0 * s}, {ParamKey::h, 5.0 * s}},
            "#8B4513",
            "#1a1a1a",
            1.0);
    } else if (container == "flask") {
        children.emplace_back(
            ShapeType::Ellipse,
            Params{{ParamKey::cx, 0.0}, {ParamKey::cy, -8.0 * s}, {ParamKey::rx, 14.0 * s}, {ParamKey::ry, 14.0 * s}},
            "#d5e8f0",
            "#1a1a1a",
            1.5);
        children.emplace_back(
            ShapeType::Ellipse,
            Params{{ParamKey::cx, 0.0}, {ParamKey::cy, -6.0 * s}, {ParamKey::rx, 11.0 * s}, {ParamKey::ry, 10.0 * s}},
            liquid_color,
            std::nullopt,
            0.0);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -3.0 * s}, {ParamKey::y, -26.0 * s}, {ParamKey::w, 6.0 * s}, {ParamKey::h, 10.0 * s}},
            "#d5e8f0",
            "#1a1a1a",
            1.0);
    } else if (container == "vial") {
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -6.0 * s}, {ParamKey::y, -24.0 * s}, {ParamKey::w, 12.0 * s}, {ParamKey::h, 28.0 * s}},
            "#d5e8f0",
            "#1a1a1a",
            1.5);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -4.0 * s}, {ParamKey::y, -16.0 * s}, {ParamKey::w, 8.0 * s}, {ParamKey::h, 18.0 * s}},
            liquid_color,
            std::nullopt,
            0.0);
    } else if (container == "vial") {
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -6.0 * s}, {ParamKey::y, -24.0 * s}, {ParamKey::w, 12.0 * s}, {ParamKey::h, 28.0 * s}},
            "#d5e8f0",
            "#1a1a1a",
            1.5);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -4.0 * s}, {ParamKey::y, -16.0 * s}, {ParamKey::w, 8.0 * s}, {ParamKey::h, 18.0 * s}},
            liquid_color,
            std::nullopt,
            0.0);
    } else {  // jar
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -12.0 * s}, {ParamKey::y, -18.0 * s}, {ParamKey::w, 24.0 * s}, {ParamKey::h, 22.0 * s}},
            "#d5e8f0",
            "#1a1a1a",
            1.5);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -10.0 * s}, {ParamKey::y, -14.0 * s}, {ParamKey::w, 20.0 * s}, {ParamKey::h, 16.0 * s}},
            liquid_color,
            std::nullopt,
            0.0);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -14.0 * s}, {ParamKey::y, -22.0 * s}, {ParamKey::w, 28.0 * s}, {ParamKey::h, 5.0 * s}},
            "#7f8c8d",
            "#1a1a1a",
            1.0);
    }

    return std::make_shared<GraphicObject>(
        ShapeType::Group,
        Params{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(children),
        std::unordered_map<std::string, Value>{{"component", std::string("potion")}, {"type", type}, {"container", container}});
}

// ==========================================================================================================================================================================================================================================═
// Chest
// ==========================================================================================================================================================================================================================================═

std::shared_ptr<GraphicObject> create_chest(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(chest_schema(), kwargs);

    std::string type = *p["type"].as_string();
    std::string state = *p["state"].as_string();
    double s = get_size_scale(*p["size"].as_string());

    std::string main_c = "#8B4513";
    std::string dark_c = "#5D4037";
    if (type == "iron") { main_c = "#7f8c8d"; dark_c = "#5d6d7e"; }
    else if (type == "gold") { main_c = "#f1c40f"; dark_c = "#d4ac0f"; }
    else if (type == "crystal") { main_c = "#85c1e9"; dark_c = "#5dade2"; }

    double w = 40.0 * s;
    double h = 28.0 * s;

    std::vector<GraphicObject> children;

    children.emplace_back(
        ShapeType::Rect,
        Params{{ParamKey::x, -w / 2.0}, {ParamKey::y, -h / 2.0}, {ParamKey::w, w}, {ParamKey::h, h}},
        main_c,
        "#1a1a1a",
        2.0);

    if (state == "open") {
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -w / 2.0}, {ParamKey::y, -h / 2.0 - h * 0.5}, {ParamKey::w, w}, {ParamKey::h, h * 0.4}},
            dark_c,
            "#1a1a1a",
            1.5);
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -w / 2.0 + 4.0}, {ParamKey::y, -h / 2.0 + 2.0}, {ParamKey::w, w - 8.0}, {ParamKey::h, h * 0.4}},
            "#f9e79f",
            std::nullopt,
            0.0);
    } else {
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -w / 2.0}, {ParamKey::y, -h / 2.0 - h * 0.3}, {ParamKey::w, w}, {ParamKey::h, h * 0.35}},
            dark_c,
            "#1a1a1a",
            1.5);
        if (state == "locked") {
            children.emplace_back(
                ShapeType::Ellipse,
                Params{{ParamKey::cx, 0.0}, {ParamKey::cy, -h * 0.15}, {ParamKey::rx, 4.0 * s}, {ParamKey::ry, 4.0 * s}},
                "#f1c40f",
                "#1a1a1a",
                1.0);
        }
    }

    children.emplace_back(
        ShapeType::Line,
        Params{{ParamKey::x1, -w / 2.0}, {ParamKey::y1, 0.0}, {ParamKey::x2, w / 2.0}, {ParamKey::y2, 0.0}},
        std::nullopt,
        "#1a1a1a",
        1.5);

    return std::make_shared<GraphicObject>(
        ShapeType::Group,
        Params{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(children),
        std::unordered_map<std::string, Value>{{"component", std::string("chest")}, {"type", type}, {"state", state}});
}

// ==========================================================================================================================================================================================================================================═
// Generic Item
// ==========================================================================================================================================================================================================================================═

std::shared_ptr<GraphicObject> create_generic_item(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(generic_item_schema(), kwargs);

    std::string color = *p["color"].as_string();
    bool outline = p["outline"].bool_val();
    std::string outline_color = outline ? "#1a1a1a" : "";
    double ow = outline ? 2.0 : 0.0;
    std::string shape = *p["base-shape"].as_string();

    std::vector<GraphicObject> children;

    if (shape == "circle") {
        children.emplace_back(
            ShapeType::Ellipse,
            Params{{ParamKey::cx, 0.0}, {ParamKey::cy, 0.0}, {ParamKey::rx, 16.0}, {ParamKey::ry, 16.0}},
            color,
            outline ? std::optional<std::string>(outline_color) : std::nullopt,
            ow);
    } else if (shape == "rect") {
        children.emplace_back(
            ShapeType::Rect,
            Params{{ParamKey::x, -16.0}, {ParamKey::y, -16.0}, {ParamKey::w, 32.0}, {ParamKey::h, 32.0}},
            color,
            outline ? std::optional<std::string>(outline_color) : std::nullopt,
            ow);
    } else if (shape == "diamond") {
        children.emplace_back(
            ShapeType::Polygon,
            Params{{ParamKey::points, make_points({0.0, -18.0, 18.0, 0.0, 0.0, 18.0, -18.0, 0.0})}},
            color,
            outline ? std::optional<std::string>(outline_color) : std::nullopt,
            ow);
    }

    const Value& detail = p["detail"];
    if (const auto* go = detail.as_graphic_object()) {
        if (*go) {
            children.push_back(**go);
        }
    }

    std::string name = "item";
    if (const auto* s = p["name"].as_string()) {
        name = *s;
    }

    return std::make_shared<GraphicObject>(
        ShapeType::Group,
        Params{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(children),
        std::unordered_map<std::string, Value>{{"component", std::string("generic-item")}, {"name", name}});
}

}  // namespace pml
