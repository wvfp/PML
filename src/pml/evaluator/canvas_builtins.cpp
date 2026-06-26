// ==========================================================================================================================================================================================================================================═
// PML Canvas Builtins
//
// Canvas management builtins only (canvas, sprite-canvas, clear-canvas, add).
// Shape, style, transform-object, and color builtins have been split into:
//   shape_builtins.cpp, object_style_builtins.cpp, color_builtins.cpp
// ==========================================================================================================================================================================================================================================═

#include "canvas_builtins.h"

#include "builtins_helpers.h"
#include "color_builtins.h"
#include "error.h"
#include "kwargs.h"
#include "object_style_builtins.h"
#include "shape_builtins.h"
#include "types.h"

using pml::kwargs::kw_double;
using pml::kwargs::kw_string;
using pml::kwargs::parse_kwargs;
using pml::kwargs::value_to_opt_string;
#include "../graphics/canvas.h"
#include "../graphics/objects.h"

#include <memory>
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
// Canvas builtins
// ==========================================================================================================================================================================================================================================═

// ---- (canvas w h [:bg "white"]) → Canvas ----------------------------------------------------------------------------
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

// ---- (clear-canvas) → nil --------------------------------------------------------------------------------------------------------─
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

// ---- (sprite-canvas w h [:bg "transparent"] [:anchor "center-bottom"]
//                     [:padding 0]) → Canvas --------------------------------------------------------------------─
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

// ---- (add canvas graphic-object) → nil --------------------------------------------------------------------------------
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

// ==========================================================================================================================================================================================================================================═
// Registration (delegates to sub-modules)
// ==========================================================================================================================================================================================================================================═

void register_canvas_builtins(std::shared_ptr<Environment> env) {
    if (!env)
        return;

    // ---- Canvas builtins ------------------------------------------------------------------------------------------------─
    def(env, "canvas", builtin_canvas, true);               // accepts :bg
    def(env, "sprite-canvas", builtin_sprite_canvas, true); // accepts :bg, :anchor, :padding
    def(env, "clear-canvas", builtin_clear_canvas);
    def(env, "add", builtin_add);

    // ---- Sub-module registrations --------------------------------------------------------------------------------
    register_shape_builtins(env);          // circle, rect, ellipse, line, polygon, text, group
    register_object_style_builtins(env);   // fill, stroke, no-fill, no-stroke, stroke-width,
                                           // with-transform, translate-object, rotate-object, scale-object
    register_color_builtins(env);          // color/rgb, color/rgba
}

} // namespace pml
