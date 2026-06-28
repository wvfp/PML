// ==========================================================================================================================================================================================================================================═
// PML Texture-Map Builtins — texture-map
// ==========================================================================================================================================================================================================================================═
//
//   (texture-map <shape>
//                 :source <texture>
//                 [:mode :planar|:harmonic|:explicit]
//                 [:wrap-x :clamp|:repeat|:mirror]
//                 [:wrap-y :clamp|:repeat|:mirror]
//                 [:filter :linear|:nearest]
//                 [:uv-vertices ((x y) ...)]
//                 [:edge-blend <pixels>])
//
//   Sets UV parameters on the shape's GraphicObject and returns it.
//   The actual textured rendering is handled by the Skia backend
//   (textured_draw.cpp) when the shape has ParamKey::uv set.
// ==========================================================================================================================================================================================================================================═

#include "pml/evaluator/texture_map_builtins.h"

#include <memory>
#include <string>
#include <vector>

#include "pml/core/error.h"
#include "pml/core/kwargs.h"
#include "pml/core/texture.h"
#include "pml/core/types.h"
#include "pml/evaluator/environment.h"
#include "pml/graphics/graphics_types.h"
#include "pml/graphics/objects.h"
#include "pml/graphics/params.h"

namespace pml {

// ---- Helper: parse wrap mode string → WrapMode ------------------------------------------------------------

static WrapMode parse_wrap_mode(const std::string& s) {
    if (s == "repeat") return WrapMode::Repeat;
    if (s == "mirror") return WrapMode::Mirror;
    return WrapMode::Clamp;
}

// ---- Helper: parse filter mode string → FilterMode ----------------------------------------------------

static FilterMode parse_filter_mode(const std::string& s) {
    if (s == "nearest") return FilterMode::Nearest;
    return FilterMode::Linear;
}

// ---- Helper: parse UV mode string → int ------------------------------------------------------------------------─

static int parse_uv_mode(const std::string& s) {
    if (s == "planar")   return 0;
    if (s == "explicit") return 2;
    return 1; // harmonic (default)
}

// ---- Helper: read a list of (x y) pairs from a Value ------------------------------------------------
//
// The value must be a list whose elements are either:
//   - a list of 2 numbers representing (x y), or
//   - a flat list of numbers interpreted pairwise [x0 y0 x1 y1 ...]
static std::vector<Vec2> read_uv_vertices(const Value& v) {
    std::vector<Vec2> result;
    const auto* lst = v.as_list();
    if (!lst || !*lst) return result;

    const auto& elements = (*lst)->elements;
    if (elements.empty()) return result;

    // Detect "list of (x y) pairs" vs "flat list of numbers".
    bool all_pairs = true;
    for (const auto& e : elements) {
        const auto* el = e.as_list();
        if (!el || !*el || (*el)->elements.size() < 2) {
            all_pairs = false;
            break;
        }
    }

    if (all_pairs) {
        for (const auto& e : elements) {
            const auto* pair = e.as_list();
            if (!pair || !*pair) continue;
            const auto& p = (*pair)->elements;
            if (p.size() < 2) continue;
            if (p[0].is_number() && p[1].is_number()) {
                double x = p[0].to_double();
                double y = p[1].to_double();
                result.push_back({x, y});
            }
        }
    } else {
        // Flat: [x0 y0 x1 y1 ...]
        for (size_t i = 0; i + 1 < elements.size(); i += 2) {
            if (elements[i].is_number() && elements[i + 1].is_number()) {
                double x = elements[i].to_double();
                double y = elements[i + 1].to_double();
                result.push_back({x, y});
            }
        }
    }

    return result;
}

// ---- texture-map ------------------------------------------------------------------------------------------------------------------------

static Result<Value> texture_map_proc(const std::vector<Value>& args,
                                       Environment& /*env*/) {
    // First positional arg: shape (GraphicObject)
    if (args.empty()) {
        return std::unexpected(type_error(
            "texture-map: requires at least 2 arguments (shape :source texture)"));
    }

    const auto* go_ptr = args[0].as_graphic_object();
    if (!go_ptr || !*go_ptr) {
        return std::unexpected(type_error(
            "texture-map: first argument must be a graphic object (got " +
            value_to_string(args[0]) + ")"));
    }

    // Parse keyword arguments (offset 1).
    auto kwargs = pml::kwargs::parse_kwargs(args, 1);

    // :source — required Texture value
    auto src_it = kwargs.find("source");
    if (src_it == kwargs.end()) {
        return std::unexpected(type_error(
            "texture-map: :source keyword is required"));
    }
    const auto* tex_ptr = src_it->second.as_texture();
    if (!tex_ptr || !*tex_ptr) {
        return std::unexpected(type_error(
            "texture-map: :source must be a texture value (got " +
            value_to_string(src_it->second) + ")"));
    }
    auto source_tex = *tex_ptr;

    // :mode — :planar | :harmonic | :explicit (default :harmonic)
    std::string mode_str = pml::kwargs::kw_string(kwargs, "mode", "harmonic");

    // :wrap-x, :wrap-y — :clamp | :repeat | :mirror (default :clamp)
    std::string wrap_x_str = pml::kwargs::kw_string(kwargs, "wrap-x", "clamp");
    std::string wrap_y_str = pml::kwargs::kw_string(kwargs, "wrap-y", "clamp");

    // :filter — :linear | :nearest (default :linear)
    std::string filter_str = pml::kwargs::kw_string(kwargs, "filter", "linear");

    // :uv-vertices — list of (x y) pairs for explicit UV
    std::vector<Vec2> uv_vertices;
    auto uv_it = kwargs.find("uv-vertices");
    if (uv_it != kwargs.end()) {
        uv_vertices = read_uv_vertices(uv_it->second);
    }

    // :edge-blend — edge blend radius in pixels (default 0)
    double edge_blend = pml::kwargs::kw_double(kwargs, "edge-blend", 0.0);

    // :perspective-correction — enable perspective texture mapping (default false)
    bool perspective_correction = pml::kwargs::kw_bool(kwargs, "perspective-correction", false);

    // Build the texture param Value from the source TextureBox.
    Value tex_value(source_tex);

    // Set all the UV-related params on the GraphicObject (immutable — return
    // a new copy with the parameters set).
    GraphicObject go = **go_ptr;
    go = go.with_param(ParamKey::uv, tex_value);
    go = go.with_param(ParamKey::uv_mode,
        Value(static_cast<double>(parse_uv_mode(mode_str))));
    go = go.with_param(ParamKey::wrap_x,
        Value(static_cast<double>(static_cast<int>(parse_wrap_mode(wrap_x_str)))));
    go = go.with_param(ParamKey::wrap_y,
        Value(static_cast<double>(static_cast<int>(parse_wrap_mode(wrap_y_str)))));
    go = go.with_param(ParamKey::filter,
        Value(static_cast<double>(static_cast<int>(parse_filter_mode(filter_str)))));
    go = go.with_param(ParamKey::edge_blend, Value(edge_blend));

    // Store explicit UVs as a flat list of x/y numbers.
    if (!uv_vertices.empty()) {
        std::vector<Value> uv_list;
        uv_list.reserve(uv_vertices.size() * 2);
        for (const auto& uv : uv_vertices) {
            uv_list.push_back(Value(uv.x));
            uv_list.push_back(Value(uv.y));
        }
        auto uv_val = Value(std::make_shared<ValueList>(std::move(uv_list)));
        go = go.with_param(ParamKey::uv_vertices, uv_val);
    }

    // Store perspective-correction flag.
    if (perspective_correction) {
        go = go.with_param(ParamKey::perspective_correction, Value(1.0));
    }

    return Value(std::make_shared<GraphicObject>(std::move(go)));
}

// ---- Registration --------------------------------------------------------------------------------------------------------------------─

void register_texture_map_builtins(std::shared_ptr<Environment> env) {
    env->define("texture-map",
                Value(std::make_shared<BuiltinProcedure>(
                    "texture-map", texture_map_proc, true)));
}

} // namespace pml
