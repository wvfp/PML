// ═══════════════════════════════════════════════════════════════════════════════
// PML Canvas / Shape / Style / Transform-Object / Color Builtins
//
// Implements all graphics-related builtins that create and manipulate
// Canvas and GraphicObject values from PML.
// ═══════════════════════════════════════════════════════════════════════════════

#include "canvas_builtins.h"

#include "error.h"
#include "kwargs.h"
#include "types.h"
#include "../graphics/canvas.h"
#include "../graphics/objects.h"
#include "../graphics/transform.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <vector>

namespace pml {

namespace {

// ═══════════════════════════════════════════════════════════════════════════════
// Internal helpers
// ═══════════════════════════════════════════════════════════════════════════════

/// Convert a Value to double. Assumes is_number(v).
[[nodiscard]] double value_to_double(const Value& v) {
    if (v.is_int())
        return static_cast<double>(v.int_val());
    if (v.is_double())
        return v.double_val();
    return 0.0;
}

/// Convert a Value to int, for dimensions. Assumes is_number(v).
[[nodiscard]] int value_to_int(const Value& v) {
    if (v.is_int())
        return static_cast<int>(v.int_val());
    if (v.is_double())
        return static_cast<int>(v.double_val());
    return 0;
}

using pml::kwargs::kw_double;
using pml::kwargs::kw_string;
using pml::kwargs::parse_kwargs;
using pml::kwargs::value_to_opt_string;

/// Expect exactly N args.
[[nodiscard]] Result<void>
expect_arity(size_t expected, const std::vector<Value>& args, std::string_view name) {
    if (args.size() != expected) {
        return std::unexpected(arity_error(
            SourceLocation{}, static_cast<int>(expected), static_cast<int>(args.size())));
    }
    return {};
}

/// Expect at least min_args args.
[[nodiscard]] Result<void>
expect_min_arity(size_t min_args, const std::vector<Value>& args, std::string_view name) {
    if (args.size() < min_args) {
        return std::unexpected(arity_error(
            SourceLocation{}, static_cast<int>(min_args), static_cast<int>(args.size())));
    }
    return {};
}

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
// Canvas builtins
// ═══════════════════════════════════════════════════════════════════════════════

// ── (canvas w h [:bg "white"]) → Canvas ──────────────────────────────────────
//
// Create a new canvas and set it as the current canvas.

static Result<Value> builtin_canvas(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_min_arity(2, args, "canvas");
    if (!r)
        return std::unexpected(r.error());

    int w = value_to_int(args[0]);
    int h = value_to_int(args[1]);

    auto kwargs = parse_kwargs(args, 2);
    std::string bg = kw_string(kwargs, "bg", "#FFFFFF");

    auto canvas = std::make_shared<Canvas>(w, h, std::move(bg));
    current_canvas_ref() = canvas;
    return Value(std::move(canvas));
}

// ── (clear-canvas) → nil ─────────────────────────────────────────────────────
//
// Clear all objects from the current canvas.  Useful for animation loops
// where the canvas itself should not be recreated each frame.

static Result<Value> builtin_clear_canvas(const std::vector<Value>& /*args*/,
                                          Environment& /*env*/) {
    auto canvas = get_current_canvas();
    if (canvas) {
        canvas->objects.clear();
    }
    return make_nil_value();
}

// ── (sprite-canvas w h [:bg "transparent"] [:anchor "center-bottom"]
//                     [:padding 0]) → Canvas ───────────────────────────────────
//
// Create a sprite canvas with transparent background and anchor point.

static Result<Value> builtin_sprite_canvas(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_min_arity(2, args, "sprite-canvas");
    if (!r)
        return std::unexpected(r.error());

    int w = value_to_int(args[0]);
    int h = value_to_int(args[1]);

    auto kwargs = parse_kwargs(args, 2);
    std::string bg = kw_string(kwargs, "bg", "transparent");
    std::string anchor = kw_string(kwargs, "anchor", "center-bottom");
    int padding = static_cast<int>(kw_double(kwargs, "padding", 0.0));

    auto canvas = std::make_shared<Canvas>(w, h, std::move(bg), std::move(anchor), padding, true);
    current_canvas_ref() = canvas;
    return Value(std::move(canvas));
}

// ── (add canvas graphic-object) → nil ────────────────────────────────────────
//
// Add a GraphicObject to a Canvas.

static Result<Value> builtin_add(const std::vector<Value>& args, Environment& /*env*/) {
    if (args.size() < 1 || args.size() > 2) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }

    std::shared_ptr<Canvas> target_canvas;
    std::shared_ptr<GraphicObject> target_obj;

    if (args.size() == 1) {
        // (add obj) — use current canvas
        const auto* go_ptr = args[0].as_graphic_object();
        if (!go_ptr || !*go_ptr) {
            return std::unexpected(type_error("add: argument must be a GraphicObject"));
        }
        target_obj = *go_ptr;

        auto current_canvas = get_current_canvas();
        if (!current_canvas) {
            return std::unexpected(
                PMLException{ErrorCode::GeneralError,
                             std::nullopt,
                             "add: no current canvas. Create one with (canvas w h) first.",
                             std::nullopt});
        }
        target_canvas = current_canvas;
    } else {
        // (add canvas obj)
        const auto* canvas_ptr = args[0].as_canvas();
        if (!canvas_ptr || !*canvas_ptr) {
            return std::unexpected(type_error("add: first argument must be a Canvas"));
        }
        target_canvas = *canvas_ptr;

        const auto* go_ptr = args[1].as_graphic_object();
        if (!go_ptr || !*go_ptr) {
            return std::unexpected(type_error("add: second argument must be a GraphicObject"));
        }
        target_obj = *go_ptr;
    }

    target_canvas->add(*target_obj);
    return make_nil_value();
}

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

// ═══════════════════════════════════════════════════════════════════════════════
// Style builtins
// ═══════════════════════════════════════════════════════════════════════════════

// Each style builtin returns a NEW GraphicObject with the style modified
// (immutable pattern matching GraphicObject's with_fill/with_stroke methods).

// ── (fill graphic-object color) → GraphicObject ──────────────────────────────

static Result<Value> builtin_fill(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_arity(2, args, "fill");
    if (!r)
        return std::unexpected(r.error());

    const auto* go_ptr = args[0].as_graphic_object();
    if (!go_ptr || !*go_ptr) {
        return std::unexpected(type_error("fill: first argument must be a GraphicObject"));
    }

    auto color = value_to_opt_string(args[1]);
    if (!color.has_value()) {
        return std::unexpected(type_error("fill: second argument must be a color string"));
    }

    auto result = std::make_shared<GraphicObject>((*go_ptr)->with_fill(std::move(*color)));
    return Value(std::move(result));
}

// ── (stroke graphic-object color) → GraphicObject ────────────────────────────

static Result<Value> builtin_stroke(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_arity(2, args, "stroke");
    if (!r)
        return std::unexpected(r.error());

    const auto* go_ptr = args[0].as_graphic_object();
    if (!go_ptr || !*go_ptr) {
        return std::unexpected(type_error("stroke: first argument must be a GraphicObject"));
    }

    auto color = value_to_opt_string(args[1]);
    if (!color.has_value()) {
        return std::unexpected(type_error("stroke: second argument must be a color string"));
    }

    auto result = std::make_shared<GraphicObject>((*go_ptr)->with_stroke(std::move(*color)));
    return Value(std::move(result));
}

// ── (no-fill graphic-object) → GraphicObject ─────────────────────────────────
//
// Remove the fill from a GraphicObject.

static Result<Value> builtin_no_fill(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_arity(1, args, "no-fill");
    if (!r)
        return std::unexpected(r.error());

    const auto* go_ptr = args[0].as_graphic_object();
    if (!go_ptr || !*go_ptr) {
        return std::unexpected(type_error("no-fill: argument must be a GraphicObject"));
    }

    auto result = std::make_shared<GraphicObject>(**go_ptr);
    result->fill = std::nullopt;
    return Value(std::move(result));
}

// ── (no-stroke graphic-object) → GraphicObject ───────────────────────────────
//
// Remove the stroke from a GraphicObject.

static Result<Value> builtin_no_stroke(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_arity(1, args, "no-stroke");
    if (!r)
        return std::unexpected(r.error());

    const auto* go_ptr = args[0].as_graphic_object();
    if (!go_ptr || !*go_ptr) {
        return std::unexpected(type_error("no-stroke: argument must be a GraphicObject"));
    }

    auto result = std::make_shared<GraphicObject>(**go_ptr);
    result->stroke = std::nullopt;
    return Value(std::move(result));
}

// ── (stroke-width graphic-object w) → GraphicObject ──────────────────────────

static Result<Value> builtin_stroke_width(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_arity(2, args, "stroke-width");
    if (!r)
        return std::unexpected(r.error());

    const auto* go_ptr = args[0].as_graphic_object();
    if (!go_ptr || !*go_ptr) {
        return std::unexpected(type_error("stroke-width: first argument must be a GraphicObject"));
    }

    double w = value_to_double(args[1]);

    auto result = std::make_shared<GraphicObject>(**go_ptr);
    result->stroke_width = w;
    return Value(std::move(result));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Group builtin
// ═══════════════════════════════════════════════════════════════════════════════

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
// Transform-object builtins
// ═══════════════════════════════════════════════════════════════════════════════

// ── (with-transform graphic-object transform) → GraphicObject ────────────────
//
// Set the transform of a GraphicObject, returning a new copy.

static Result<Value> builtin_with_transform(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_arity(2, args, "with-transform");
    if (!r)
        return std::unexpected(r.error());

    const auto* go_ptr = args[0].as_graphic_object();
    if (!go_ptr || !*go_ptr) {
        return std::unexpected(
            type_error("with-transform: first argument must be a GraphicObject"));
    }

    const auto* t_ptr = args[1].as_transform();
    if (!t_ptr || !*t_ptr) {
        return std::unexpected(
            type_error("with-transform: second argument must be an AffineTransform"));
    }

    auto result = std::make_shared<GraphicObject>((*go_ptr)->with_transform(**t_ptr));
    return Value(std::move(result));
}

// ── (translate-object graphic-object tx ty) → GraphicObject ──────────────────
//
// Compose a translation into the object's existing transform:
//   new_transform = obj.transform · translate(tx, ty)

static Result<Value> builtin_translate_object(const std::vector<Value>& args,
                                              Environment& /*env*/) {
    auto r = expect_arity(3, args, "translate-object");
    if (!r)
        return std::unexpected(r.error());

    const auto* go_ptr = args[0].as_graphic_object();
    if (!go_ptr || !*go_ptr) {
        return std::unexpected(
            type_error("translate-object: first argument must be a GraphicObject"));
    }

    double tx = value_to_double(args[1]);
    double ty = value_to_double(args[2]);

    auto t = AffineTransform::translate(tx, ty);
    auto new_t = (*go_ptr)->transform.compose(t);
    // Use with_transform to preserve the original object ID so that
    // animations registered against this object can still find it.
    auto translated = (*go_ptr)->with_transform(new_t);
    return Value(std::make_shared<GraphicObject>(std::move(translated)));
}

// ── (rotate-object graphic-object angle-deg) → GraphicObject ─────────────────
//
// Compose a rotation into the object's existing transform.

static Result<Value> builtin_rotate_object(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_arity(2, args, "rotate-object");
    if (!r)
        return std::unexpected(r.error());

    const auto* go_ptr = args[0].as_graphic_object();
    if (!go_ptr || !*go_ptr) {
        return std::unexpected(type_error("rotate-object: first argument must be a GraphicObject"));
    }

    double angle = value_to_double(args[1]);

    auto t = AffineTransform::rotate(angle);
    auto new_t = (*go_ptr)->transform.compose(t);
    auto result = std::make_shared<GraphicObject>((*go_ptr)->with_transform(new_t));
    return Value(std::move(result));
}

// ── (scale-object graphic-object sx [sy]) → GraphicObject ────────────────────
//
// Compose a scale into the object's existing transform.
// If sy is omitted, uniform scale is assumed.

static Result<Value> builtin_scale_object(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_min_arity(2, args, "scale-object");
    if (!r)
        return std::unexpected(r.error());

    if (args.size() > 3) {
        return std::unexpected(arity_error(SourceLocation{}, 3, static_cast<int>(args.size())));
    }

    const auto* go_ptr = args[0].as_graphic_object();
    if (!go_ptr || !*go_ptr) {
        return std::unexpected(type_error("scale-object: first argument must be a GraphicObject"));
    }

    double sx = value_to_double(args[1]);
    double sy = sx; // uniform by default
    if (args.size() == 3) {
        sy = value_to_double(args[2]);
    }

    auto t = AffineTransform::scale(sx, sy);
    auto new_t = (*go_ptr)->transform.compose(t);
    auto result = std::make_shared<GraphicObject>((*go_ptr)->with_transform(new_t));
    return Value(std::move(result));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Color helper builtins
// ═══════════════════════════════════════════════════════════════════════════════

// ── (color/rgb r g b) → "#RRGGBB" ───────────────────────────────────────────
//
// r, g, b are integers 0-255.

static Result<Value> builtin_color_rgb(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_arity(3, args, "color/rgb");
    if (!r)
        return std::unexpected(r.error());

    int red = std::clamp(value_to_int(args[0]), 0, 255);
    int green = std::clamp(value_to_int(args[1]), 0, 255);
    int blue = std::clamp(value_to_int(args[2]), 0, 255);

    return Value(std::format("#{:02x}{:02x}{:02x}", red, green, blue));
}

// ── (color/rgba r g b a) → "#RRGGBBAA" ──────────────────────────────────────
//
// r, g, b are integers 0-255. a is a float 0.0-1.0 (or integer 0-255).

static Result<Value> builtin_color_rgba(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_arity(4, args, "color/rgba");
    if (!r)
        return std::unexpected(r.error());

    int red = std::clamp(value_to_int(args[0]), 0, 255);
    int green = std::clamp(value_to_int(args[1]), 0, 255);
    int blue = std::clamp(value_to_int(args[2]), 0, 255);

    int alpha;
    if (args[3].is_double()) {
        // Float 0.0-1.0
        double a = args[3].double_val();
        alpha = std::clamp(static_cast<int>(std::round(a * 255.0)), 0, 255);
    } else {
        alpha = std::clamp(value_to_int(args[3]), 0, 255);
    }

    return Value(std::format("#{:02x}{:02x}{:02x}{:02x}", red, green, blue, alpha));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Registration
// ═══════════════════════════════════════════════════════════════════════════════

void register_canvas_builtins(std::shared_ptr<Environment> env) {
    if (!env)
        return;

    // ── Canvas builtins ─────────────────────────────────────────────────
    def(env, "canvas", builtin_canvas, true);               // accepts :bg
    def(env, "sprite-canvas", builtin_sprite_canvas, true); // accepts :bg, :anchor, :padding
    def(env, "clear-canvas", builtin_clear_canvas);
    def(env, "add", builtin_add);

    // ── Shape factory builtins ──────────────────────────────────────────
    def(env, "circle", builtin_circle, true);   // accepts :fill, :stroke, :stroke-width
    def(env, "rect", builtin_rect, true);       // accepts :fill, :stroke, :stroke-width, :rx
    def(env, "ellipse", builtin_ellipse, true); // accepts :fill, :stroke, :stroke-width
    def(env, "line", builtin_line, true);       // accepts :stroke, :stroke-width
    def(env, "polygon", builtin_polygon, true); // accepts :fill, :stroke, :stroke-width
    def(env, "text", builtin_text, true);       // accepts :fill, :font-size

    // ── Style builtins ─────────────────────────────────────────────────
    def(env, "fill", builtin_fill);
    def(env, "stroke", builtin_stroke);
    def(env, "no-fill", builtin_no_fill);
    def(env, "no-stroke", builtin_no_stroke);
    def(env, "stroke-width", builtin_stroke_width);

    // ── Group builtin ───────────────────────────────────────────────────
    def(env, "group", builtin_group);

    // ── Transform-object builtins ───────────────────────────────────────
    def(env, "with-transform", builtin_with_transform);
    def(env, "translate-object", builtin_translate_object);
    def(env, "rotate-object", builtin_rotate_object);
    def(env, "scale-object", builtin_scale_object);

    // ── Color helper builtins ───────────────────────────────────────────
    def(env, "color/rgb", builtin_color_rgb);
    def(env, "color/rgba", builtin_color_rgba);
}

} // namespace pml
