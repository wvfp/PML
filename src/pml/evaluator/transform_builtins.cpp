// ==========================================================================================================================================================================================================================================═
// PML Transform Builtins — Implementation
//
// Each builtin wraps an AffineTransform operation and returns the result
// as a Value containing a shared_ptr<AffineTransform>.
// ==========================================================================================================================================================================================================================================═

#include "transform_builtins.h"

#include "builtins_helpers.h"
#include "error.h"
#include "types.h"
#include "../graphics/transform.h"

#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace pml {

namespace {

/// Expect that arg[index] is a number and return its double value.
[[nodiscard]] Result<double> arg_as_double(
    const std::vector<Value>& args, size_t index, std::string_view name)
{
    if (index >= args.size() || !is_number(args[index])) {
        return std::unexpected(type_error(
            std::string(name) + ": argument " + std::to_string(index) +
            " must be a number"));
    }
    return to_double(args[index]);
}

// ---- Registration helper ------------------------------------------------------------------------------------------------─

void def(std::shared_ptr<Environment> env, const std::string& name,
         BuiltinProcedure::BuiltinFn fn, bool accepts_kwargs = false)
{
    auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn), accepts_kwargs);
    env->define(name, Value(proc));
}

}  // anonymous namespace

// ==========================================================================================================================================================================================================================================═
// Builtin implementations
// ==========================================================================================================================================================================================================================================═

// ---- (translate tx ty) → AffineTransform ----------------------------------------------------------------------------
//
// Create a translation transform.

static Result<Value> builtin_translate(
    const std::vector<Value>& args, Environment& /*env*/)
{
    auto r = expect_arity(2, args, "translate");
    if (!r) return std::unexpected(r.error());

    auto tx = arg_as_double(args, 0, "translate");
    if (!tx) return std::unexpected(tx.error());
    auto ty = arg_as_double(args, 1, "translate");
    if (!ty) return std::unexpected(ty.error());

    return Value(std::make_shared<AffineTransform>(
        AffineTransform::translate(*tx, *ty)));
}

// ---- (rotate angle-deg) → AffineTransform ------------------------------------------------------------------------─
//
// Create a rotation transform (counter-clockwise, degrees).

static Result<Value> builtin_rotate(
    const std::vector<Value>& args, Environment& /*env*/)
{
    auto r = expect_arity(1, args, "rotate");
    if (!r) return std::unexpected(r.error());

    auto angle = arg_as_double(args, 0, "rotate");
    if (!angle) return std::unexpected(angle.error());

    return Value(std::make_shared<AffineTransform>(
        AffineTransform::rotate(*angle)));
}

// ---- (scale sx [sy]) → AffineTransform --------------------------------------------------------------------------------
//
// Create a scale transform. If sy is omitted, it defaults to sx (uniform scale).

static Result<Value> builtin_scale(
    const std::vector<Value>& args, Environment& /*env*/)
{
    auto r = expect_min_arity(1, args, "scale");
    if (!r) return std::unexpected(r.error());

    if (args.size() > 2) {
        return std::unexpected(arity_error(
            SourceLocation{}, 2, static_cast<int>(args.size())));
    }

    auto sx = arg_as_double(args, 0, "scale");
    if (!sx) return std::unexpected(sx.error());

    double sy = *sx;
    if (args.size() == 2) {
        auto sy_r = arg_as_double(args, 1, "scale");
        if (!sy_r) return std::unexpected(sy_r.error());
        sy = *sy_r;
    }

    return Value(std::make_shared<AffineTransform>(
        AffineTransform::scale(*sx, sy)));
}

// ---- (shear shx shy) → AffineTransform --------------------------------------------------------------------------------
//
// Create a shear transform.

static Result<Value> builtin_shear(
    const std::vector<Value>& args, Environment& /*env*/)
{
    auto r = expect_arity(2, args, "shear");
    if (!r) return std::unexpected(r.error());

    auto shx = arg_as_double(args, 0, "shear");
    if (!shx) return std::unexpected(shx.error());
    auto shy = arg_as_double(args, 1, "shear");
    if (!shy) return std::unexpected(shy.error());

    return Value(std::make_shared<AffineTransform>(
        AffineTransform::shear(*shx, *shy)));
}

// ---- (identity-matrix) → AffineTransform ----------------------------------------------------------------------------
//
// Return the identity affine transform.

static Result<Value> builtin_identity_matrix(
    const std::vector<Value>& args, Environment& /*env*/)
{
    auto r = expect_arity(0, args, "identity-matrix");
    if (!r) return std::unexpected(r.error());

    return Value(std::make_shared<AffineTransform>(
        AffineTransform::identity()));
}

// ---- (compose t1 t2) → AffineTransform --------------------------------------------------------------------------------
//
// Compose two transforms: result = t1 · t2 (apply t2 first, then t1).

static Result<Value> builtin_compose(
    const std::vector<Value>& args, Environment& /*env*/)
{
    auto r = expect_arity(2, args, "compose");
    if (!r) return std::unexpected(r.error());

    const auto* t1 = args[0].as_transform();
    if (!t1 || !*t1) {
        return std::unexpected(type_error(
            "compose: first argument must be an AffineTransform"));
    }
    const auto* t2 = args[1].as_transform();
    if (!t2 || !*t2) {
        return std::unexpected(type_error(
            "compose: second argument must be an AffineTransform"));
    }

    return Value(std::make_shared<AffineTransform>(
        (*t1)->compose(**t2)));
}

// ---- (matrix-inverse t) → AffineTransform or nil ------------------------------------------------------------
//
// Return the inverse of a transform, or nil if the matrix is singular.

static Result<Value> builtin_matrix_inverse(
    const std::vector<Value>& args, Environment& /*env*/)
{
    auto r = expect_arity(1, args, "matrix-inverse");
    if (!r) return std::unexpected(r.error());

    const auto* t = args[0].as_transform();
    if (!t || !*t) {
        return std::unexpected(type_error(
            "matrix-inverse: argument must be an AffineTransform"));
    }

    auto inv = (*t)->inverse();
    if (!inv.has_value()) {
        // Singular matrix → return nil (matching Python PML behaviour)
        return make_nil_value();
    }
    return Value(std::make_shared<AffineTransform>(std::move(*inv)));
}

// ---- (matrix-apply t x y) → (x' y') ------------------------------------------------------------------------------------
//
// Apply the transform to point (x, y) and return the result as a list (x' y').

static Result<Value> builtin_matrix_apply(
    const std::vector<Value>& args, Environment& /*env*/)
{
    auto r = expect_arity(3, args, "matrix-apply");
    if (!r) return std::unexpected(r.error());

    const auto* t = args[0].as_transform();
    if (!t || !*t) {
        return std::unexpected(type_error(
            "matrix-apply: first argument must be an AffineTransform"));
    }

    auto x = arg_as_double(args, 1, "matrix-apply");
    if (!x) return std::unexpected(x.error());
    auto y = arg_as_double(args, 2, "matrix-apply");
    if (!y) return std::unexpected(y.error());

    auto [rx, ry] = (*t)->apply(*x, *y);
    return make_list_value({Value(rx), Value(ry)});
}

// ---- (matrix? x) → boolean ----------------------------------------------------------------------------------------------------─
//
// Type predicate for AffineTransform values.

static Result<Value> builtin_matrix_p(
    const std::vector<Value>& args, Environment& /*env*/)
{
    auto r = expect_arity(1, args, "matrix?");
    if (!r) return std::unexpected(r.error());
    return Value(args[0].is_transform());
}

// ==========================================================================================================================================================================================================================================═
// Registration
// ==========================================================================================================================================================================================================================================═

void register_transform_builtins(std::shared_ptr<Environment> env)
{
    if (!env) return;

    def(env, "translate",       builtin_translate);
    def(env, "rotate",          builtin_rotate);
    def(env, "scale",           builtin_scale);
    def(env, "shear",           builtin_shear);
    def(env, "identity-matrix", builtin_identity_matrix);
    def(env, "compose",         builtin_compose);
    def(env, "matrix-inverse",  builtin_matrix_inverse);
    def(env, "matrix-apply",    builtin_matrix_apply);
    def(env, "matrix?",         builtin_matrix_p);
}

}  // namespace pml
