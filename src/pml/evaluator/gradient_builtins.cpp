// ==========================================================================================================================================================================================================================================═
// PML Gradient Builtins
//
// Implements:
//   (linear ((pos color) ...) [:x1 :y1 :x2 :y2])  — linear gradient
//   (radial ((pos color) ...) [:cx :cy :r])         — radial gradient
//
// Both return a Gradient value that can be passed as :fill-gradient to shape
// constructors (circle, rect, polygon, etc.).
// ==========================================================================================================================================================================================================================================═

#include "gradient_builtins.h"

#include "builtins_helpers.h"
#include "error.h"
#include "kwargs.h"
#include "types.h"
#include "../graphics/gradient.h"

#include <memory>
#include <string>
#include <vector>

namespace pml {

using pml::kwargs::kw_double;
using pml::kwargs::parse_kwargs;
using pml::kwargs::value_to_opt_string;

namespace {

// ---- Registration helper ------------------------------------------------------------------------------------------------─

void def(std::shared_ptr<Environment> env,
         const std::string& name,
         BuiltinProcedure::BuiltinFn fn,
         bool accepts_kwargs = false) {
    auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn), accepts_kwargs);
    env->define(name, Value(proc));
}

// ---- Stops parser ---------------------------------------------------------------------------------------------------------------─
//
// Parse a list of (position color) pairs from the first positional argument.
// Expected format: ((0.0 "#267A16") (0.5 "#449e2d") (1.0 "#74c44f"))

Result<std::vector<GradientStop>> parse_stops(const Value& arg) {
    const auto* list = arg.as_list();
    if (!list || !*list) {
        return std::unexpected(
            type_error("gradient: first argument must be a list of (position color) pairs"));
    }

    std::vector<GradientStop> stops;
    stops.reserve((*list)->elements.size());

    for (size_t i = 0; i < (*list)->elements.size(); ++i) {
        const auto& entry = (*list)->elements[i];
        const auto* pair = entry.as_list();
        if (!pair || !*pair || (*pair)->elements.size() < 2) {
            return std::unexpected(type_error(
                std::format("gradient: stop entry {} must be a (position color) list", i)));
        }

        const auto& pos_val = (*pair)->elements[0];
        if (!pos_val.is_number()) {
            return std::unexpected(type_error(
                std::format("gradient: stop entry {} position must be a number", i)));
        }

        auto color = value_to_opt_string((*pair)->elements[1]);
        if (!color) {
            return std::unexpected(type_error(
                std::format("gradient: stop entry {} color must be a string", i)));
        }

        GradientStop stop;
        stop.position = pos_val.to_double();
        stop.color = std::move(*color);
        stops.push_back(std::move(stop));
    }

    if (stops.empty()) {
        return std::unexpected(type_error("gradient: at least one colour stop is required"));
    }

    return stops;
}

// ---- (linear ((pos color) ...) [:x1 0 :y1 0 :x2 0 :y2 1]) → Gradient ----------------------------─

Result<Value> builtin_linear(const std::vector<Value>& args, Environment& /*env*/) {
    if (args.empty()) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }

    auto stops = parse_stops(args[0]);
    if (!stops) return std::unexpected(stops.error());

    auto kwargs = parse_kwargs(args, 1);
    double x1 = kw_double(kwargs, "x1", 0.0);
    double y1 = kw_double(kwargs, "y1", 0.0);
    double x2 = kw_double(kwargs, "x2", 0.0);
    double y2 = kw_double(kwargs, "y2", 1.0);

    auto grad = std::make_shared<Gradient>();
    grad->type = GradientType::Linear;
    grad->x1 = x1;
    grad->y1 = y1;
    grad->x2 = x2;
    grad->y2 = y2;
    grad->stops = std::move(*stops);

    return Value(std::move(grad));
}

// ---- (radial ((pos color) ...) [:cx 0.5 :cy 0.5 :r 0.5]) → Gradient ----------------------------─

Result<Value> builtin_radial(const std::vector<Value>& args, Environment& /*env*/) {
    if (args.empty()) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }

    auto stops = parse_stops(args[0]);
    if (!stops) return std::unexpected(stops.error());

    auto kwargs = parse_kwargs(args, 1);
    double cx = kw_double(kwargs, "cx", 0.5);
    double cy = kw_double(kwargs, "cy", 0.5);
    double r  = kw_double(kwargs, "r", 0.5);

    auto grad = std::make_shared<Gradient>();
    grad->type = GradientType::Radial;
    grad->cx = cx;
    grad->cy = cy;
    grad->r = r;
    grad->stops = std::move(*stops);

    return Value(std::move(grad));
}

// ---- (sweep ((pos color) ...) [:cx 0.5 :cy 0.5 :start-angle 0 :end-angle 360]) → Gradient ─

Result<Value> builtin_sweep(const std::vector<Value>& args, Environment& /*env*/) {
    if (args.empty()) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }

    auto stops = parse_stops(args[0]);
    if (!stops) return std::unexpected(stops.error());

    auto kwargs = parse_kwargs(args, 1);
    double cx  = kw_double(kwargs, "cx", 0.5);
    double cy  = kw_double(kwargs, "cy", 0.5);
    double sa  = kw_double(kwargs, "start-angle", 0.0);
    double ea  = kw_double(kwargs, "end-angle", 360.0);

    auto grad = std::make_shared<Gradient>();
    grad->type = GradientType::Sweep;
    grad->cx = cx;
    grad->cy = cy;
    grad->start_angle = sa;
    grad->end_angle = ea;
    grad->stops = std::move(*stops);

    return Value(std::move(grad));
}

} // anonymous namespace

// ---- Public registration ----------------------------------------------------------------------------------------------------─

void register_gradient_builtins(std::shared_ptr<Environment> env) {
    if (!env) return;

    def(env, "linear", builtin_linear, true);
    def(env, "radial", builtin_radial, true);
    def(env, "sweep", builtin_sweep, true);
}

}  // namespace pml
