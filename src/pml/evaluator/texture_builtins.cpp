// ==========================================================================================================================================================================================================================================═
// PML Texture Builtins — define-texture, texture?, texture-width, etc.
// ==========================================================================================================================================================================================================================================═
//
//   (define-texture name (width height) graphic-object-expr)
//       name — symbol to bind (NOT evaluated — special form)
//       (width height) — list of two positive integers
//       graphic-object-expr — expression producing a GraphicObject
//
//   (texture? v)          → boolean
//   (texture-width tex)   → number
//   (texture-height tex)  → number
//   (texture-id tex)      → number (stable cache id)
//
// NOTE: define-texture is registered as a SPECIAL FORM (not a builtin)
// because its first argument is a symbol name that must NOT be evaluated.
// Builtins evaluate all arguments, which would cause "unbound variable".
// ==========================================================================================================================================================================================================================================═

#include "pml/evaluator/texture_builtins.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "pml/core/error.h"
#include "pml/core/kwargs.h"
#include "pml/core/texture.h"
#include "pml/core/types.h"
#include "pml/evaluator/environment.h"
#include "pml/evaluator/evaluator.h"

namespace pml {

// Helper: get symbol name from an Expr (like define/lambda do).
static std::optional<std::string> tex_symbol_name(const Expr& expr) {
    if (const auto* sym = std::get_if<Symbol>(&expr)) {
        return sym->name;
    }
    return std::nullopt;
}

// ==========================================================================================================================================================================================================================================═
// define-texture — special form
// ==========================================================================================================================================================================================================================================═

Result<EvalResult> define_texture_special(
    const ArenaExprVector& expr,
    std::shared_ptr<Environment> env,
    SourceLocation call_site)
{
    // expr = [define-texture, name, (width height), body...]
    if (expr.size() < 4) {
        return std::unexpected(
            arity_error(call_site, 3,
                        static_cast<int>(expr.size()) - 1));
    }

    // 1) Name — must be a symbol expression (unevaluated).
    auto name_opt = tex_symbol_name(expr[1]);
    if (!name_opt) {
        return std::unexpected(
            type_error("define-texture: first argument must be a symbol"));
    }
    std::string name = std::move(*name_opt);

    // 2) Size — must be a list of two number expressions.
    const auto* size_lst = get_list(expr[2]);
    if (!size_lst || size_lst->size() != 2) {
        return std::unexpected(type_error(
            "define-texture: second argument must be a list of two numbers (width height)"));
    }
    auto eval_w = eval_to_value((*size_lst)[0], env);
    if (!eval_w) return std::unexpected(eval_w.error());
    auto eval_h = eval_to_value((*size_lst)[1], env);
    if (!eval_h) return std::unexpected(eval_h.error());

    auto to_int = [](const Value& v) -> int {
        return v.is_int() ? static_cast<int>(v.int_val())
             : v.is_double() ? static_cast<int>(v.double_val()) : 0;
    };
    int width = to_int(*eval_w);
    int height = to_int(*eval_h);
    if (width <= 0 || height <= 0) {
        return std::unexpected(type_error(
            "define-texture: width and height must be positive integers"));
    }

    // 3) Evaluate body expressions — last one is the GraphicObject.
    Expr body_expr = expr[expr.size() - 1];
    auto body_val = eval_to_value(body_expr, env);
    if (!body_val) return std::unexpected(body_val.error());

    const auto* go_ptr = body_val->as_graphic_object();
    if (!go_ptr || !*go_ptr) {
        return std::unexpected(type_error(
            "define-texture: body must produce a graphic object (got " +
            value_to_string(*body_val) + ")"));
    }

    // 4) Create TextureBox and bind the name.
    auto tex = std::make_shared<TextureBox>(*go_ptr, width, height);
    env->define(name, Value(std::move(tex)));
    return Value(nullptr);
}

// ==========================================================================================================================================================================================================================================═
// Regular builtins (args are evaluated normally)
// ==========================================================================================================================================================================================================================================═

namespace {

Result<Value> texture_predicate(const std::vector<Value>& args, Environment&) {
    if (args.size() != 1) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }
    return Value(args[0].is_texture());
}

Result<Value> texture_width(const std::vector<Value>& args, Environment&) {
    if (args.size() != 1) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }
    const auto* t = args[0].as_texture();
    if (!t || !*t) {
        return std::unexpected(type_error("texture-width: argument must be a texture"));
    }
    return Value(static_cast<double>((*t)->width));
}

Result<Value> texture_height(const std::vector<Value>& args, Environment&) {
    if (args.size() != 1) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }
    const auto* t = args[0].as_texture();
    if (!t || !*t) {
        return std::unexpected(type_error("texture-height: argument must be a texture"));
    }
    return Value(static_cast<double>((*t)->height));
}

Result<Value> texture_id(const std::vector<Value>& args, Environment&) {
    if (args.size() != 1) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }
    const auto* t = args[0].as_texture();
    if (!t || !*t) {
        return std::unexpected(type_error("texture-id: argument must be a texture"));
    }
    return Value(static_cast<double>((*t)->stable_id));
}

// ---- texture-wrap! ----------------------------------------------------------------------------------------------------------------
//   (texture-wrap! <texture> [:x :clamp|:repeat|:mirror]
//                          [:y :clamp|:repeat|:mirror])

static WrapMode parse_wrap(const std::string& s) {
    if (s == "repeat")  return WrapMode::Repeat;
    if (s == "mirror")  return WrapMode::Mirror;
    return WrapMode::Clamp;
}

Result<Value> texture_wrap(const std::vector<Value>& args, Environment&) {
    if (args.size() < 1) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }
    const auto* t = args[0].as_texture();
    if (!t || !*t) {
        return std::unexpected(type_error("texture-wrap!: argument must be a texture"));
    }
    auto kwargs = pml::kwargs::parse_kwargs(args, 1);
    auto tex = *t;
    std::string wx = pml::kwargs::kw_string(kwargs, "x", "");
    if (!wx.empty()) tex->wrap_x = parse_wrap(wx);
    std::string wy = pml::kwargs::kw_string(kwargs, "y", "");
    if (!wy.empty()) tex->wrap_y = parse_wrap(wy);
    return Value(tex);
}

// ---- texture-filter! ------------------------------------------------------------------------------------------------------------
//   (texture-filter! <texture> :mode :linear|:nearest)

static FilterMode parse_filter(const std::string& s) {
    if (s == "nearest") return FilterMode::Nearest;
    return FilterMode::Linear;
}

Result<Value> texture_filter(const std::vector<Value>& args, Environment&) {
    if (args.size() < 1) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }
    const auto* t = args[0].as_texture();
    if (!t || !*t) {
        return std::unexpected(type_error("texture-filter!: argument must be a texture"));
    }
    auto kwargs = pml::kwargs::parse_kwargs(args, 1);
    auto tex = *t;
    std::string mode = pml::kwargs::kw_string(kwargs, "mode", "");
    if (!mode.empty()) tex->filter = parse_filter(mode);
    return Value(tex);
}

// ---- render-to-texture ----------------------------------------------------------------------------------------------------------------
//   (render-to-texture <width> <height> <graphic-object>) → TextureBox value
//   Renders a GraphicObject at the given resolution and wraps it as a TextureBox.

Result<Value> render_to_texture(const std::vector<Value>& args, Environment&) {
    if (args.size() != 3) {
        return std::unexpected(arity_error(SourceLocation{}, 3, static_cast<int>(args.size())));
    }
    auto to_int = [](const Value& v) -> int {
        return v.is_int() ? static_cast<int>(v.int_val())
             : v.is_double() ? static_cast<int>(v.double_val()) : 0;
    };
    int width = to_int(args[0]);
    int height = to_int(args[1]);
    if (width <= 0 || height <= 0) {
        return std::unexpected(type_error(
            "render-to-texture: width and height must be positive integers"));
    }
    const auto* go_ptr = args[2].as_graphic_object();
    if (!go_ptr || !*go_ptr) {
        return std::unexpected(type_error(
            "render-to-texture: third argument must be a graphic object (got " +
            value_to_string(args[2]) + ")"));
    }
    auto tex = std::make_shared<TextureBox>(*go_ptr, width, height);
    return Value(std::move(tex));
}

} // anonymous namespace

// ---- Registration ----------------------------------------------------------------------------------------------------------------─

void register_texture_builtins(std::shared_ptr<Environment> env) {
    auto def = [&](const std::string& name, auto fn, bool kw = false) {
        env->define(name, Value(std::make_shared<BuiltinProcedure>(name, fn, kw)));
    };

    // define-texture is a special form (name not evaluated)
    register_special_form("define-texture", define_texture_special);

    // Regular builtins
    def("texture?",         texture_predicate);
    def("texture-width",    texture_width);
    def("texture-height",   texture_height);
    def("texture-id",       texture_id);
    def("texture-wrap!",    texture_wrap,     true);
    def("texture-filter!",  texture_filter,   true);
    def("render-to-texture", render_to_texture);
}

} // namespace pml
