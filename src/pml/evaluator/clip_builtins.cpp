// ==========================================================================================================================================================================================================================================═
// PML Clip/Mask Builtins — Implementation
//
// Implements:
//   (with-clip <clip-shape-expr> <body-expr>)
//
// Returns a Group GraphicObject where the first child is the clip shape and
// the second child is the body/content. A metadata flag marks the group as a
// clip container so the Skia backend can apply clipPath().
// ==========================================================================================================================================================================================================================================═

#include "pml/evaluator/clip_builtins.h"

#include <memory>

#include "pml/core/error.h"
#include "pml/evaluator/environment.h"
#include "pml/evaluator/evaluator.h"
#include "pml/graphics/objects.h"

namespace pml {

// ==========================================================================================================================================================================================================================================═
// with-clip special form
// ==========================================================================================================================================================================================================================================═
//   (with-clip <clip-shape-expr> <body-expr>)
//
// 1. Evaluates clip-shape-expr → GraphicObject (the clipping path)
// 2. Evaluates body-expr → GraphicObject (the content to be clipped)
// 3. Returns a Group with both objects and clip metadata

Result<EvalResult> with_clip_special(
    const ArenaExprVector& expr,
    std::shared_ptr<Environment> env,
    SourceLocation call_site)
{
    // expr = [with-clip, clip-shape-expr, body-expr]
    if (expr.size() != 3) {
        return std::unexpected(
            arity_error(call_site, 2,
                        static_cast<int>(expr.size()) - 1));
    }

    // 1) Evaluate clip shape expression.
    auto clip_val = eval_to_value(expr[1], env);
    if (!clip_val) return std::unexpected(clip_val.error());

    const auto* clip_go_ptr = clip_val->as_graphic_object();
    if (!clip_go_ptr || !*clip_go_ptr) {
        return std::unexpected(type_error(
            "with-clip: first argument must evaluate to a graphic object"));
    }

    // 2) Evaluate body expression.
    auto body_val = eval_to_value(expr[expr.size() - 1], env);
    if (!body_val) return std::unexpected(body_val.error());

    const auto* body_go_ptr = body_val->as_graphic_object();
    if (!body_go_ptr || !*body_go_ptr) {
        return std::unexpected(type_error(
            "with-clip: body must evaluate to a graphic object"));
    }

    // 3) Build clip group: children = [clip_shape, body]
    GraphicObject result(ShapeType::Group);
    result.children.reserve(2);
    result.children.push_back(**clip_go_ptr);
    result.children.push_back(**body_go_ptr);
    result.metadata["_clip_path"] = Value(true);

    return Value(std::make_shared<GraphicObject>(std::move(result)));
}

// ==========================================================================================================================================================================================================================================═
// Registration
// ==========================================================================================================================================================================================================================================═

void register_clip_builtins(std::shared_ptr<Environment> env) {
    if (!env) return;

    register_special_form("with-clip", with_clip_special);
}

} // namespace pml
