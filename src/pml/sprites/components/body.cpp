// ==========================================================================================================================================================================================================================================═
// PML Body Component — Implementation
// ==========================================================================================================================================================================================================================================═

#include "body.h"

#include "pml/graphics/objects.h"
#include "pml/sprites/validator.h"

#include <cmath>
#include <unordered_map>

namespace pml {

namespace {

// Build width multipliers
const std::unordered_map<std::string, double> k_build_widths = {
    {"slim", 0.8}, {"average", 1.0}, {"muscular", 1.15}, {"chubby", 1.25}};

// Proportion ratios (torso height as fraction of total height)
const std::unordered_map<std::string, double> k_prop_ratios = {
    {"realistic", 0.45}, {"anime", 0.40}, {"chibi", 0.30}};

[[nodiscard]] double get_or_default(const std::unordered_map<std::string, double>& m,
                                    const std::string& key,
                                    double default_val) {
    auto it = m.find(key);
    return it != m.end() ? it->second : default_val;
}

}  // anonymous namespace

// ==========================================================================================================================================================================================================================================═
// Schema
// ==========================================================================================================================================================================================================================================═

ParamSchema body_schema() {
    return ParamSchema()
        .number("height", 160.0, 80.0, 300.0)
        .enum_("build", {"slim", "average", "muscular", "chubby"}, "average")
        .color("skin", "#fce4c8")
        .enum_("proportions", {"realistic", "anime", "chibi"}, "anime");
}

// ==========================================================================================================================================================================================================================================═
// Factory
// ==========================================================================================================================================================================================================================================═

std::shared_ptr<GraphicObject> create_body(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(body_schema(), kwargs);

    double height = p["height"].double_val();
    std::string build = *p["build"].as_string();
    std::string skin = *p["skin"].as_string();
    std::string proportions = *p["proportions"].as_string();

    double torso_h = height * get_or_default(k_prop_ratios, proportions, 0.40);
    double torso_w = 50.0 * get_or_default(k_build_widths, build, 1.0);

    // Torso center at origin
    GraphicObject torso(
        ShapeType::Rect, Params{{ParamKey::x, -torso_w / 2.0}, {ParamKey::y, 0.0}, {ParamKey::w, torso_w}, {ParamKey::h, torso_h}},
        skin,
        "#1a1a1a",
        2.0,
        AffineTransform::identity(),
        {},
        {{"component", std::string("body")},
         {"build", build},
         {"proportions", proportions}});

    // Neck
    double neck_w = torso_w * 0.35;
    double neck_h = height * 0.06;
    GraphicObject neck(
        ShapeType::Rect, Params{{ParamKey::x, -neck_w / 2.0}, {ParamKey::y, -neck_h}, {ParamKey::w, neck_w}, {ParamKey::h, neck_h}},
        skin,
        "#1a1a1a",
        1.5);

    // Shoulders (ellipses at top of torso)
    double shoulder_r = torso_w * 0.18;
    GraphicObject left_shoulder(
        ShapeType::Ellipse, Params{{ParamKey::cx, -torso_w / 2.0},
         {ParamKey::cy, torso_h * 0.08},
         {ParamKey::rx, shoulder_r},
         {ParamKey::ry, shoulder_r * 0.8}},
        skin,
        "#1a1a1a",
        1.5);
    GraphicObject right_shoulder(
        ShapeType::Ellipse, Params{{ParamKey::cx, torso_w / 2.0},
         {ParamKey::cy, torso_h * 0.08},
         {ParamKey::rx, shoulder_r},
         {ParamKey::ry, shoulder_r * 0.8}},
        skin,
        "#1a1a1a",
        1.5);

    return std::make_shared<GraphicObject>(
        ShapeType::Group,
        Params{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::vector<GraphicObject>{torso, neck, left_shoulder, right_shoulder},
        std::unordered_map<std::string, Value>{
            {"component", std::string("body")},
            {"height", height},
            {"torso_height", torso_h},
            {"torso_width", torso_w},
            {"build", build},
            {"skin", skin}});
}

}  // namespace pml
