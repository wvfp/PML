// ═══════════════════════════════════════════════════════════════════════════════
// PML Shape Factory Builtins
//
// Extracted from canvas_builtins.cpp.  Implements:
//   circle, rect, ellipse, line, polygon, text, group
// ═══════════════════════════════════════════════════════════════════════════════

#include "shape_builtins.h"

#include "builtins_helpers.h"
#include "error.h"
#include "kwargs.h"
#include "types.h"

using pml::kwargs::kw_double;
using pml::kwargs::kw_string;
using pml::kwargs::parse_kwargs;
using pml::kwargs::value_to_opt_string;
#include "../graphics/canvas.h"
#include "../graphics/objects.h"
#include "../graphics/transform.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace pml {

namespace {

// ── Registration helper ─────────────────────────────────────────────────

void def(std::shared_ptr<Environment> env,
         const std::string& name,
         BuiltinProcedure::BuiltinFn fn,
         bool accepts_kwargs = false) {
    auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn), accepts_kwargs);
    env->define(name, Value(proc));
}

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// Shape factory builtins
// ═══════════════════════════════════════════════════════════════════════════════

// ── (circle cx cy r [:fill color] [:stroke color] [:stroke-width w]) → GO ───

static Result<Value> builtin_circle(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_min_arity(3, args, "circle");
    if (!r)
        return std::unexpected(r.error());

    double cx = value_to_double(args[0]);
    double cy = value_to_double(args[1]);
    double r_val = value_to_double(args[2]);

    auto kwargs = parse_kwargs(args, 3);
    std::optional<std::string> fill_color;
    std::optional<std::string> stroke_color;
    double sw = kw_double(kwargs, "stroke-width", 1.0);

    auto fill_it = kwargs.find("fill");
    if (fill_it != kwargs.end()) {
        fill_color = value_to_opt_string(fill_it->second);
    }
    auto stroke_it = kwargs.find("stroke");
    if (stroke_it != kwargs.end()) {
        stroke_color = value_to_opt_string(stroke_it->second);
    }

    auto obj = std::make_shared<GraphicObject>(
        "circle",
        Params{
            {ParamKey::cx, Value(cx)},
            {ParamKey::cy, Value(cy)},
            {ParamKey::r, Value(r_val)},
        },
        fill_color ? std::optional<std::string>(*fill_color) : std::nullopt,
        stroke_color ? std::optional<std::string>(*stroke_color) : std::nullopt,
        sw);
    return Value(std::move(obj));
}

// ── (rect x y w h [:fill color] [:stroke color] [:stroke-width w] [:rx r]) → GO

static Result<Value> builtin_rect(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_min_arity(4, args, "rect");
    if (!r)
        return std::unexpected(r.error());

    double x = value_to_double(args[0]);
    double y = value_to_double(args[1]);
    double w = value_to_double(args[2]);
    double h = value_to_double(args[3]);

    auto kwargs = parse_kwargs(args, 4);
    std::optional<std::string> fill_color;
    std::optional<std::string> stroke_color;
    double sw = kw_double(kwargs, "stroke-width", 1.0);
    double rx = kw_double(kwargs, "rx", 0.0);

    auto fill_it = kwargs.find("fill");
    if (fill_it != kwargs.end()) {
        fill_color = value_to_opt_string(fill_it->second);
    }
    auto stroke_it = kwargs.find("stroke");
    if (stroke_it != kwargs.end()) {
        stroke_color = value_to_opt_string(stroke_it->second);
    }

    Params params;
    params.set(ParamKey::x, Value(x));
    params.set(ParamKey::y, Value(y));
    params.set(ParamKey::w, Value(w));
    params.set(ParamKey::h, Value(h));
    if (rx > 0.0) {
        params.set(ParamKey::rx, Value(rx));
    }

    auto obj = std::make_shared<GraphicObject>(
        "rect",
        std::move(params),
        fill_color ? std::optional<std::string>(*fill_color) : std::nullopt,
        stroke_color ? std::optional<std::string>(*stroke_color) : std::nullopt,
        sw);
    return Value(std::move(obj));
}

// ── (ellipse cx cy rx ry [:fill color] [:stroke color] [:stroke-width w]) → GO

static Result<Value> builtin_ellipse(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_min_arity(4, args, "ellipse");
    if (!r)
        return std::unexpected(r.error());

    double cx = value_to_double(args[0]);
    double cy = value_to_double(args[1]);
    double rx = value_to_double(args[2]);
    double ry = value_to_double(args[3]);

    auto kwargs = parse_kwargs(args, 4);
    std::optional<std::string> fill_color;
    std::optional<std::string> stroke_color;
    double sw = kw_double(kwargs, "stroke-width", 1.0);

    auto fill_it = kwargs.find("fill");
    if (fill_it != kwargs.end()) {
        fill_color = value_to_opt_string(fill_it->second);
    }
    auto stroke_it = kwargs.find("stroke");
    if (stroke_it != kwargs.end()) {
        stroke_color = value_to_opt_string(stroke_it->second);
    }

    auto obj = std::make_shared<GraphicObject>(
        "ellipse",
        Params{
            {ParamKey::cx, Value(cx)},
            {ParamKey::cy, Value(cy)},
            {ParamKey::rx, Value(rx)},
            {ParamKey::ry, Value(ry)},
        },
        fill_color ? std::optional<std::string>(*fill_color) : std::nullopt,
        stroke_color ? std::optional<std::string>(*stroke_color) : std::nullopt,
        sw);
    return Value(std::move(obj));
}

// ── (line x1 y1 x2 y2 [:stroke color] [:stroke-width w]) → GO ───────────────

static Result<Value> builtin_line(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_min_arity(4, args, "line");
    if (!r)
        return std::unexpected(r.error());

    double x1 = value_to_double(args[0]);
    double y1 = value_to_double(args[1]);
    double x2 = value_to_double(args[2]);
    double y2 = value_to_double(args[3]);

    auto kwargs = parse_kwargs(args, 4);
    std::optional<std::string> stroke_color;
    double sw = kw_double(kwargs, "stroke-width", 1.0);

    auto stroke_it = kwargs.find("stroke");
    if (stroke_it != kwargs.end()) {
        stroke_color = value_to_opt_string(stroke_it->second);
    }

    auto obj = std::make_shared<GraphicObject>(
        "line",
        Params{
            {ParamKey::x1, Value(x1)},
            {ParamKey::y1, Value(y1)},
            {ParamKey::x2, Value(x2)},
            {ParamKey::y2, Value(y2)},
        },
        std::nullopt, // lines don't have fill
        stroke_color ? std::optional<std::string>(*stroke_color) : std::nullopt,
        sw);
    return Value(std::move(obj));
}

// ── (polygon points [:fill color] [:stroke color] [:stroke-width w]) → GO ────
//
// points is a list of (x y) pairs: (list (list x1 y1) (list x2 y2) ...)

static Result<Value> builtin_polygon(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_min_arity(1, args, "polygon");
    if (!r)
        return std::unexpected(r.error());

    // First positional arg: list of points
    const auto* points_list = args[0].as_list();
    if (!points_list || !*points_list) {
        return std::unexpected(type_error("polygon: first argument must be a list of (x y) pairs"));
    }

    // Build the points structure as a flat list of alternating x,y values
    std::vector<Value> flat_points;
    for (const auto& pt : (*points_list)->elements) {
        const auto* pt_list = pt.as_list();
        if (!pt_list || !*pt_list || (*pt_list)->elements.size() < 2) {
            return std::unexpected(type_error("polygon: each point must be a list (x y)"));
        }
        flat_points.push_back((*pt_list)->elements[0]); // x
        flat_points.push_back((*pt_list)->elements[1]); // y
    }

    auto kwargs = parse_kwargs(args, 1);
    std::optional<std::string> fill_color;
    std::optional<std::string> stroke_color;
    double sw = kw_double(kwargs, "stroke-width", 1.0);

    auto fill_it = kwargs.find("fill");
    if (fill_it != kwargs.end()) {
        fill_color = value_to_opt_string(fill_it->second);
    }
    auto stroke_it = kwargs.find("stroke");
    if (stroke_it != kwargs.end()) {
        stroke_color = value_to_opt_string(stroke_it->second);
    }

    auto obj = std::make_shared<GraphicObject>(
        "polygon",
        Params{
            {ParamKey::points, make_list_value(std::move(flat_points))},
        },
        fill_color ? std::optional<std::string>(*fill_color) : std::nullopt,
        stroke_color ? std::optional<std::string>(*stroke_color) : std::nullopt,
        sw);
    return Value(std::move(obj));
}

// ── (text x y content [:fill color] [:font-size size]) → GO ─────────────────

static Result<Value> builtin_text(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_min_arity(3, args, "text");
    if (!r)
        return std::unexpected(r.error());

    double x = value_to_double(args[0]);
    double y = value_to_double(args[1]);

    // Content (string)
    auto content = value_to_opt_string(args[2]);
    if (!content.has_value()) {
        return std::unexpected(type_error("text: third argument must be a string"));
    }

    auto kwargs = parse_kwargs(args, 3);
    std::optional<std::string> fill_color;
    double font_size = kw_double(kwargs, "font-size", 16.0);

    auto fill_it = kwargs.find("fill");
    if (fill_it != kwargs.end()) {
        fill_color = value_to_opt_string(fill_it->second);
    }

    Params params;
    params.set(ParamKey::x, Value(x));
    params.set(ParamKey::y, Value(y));
    params.set(ParamKey::text, Value(std::move(*content)));
    params.set(ParamKey::font_size, Value(font_size));

    auto obj = std::make_shared<GraphicObject>("text",
                                               std::move(params),
                                               fill_color ? std::optional<std::string>(*fill_color)
                                                          : std::nullopt,
                                               std::nullopt, // text typically has no stroke
                                               1.0);
    return Value(std::move(obj));
}

// ── (group object1 object2 ...) → GraphicObject ─────────────────────────────
//
// Create a group GraphicObject containing the given objects as children.

static Result<Value> builtin_group(const std::vector<Value>& args, Environment& /*env*/) {
    if (args.empty()) {
        return std::unexpected(arity_error(SourceLocation{}, 1, 0));
    }

    std::vector<GraphicObject> children;
    children.reserve(args.size());

    for (const auto& arg : args) {
        const auto* go_ptr = arg.as_graphic_object();
        if (!go_ptr || !*go_ptr) {
            return std::unexpected(type_error("group: all arguments must be GraphicObjects"));
        }
        children.push_back(**go_ptr);
    }

    auto obj = std::make_shared<GraphicObject>("group",
                                               Params{},
                                               std::nullopt,
                                               std::nullopt,
                                               1.0,
                                               AffineTransform::identity(),
                                               std::move(children));
    return Value(std::move(obj));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Registration
// ═══════════════════════════════════════════════════════════════════════════════

void register_shape_builtins(std::shared_ptr<Environment> env) {
    if (!env)
        return;

    // ── Shape factory builtins ──────────────────────────────────────────
    def(env, "circle", builtin_circle, true);   // accepts :fill, :stroke, :stroke-width
    def(env, "rect", builtin_rect, true);       // accepts :fill, :stroke, :stroke-width, :rx
    def(env, "ellipse", builtin_ellipse, true); // accepts :fill, :stroke, :stroke-width
    def(env, "line", builtin_line, true);       // accepts :stroke, :stroke-width
    def(env, "polygon", builtin_polygon, true); // accepts :fill, :stroke, :stroke-width
    def(env, "text", builtin_text, true);       // accepts :fill, :font-size

    // ── Group builtin ───────────────────────────────────────────────────
    def(env, "group", builtin_group);
}

} // namespace pml
