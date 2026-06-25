// ═══════════════════════════════════════════════════════════════════════════════
// PML Texture Builtins — define-texture, texture?, texture-width, etc.
// ═══════════════════════════════════════════════════════════════════════════════
//
//   (define-texture name (width height) graphic-object)
//       name           — symbol to bind the texture to
//       (width height) — list of two positive integers
//       graphic-object — any GraphicObject (rect, circle, polygon, group, …)
//
//   (texture? v)          → boolean
//   (texture-width tex)   → number
//   (texture-height tex)  → number
//   (texture-id tex)      → number (stable cache id)
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/evaluator/texture_builtins.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "pml/core/error.h"
#include "pml/core/texture.h"
#include "pml/core/types.h"
#include "pml/evaluator/environment.h"

namespace pml {

namespace {

// ── define-texture ─────────────────────────────────────────────────────────

Result<Value> builtin_define_texture(const std::vector<Value>& args,
                                      Environment& env) {
    if (args.size() != 3) {
        return std::unexpected(arity_error(
            SourceLocation{}, 3, static_cast<int>(args.size())));
    }

    // 1) Name must be a symbol.
    const auto* name_sym = args[0].as_symbol();
    if (!name_sym) {
        return std::unexpected(type_error(
            "define-texture: first argument must be a symbol (got " +
            value_to_string(args[0]) + ")"));
    }

    // 2) Size must be a list of two positive integers.
    const auto* size_lst = args[1].as_list();
    if (!size_lst || !*size_lst || (*size_lst)->elements.size() != 2) {
        return std::unexpected(type_error(
            "define-texture: second argument must be a list of two numbers (width height)"));
    }
    const auto& sz = (*size_lst)->elements;
    if (!sz[0].is_number() || !sz[1].is_number()) {
        return std::unexpected(type_error(
            "define-texture: width/height must be numbers"));
    }
    int width = 0;
    int height = 0;
    if (sz[0].is_int()) {
        width = static_cast<int>(sz[0].int_val());
    } else if (sz[0].is_double()) {
        width = static_cast<int>(sz[0].double_val());
    }
    if (sz[1].is_int()) {
        height = static_cast<int>(sz[1].int_val());
    } else if (sz[1].is_double()) {
        height = static_cast<int>(sz[1].double_val());
    }
    if (width <= 0 || height <= 0) {
        return std::unexpected(type_error(
            "define-texture: width and height must be positive integers"));
    }

    // 3) Body must be a GraphicObject.
    const auto* go_ptr = args[2].as_graphic_object();
    if (!go_ptr || !*go_ptr) {
        return std::unexpected(type_error(
            "define-texture: third argument must be a graphic object (got " +
            value_to_string(args[2]) + ")"));
    }

    // 4) Wrap in a TextureBox and bind the name in the environment.
    auto tex = std::make_shared<TextureBox>(*go_ptr, width, height);
    env.define(name_sym->name, Value(std::move(tex)));
    return make_nil_value();
}

// ── texture? ───────────────────────────────────────────────────────────────

Result<Value> builtin_texture_predicate(const std::vector<Value>& args,
                                         Environment& /*env*/) {
    if (args.size() != 1) {
        return std::unexpected(arity_error(
            SourceLocation{}, 1, static_cast<int>(args.size())));
    }
    return Value(args[0].is_texture());
}

// ── texture-width ──────────────────────────────────────────────────────────

Result<Value> builtin_texture_width(const std::vector<Value>& args,
                                     Environment& /*env*/) {
    if (args.size() != 1) {
        return std::unexpected(arity_error(
            SourceLocation{}, 1, static_cast<int>(args.size())));
    }
    const auto* t = args[0].as_texture();
    if (!t || !*t) {
        return std::unexpected(type_error(
            "texture-width: argument must be a texture"));
    }
    return Value(static_cast<double>((*t)->width));
}

// ── texture-height ─────────────────────────────────────────────────────────

Result<Value> builtin_texture_height(const std::vector<Value>& args,
                                      Environment& /*env*/) {
    if (args.size() != 1) {
        return std::unexpected(arity_error(
            SourceLocation{}, 1, static_cast<int>(args.size())));
    }
    const auto* t = args[0].as_texture();
    if (!t || !*t) {
        return std::unexpected(type_error(
            "texture-height: argument must be a texture"));
    }
    return Value(static_cast<double>((*t)->height));
}

// ── texture-id ─────────────────────────────────────────────────────────────

Result<Value> builtin_texture_id(const std::vector<Value>& args,
                                  Environment& /*env*/) {
    if (args.size() != 1) {
        return std::unexpected(arity_error(
            SourceLocation{}, 1, static_cast<int>(args.size())));
    }
    const auto* t = args[0].as_texture();
    if (!t || !*t) {
        return std::unexpected(type_error(
            "texture-id: argument must be a texture"));
    }
    return Value(static_cast<double>((*t)->stable_id));
}

} // anonymous namespace

// ═════════════════════════════════════════════════════════════════════════════
// Registration
// ═════════════════════════════════════════════════════════════════════════════

void register_texture_builtins(std::shared_ptr<Environment> env) {
    env->define("define-texture",
                Value(std::make_shared<BuiltinProcedure>(
                    "define-texture", builtin_define_texture, false)));
    env->define("texture?",
                Value(std::make_shared<BuiltinProcedure>(
                    "texture?", builtin_texture_predicate, false)));
    env->define("texture-width",
                Value(std::make_shared<BuiltinProcedure>(
                    "texture-width", builtin_texture_width, false)));
    env->define("texture-height",
                Value(std::make_shared<BuiltinProcedure>(
                    "texture-height", builtin_texture_height, false)));
    env->define("texture-id",
                Value(std::make_shared<BuiltinProcedure>(
                    "texture-id", builtin_texture_id, false)));
}

} // namespace pml
