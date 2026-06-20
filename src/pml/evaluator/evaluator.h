#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Evaluator — Tree-walking Interpreter with Special Forms
// ═══════════════════════════════════════════════════════════════════════════════
//
// Matches the Python PML evaluator (pml/evaluator.py) semantics exactly.
// Implements 21+ special forms, macro expansion, lexical scoping, and
// function application.
//
// Depends on: pml_core (types, error), pml_frontend (lexer/parser/expander),
//             pml_evaluator (environment)
// ═══════════════════════════════════════════════════════════════════════════════

#include "environment.h"
#include "error.h"
#include "types.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Macro expansion depth tracking
// ═══════════════════════════════════════════════════════════════════════════════

/// Module-level macro expansion depth counter.
/// Incremented when entering a macro expansion, decremented on exit.
/// Matches Python's _macro_depth module-level variable.
inline int g_macro_depth = 0;

/// Maximum allowed macro expansion nesting depth.
inline constexpr int MAX_MACRO_DEPTH = 256;

// ═══════════════════════════════════════════════════════════════════════════════
// Tail-call optimization support
// ═══════════════════════════════════════════════════════════════════════════════

/// A pending tail evaluation.  Instead of recursing, the evaluator returns a
/// TailCall and the trampoline loop evaluates the expression iteratively.
struct TailCall {
    Expr expr;
    std::shared_ptr<Environment> env;
};

/// Result of a single evaluator step: either a final value or a pending tail
/// evaluation.
using EvalResult = std::variant<Value, TailCall>;

/// Convert a step result into a final value by repeatedly applying tail calls.
[[nodiscard]] Result<Value> trampoline(Result<EvalResult> result);

// ═══════════════════════════════════════════════════════════════════════════════
// Main evaluate function
// ═══════════════════════════════════════════════════════════════════════════════

/// Evaluate a PML expression in the given environment.
/// Primary overload: takes a shared_ptr to Environment.
/// Handles self-evaluating atoms, symbol lookup, keyword passthrough,
/// special forms, macro expansion, and regular function calls.
///
/// Returns either a final Value or a TailCall; callers should use trampoline()
/// if they need a final Value.
[[nodiscard]] Result<EvalResult> evaluate(
    const Expr& expr, std::shared_ptr<Environment> env);

/// Convenience overload: takes a reference to Environment.
/// Calls shared_from_this() to obtain a shared_ptr.
inline Result<EvalResult> evaluate(const Expr& expr, Environment& env) {
    return evaluate(expr, env.shared_from_this());
}

/// Evaluate an expression to a final value (runs the trampoline internally).
[[nodiscard]] inline Result<Value> eval_to_value(
    const Expr& expr, std::shared_ptr<Environment> env) {
    return trampoline(evaluate(expr, env));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Argument evaluation
// ═══════════════════════════════════════════════════════════════════════════════

/// Result of evaluating a list of argument expressions.
struct EvaluatedArguments {
    std::vector<Value> positional;
    std::unordered_map<std::string, Value> keyword;
};

/// Evaluate argument expressions, separating positional and keyword args.
/// Keywords in the expression list indicate keyword arguments.
/// e.g., (:key value) produces keyword["key"] = evaluated_value
[[nodiscard]] Result<EvaluatedArguments> evaluate_arguments(
    const std::vector<Expr>& exprs, std::shared_ptr<Environment> env);

// ═══════════════════════════════════════════════════════════════════════════════
// Function application
// ═══════════════════════════════════════════════════════════════════════════════

/// Apply a function (Procedure, BuiltinProcedure, or callable Value) to
/// evaluated positional and keyword arguments.
[[nodiscard]] Result<EvalResult> apply_function(
    const Value& func,
    const std::vector<Value>& args,
    const std::unordered_map<std::string, Value>& kwargs,
    std::shared_ptr<Environment> env);

// ═══════════════════════════════════════════════════════════════════════════════
// Truthiness
// ═══════════════════════════════════════════════════════════════════════════════

/// PML truthiness: everything is truthy except #f, 0, nil, and empty list.
[[nodiscard]] bool is_truthy(const Value& value) noexcept;

// ═══════════════════════════════════════════════════════════════════════════════
// Special forms
// ═══════════════════════════════════════════════════════════════════════════════

/// Signature for all special form handlers.
using SpecialForm = std::function<Result<EvalResult>(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env)>;

// ── Core special forms ──────────────────────────────────────────────────────

/// (quote <expr>) → return expr unevaluated.
[[nodiscard]] Result<EvalResult> eval_quote(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (if <cond> <then> [<else>])
[[nodiscard]] Result<EvalResult> eval_if(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (cond (<test> <expr> ...) ... (else <default> ...))
[[nodiscard]] Result<EvalResult> eval_cond(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (define <name> <expr>) or (define (<name> <params>) <body>...)
[[nodiscard]] Result<EvalResult> eval_define(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (lambda (<params>) <body>...)
[[nodiscard]] Result<EvalResult> eval_lambda(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (let ((name expr) ...) <body>...) — parallel bindings.
[[nodiscard]] Result<EvalResult> eval_let(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (let* ((name expr) ...) <body>...) — sequential bindings.
[[nodiscard]] Result<EvalResult> eval_let_star(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (letrec ((name expr) ...) <body>...) — recursive bindings.
[[nodiscard]] Result<EvalResult> eval_letrec(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (begin <expr> ...) — evaluate sequentially, return last.
[[nodiscard]] Result<EvalResult> eval_begin(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (set! <name> <expr>)
[[nodiscard]] Result<EvalResult> eval_set(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (and <expr> ...) — short-circuit, return last truthy or first falsy.
[[nodiscard]] Result<EvalResult> eval_and(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (or <expr> ...) — short-circuit, return first truthy.
[[nodiscard]] Result<EvalResult> eval_or(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (do ((var init step) ...) (test result...) <body>...)
[[nodiscard]] Result<EvalResult> eval_do(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (quasiquote <expr>) — template with unquote interpolation.
[[nodiscard]] Result<EvalResult> eval_quasiquote(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

// ── Module system special forms ────────────────────────────────────────────

/// (provide sym1 sym2 ...) — declare exported symbols from this module.
[[nodiscard]] Result<EvalResult> eval_provide(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (import "path.pml" [as prefix]) — load a module and bind it.
[[nodiscard]] Result<EvalResult> eval_import(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

// ── Macro system special forms ─────────────────────────────────────────────

/// (defmacro name (params) <body>...) — define a macro.
[[nodiscard]] Result<EvalResult> eval_defmacro(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (macroexpand <form>) — expand macros without evaluating.
[[nodiscard]] Result<EvalResult> eval_macroexpand(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

// ── Utility special forms ──────────────────────────────────────────────────

/// (assert <expr>) — evaluate and error if falsy.
[[nodiscard]] Result<EvalResult> eval_assert(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (gensym) or (gensym "prefix") — generate a unique symbol.
[[nodiscard]] Result<EvalResult> eval_gensym(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// (with-exception-handler <handler> <thunk>) — catch errors from thunk.
/// Calls handler with the error as a value (list: (error <type> <message>)).
[[nodiscard]] Result<EvalResult> eval_with_exception_handler(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

// ── Special forms dispatch table ───────────────────────────────────────────

/// Get the special forms dispatch table (function-local static).
[[nodiscard]] const std::unordered_map<std::string, SpecialForm>&
get_special_forms();

/// Register a new special form from another module (e.g., defskeleton).
/// Called during module registration to add to the dispatch table.
void register_special_form(const std::string& name, const SpecialForm& form);

// ═══════════════════════════════════════════════════════════════════════════════
// Quasiquote expansion (internal helper)
// ═══════════════════════════════════════════════════════════════════════════════

/// Recursively expand a quasiquote template into a runtime Value.
/// Handles (unquote x) by evaluating x and (unquote-splicing x) by splicing.
/// Tracks nesting depth so nested quasiquote forms are preserved correctly.
[[nodiscard]] Result<Value> expand_quasiquote(
    const Expr& template_expr, std::shared_ptr<Environment> env,
    int depth = 0);

// ═══════════════════════════════════════════════════════════════════════════════
// Macro expansion helper (internal)
// ═══════════════════════════════════════════════════════════════════════════════

/// Expand a macro and evaluate the result, with depth tracking.
[[nodiscard]] Result<EvalResult> expand_macro(
    Macro& macro, const std::vector<Expr>& args,
    std::shared_ptr<Environment> env);

}  // namespace pml
