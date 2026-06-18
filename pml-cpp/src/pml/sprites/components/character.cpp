// ═══════════════════════════════════════════════════════════════════════════════
// PML Character Component — Implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "character.h"

#include "body.h"
#include "eyes.h"
#include "hair.h"
#include "head.h"
#include "mouth.h"
#include "pml/graphics/objects.h"
#include "pml/graphics/transform.h"
#include "pml/sprites/palette.h"
#include "pml/sprites/style.h"
#include "pml/sprites/validator.h"

#include <vector>

namespace pml {

namespace {

[[nodiscard]] std::shared_ptr<GraphicObject> extract_graphic_object(
    const Value& v) {
    if (const auto* sp = std::get_if<std::shared_ptr<GraphicObject>>(&v)) {
        return *sp;
    }
    return nullptr;
}

[[nodiscard]] std::string extract_string_value(const Value& v) {
    if (const auto* s = std::get_if<std::string>(&v)) return *s;
    if (const auto* sym = std::get_if<Symbol>(&v)) return sym->name;
    if (const auto* kw = std::get_if<Keyword>(&v)) return kw->name;
    return "";
}

[[nodiscard]] GraphicObject apply_style_to_object(
    const GraphicObject& obj, const StyleDescriptor& style) {
    if (style.outline_style == "none") {
        return obj;
    }

    if (obj.shape_type == "group") {
        std::vector<GraphicObject> styled_children;
        styled_children.reserve(obj.children.size());
        for (const auto& child : obj.children) {
            styled_children.push_back(apply_style_to_object(child, style));
        }
        return GraphicObject(
            "group",
            obj.params,
            obj.fill,
            obj.stroke,
            obj.stroke_width,
            obj.transform,
            std::move(styled_children),
            obj.metadata);
    }

    std::optional<std::string> new_stroke = obj.stroke;
    double new_stroke_width = obj.stroke_width;
    if (obj.stroke && *obj.stroke != "#FFFFFF") {
        new_stroke = style.outline_color;
        new_stroke_width = style.outline_width;
    }

    return GraphicObject(
        obj.shape_type,
        obj.params,
        obj.fill,
        new_stroke,
        new_stroke_width,
        obj.transform,
        obj.children,
        obj.metadata);
}

}  // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// Schema
// ═══════════════════════════════════════════════════════════════════════════════

ParamSchema character_schema() {
    return ParamSchema()
        .any_type("body", nullptr)
        .any_type("head", nullptr)
        .any_type("hair", nullptr)
        .any_type("outfit", nullptr)
        .any_type("accessory", nullptr)
        .any_type("weapon", nullptr)
        .enum_("pose", {"standing", "walking", "running", "sitting", "action"}, "standing")
        .enum_("direction", {"front", "back", "left", "right", "front-left", "front-right"}, "front")
        .enum_("expression", {"neutral", "happy", "angry", "sad", "surprised"}, "neutral")
        .any_type("style", nullptr)
        .any_type("palette", nullptr)
        .number("scale", 1.0, 0.1, 10.0);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Factory
// ═══════════════════════════════════════════════════════════════════════════════

std::shared_ptr<GraphicObject> create_character(
    const std::unordered_map<std::string, Value>& kwargs) {
    auto p = validate_params(character_schema(), kwargs);

    std::string expression = std::get<std::string>(p["expression"]);
    double scale = std::get<double>(p["scale"]);

    // Resolve style
    StyleDescriptor style;
    auto style_it = p.find("style");
    if (style_it != p.end() && !std::holds_alternative<std::nullptr_t>(style_it->second)) {
        style = resolve_style(style_it->second);
    }

    // Resolve palette
    std::shared_ptr<Palette> palette;
    auto palette_it = p.find("palette");
    if (palette_it != p.end() && !std::holds_alternative<std::nullptr_t>(palette_it->second)) {
        std::string palette_name = extract_string_value(palette_it->second);
        palette = PaletteManager::instance().get(palette_name);
        PaletteManager::instance().set_active(palette);
    }

    std::string skin = palette ? palette->get("skin") : "#fce4c8";

    // Body
    std::shared_ptr<GraphicObject> body_obj = extract_graphic_object(p["body"]);
    if (!body_obj) {
        body_obj = create_body({{"skin", skin}, {"build", std::string("average")}, {"proportions", std::string("anime")}});
    }

    // Head
    std::shared_ptr<GraphicObject> head_obj = extract_graphic_object(p["head"]);
    if (!head_obj) {
        head_obj = create_head({{"skin", skin}, {"shape", std::string("oval")}, {"ears", std::string("normal")}});
    }

    // Hair
    std::shared_ptr<GraphicObject> hair_obj = extract_graphic_object(p["hair"]);
    if (!hair_obj) {
        hair_obj = create_hair({{"style", std::string("medium")}, {"color", std::string("#2c2c2c")}});
    }

    // Eyes based on expression
    std::unordered_map<std::string, std::string> eye_style_map = {
        {"neutral", "shoujo"},
        {"happy", "round"},
        {"angry", "sharp"},
        {"sad", "sleepy"},
        {"surprised", "round"}};
    std::string eye_style = eye_style_map.count(expression) ? eye_style_map[expression] : "shoujo";
    auto eyes_obj = create_eyes({{"style", eye_style}, {"color", std::string("#4a90d9")}, {"size", 1.0}});

    // Mouth based on expression
    std::unordered_map<std::string, std::string> mouth_style_map = {
        {"neutral", "neutral"},
        {"happy", "smile"},
        {"angry", "frown"},
        {"sad", "frown"},
        {"surprised", "open"}};
    std::string mouth_style = mouth_style_map.count(expression) ? mouth_style_map[expression] : "neutral";
    auto mouth_obj = create_mouth({{"style", mouth_style}, {"size", 1.0}});

    // Layout
    double head_w = 56.0;
    double head_h = 64.0;
    double body_h = 64.0;

    if (const auto* meta_head_w = std::get_if<double>(&head_obj->metadata.at("head_width"))) {
        head_w = *meta_head_w;
    }
    if (const auto* meta_head_h = std::get_if<double>(&head_obj->metadata.at("head_height"))) {
        head_h = *meta_head_h;
    }
    if (const auto* meta_body_h = std::get_if<double>(&body_obj->metadata.at("torso_height"))) {
        body_h = *meta_body_h;
    }

    double head_cy = -(body_h * 0.3 + head_h * 0.5);
    double body_top = 0.0;

    std::vector<GraphicObject> positioned;
    positioned.push_back(body_obj->with_transform(AffineTransform::translate(0.0, body_top)));
    positioned.push_back(head_obj->with_transform(AffineTransform::translate(0.0, head_cy)));
    positioned.push_back(eyes_obj->with_transform(AffineTransform::translate(0.0, head_cy + 2.0)));
    positioned.push_back(mouth_obj->with_transform(AffineTransform::translate(0.0, head_cy + head_h * 0.22)));
    positioned.push_back(hair_obj->with_transform(AffineTransform::translate(0.0, head_cy)));

    // Outfit
    std::shared_ptr<GraphicObject> outfit_obj = extract_graphic_object(p["outfit"]);
    if (outfit_obj) {
        positioned.push_back(outfit_obj->with_transform(AffineTransform::translate(0.0, body_top)));
    }

    // Weapon
    std::shared_ptr<GraphicObject> weapon_obj = extract_graphic_object(p["weapon"]);
    if (weapon_obj) {
        double weapon_x = head_w * 0.6 + 10.0;
        double weapon_y = body_top + body_h * 0.1;
        positioned.push_back(weapon_obj->with_transform(AffineTransform::translate(weapon_x, weapon_y)));
    }

    // Global scale
    if (scale != 1.0) {
        AffineTransform scale_t = AffineTransform::scale(scale, scale);
        for (auto& obj : positioned) {
            obj = obj.with_transform(scale_t.compose(obj.transform));
        }
    }

    // Apply style
    if (style.outline_style != "none") {
        for (auto& obj : positioned) {
            obj = apply_style_to_object(obj, style);
        }
    }

    PaletteManager::instance().set_active(nullptr);

    return std::make_shared<GraphicObject>(
        "group",
        std::unordered_map<std::string, Value>{},
        std::nullopt,
        std::nullopt,
        1.0,
        AffineTransform::identity(),
        std::move(positioned),
        std::unordered_map<std::string, Value>{
            {"component", std::string("character")},
            {"pose", p.count("pose") ? p.at("pose") : std::string("standing")},
            {"direction", p.count("direction") ? p.at("direction") : std::string("front")},
            {"expression", expression},
            {"scale", scale}});
}

}  // namespace pml
