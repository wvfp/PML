// ==========================================================================================================================================================================================================================================═
// PML Polygon Perturbation Builtins — Implementation
//
// Provides:
//   (perturb-polygon points :edge-noise N ...) → perturbed ((x y) ...)
// ==========================================================================================================================================================================================================================================═

#include "perturb_builtins.h"

#include "builtins_helpers.h"
#include "error.h"
#include "kwargs.h"
#include "types.h"

#include "../graphics/perlin_noise.h"
#include "../graphics/polygon_perturb.h"
#include "../graphics/rough.h"
#include "../graphics/objects.h"
#include "../graphics/params.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using pml::kwargs::kw_int;
using pml::kwargs::kw_double;
using pml::kwargs::parse_kwargs;

namespace pml {

namespace {

// ---- Registration helper (same pattern as shape_builtins.cpp) ----------------------------

void def(std::shared_ptr<Environment> env,
         const std::string& name,
         BuiltinProcedure::BuiltinFn fn,
         bool accepts_kwargs = false) {
    auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn), accepts_kwargs);
    env->define(name, Value(proc));
}

// ---- Vector-valued kwarg parsing helpers --------------------------------------------------------------------

/// Parse a kwarg that can be either a single double or a list of doubles.
static std::vector<double> parse_double_vector(const Value& v) {
    std::vector<double> result;
    if (const auto* list = v.as_list()) {
        if (*list) {
            result.reserve((*list)->elements.size());
            for (const auto& elem : (*list)->elements) {
                result.push_back(value_to_double(elem));
            }
        }
    } else {
        result.push_back(value_to_double(v));
    }
    return result;
}

/// Parse a kwarg that can be either a single int or a list of ints.
static std::vector<int> parse_int_vector(const Value& v) {
    std::vector<int> result;
    if (const auto* list = v.as_list()) {
        if (*list) {
            result.reserve((*list)->elements.size());
            for (const auto& elem : (*list)->elements) {
                result.push_back(value_to_int(elem));
            }
        }
    } else {
        result.push_back(value_to_int(v));
    }
    return result;
}

/// Parse a kwarg that can be either a single bool or a list of bools.
static std::vector<bool> parse_bool_vector(const Value& v) {
    std::vector<bool> result;
    if (const auto* list = v.as_list()) {
        if (*list) {
            result.reserve((*list)->elements.size());
            for (const auto& elem : (*list)->elements) {
                result.push_back(elem.is_bool() ? elem.bool_val() : true);
            }
        }
    } else {
        result.push_back(v.is_bool() ? v.bool_val() : true);
    }
    return result;
}

// ---- builtin_perturb_polygon --------------------------------------------------------------------------------------------─

static Result<Value> builtin_perturb_polygon(
    const std::vector<Value>& args, Environment& /*env*/)
{
    // Require at least 1 positional arg: the vertex list
    auto r = expect_min_arity(1, args, "perturb-polygon");
    if (!r)
        return std::unexpected(r.error());

    // ---- Parse vertices (same layout as polygon's first argument) --------------------
    //
    // Input: ((x1 y1) (x2 y2) ...)

    const auto* points_list = args[0].as_list();
    if (!points_list || !*points_list) {
        return std::unexpected(
            type_error("perturb-polygon: first argument must be a list of (x y) pairs"));
    }

    std::vector<RoughPoint> vertices;
    vertices.reserve((*points_list)->elements.size());
    for (const auto& pt : (*points_list)->elements) {
        const auto* pt_list = pt.as_list();
        if (!pt_list || !*pt_list || (*pt_list)->elements.size() < 2) {
            return std::unexpected(
                type_error("perturb-polygon: each point must be a list (x y)"));
        }
        double x = value_to_double((*pt_list)->elements[0]);
        double y = value_to_double((*pt_list)->elements[1]);
        vertices.push_back(RoughPoint{x, y});
    }

    // ---- Parse keyword arguments ------------------------------------------------------------------------------------─

    auto kwargs = parse_kwargs(args, 1);

    // :seed (int, default 0)
    int seed = kw_int(kwargs, "seed", 0);

    // :edge-noise (double or list<double>, default {0.0})
    std::vector<double> edge_noise;
    auto noise_it = kwargs.find("edge-noise");
    if (noise_it != kwargs.end()) {
        edge_noise = parse_double_vector(noise_it->second);
    } else {
        edge_noise = {0.0};
    }

    // :edge-mask (bool or list<bool>, default {true})
    std::vector<bool> edge_mask;
    auto mask_it = kwargs.find("edge-mask");
    if (mask_it != kwargs.end()) {
        edge_mask = parse_bool_vector(mask_it->second);
    } else {
        edge_mask = {true};
    }

    // :edge-subdiv (int or list<int>, default {0})
    std::vector<int> edge_subdivisions;
    auto subdiv_it = kwargs.find("edge-subdiv");
    if (subdiv_it != kwargs.end()) {
        edge_subdivisions = parse_int_vector(subdiv_it->second);
    } else {
        edge_subdivisions = {0};
    }

    // :corner-radius (double or list<double>, default {0.0})
    std::vector<double> corner_radius;
    auto corner_r_it = kwargs.find("corner-radius");
    if (corner_r_it != kwargs.end()) {
        corner_radius = parse_double_vector(corner_r_it->second);
    } else {
        corner_radius = {0.0};
    }

    // :corner-mask (bool or list<bool>, default {true})
    std::vector<bool> corner_mask;
    auto corner_m_it = kwargs.find("corner-mask");
    if (corner_m_it != kwargs.end()) {
        corner_mask = parse_bool_vector(corner_m_it->second);
    } else {
        corner_mask = {true};
    }

    // ---- Build configuration --------------------------------------------------------------------------------------------─

    PerturbConfig config;
    config.edge_noise       = std::move(edge_noise);
    config.edge_mask        = std::move(edge_mask);
    config.edge_subdivisions = std::move(edge_subdivisions);
    config.corner_radius    = std::move(corner_radius);
    config.corner_mask      = std::move(corner_mask);
    config.seed             = seed;

    // ---- Create Perlin noise and perturb --------------------------------------------------------------------─

    PerlinNoise2D noise(seed);
    PerturbResult perturbed;
    try {
        perturbed = perturb_polygon(vertices, config, noise);
    } catch (const std::invalid_argument& e) {
        return std::unexpected(
            type_error(std::string("perturb-polygon: ") + e.what()));
    }

    // ---- Flatten result → nested PML list --------------------------------------------------------------------
    //
    // flatten_perturb_result concatenates all edge point lists into one flat
    // vector of RoughPoints.  Convert back to: ((x1 y1) (x2 y2) ...)

    auto flat = flatten_perturb_result(perturbed);

    // Build polygon vertices as nested PML list: ((x1 y1) (x2 y2) ...)
    std::vector<Value> point_pairs;
    point_pairs.reserve(flat.size());
    for (const auto& pt : flat) {
        point_pairs.push_back(make_list_value({Value(pt.x), Value(pt.y)}));
    }

    // Wrap in a GraphicObject so style params (:fill, :stroke, :opacity) work.
    Params params;
    params.set(ParamKey::points, make_list_value(std::move(point_pairs)));

    // Pass through recognised style keyword args.
    auto kw = parse_kwargs(args, 1);
    std::optional<std::string> fill_color;
    std::optional<std::string> stroke_color;
    double stroke_width = kw_double(kw, "stroke-width", 1.0);

    auto fill_it = kw.find("fill");
    if (fill_it != kw.end()) {
        if (const auto* s = fill_it->second.as_string()) fill_color = *s;
    }
    auto stroke_it = kw.find("stroke");
    if (stroke_it != kw.end()) {
        if (const auto* s = stroke_it->second.as_string()) stroke_color = *s;
    }

    auto obj = std::make_shared<GraphicObject>(
        "polygon", std::move(params),
        fill_color, stroke_color, stroke_width);

    return Value(std::move(obj));
}

} // anonymous namespace

// ==========================================================================================================================================================================================================================================═
// Registration
// ==========================================================================================================================================================================================================================================═

void register_perturb_builtins(std::shared_ptr<Environment> env) {
    if (!env)
        return;

    def(env, "perturb-polygon", builtin_perturb_polygon, true);
}

} // namespace pml
