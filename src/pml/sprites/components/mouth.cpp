// ═══════════════════════════════════════════════════════════════════════════════
// PML Mouth Component — Implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "mouth.h"

#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Schema
// ═══════════════════════════════════════════════════════════════════════════════

ParamSchema mouth_schema() {
    return ParamSchema()
        .enum_("style", {"neutral", "smile", "frown", "open", "cat", "fang"}, "neutral")
        .number("size", 1.0, 0.3, 3.0);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Factory
// ═══════════════════════════════════════════════════════════════════════════════

std::shared_ptr<GraphicObject> create_mouth(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(mouth_schema(), kwargs);

    std::string style = std::get<std::string>(p["style"]);
    double s = std::get<double>(p["size"]);
    double w = 12.0 * s;

    std::vector<GraphicObject> children;

    if (style == "neutral") {
        children.emplace_back(
            "line",
            std::unordered_map<std::string, Value>{{"x1", -w / 2.0}, {"y1", 0.0}, {"x2", w / 2.0}, {"y2", 0.0}},
            std::nullopt,
            "#c0392b",
            2.0 * s);
    } else if (style == "smile") {
        children.emplace_back(
            "line",
            std::unordered_map<std::string, Value>{{"x1", -w / 2.0}, {"y1", -2.0 * s}, {"x2", 0.0}, {"y2", 4.0 * s}},
            std::nullopt,
            "#c0392b",
            2.0 * s);
        children.emplace_back(
            "line",
            std::unordered_map<std::string, Value>{{"x1", 0.0}, {"y1", 4.0 * s}, {"x2", w / 2.0}, {"y2", -2.0 * s}},
            std::nullopt,
            "#c0392b",
            2.0 * s);
    } else if (style == "frown") {
        children.emplace_back(
            "line",
            std::unordered_map<std::string, Value>{{"x1", -w / 2.0}, {"y1", 4.0 * s}, {"x2", 0.0}, {"y2", -2.0 * s}},
            std::nullopt,
            "#c0392b",
            2.0 * s);
        children.emplace_back(
            "line",
            std::unordered_map<std::string, Value>{{"x1", 0.0}, {"y1", -2.0 * s}, {"x2", w / 2.0}, {"y2", 4.0 * s}},
            std::nullopt,
            "#c0392b",
            2.0 * s);
    } else if (style == "open") {
        children.emplace_back(
            "ellipse",
            std::unordered_map<std::string, Value>{{"cx", 0.0}, {"cy", 2.0 * s}, {"rx", w / 2.0}, {"ry", 6.0 * s}},
            "#3d1010",
            "#c0392b",
            1.5);
        children.emplace_back(
            "rect",
            std::unordered_map<std::string, Value>{{"x", -w / 3.0}, {"y", -1.0 * s}, {"w", w * 2.0 / 3.0}, {"h", 3.0 * s}},
            "#FFFFFF");
    } else if (style == "cat") {
        double cat_w = 8.0 * s;
        for (double dx : {-cat_w, 0.0, cat_w}) {
            children.emplace_back(
                "ellipse",
                std::unordered_map<std::string, Value>{{"cx", dx}, {"cy", 2.0 * s}, {"rx", 4.0 * s}, {"ry", 3.0 * s}},
                std::nullopt,
                "#c0392b",
                1.5);
        }
    } else if (style == "fang") {
        children.emplace_back(
            "line",
            std::unordered_map<std::string, Value>{{"x1", -w / 2.0}, {"y1", -2.0 * s}, {"x2", w / 2.0}, {"y2", -2.0 * s}},
            std::nullopt,
            "#c0392b",
            2.0 * s);
        double fang_x = w / 3.0;
        children.emplace_back(
            "polygon",
            std::unordered_map<std::string, Value>{
                {"points",
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
        std::unordered_map<std::string, Value>{},
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
