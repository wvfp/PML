// ═══════════════════════════════════════════════════════════════════════════════
// PML Eyes Component — Implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "eyes.h"

#include <vector>

namespace pml {

namespace {

using EyeParts = std::vector<GraphicObject>;

EyeParts make_eye_shoujo(double cx, double cy, double size,
                         const std::string& color, bool highlight) {
    EyeParts parts;
    double ew = 14.0 * size;
    double eh = 18.0 * size;

    parts.emplace_back(
        "ellipse",
        Params{{ParamKey::cx, cx}, {ParamKey::cy, cy}, {ParamKey::rx, ew}, {ParamKey::ry, eh}},
        "#FFFFFF",
        "#1a1a1a",
        2.0);

    double iris_r = ew * 0.7;
    parts.emplace_back(
        "ellipse",
        Params{{ParamKey::cx, cx}, {ParamKey::cy, cy + 2.0}, {ParamKey::rx, iris_r}, {ParamKey::ry, iris_r * 1.1}},
        color,
        "#1a1a1a",
        1.0);

    double pupil_r = iris_r * 0.5;
    parts.emplace_back(
        "ellipse",
        Params{{ParamKey::cx, cx}, {ParamKey::cy, cy + 2.0}, {ParamKey::rx, pupil_r}, {ParamKey::ry, pupil_r}},
        "#0a0a0a");

    if (highlight) {
        double hl_r = ew * 0.3;
        parts.emplace_back(
            "ellipse",
            Params{{ParamKey::cx, cx - ew * 0.2}, {ParamKey::cy, cy - eh * 0.2}, {ParamKey::rx, hl_r}, {ParamKey::ry, hl_r}},
            "#FFFFFF");
        parts.emplace_back(
            "ellipse",
            Params{{ParamKey::cx, cx + ew * 0.25}, {ParamKey::cy, cy + eh * 0.15}, {ParamKey::rx, hl_r * 0.5}, {ParamKey::ry, hl_r * 0.5}},
            "#FFFFFF");
    }
    return parts;
}

EyeParts make_eye_shounen(double cx, double cy, double size,
                          const std::string& color, bool highlight) {
    EyeParts parts;
    double ew = 12.0 * size;
    double eh = 10.0 * size;

    parts.emplace_back(
        "rect",
        Params{{ParamKey::x, cx - ew}, {ParamKey::y, cy - eh / 2.0}, {ParamKey::w, ew * 2.0}, {ParamKey::h, eh}},
        "#FFFFFF",
        "#1a1a1a",
        2.0);

    parts.emplace_back(
        "ellipse",
        Params{{ParamKey::cx, cx}, {ParamKey::cy, cy}, {ParamKey::rx, ew * 0.5}, {ParamKey::ry, eh * 0.45}},
        color,
        "#1a1a1a",
        1.0);

    parts.emplace_back(
        "ellipse",
        Params{{ParamKey::cx, cx}, {ParamKey::cy, cy}, {ParamKey::rx, ew * 0.25}, {ParamKey::ry, eh * 0.3}},
        "#0a0a0a");

    if (highlight) {
        parts.emplace_back(
            "ellipse",
            Params{{ParamKey::cx, cx - ew * 0.2}, {ParamKey::cy, cy - eh * 0.15}, {ParamKey::rx, ew * 0.15}, {ParamKey::ry, ew * 0.15}},
            "#FFFFFF");
    }

    parts.emplace_back(
        "line",
        Params{{ParamKey::x1, cx - ew}, {ParamKey::y1, cy - eh - 4.0}, {ParamKey::x2, cx + ew * 0.8}, {ParamKey::y2, cy - eh - 2.0}},
        std::nullopt,
        "#1a1a1a",
        3.0);
    return parts;
}

EyeParts make_eye_round(double cx, double cy, double size,
                        const std::string& color, bool highlight) {
    EyeParts parts;
    double r = 10.0 * size;

    parts.emplace_back(
        "ellipse",
        Params{{ParamKey::cx, cx}, {ParamKey::cy, cy}, {ParamKey::rx, r}, {ParamKey::ry, r}},
        "#1a1a1a");

    parts.emplace_back(
        "ellipse",
        Params{{ParamKey::cx, cx}, {ParamKey::cy, cy + 1.0}, {ParamKey::rx, r * 0.7}, {ParamKey::ry, r * 0.7}},
        color);

    parts.emplace_back(
        "ellipse",
        Params{{ParamKey::cx, cx}, {ParamKey::cy, cy + 1.0}, {ParamKey::rx, r * 0.35}, {ParamKey::ry, r * 0.35}},
        "#0a0a0a");

    if (highlight) {
        parts.emplace_back(
            "ellipse",
            Params{{ParamKey::cx, cx - r * 0.3}, {ParamKey::cy, cy - r * 0.3}, {ParamKey::rx, r * 0.35}, {ParamKey::ry, r * 0.35}},
            "#FFFFFF");
    }
    return parts;
}

EyeParts make_eye_sharp(double cx, double cy, double size,
                        const std::string& color, bool highlight) {
    EyeParts parts;
    double ew = 14.0 * size;
    double eh = 5.0 * size;

    parts.emplace_back(
        "ellipse",
        Params{{ParamKey::cx, cx}, {ParamKey::cy, cy}, {ParamKey::rx, ew}, {ParamKey::ry, eh}},
        "#FFFFFF",
        "#1a1a1a",
        2.5);

    parts.emplace_back(
        "ellipse",
        Params{{ParamKey::cx, cx}, {ParamKey::cy, cy}, {ParamKey::rx, ew * 0.4}, {ParamKey::ry, eh * 0.85}},
        color);

    parts.emplace_back(
        "ellipse",
        Params{{ParamKey::cx, cx}, {ParamKey::cy, cy}, {ParamKey::rx, ew * 0.2}, {ParamKey::ry, eh * 0.6}},
        "#0a0a0a");

    if (highlight) {
        parts.emplace_back(
            "ellipse",
            Params{{ParamKey::cx, cx - ew * 0.15}, {ParamKey::cy, cy - eh * 0.3}, {ParamKey::rx, ew * 0.1}, {ParamKey::ry, ew * 0.1}},
            "#FFFFFF");
    }
    return parts;
}

EyeParts make_eye_sleepy(double cx, double cy, double size,
                         const std::string& color, bool /*highlight*/) {
    EyeParts parts;
    double ew = 12.0 * size;
    double eh = 4.0 * size;

    parts.emplace_back(
        "ellipse",
        Params{{ParamKey::cx, cx}, {ParamKey::cy, cy + eh}, {ParamKey::rx, ew}, {ParamKey::ry, eh * 2.0}},
        "#FFFFFF",
        "#1a1a1a",
        1.5);

    parts.emplace_back(
        "line",
        Params{{ParamKey::x1, cx - ew}, {ParamKey::y1, cy}, {ParamKey::x2, cx + ew}, {ParamKey::y2, cy}},
        std::nullopt,
        "#1a1a1a",
        2.5);

    parts.emplace_back(
        "ellipse",
        Params{{ParamKey::cx, cx}, {ParamKey::cy, cy + eh * 0.5}, {ParamKey::rx, ew * 0.35}, {ParamKey::ry, eh * 0.7}},
        color);

    return parts;
}

EyeParts make_eye_closed(double cx, double cy, double size,
                         const std::string& /*color*/, bool /*highlight*/) {
    EyeParts parts;
    double ew = 12.0 * size;

    parts.emplace_back(
        "line",
        Params{{ParamKey::x1, cx - ew}, {ParamKey::y1, cy}, {ParamKey::x2, cx + ew}, {ParamKey::y2, cy}},
        std::nullopt,
        "#1a1a1a",
        3.0 * size);

    for (double dx : {-ew * 0.7, 0.0, ew * 0.7}) {
        parts.emplace_back(
            "line",
            Params{{ParamKey::x1, cx + dx}, {ParamKey::y1, cy}, {ParamKey::x2, cx + dx}, {ParamKey::y2, cy - 4.0 * size}},
            std::nullopt,
            "#1a1a1a",
            1.5);
    }
    return parts;
}

using EyeMaker = EyeParts(*)(double, double, double, const std::string&, bool);

const std::unordered_map<std::string, EyeMaker> k_eye_makers = {
    {"shoujo", make_eye_shoujo},
    {"shounen", make_eye_shounen},
    {"round", make_eye_round},
    {"sharp", make_eye_sharp},
    {"sleepy", make_eye_sleepy},
    {"closed", make_eye_closed},
};

}  // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// Schema
// ═══════════════════════════════════════════════════════════════════════════════

ParamSchema eyes_schema() {
    return ParamSchema()
        .enum_("style", {"shoujo", "shounen", "round", "sharp", "sleepy", "closed"}, "shoujo")
        .color("color", "#4a90d9")
        .number("size", 1.0, 0.3, 3.0)
        .number("spacing", 1.0, 0.5, 2.0)
        .boolean("highlight", true)
        .enum_("lashes", {"none", "short", "long"}, "none");
}

// ═══════════════════════════════════════════════════════════════════════════════
// Factory
// ═══════════════════════════════════════════════════════════════════════════════

std::shared_ptr<GraphicObject> create_eyes(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(eyes_schema(), kwargs);

    std::string style = *p["style"].as_string();
    std::string color = *p["color"].as_string();
    double size = p["size"].double_val();
    double spacing = p["spacing"].double_val();
    bool highlight = p["highlight"].bool_val();
    std::string lashes = *p["lashes"].as_string();

    double base_spacing = 24.0 * spacing;
    double left_cx = -base_spacing / 2.0;
    double right_cx = base_spacing / 2.0;

    EyeMaker maker = make_eye_shoujo;
    auto it = k_eye_makers.find(style);
    if (it != k_eye_makers.end()) {
        maker = it->second;
    }

    EyeParts all_children;
    auto left = maker(left_cx, 0.0, size, color, highlight);
    auto right = maker(right_cx, 0.0, size, color, highlight);
    all_children.insert(all_children.end(), left.begin(), left.end());
    all_children.insert(all_children.end(), right.begin(), right.end());

    if (lashes == "short" || lashes == "long") {
        double lash_len = (lashes == "short") ? 6.0 : 10.0;
        for (double cx : {left_cx, right_cx}) {
            for (double dx : {-10.0 * size, 0.0, 10.0 * size}) {
                all_children.emplace_back(
                    "line",
                    Params{
                        {ParamKey::x1, cx + dx},
                        {ParamKey::y1, -12.0 * size},
                        {ParamKey::x2, cx + dx + 2.0},
                        {ParamKey::y2, -12.0 * size - lash_len * size}},
                    std::nullopt,
                    "#1a1a1a",
                    1.5);
            }
        }
    }

    return std::make_shared<GraphicObject>(
        "group",
        Params{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(all_children),
        std::unordered_map<std::string, Value>{
            {"component", std::string("eyes")},
            {"style", style},
            {"color", color}});
}

}  // namespace pml
