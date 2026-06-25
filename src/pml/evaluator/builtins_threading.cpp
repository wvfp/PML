// ═══════════════════════════════════════════════════════════════════════════════
// PML Threading Special Forms (→ and ->>)
// ───────────────────────────────────────────────────────────────────────────────
// Implemented as special forms so we can manipulate raw expressions and
// construct function calls with the threaded value injected at the right
// position (first arg for ->, last arg for ->>).
//
// (->  val (fn arg1 arg2) ...)  → fn(val, arg1, arg2), then fn2(result, ...)
// (->> val (fn arg1 arg2) ...)  → fn(arg1, arg2, val), then fn2(..., result)
// ═══════════════════════════════════════════════════════════════════════════════

#include "evaluator.h"
#include "environment.h"
#include "error.h"
#include "types.h"

#include <memory>
#include <string>
#include <vector>

namespace pml {

// ── Forward declarations from evaluator.cpp ──────────────────────────────────
Result<EvalResult> evaluate(const Expr& expr, std::shared_ptr<Environment> env);

// ═══════════════════════════════════════════════════════════════════════════════
// Shared helper: evaluate arguments and apply with threaded value
// ═══════════════════════════════════════════════════════════════════════════════

/// Apply form `(fn arg1 arg2 ...)` with `thread_val` inserted at position
/// `insert_pos` (0 = first arg, -1 = last arg). Returns the result value.
static Result<Value> apply_threaded(
    const Value& thread_val,
    const Expr& form_expr,
    int insert_pos,
    std::shared_ptr<Environment> env)
{
    // Get the list elements (fn arg1 arg2 ...)
    const auto* form_list = get_list(form_expr);
    if (!form_list || form_list->empty()) {
        return std::unexpected(
            type_error(SourceLocation{},
                "->: each form must be a non-empty list"));
    }

    // Evaluate the function (element 0)
    auto fn_result = eval_to_value((*form_list)[0], env);
    if (!fn_result) {
        return std::unexpected(fn_result.error());
    }
    Value fn = std::move(*fn_result);

    // Evaluate each argument expression
    std::vector<Value> evaluated_args;
    for (size_t j = 1; j < form_list->size(); ++j) {
        auto arg_result = eval_to_value((*form_list)[j], env);
        if (!arg_result) {
            return std::unexpected(arg_result.error());
        }
        evaluated_args.push_back(std::move(*arg_result));
    }

    // Build the full argument list with threaded value inserted
    std::vector<Value> call_args;
    if (insert_pos == 0) {
        // thread-first: val goes FIRST
        call_args.reserve(1 + evaluated_args.size());
        call_args.push_back(thread_val);
        for (auto& a : evaluated_args) {
            call_args.push_back(std::move(a));
        }
    } else {
        // thread-last: val goes LAST
        call_args.reserve(evaluated_args.size() + 1);
        for (auto& a : evaluated_args) {
            call_args.push_back(std::move(a));
        }
        call_args.push_back(thread_val);
    }

    // Apply the function with all arguments
    auto app_result = apply_function(fn, call_args, {}, env);
    if (!app_result) {
        return std::unexpected(app_result.error());
    }

    // Handle tail calls via trampoline
    return trampoline(std::move(*app_result));
}

// ═══════════════════════════════════════════════════════════════════════════════
// thread-first
// ═══════════════════════════════════════════════════════════════════════════════

Result<EvalResult> eval_thread_first(
    const std::vector<Expr>& expr,
    std::shared_ptr<Environment> env)
{
    if (expr.size() < 2) {
        return std::unexpected(
            arity_error(SourceLocation{}, 1, static_cast<int>(expr.size()) - 1));
    }

    // Evaluate initial value
    auto val_result = eval_to_value(expr[1], env);
    if (!val_result) {
        return std::unexpected(val_result.error());
    }
    Value current = std::move(*val_result);

    // Apply each remaining form
    for (size_t i = 2; i < expr.size(); ++i) {
        auto result = apply_threaded(current, expr[i], 0, env);
        if (!result) {
            return std::unexpected(result.error());
        }
        current = std::move(*result);
    }

    return EvalResult{Value(current)};
}

// ═══════════════════════════════════════════════════════════════════════════════
// thread-last
// ═══════════════════════════════════════════════════════════════════════════════

Result<EvalResult> eval_thread_last(
    const std::vector<Expr>& expr,
    std::shared_ptr<Environment> env)
{
    if (expr.size() < 2) {
        return std::unexpected(
            arity_error(SourceLocation{}, 1, static_cast<int>(expr.size()) - 1));
    }

    // Evaluate initial value
    auto val_result = eval_to_value(expr[1], env);
    if (!val_result) {
        return std::unexpected(val_result.error());
    }
    Value current = std::move(*val_result);

    // Apply each remaining form
    for (size_t i = 2; i < expr.size(); ++i) {
        auto result = apply_threaded(current, expr[i], -1, env);
        if (!result) {
            return std::unexpected(result.error());
        }
        current = std::move(*result);
    }

    return EvalResult{Value(current)};
}

// ═══════════════════════════════════════════════════════════════════════════════
// Registration
// ═══════════════════════════════════════════════════════════════════════════════

void register_threading_builtins(std::shared_ptr<Environment> env) {
    (void)env;  // threading is registered as special forms, not as env bindings

    // Register as special forms (they receive unevaluated arguments)
    // Note: the special forms must know their name to dispatch correctly.
    // We register them with unique names and the eval_* functions handle
    // position logic based on the function name.
    register_special_form("->",  eval_thread_first);
    register_special_form("->>", eval_thread_last);
}

} // namespace pml
