// ═══════════════════════════════════════════════════════════════════════════════
// PML Scene Element Components — Implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "scene_elements.h"

#include "pml/graphics/objects.h"
#include "pml/sprites/validator.h"

#include <cmath>
#include <functional>
#include <random>
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

[[nodiscard]] std::mt19937 make_rng(uint32_t seed) {
    return std::mt19937(seed);
}

[[nodiscard]] double uniform(std::mt19937& rng, double a, double b) {
    std::uniform_real_distribution<double> dist(a, b);
    return dist(rng);
}

[[nodiscard]] uint32_t string_hash(const std::string& s) {
    return static_cast<uint32_t>(std::hash<std::string>{}(s));
}

struct TileColors {
    std::string base;
    std::string detail;
    std::string edge;
};

const std::unordered_map<std::string, TileColors> k_tile_colors = {
    {"grass", {"#4a7c2e", "#5d9636", "#3a6424"}},
    {"stone", {"#7f8c8d", "#95a5a6", "#616a6b"}},
    {"wood", {"#8B6914", "#a37e2c", "#6b4f10"}},
    {"sand", {"#d4b96a", "#e0ca82", "#b89e52"}},
    {"water", {"#2980b9", "#3498db", "#1f6391"}},
    {"snow", {"#ecf0f1", "#ffffff", "#bdc3c7"}},
    {"brick", {"#a0522d", "#c0603a", "#7a3e22"}},
};

const std::unordered_map<std::string, double> k_size_scale = {
    {"small", 0.6},
    {"medium", 1.0},
    {"large", 1.5}};

struct SeasonColors {
    std::string leaf;
    std::string flower;
    std::string trunk;
};

const std::unordered_map<std::string, SeasonColors> k_season_colors = {
    {"spring", {"#6abf69", "#f06292", "#6d4c2e"}},
    {"summer", {"#2e7d32", "#ffeb3b", "#5d4037"}},
    {"autumn", {"#e65100", "#ff8f00", "#4e342e"}},
    {"winter", {"#b0bec5", "#e0e0e0", "#6d4c41"}},
};

struct TimeColors {
    std::string sky_top;
    std::string sky_bot;
    std::string ambient;
};

const std::unordered_map<std::string, TimeColors> k_time_colors = {
    {"day", {"#87ceeb", "#e0f7fa", "#ffffff"}},
    {"dusk", {"#e65100", "#ff8f00", "#ffcc80"}},
    {"night", {"#0d1b2a", "#1b2838", "#37474f"}},
    {"dawn", {"#4a148c", "#ff6f00", "#ce93d8"}},
};

[[nodiscard]] std::vector<GraphicObject> tile_detail(
    const std::string& tile_type,
    int sz,
    int variant,
    const TileColors& colors) {
    std::vector<GraphicObject> parts;
    double half = sz / 2.0;
    uint32_t seed = static_cast<uint32_t>(variant) * 31u + string_hash(tile_type) % 100u;
    auto rng = make_rng(seed);

    if (tile_type == "grass") {
        for (int i = 0; i < 6; ++i) {
            double x = uniform(rng, -half * 0.8, half * 0.8);
            double y = uniform(rng, -half * 0.8, half * 0.8);
            double h = uniform(rng, 3.0, 6.0);
            parts.emplace_back(
                "line",
                std::unordered_map<std::string, Value>{{"x1", x}, {"y1", y}, {"x2", x + uniform(rng, -1.0, 1.0)}, {"y2", y - h}},
                std::nullopt,
                colors.detail,
                1.0);
        }
    } else if (tile_type == "stone") {
        for (int i = 0; i < 3; ++i) {
            double x = uniform(rng, -half * 0.6, half * 0.6);
            double y = uniform(rng, -half * 0.6, half * 0.6);
            double r = uniform(rng, 2.0, 4.0);
            parts.emplace_back(
                "ellipse",
                std::unordered_map<std::string, Value>{{"cx", x}, {"cy", y}, {"rx", r}, {"ry", r * 0.7}},
                colors.detail,
                std::nullopt,
                0.0);
        }
    } else if (tile_type == "wood") {
        for (int i = 0; i < 3; ++i) {
            double y = -half + (i + 1) * (sz / 4.0);
            parts.emplace_back(
                "line",
                std::unordered_map<std::string, Value>{{"x1", -half}, {"y1", y}, {"x2", half}, {"y2", y}},
                std::nullopt,
                colors.detail,
                1.0);
        }
    } else if (tile_type == "sand") {
        for (int i = 0; i < 8; ++i) {
            double x = uniform(rng, -half * 0.8, half * 0.8);
            double y = uniform(rng, -half * 0.8, half * 0.8);
            parts.emplace_back(
                "circle",
                std::unordered_map<std::string, Value>{{"cx", x}, {"cy", y}, {"r", 1.0}},
                colors.detail,
                std::nullopt,
                0.0);
        }
    } else if (tile_type == "water") {
        for (int i = 0; i < 2; ++i) {
            double y = -half * 0.4 + i * (sz * 0.4);
            parts.emplace_back(
                "line",
                std::unordered_map<std::string, Value>{{"x1", -half * 0.6}, {"y1", y}, {"x2", half * 0.6}, {"y2", y + 2.0}},
                std::nullopt,
                colors.detail,
                1.5);
        }
    } else if (tile_type == "snow") {
        for (int i = 0; i < 4; ++i) {
            double x = uniform(rng, -half * 0.7, half * 0.7);
            double y = uniform(rng, -half * 0.7, half * 0.7);
            parts.emplace_back(
                "circle",
                std::unordered_map<std::string, Value>{{"cx", x}, {"cy", y}, {"r", uniform(rng, 1.0, 2.0)}},
                colors.detail,
                std::nullopt,
                0.0);
        }
    } else if (tile_type == "brick") {
        double brick_h = sz / 4.0;
        double brick_w = sz / 2.0;
        for (int row = 0; row < 4; ++row) {
            double y = -half + row * brick_h;
            double offset = (row % 2) ? brick_w / 2.0 : 0.0;
            for (int col = -1; col < 3; ++col) {
                double x = -half + col * brick_w + offset;
                parts.emplace_back(
                    "rect",
                    std::unordered_map<std::string, Value>{{"x", x + 0.5}, {"y", y + 0.5}, {"w", brick_w - 1.0}, {"h", brick_h - 1.0}},
                    std::nullopt,
                    colors.edge,
                    0.8);
            }
        }
    }

    return parts;
}

[[nodiscard]] std::vector<GraphicObject> tile_edge(
    const std::string& edge,
    int sz,
    const TileColors& colors) {
    double half = sz / 2.0;
    if (edge == "top") {
        return {GraphicObject(
            "line",
            std::unordered_map<std::string, Value>{{"x1", -half}, {"y1", -half}, {"x2", half}, {"y2", -half}},
            std::nullopt,
            colors.edge,
            2.0)};
    }
    if (edge == "bottom") {
        return {GraphicObject(
            "line",
            std::unordered_map<std::string, Value>{{"x1", -half}, {"y1", half}, {"x2", half}, {"y2", half}},
            std::nullopt,
            colors.edge,
            2.0)};
    }
    if (edge == "left") {
        return {GraphicObject(
            "line",
            std::unordered_map<std::string, Value>{{"x1", -half}, {"y1", -half}, {"x2", -half}, {"y2", half}},
            std::nullopt,
            colors.edge,
            2.0)};
    }
    if (edge == "right") {
        return {GraphicObject(
            "line",
            std::unordered_map<std::string, Value>{{"x1", half}, {"y1", -half}, {"x2", half}, {"y2", half}},
            std::nullopt,
            colors.edge,
            2.0)};
    }
    return {};
}

using MakerFn = std::vector<GraphicObject>(*)(double scale, const SeasonColors& sc, int variant);

[[nodiscard]] std::vector<GraphicObject> make_tree(double scale, const SeasonColors& sc, int variant) {
    std::vector<GraphicObject> parts;
    double trunk_w = 8.0 * scale;
    double trunk_h = 30.0 * scale;
    double canopy_r = 22.0 * scale;

    parts.emplace_back(
        "rect",
        std::unordered_map<std::string, Value>{{"x", -trunk_w / 2.0}, {"y", 0.0}, {"w", trunk_w}, {"h", trunk_h}},
        sc.trunk,
        "#3e2723",
        1.0);

    auto rng = make_rng(static_cast<uint32_t>(variant) * 17u);
    int layers = (variant % 2 == 0) ? 2 : 3;
    for (int i = 0; i < layers; ++i) {
        double ox = uniform(rng, -5.0, 5.0) * scale;
        double oy = -trunk_h * 0.3 - i * canopy_r * 0.5;
        double r = canopy_r * (1.0 - i * 0.15);
        parts.emplace_back(
            "ellipse",
            std::unordered_map<std::string, Value>{{"cx", ox}, {"cy", oy}, {"rx", r}, {"ry", r * 0.85}},
            sc.leaf,
            "#1b5e20",
            1.0);
    }

    return parts;
}

[[nodiscard]] std::vector<GraphicObject> make_bush(double scale, const SeasonColors& sc, int variant) {
    std::vector<GraphicObject> parts;
    double r = 16.0 * scale;
    auto rng = make_rng(static_cast<uint32_t>(variant) * 13u);

    for (int i = 0; i < 3; ++i) {
        double ox = (i - 1) * r * 0.6 + uniform(rng, -2.0, 2.0);
        parts.emplace_back(
            "ellipse",
            std::unordered_map<std::string, Value>{{"cx", ox}, {"cy", -r * 0.3}, {"rx", r * 0.7}, {"ry", r * 0.5}},
            sc.leaf,
            "#2e7d32",
            1.0);
    }

    return parts;
}

[[nodiscard]] std::vector<GraphicObject> make_rock(double scale, const SeasonColors& /*sc*/, int variant) {
    std::vector<GraphicObject> parts;
    double r = 14.0 * scale;
    auto rng = make_rng(static_cast<uint32_t>(variant) * 23u + 7u);

    std::vector<double> points;
    const int n_sides = 6;
    for (int i = 0; i < n_sides; ++i) {
        double angle = (2.0 * std::numbers::pi_v<double> * i / n_sides) - std::numbers::pi_v<double> / 2.0;
        double dist = r * uniform(rng, 0.7, 1.1);
        points.push_back(std::cos(angle) * dist);
        points.push_back(std::sin(angle) * dist * 0.7);
    }

    parts.emplace_back(
        "polygon",
        std::unordered_map<std::string, Value>{{"points", make_points(points)}},
        "#78909c",
        "#455a64",
        1.5);
    parts.emplace_back(
        "ellipse",
        std::unordered_map<std::string, Value>{{"cx", -r * 0.2}, {"cy", -r * 0.2}, {"rx", r * 0.25}, {"ry", r * 0.15}},
        "#90a4ae",
        std::nullopt,
        0.0);

    return parts;
}

[[nodiscard]] std::vector<GraphicObject> make_flower(double scale, const SeasonColors& sc, int /*variant*/) {
    std::vector<GraphicObject> parts;
    double stem_h = 20.0 * scale;
    double petal_r = 4.0 * scale;

    parts.emplace_back(
        "line",
        std::unordered_map<std::string, Value>{{"x1", 0.0}, {"y1", 0.0}, {"x2", 0.0}, {"y2", -stem_h}},
        std::nullopt,
        "#4caf50",
        1.5);

    const int n_petals = 5;
    double cy = -stem_h;
    for (int i = 0; i < n_petals; ++i) {
        double angle = (2.0 * std::numbers::pi_v<double> * i / n_petals) - std::numbers::pi_v<double> / 2.0;
        double px = std::cos(angle) * petal_r * 1.5;
        double py = cy + std::sin(angle) * petal_r * 1.5;
        parts.emplace_back(
            "ellipse",
            std::unordered_map<std::string, Value>{{"cx", px}, {"cy", py}, {"rx", petal_r}, {"ry", petal_r * 0.7}},
            sc.flower,
            std::nullopt,
            0.0);
    }

    parts.emplace_back(
        "circle",
        std::unordered_map<std::string, Value>{{"cx", 0.0}, {"cy", cy}, {"r", petal_r * 0.6}},
        "#ffeb3b",
        std::nullopt,
        0.0);

    return parts;
}

[[nodiscard]] std::vector<GraphicObject> make_mushroom(double scale, const SeasonColors& /*sc*/, int variant) {
    std::vector<GraphicObject> parts;
    double stem_w = 6.0 * scale;
    double stem_h = 12.0 * scale;
    double cap_r = 12.0 * scale;

    parts.emplace_back(
        "rect",
        std::unordered_map<std::string, Value>{{"x", -stem_w / 2.0}, {"y", -stem_h}, {"w", stem_w}, {"h", stem_h}},
        "#f5f5dc",
        "#bfae8e",
        1.0);
    parts.emplace_back(
        "ellipse",
        std::unordered_map<std::string, Value>{{"cx", 0.0}, {"cy", -stem_h - cap_r * 0.3}, {"rx", cap_r}, {"ry", cap_r * 0.6}},
        "#e53935",
        "#b71c1c",
        1.0);

    auto rng = make_rng(static_cast<uint32_t>(variant) * 11u);
    for (int i = 0; i < 3; ++i) {
        double sx = uniform(rng, -cap_r * 0.5, cap_r * 0.5);
        double sy = -stem_h - cap_r * 0.3 + uniform(rng, -cap_r * 0.2, cap_r * 0.1);
        parts.emplace_back(
            "circle",
            std::unordered_map<std::string, Value>{{"cx", sx}, {"cy", sy}, {"r", 2.0 * scale}},
            "#ffffff",
            std::nullopt,
            0.0);
    }

    return parts;
}

[[nodiscard]] std::vector<GraphicObject> make_crate(double scale, const SeasonColors& /*sc*/, int /*variant*/) {
    std::vector<GraphicObject> parts;
    double sz = 28.0 * scale;

    parts.emplace_back(
        "rect",
        std::unordered_map<std::string, Value>{{"x", -sz / 2.0}, {"y", -sz}, {"w", sz}, {"h", sz}},
        "#a1887f",
        "#5d4037",
        1.5);
    parts.emplace_back(
        "line",
        std::unordered_map<std::string, Value>{{"x1", -sz / 2.0}, {"y1", -sz / 2.0}, {"x2", sz / 2.0}, {"y2", -sz / 2.0}},
        std::nullopt,
        "#6d4c41",
        1.5);
    parts.emplace_back(
        "line",
        std::unordered_map<std::string, Value>{{"x1", 0.0}, {"y1", -sz}, {"x2", 0.0}, {"y2", 0.0}},
        std::nullopt,
        "#6d4c41",
        1.5);

    return parts;
}

[[nodiscard]] std::vector<GraphicObject> make_barrel(double scale, const SeasonColors& /*sc*/, int /*variant*/) {
    std::vector<GraphicObject> parts;
    double w = 22.0 * scale;
    double h = 30.0 * scale;

    parts.emplace_back(
        "rect",
        std::unordered_map<std::string, Value>{{"x", -w / 2.0}, {"y", -h}, {"w", w}, {"h", h}},
        "#8d6e63",
        "#4e342e",
        1.5);

    for (double frac : {0.2, 0.5, 0.8}) {
        double by = -h + h * frac;
        parts.emplace_back(
            "line",
            std::unordered_map<std::string, Value>{{"x1", -w / 2.0}, {"y1", by}, {"x2", w / 2.0}, {"y2", by}},
            std::nullopt,
            "#5d4037",
            2.0);
    }

    return parts;
}

[[nodiscard]] std::vector<GraphicObject> make_torch(double scale, const SeasonColors& /*sc*/, int /*variant*/) {
    std::vector<GraphicObject> parts;
    double stick_h = 28.0 * scale;

    parts.emplace_back(
        "line",
        std::unordered_map<std::string, Value>{{"x1", 0.0}, {"y1", 0.0}, {"x2", 0.0}, {"y2", -stick_h}},
        std::nullopt,
        "#6d4c41",
        3.0 * scale);

    double flame_y = -stick_h - 6.0 * scale;
    parts.emplace_back(
        "ellipse",
        std::unordered_map<std::string, Value>{{"cx", 0.0}, {"cy", flame_y}, {"rx", 5.0 * scale}, {"ry", 8.0 * scale}},
        "#ff9800",
        std::nullopt,
        0.0);
    parts.emplace_back(
        "ellipse",
        std::unordered_map<std::string, Value>{{"cx", 0.0}, {"cy", flame_y - 2.0 * scale}, {"rx", 3.0 * scale}, {"ry", 5.0 * scale}},
        "#ffeb3b",
        std::nullopt,
        0.0);

    return parts;
}

[[nodiscard]] std::vector<GraphicObject> make_sign(double scale, const SeasonColors& /*sc*/, int /*variant*/) {
    std::vector<GraphicObject> parts;
    double post_h = 36.0 * scale;
    double board_w = 30.0 * scale;
    double board_h = 18.0 * scale;

    parts.emplace_back(
        "line",
        std::unordered_map<std::string, Value>{{"x1", 0.0}, {"y1", 0.0}, {"x2", 0.0}, {"y2", -post_h}},
        std::nullopt,
        "#6d4c41",
        3.0 * scale);

    double board_y = -post_h + board_h / 2.0;
    parts.emplace_back(
        "rect",
        std::unordered_map<std::string, Value>{{"x", -board_w / 2.0}, {"y", board_y - board_h / 2.0}, {"w", board_w}, {"h", board_h}},
        "#d7ccc8",
        "#5d4037",
        1.5);

    return parts;
}

[[nodiscard]] std::vector<GraphicObject> make_fence(double scale, const SeasonColors& /*sc*/, int /*variant*/) {
    std::vector<GraphicObject> parts;
    double post_h = 24.0 * scale;
    double rail_w = 32.0 * scale;
    double post_w = 4.0 * scale;

    for (double x : {-rail_w / 2.0, rail_w / 2.0}) {
        parts.emplace_back(
            "rect",
            std::unordered_map<std::string, Value>{{"x", x - post_w / 2.0}, {"y", -post_h}, {"w", post_w}, {"h", post_h}},
            "#8d6e63",
            "#4e342e",
            1.0);
    }

    for (double frac : {0.35, 0.7}) {
        double ry = -post_h * frac;
        parts.emplace_back(
            "line",
            std::unordered_map<std::string, Value>{{"x1", -rail_w / 2.0}, {"y1", ry}, {"x2", rail_w / 2.0}, {"y2", ry}},
            std::nullopt,
            "#6d4c41",
            2.0 * scale);
    }

    return parts;
}

[[nodiscard]] std::vector<GraphicObject> make_lamp(double scale, const SeasonColors& /*sc*/, int /*variant*/) {
    std::vector<GraphicObject> parts;
    double post_h = 40.0 * scale;
    double lantern_r = 6.0 * scale;

    parts.emplace_back(
        "line",
        std::unordered_map<std::string, Value>{{"x1", 0.0}, {"y1", 0.0}, {"x2", 0.0}, {"y2", -post_h}},
        std::nullopt,
        "#37474f",
        3.0 * scale);

    double ly = -post_h - lantern_r;
    parts.emplace_back(
        "rect",
        std::unordered_map<std::string, Value>{{"x", -lantern_r}, {"y", ly - lantern_r}, {"w", lantern_r * 2.0}, {"h", lantern_r * 2.0}},
        "#455a64",
        "#263238",
        1.0);
    parts.emplace_back(
        "ellipse",
        std::unordered_map<std::string, Value>{{"cx", 0.0}, {"cy", ly}, {"rx", lantern_r * 0.7}, {"ry", lantern_r * 0.7}},
        "#fff9c4",
        std::nullopt,
        0.0);

    return parts;
}

}  // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// Schemas
// ═══════════════════════════════════════════════════════════════════════════════

ParamSchema tile_schema() {
    return ParamSchema()
        .enum_("type", {"grass", "stone", "wood", "sand", "water", "snow", "brick"}, "grass")
        .number("size", 32.0, 8.0, 128.0)
        .number("variant", 0.0, 0.0, 3.0)
        .enum_("edge", {"none", "top", "bottom", "left", "right"}, "none");
}

ParamSchema decoration_schema() {
    return ParamSchema()
        .enum_("type", {
            "tree", "bush", "rock", "flower", "mushroom",
            "crate", "barrel", "torch", "sign", "fence", "lamp"}, "tree")
        .enum_("size", {"small", "medium", "large"}, "medium")
        .enum_("season", {"spring", "summer", "autumn", "winter"}, "summer")
        .number("variant", 0.0, 0.0, 3.0);
}

ParamSchema background_schema() {
    return ParamSchema()
        .enum_("type", {"sky", "forest", "dungeon", "town", "ocean", "mountain"}, "sky")
        .enum_("time", {"day", "dusk", "night", "dawn"}, "day")
        .enum_("weather", {"clear", "cloudy", "rain", "snow", "fog"}, "clear")
        .number("width", 256.0, 64.0, 1024.0)
        .number("height", 256.0, 64.0, 1024.0)
        .number("parallax", 1.0, 0.1, 5.0);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Tile
// ═══════════════════════════════════════════════════════════════════════════════

std::shared_ptr<GraphicObject> create_tile(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(tile_schema(), kwargs);

    std::string tile_type = std::get<std::string>(p["type"]);
    int sz = static_cast<int>(std::get<double>(p["size"]));
    int variant = static_cast<int>(std::get<double>(p["variant"]));
    std::string edge = std::get<std::string>(p["edge"]);

    auto it = k_tile_colors.find(tile_type);
    const TileColors& colors = (it != k_tile_colors.end()) ? it->second : k_tile_colors.at("grass");

    std::vector<GraphicObject> children;
    double half = sz / 2.0;

    children.emplace_back(
        "rect",
        std::unordered_map<std::string, Value>{{"x", -half}, {"y", -half}, {"w", static_cast<double>(sz)}, {"h", static_cast<double>(sz)}},
        colors.base,
        std::nullopt,
        0.0);

    auto details = tile_detail(tile_type, sz, variant, colors);
    children.insert(children.end(), details.begin(), details.end());

    if (edge != "none") {
        auto edges = tile_edge(edge, sz, colors);
        children.insert(children.end(), edges.begin(), edges.end());
    }

    return std::make_shared<GraphicObject>(
        "group",
        std::unordered_map<std::string, Value>{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(children),
        std::unordered_map<std::string, Value>{
            {"component", std::string("tile")},
            {"tile_type", tile_type},
            {"size", static_cast<int64_t>(sz)},
            {"variant", static_cast<int64_t>(variant)},
            {"edge", edge}});
}

// ═══════════════════════════════════════════════════════════════════════════════
// Decoration
// ═══════════════════════════════════════════════════════════════════════════════

std::shared_ptr<GraphicObject> create_decoration(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(decoration_schema(), kwargs);

    std::string dec_type = std::get<std::string>(p["type"]);
    double scale = k_size_scale.at(std::get<std::string>(p["size"]));
    std::string season = std::get<std::string>(p["season"]);
    int variant = static_cast<int>(std::get<double>(p["variant"]));

    auto season_it = k_season_colors.find(season);
    const SeasonColors& sc = (season_it != k_season_colors.end()) ? season_it->second : k_season_colors.at("summer");

    MakerFn maker = nullptr;
    if (dec_type == "tree") maker = make_tree;
    else if (dec_type == "bush") maker = make_bush;
    else if (dec_type == "rock") maker = make_rock;
    else if (dec_type == "flower") maker = make_flower;
    else if (dec_type == "mushroom") maker = make_mushroom;
    else if (dec_type == "crate") maker = make_crate;
    else if (dec_type == "barrel") maker = make_barrel;
    else if (dec_type == "torch") maker = make_torch;
    else if (dec_type == "sign") maker = make_sign;
    else if (dec_type == "fence") maker = make_fence;
    else if (dec_type == "lamp") maker = make_lamp;
    else maker = make_tree;

    auto children = maker(scale, sc, variant);

    return std::make_shared<GraphicObject>(
        "group",
        std::unordered_map<std::string, Value>{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(children),
        std::unordered_map<std::string, Value>{
            {"component", std::string("decoration")},
            {"deco_type", dec_type},
            {"size", std::get<std::string>(p["size"])},
            {"season", season},
            {"variant", static_cast<int64_t>(variant)}});
}

// ═══════════════════════════════════════════════════════════════════════════════
// Background
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

[[nodiscard]] std::vector<GraphicObject> bg_terrain(
    const std::string& bg_type,
    int w,
    int h,
    const TimeColors& tc) {
    std::vector<GraphicObject> parts;
    double half_w = w / 2.0;
    double ground_y = h * 0.15;

    struct TerrainColor {
        std::string ground;
        std::string detail;
    };

    const std::unordered_map<std::string, TerrainColor> terrain_colors = {
        {"forest", {"#2e7d32", "#1b5e20"}},
        {"dungeon", {"#424242", "#212121"}},
        {"town", {"#8d6e63", "#5d4037"}},
        {"ocean", {"#0277bd", "#01579b"}},
        {"mountain", {"#607d8b", "#455a64"}},
    };

    auto it = terrain_colors.find(bg_type);
    if (it == terrain_colors.end()) return parts;

    const auto& tc2 = it->second;
    parts.emplace_back(
        "rect",
        std::unordered_map<std::string, Value>{{"x", -half_w}, {"y", ground_y}, {"w", static_cast<double>(w)}, {"h", h / 2.0 - ground_y + h * 0.05}},
        tc2.ground,
        std::nullopt,
        0.0);

    if (bg_type == "forest") {
        auto rng = make_rng(42u);
        for (int i = 0; i < 5; ++i) {
            double tx = -half_w + (i + 0.5) * (w / 5.0) + uniform(rng, -10.0, 10.0);
            double tree_h = uniform(rng, 30.0, 50.0);
            parts.emplace_back(
                "line",
                std::unordered_map<std::string, Value>{{"x1", tx}, {"y1", ground_y}, {"x2", tx}, {"y2", ground_y - tree_h}},
                std::nullopt,
                "#5d4037",
                4.0);
            parts.emplace_back(
                "ellipse",
                std::unordered_map<std::string, Value>{{"cx", tx}, {"cy", ground_y - tree_h - 10.0}, {"rx", 15.0}, {"ry", 18.0}},
                tc2.detail,
                std::nullopt,
                0.0);
        }
    } else if (bg_type == "mountain") {
        for (int i = 0; i < 3; ++i) {
            double mx = -half_w * 0.6 + i * half_w * 0.6;
            double mh = 40.0 + i * 15.0;
            parts.emplace_back(
                "polygon",
                std::unordered_map<std::string, Value>{{"points", make_points({mx - 30.0, ground_y, mx, ground_y - mh, mx + 30.0, ground_y})}},
                tc2.detail,
                "#37474f",
                1.0);
        }
    } else if (bg_type == "ocean") {
        auto rng = make_rng(99u);
        for (int i = 0; i < 4; ++i) {
            double wy = ground_y + 5.0 + i * 8.0;
            parts.emplace_back(
                "line",
                std::unordered_map<std::string, Value>{{"x1", -half_w * 0.8}, {"y1", wy}, {"x2", half_w * 0.8}, {"y2", wy + uniform(rng, -2.0, 2.0)}},
                std::nullopt,
                "#0288d1",
                1.5);
        }
    } else if (bg_type == "town") {
        auto rng = make_rng(77u);
        for (int i = 0; i < 4; ++i) {
            double bx = -half_w + (i + 0.5) * (w / 4.0) + uniform(rng, -8.0, 8.0);
            double bw = uniform(rng, 18.0, 28.0);
            double bh = uniform(rng, 25.0, 45.0);
            parts.emplace_back(
                "rect",
                std::unordered_map<std::string, Value>{{"x", bx - bw / 2.0}, {"y", ground_y - bh}, {"w", bw}, {"h", bh}},
                "#a1887f",
                "#5d4037",
                1.0);
        }
    } else if (bg_type == "dungeon") {
        for (int i = 0; i < 3; ++i) {
            double px = -half_w * 0.6 + i * half_w * 0.6;
            parts.emplace_back(
                "rect",
                std::unordered_map<std::string, Value>{{"x", px - 6.0}, {"y", ground_y - 50.0}, {"w", 12.0}, {"h", 50.0}},
                "#616161",
                "#424242",
                1.0);
        }
    }

    return parts;
}

[[nodiscard]] std::vector<GraphicObject> bg_weather(
    const std::string& weather,
    int w,
    int h) {
    std::vector<GraphicObject> parts;
    double half_w = w / 2.0;
    double half_h = h / 2.0;
    auto rng = make_rng(string_hash(weather) % 100u);

    if (weather == "cloudy") {
        for (int i = 0; i < 4; ++i) {
            double cx = uniform(rng, -half_w * 0.7, half_w * 0.7);
            double cy = uniform(rng, -half_h * 0.7, -half_h * 0.3);
            parts.emplace_back(
                "ellipse",
                std::unordered_map<std::string, Value>{{"cx", cx}, {"cy", cy}, {"rx", 25.0}, {"ry", 12.0}},
                "#b0bec580",
                std::nullopt,
                0.0);
        }
    } else if (weather == "rain") {
        for (int i = 0; i < 20; ++i) {
            double rx = uniform(rng, -half_w * 0.9, half_w * 0.9);
            double ry = uniform(rng, -half_h, half_h);
            parts.emplace_back(
                "line",
                std::unordered_map<std::string, Value>{{"x1", rx}, {"y1", ry}, {"x2", rx - 1.0}, {"y2", ry + 8.0}},
                std::nullopt,
                "#90a4ae80",
                1.0);
        }
    } else if (weather == "snow") {
        for (int i = 0; i < 15; ++i) {
            double sx = uniform(rng, -half_w * 0.9, half_w * 0.9);
            double sy = uniform(rng, -half_h, half_h);
            parts.emplace_back(
                "circle",
                std::unordered_map<std::string, Value>{{"cx", sx}, {"cy", sy}, {"r", 2.0}},
                "#ffffffcc",
                std::nullopt,
                0.0);
        }
    } else if (weather == "fog") {
        parts.emplace_back(
            "rect",
            std::unordered_map<std::string, Value>{{"x", -half_w}, {"y", -half_h * 0.3}, {"w", static_cast<double>(w)}, {"h", h * 0.6}},
            "#cfd8dc60",
            std::nullopt,
            0.0);
    }

    return parts;
}

}  // anonymous namespace

std::shared_ptr<GraphicObject> create_background(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(background_schema(), kwargs);

    std::string bg_type = std::get<std::string>(p["type"]);
    std::string time = std::get<std::string>(p["time"]);
    std::string weather = std::get<std::string>(p["weather"]);
    int w = static_cast<int>(std::get<double>(p["width"]));
    int h = static_cast<int>(std::get<double>(p["height"]));
    double parallax = std::get<double>(p["parallax"]);

    auto time_it = k_time_colors.find(time);
    const TimeColors& tc = (time_it != k_time_colors.end()) ? time_it->second : k_time_colors.at("day");

    std::vector<GraphicObject> children;
    double half_w = w / 2.0;
    double half_h = h / 2.0;

    children.emplace_back(
        "rect",
        std::unordered_map<std::string, Value>{{"x", -half_w}, {"y", -half_h}, {"w", static_cast<double>(w)}, {"h", h / 2.0}},
        tc.sky_top,
        std::nullopt,
        0.0);
    children.emplace_back(
        "rect",
        std::unordered_map<std::string, Value>{{"x", -half_w}, {"y", 0.0}, {"w", static_cast<double>(w)}, {"h", h / 2.0}},
        tc.sky_bot,
        std::nullopt,
        0.0);

    auto terrain = bg_terrain(bg_type, w, h, tc);
    children.insert(children.end(), terrain.begin(), terrain.end());

    auto weather_parts = bg_weather(weather, w, h);
    children.insert(children.end(), weather_parts.begin(), weather_parts.end());

    return std::make_shared<GraphicObject>(
        "group",
        std::unordered_map<std::string, Value>{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(children),
        std::unordered_map<std::string, Value>{
            {"component", std::string("background")},
            {"bg_type", bg_type},
            {"time", time},
            {"weather", weather},
            {"width", static_cast<int64_t>(w)},
            {"height", static_cast<int64_t>(h)},
            {"parallax", parallax}});
}

}  // namespace pml
