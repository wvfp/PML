// ==========================================================================================================================================================================================================================================═
// PML Object Style + Transform-Object Builtins
//
// Extracted from canvas_builtins.cpp.  Implements:
//   fill, stroke, no-fill, no-stroke, stroke-width,
//   with-transform, translate-object, rotate-object, scale-object
// ==========================================================================================================================================================================================================================================═

#include "object_style_builtins.h"

#include "builtins_helpers.h"
#include "error.h"
#include "kwargs.h"
#include "types.h"

using pml::kwargs::value_to_opt_string;
#include "../graphics/objects.h"
#include "../graphics/transform.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace pml {

namespace {

// ---- Registration helper ------------------------------------------------------------------------------------------------─

void def(std::shared_ptr<Environment> env,
         const std::string& name,
         BuiltinProcedure::BuiltinFn fn,
         bool accepts_kwargs = false) {
    auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn), accepts_kwargs);
    env->define(name, Value(proc));
}

} // anonymous namespace

// ==========================================================================================================================================================================================================================================═
// Style builtins
// ==========================================================================================================================================================================================================================================═

// Each style builtin returns a NEW GraphicObject with the style modified
// (immutable pattern matching GraphicObject's with_fill/with_stroke methods).

// ---- (fill graphic-object color) → GraphicObject ------------------------------------------------------------

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

// ---- (stroke graphic-object color) → GraphicObject --------------------------------------------------------

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

// ---- (no-fill graphic-object) → GraphicObject ----------------------------------------------------------------─
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

// ---- (no-stroke graphic-object) → GraphicObject ------------------------------------------------------------─
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

// ---- (stroke-width graphic-object w) → GraphicObject ----------------------------------------------------

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

// ==========================================================================================================================================================================================================================================═
// Transform-object builtins
// ==========================================================================================================================================================================================================================================═

// ---- (with-transform graphic-object transform) → GraphicObject --------------------------------
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

// ---- (translate-object graphic-object tx ty) → GraphicObject ------------------------------------
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

// ---- (rotate-object graphic-object angle-deg) → GraphicObject --------------------------------─
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

// ---- (scale-object graphic-object sx [sy]) → GraphicObject ----------------------------------------
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

// ==========================================================================================================================================================================================================================================═
// Registration
// ==========================================================================================================================================================================================================================================═

void register_object_style_builtins(std::shared_ptr<Environment> env) {
    if (!env)
        return;

    // ---- Style builtins ------------------------------------------------------------------------------------------------─
    def(env, "fill", builtin_fill);
    def(env, "stroke", builtin_stroke);
    def(env, "no-fill", builtin_no_fill);
    def(env, "no-stroke", builtin_no_stroke);
    def(env, "stroke-width", builtin_stroke_width);

    // ---- Transform-object builtins ----------------------------------------------------------------------------─
    def(env, "with-transform", builtin_with_transform);
    def(env, "translate-object", builtin_translate_object);
    def(env, "rotate-object", builtin_rotate_object);
    def(env, "scale-object", builtin_scale_object);
}

} // namespace pml
