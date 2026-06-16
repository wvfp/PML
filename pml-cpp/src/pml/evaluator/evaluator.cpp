// ═══════════════════════════════════════════════════════════════════════════════
// PML Evaluator — Tree-walking Interpreter with Special Forms
// ═══════════════════════════════════════════════════════════════════════════════
//
// Matches Python pml/evaluator.py (640 lines).
// Implements 21+ special forms, macro expansion, lexical scoping, and
// function application.
// ═══════════════════════════════════════════════════════════════════════════════

#include "evaluator.h"

#include "module_loader.h"

#include <algorithm>
#include <atomic>
#include <format>
#include <sstream>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Internal helpers
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

// ── expr_to_value: Convert an AST Expr to a runtime Value ─────────────────────

/// Convert an AST expression to a runtime value.
/// Self-evaluating atoms (nil, int, double, string, bool, Symbol, Keyword)
/// map directly. List Expr nodes are recursively converted to ValueList.
Value expr_to_value(const Expr& expr) {
    return std::visit(
        [](const auto& arg) -> Value {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                return nullptr;
            } else if constexpr (std::is_same_v<T, int64_t>) {
                return arg;
            } else if constexpr (std::is_same_v<T, double>) {
                return arg;
            } else if constexpr (std::is_same_v<T, std::string>) {
                return arg;
            } else if constexpr (std::is_same_v<T, bool>) {
                return arg;
            } else if constexpr (std::is_same_v<T, Symbol>) {
                return arg;
            } else if constexpr (std::is_same_v<T, Keyword>) {
                return arg;
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<ListExpr>>) {
                auto vl = std::make_shared<ValueList>();
                vl->elements.reserve(arg->elements.size());
                for (const auto& elem : arg->elements) {
                    vl->elements.push_back(expr_to_value(elem));
                }
                return vl;
            } else {
                return nullptr;  // unreachable
            }
        },
        expr);
}

// ── make_body: Wrap body expression list into a single Expr ───────────────────

/// Convert a vector of body expressions into a single Expr suitable for
/// Procedure::body. If there are multiple expressions, they are wrapped
/// in a (begin ...) form. Single expressions are returned as-is.
Expr make_body_from_vector(const std::vector<Expr>& body_exprs) {
    if (body_exprs.empty()) {
        return make_nil();
    }
    if (body_exprs.size() == 1) {
        return body_exprs[0];
    }
    // Wrap multiple expressions in (begin ...)
    std::vector<Expr> begin_form;
    begin_form.reserve(1 + body_exprs.size());
    begin_form.push_back(Symbol("begin"));
    begin_form.insert(begin_form.end(), body_exprs.begin(), body_exprs.end());
    return make_list(std::move(begin_form));
}

// ── extract_symbol_name: Get param name from Expr for lambda/defmacro ────────

/// Extract a symbol name from an Expr that should be a Symbol.
/// Returns nullopt if the expression is not a Symbol.
std::optional<std::string> extract_symbol_name(const Expr& expr) {
    if (const auto* sym = std::get_if<Symbol>(&expr)) {
        return sym->name;
    }
    return std::nullopt;
}

// ── check_arity: Validate argument count ─────────────────────────────────────

/// Return an ArityError if `got` != `expected`.
Result<void> check_arity(int expected, int got, std::string_view form_name) {
    if (expected != got) {
        return std::unexpected(arity_error(
            SourceLocation{}, expected, got));
    }
    return {};
}

/// Return an ArityError if `got` < `min_expected`.
Result<void> check_min_arity(int min_expected, int got,
                             std::string_view form_name) {
    if (got < min_expected) {
        return std::unexpected(arity_error(
            SourceLocation{}, min_expected, got));
    }
    return {};
}

/// Return an ArityError if `got` is outside [min_expected, max_expected].
Result<void> check_arity_range(int min_expected, int max_expected, int got,
                               std::string_view form_name) {
    if (got < min_expected || got > max_expected) {
        return std::unexpected(general_error(
            std::format("{}: expected {} to {} arguments, got {}",
                        form_name, min_expected, max_expected, got)));
    }
    return {};
}

}  // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// Procedure::call — implemented here to avoid circular dependency
// ═══════════════════════════════════════════════════════════════════════════════

Result<Value> Procedure::call(
    const std::vector<Value>& args,
    Environment& /*env*/) {
    // Create call environment by extending closure_env with parameter bindings.
    // This method is called from outside the evaluator; the evaluator's own
    // apply_function handles Procedure calls more directly.
    if (params.size() != args.size()) {
        // Check for rest parameter (convention: last param is ".")
        if (!params.empty() && params.back() == ".") {
            // Rest-param not fully supported here — delegate to evaluator's
            // apply_function which has the full logic.
            return std::unexpected(general_error(
                "Procedure::call does not support rest-param calls; "
                "use the evaluator's apply_function instead"));
        }
        return std::unexpected(arity_error(
            SourceLocation{},
            static_cast<int>(params.size()),
            static_cast<int>(args.size())));
    }

    auto call_env = closure_env->extend(params, args);
    return evaluate(body, call_env);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Macro expansion with depth tracking
// ═══════════════════════════════════════════════════════════════════════════════

Result<Value> expand_macro(
    Macro& macro, const std::vector<Expr>& args,
    std::shared_ptr<Environment> env) {

    g_macro_depth += 1;
    if (g_macro_depth > MAX_MACRO_DEPTH) {
        g_macro_depth = 0;
        return std::unexpected(macro_expansion_depth_error(
            SourceLocation{}, MAX_MACRO_DEPTH));
    }

    // Expand the macro (substitute args into body)
    Expr expanded = macro.expand(args);

    // Evaluate the expanded expression
    auto result = evaluate(expanded, std::move(env));

    g_macro_depth -= 1;
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════════
// is_truthy
// ═══════════════════════════════════════════════════════════════════════════════

bool is_truthy(const Value& value) noexcept {
    return std::visit(
        [](const auto& arg) -> bool {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                return false;  // nil is falsy
            } else if constexpr (std::is_same_v<T, bool>) {
                return arg;  // #t = truthy, #f = falsy
            } else if constexpr (std::is_same_v<T, int64_t>) {
                return arg != 0;  // 0 is falsy
            } else if constexpr (std::is_same_v<T, double>) {
                return arg != 0.0;  // 0.0 is falsy
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<ValueList>>) {
                return !arg->elements.empty();  // empty list is falsy
            } else {
                return true;  // everything else is truthy
            }
        },
        value);
}

// ═══════════════════════════════════════════════════════════════════════════════
// evaluate_arguments
// ═══════════════════════════════════════════════════════════════════════════════

Result<EvaluatedArguments> evaluate_arguments(
    const std::vector<Expr>& exprs, std::shared_ptr<Environment> env) {

    EvaluatedArguments result;
    size_t i = 0;

    while (i < exprs.size()) {
        const Expr& current = exprs[i];

        // Check for keyword argument — starts with a Keyword
        if (const auto* kw = std::get_if<Keyword>(&current)) {
            if (i + 1 >= exprs.size()) {
                return std::unexpected(type_error(
                    std::format("Keyword :{} has no following value", kw->name)));
            }
            auto val = evaluate(exprs[i + 1], env);
            if (!val) {
                return std::unexpected(val.error());
            }
            result.keyword[kw->name] = std::move(*val);
            i += 2;
        } else {
            // Positional argument — evaluate normally
            auto val = evaluate(current, env);
            if (!val) {
                return std::unexpected(val.error());
            }
            result.positional.push_back(std::move(*val));
            i += 1;
        }
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════════
// apply_function
// ═══════════════════════════════════════════════════════════════════════════════

Result<Value> apply_function(
    const Value& func,
    const std::vector<Value>& args,
    const std::unordered_map<std::string, Value>& kwargs,
    std::shared_ptr<Environment> env) {

    return std::visit(
        [&func, &args, &kwargs, &env](const auto& arg) -> Result<Value> {
            using T = std::decay_t<decltype(arg)>;

            // ── User-defined Procedure ─────────────────────────────────
            if constexpr (std::is_same_v<T, std::shared_ptr<Procedure>>) {
                if (!arg) {
                    return std::unexpected(
                        type_error("Cannot apply null procedure"));
                }

                const auto& params = arg->params;
                size_t expected = params.size();
                size_t got = args.size();

                // Handle rest parameter via "." convention
                // (lambda (x . rest) body) → params = [x, ".", "rest"]
                bool has_rest = false;
                size_t fixed_count = expected;

                auto dot_it = std::find(params.begin(), params.end(), ".");
                if (dot_it != params.end()) {
                    has_rest = true;
                    fixed_count = std::distance(params.begin(), dot_it);
                    // fixed_count is the number of params before the "."
                    // The param after "." is the rest param name
                }

                if (has_rest) {
                    // Need at least `fixed_count` args
                    if (got < fixed_count) {
                        return std::unexpected(arity_error(
                            SourceLocation{},
                            static_cast<int>(fixed_count),
                            static_cast<int>(got)));
                    }

                    // Bind fixed params
                    std::vector<std::string> fixed_params;
                    std::vector<Value> fixed_args;
                    fixed_params.reserve(fixed_count);
                    fixed_args.reserve(fixed_count);
                    for (size_t j = 0; j < fixed_count; ++j) {
                        fixed_params.push_back(params[j]);
                        fixed_args.push_back(args[j]);
                    }

                    auto call_env =
                        arg->closure_env->extend(fixed_params, fixed_args);

                    // Rest param name
                    std::string rest_name = *(dot_it + 1);

                    // Create rest list from remaining args
                    std::vector<Value> rest_values;
                    rest_values.reserve(got - fixed_count);
                    for (size_t j = fixed_count; j < got; ++j) {
                        rest_values.push_back(args[j]);
                    }
                    call_env->define(rest_name,
                                     make_list_value(std::move(rest_values)));

                    return evaluate(arg->body, call_env);

                } else {
                    // Simple parameter matching
                    if (expected != got) {
                        return std::unexpected(arity_error(
                            SourceLocation{},
                            static_cast<int>(expected),
                            static_cast<int>(got)));
                    }

                    auto call_env = arg->closure_env->extend(params, args);
                    return evaluate(arg->body, call_env);
                }
            }

            // ── BuiltinProcedure ──────────────────────────────────────
            if constexpr (std::is_same_v<T, std::shared_ptr<BuiltinProcedure>>) {
                if (!arg) {
                    return std::unexpected(
                        type_error("Cannot apply null builtin procedure"));
                }
                // Forward kwargs merged into args vector for builtins that
                // accept keyword arguments (accepts_kwargs=true).
                // The flat-list pattern (:key val :key2 val2) is used so
                // builtins can call parse_kwargs(args, pos) to extract them.
                if (arg->accepts_kwargs && !kwargs.empty()) {
                    std::vector<Value> merged = args;
                    merged.reserve(args.size() + kwargs.size() * 2);
                    for (const auto& [key, val] : kwargs) {
                        merged.push_back(Value(Keyword(key)));
                        merged.push_back(val);
                    }
                    return arg->fn(merged, *env);
                }
                return arg->fn(args, *env);
            }

            // ── Anything else is not callable ─────────────────────────
            return std::unexpected(
                type_error(std::format("Not a procedure: {}",
                                       value_to_string(func))));
        },
        func);
}

// ═══════════════════════════════════════════════════════════════════════════════
// evaluate — main dispatch
// ═══════════════════════════════════════════════════════════════════════════════

Result<Value> evaluate(
    const Expr& expr, std::shared_ptr<Environment> env) {

    return std::visit(
        [&](const auto& arg) -> Result<Value> {
            using T = std::decay_t<decltype(arg)>;

            // ── Self-evaluating atoms ──────────────────────────────────
            // nil, int, float, string, bool: return as-is
            if constexpr (std::is_same_v<T, std::nullptr_t> ||
                          std::is_same_v<T, int64_t> ||
                          std::is_same_v<T, double> ||
                          std::is_same_v<T, std::string> ||
                          std::is_same_v<T, bool>) {
                return Value(arg);
            }

            // ── Symbol lookup ──────────────────────────────────────────
            if constexpr (std::is_same_v<T, Symbol>) {
                const std::string& name = arg.name;

                // Handle module access: prefix/symbol
                auto slash_pos = name.find('/');
                if (slash_pos != std::string::npos) {
                    std::string prefix = name.substr(0, slash_pos);
                    std::string member = name.substr(slash_pos + 1);

                    auto prefix_val = env->try_lookup(prefix);
                    if (prefix_val.has_value()) {
                        // Check if prefix_val is a Module
                        // Module is forward-declared — check via type
                        if (const auto* mod =
                                std::get_if<std::shared_ptr<Module>>(
                                    &*prefix_val)) {
                            if (*mod) {
                                // Module::get is not yet defined
                                // Fall through to normal lookup for now
                            }
                        }
                    }
                }

                return env->lookup(name);
            }

            // ── Keyword — return as-is (used as parameter markers) ────
            if constexpr (std::is_same_v<T, Keyword>) {
                return Value(arg);
            }

            // ── List — special form, macro, or function call ───────────
            if constexpr (std::is_same_v<T, std::shared_ptr<ListExpr>>) {
                const auto& elements = arg->elements;

                if (elements.empty()) {
                    return Value(make_list_value({}));  // empty list
                }

                const Expr& head = elements[0];

                // ── Special forms ──────────────────────────────────────
                if (const auto* head_sym = std::get_if<Symbol>(&head)) {
                    const auto& forms = get_special_forms();
                    auto it = forms.find(head_sym->name);
                    if (it != forms.end()) {
                        return it->second(elements, env);
                    }

                    // ── Macro expansion (by name) ──────────────────────
                    auto macro_val = env->try_lookup(head_sym->name);
                    if (macro_val.has_value()) {
                        if (const auto* mac =
                                std::get_if<std::shared_ptr<Macro>>(
                                    &*macro_val)) {
                            if (*mac) {
                                // Collect args (rest of list)
                                std::vector<Expr> macro_args;
                                macro_args.reserve(elements.size() - 1);
                                for (size_t i = 1; i < elements.size(); ++i) {
                                    macro_args.push_back(elements[i]);
                                }
                                return expand_macro(**mac, macro_args, env);
                            }
                        }
                    }
                }

                // ── Regular function call ──────────────────────────────
                auto func_val = evaluate(head, env);
                if (!func_val) {
                    return std::unexpected(func_val.error());
                }

                // Check if the evaluated function is a Macro
                // (for higher-order macro use / immediate macro values)
                if (const auto* mac =
                        std::get_if<std::shared_ptr<Macro>>(&*func_val)) {
                    if (*mac) {
                        std::vector<Expr> macro_args;
                        macro_args.reserve(elements.size() - 1);
                        for (size_t i = 1; i < elements.size(); ++i) {
                            macro_args.push_back(elements[i]);
                        }
                        return expand_macro(**mac, macro_args, env);
                    }
                }

                // Evaluate arguments and apply function
                std::vector<Expr> arg_exprs;
                arg_exprs.reserve(elements.size() - 1);
                for (size_t i = 1; i < elements.size(); ++i) {
                    arg_exprs.push_back(elements[i]);
                }

                auto eval_args = evaluate_arguments(arg_exprs, env);
                if (!eval_args) {
                    return std::unexpected(eval_args.error());
                }

                return apply_function(
                    *func_val,
                    eval_args->positional,
                    eval_args->keyword,
                    env);
            }

            // ── Unreachable (exhaustive variant) ──────────────────────
            return std::unexpected(
                type_error("Cannot evaluate: unknown expression type"));
        },
        expr);
}

// Forward declaration for value_to_expr (defined after expand_quasiquote)
Expr value_to_expr(const Value& value);

// ═══════════════════════════════════════════════════════════════════════════════
// Quasiquote expansion
// ═══════════════════════════════════════════════════════════════════════════════

Expr expand_quasiquote(
    const Expr& tmpl, std::shared_ptr<Environment> env) {

    // Atoms (nil, int, double, string, bool, Symbol, Keyword) — return as-is
    if (!is_list(tmpl)) {
        return tmpl;
    }

    const auto* list_ptr = std::get_if<std::shared_ptr<ListExpr>>(&tmpl);
    if (!list_ptr || !*list_ptr) {
        return tmpl;
    }

    const auto& elements = (*list_ptr)->elements;

    // Empty list
    if (elements.empty()) {
        return tmpl;
    }

    // Check for (unquote x) at top level
    if (elements.size() == 2) {
        if (const auto* head_sym =
                std::get_if<Symbol>(&elements[0])) {
            if (head_sym->name == "unquote") {
                // Evaluate and return the result as a quoted expression
                auto val = evaluate(elements[1], env);
                if (val) {
                    return make_list({
                        Symbol("quote"),
                        value_to_expr(*val)
                    });
                }
                return make_nil();
            }
        }
    }

    // Recurse into list elements, handling unquote-splicing
    std::vector<Expr> new_elements;
    new_elements.reserve(elements.size());

    for (const auto& item : elements) {
        // Check for (unquote-splicing x) at element level
        if (const auto* item_list =
                std::get_if<std::shared_ptr<ListExpr>>(&item)) {
            const auto& sub = (*item_list)->elements;
            if (sub.size() == 2) {
                if (const auto* sub_sym =
                        std::get_if<Symbol>(&sub[0])) {
                    if (sub_sym->name == "unquote-splicing") {
                        // Evaluate the spliced expression
                        auto val = evaluate(sub[1], env);
                        if (val) {
                            if (const auto* vl =
                                    std::get_if<
                                        std::shared_ptr<ValueList>>(
                                        &*val)) {
                                for (const auto& v :
                                     (*vl)->elements) {
                                    new_elements.push_back(
                                        value_to_expr(v));
                                }
                            }
                        }
                        continue;
                    }
                }
            }
        }

        // Regular element — recursively quasiquote
        new_elements.push_back(
            expand_quasiquote(item, env));
    }

    return make_list(std::move(new_elements));
}

// ═══════════════════════════════════════════════════════════════════════════════
// value_to_expr — convert a Value back to an Expr (for quotation/metaprogramming)
// ═══════════════════════════════════════════════════════════════════════════════

Expr value_to_expr(const Value& value) {
    return std::visit(
        [](const auto& arg) -> Expr {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                return nullptr;
            } else if constexpr (std::is_same_v<T, int64_t>) {
                return arg;
            } else if constexpr (std::is_same_v<T, double>) {
                return arg;
            } else if constexpr (std::is_same_v<T, std::string>) {
                return arg;
            } else if constexpr (std::is_same_v<T, bool>) {
                return arg;
            } else if constexpr (std::is_same_v<T, Symbol>) {
                return arg;
            } else if constexpr (std::is_same_v<T, Keyword>) {
                return arg;
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<ValueList>>) {
                std::vector<Expr> elems;
                elems.reserve(arg->elements.size());
                for (const auto& v : arg->elements) {
                    elems.push_back(value_to_expr(v));
                }
                return make_list(std::move(elems));
            } else {
                // Complex types (Procedure, etc.) — wrap as quote form
                // that will error at evaluation time.
                return make_list({Symbol("quote"),
                                  Symbol(std::format(
                                      "<unevaluable:{}>",
                                      value_to_string(arg)))});
            }
        },
        value);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Special forms
// ═══════════════════════════════════════════════════════════════════════════════

// ── quote: (quote <expr>) → return expr unevaluated ─────────────────────────

Result<Value> eval_quote(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> /*env*/) {
    if (expr.size() != 2) {
        return std::unexpected(
            arity_error(SourceLocation{}, 1,
                        static_cast<int>(expr.size()) - 1));
    }
    // Convert AST expression to runtime value (without evaluating it)
    return expr_to_value(expr[1]);
}

// ── if: (if <cond> <then> [<else>]) ─────────────────────────────────────────

Result<Value> eval_if(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() < 3 || expr.size() > 4) {
        return std::unexpected(general_error(
            "if expects 2 or 3 arguments"));
    }

    auto cond = evaluate(expr[1], env);
    if (!cond) {
        return std::unexpected(cond.error());
    }

    if (is_truthy(*cond)) {
        return evaluate(expr[2], env);
    } else if (expr.size() == 4) {
        return evaluate(expr[3], env);
    }

    return Value(nullptr);  // no else clause → nil
}

// ── cond: (cond (<test> <expr>...) ... (else <default>...)) ─────────────────

Result<Value> eval_cond(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {

    // Iterate over clauses (expr[1:] in Python, but here expr IS the full list
    // from the dispatch, which includes the head. In our dispatch, we pass the
    // full list including the head. So clauses start at index 1.
    for (size_t i = 1; i < expr.size(); ++i) {
        const Expr& clause = expr[i];

        // Clause must be a list: (test expr...)
        const auto* clause_list = get_list(clause);
        if (!clause_list || clause_list->size() < 2) {
            return std::unexpected(
                type_error("cond clause must be (test expr)"));
        }

        const Expr& test = (*clause_list)[0];

        // Check for else clause
        bool is_else = false;
        if (const auto* test_sym = std::get_if<Symbol>(&test)) {
            if (test_sym->name == "else") {
                is_else = true;
            }
        }

        if (is_else) {
            // Evaluate all expressions in the else clause, return last
            Value result(nullptr);
            for (size_t j = 1; j < clause_list->size(); ++j) {
                auto val = evaluate((*clause_list)[j], env);
                if (!val) return std::unexpected(val.error());
                result = std::move(*val);
            }
            return result;
        }

        // Evaluate test
        auto test_val = evaluate(test, env);
        if (!test_val) return std::unexpected(test_val.error());

        if (is_truthy(*test_val)) {
            // Evaluate all expressions in this clause, return last
            Value result(nullptr);
            for (size_t j = 1; j < clause_list->size(); ++j) {
                auto val = evaluate((*clause_list)[j], env);
                if (!val) return std::unexpected(val.error());
                result = std::move(*val);
            }
            return result;
        }
    }

    return Value(nullptr);  // no clause matched → nil
}

// ── define: (define <name> <expr>) or (define (<name> <params>) <body>...) ──

Result<Value> eval_define(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() < 3) {
        return std::unexpected(
            arity_error(SourceLocation{}, 2,
                        static_cast<int>(expr.size()) - 1));
    }

    const Expr& target = expr[1];

    // Function shorthand: (define (name params...) body...)
    if (const auto* target_list = get_list(target)) {
        if (target_list->empty()) {
            return std::unexpected(
                type_error("define function form requires a name"));
        }

        // First element is the function name
        const Expr& name_expr = (*target_list)[0];
        auto name_opt = extract_symbol_name(name_expr);
        if (!name_opt) {
            return std::unexpected(
                type_error("define: expected symbol for function name"));
        }

        // Remaining elements are parameter names
        std::vector<std::string> params;
        bool saw_dot = false;
        std::string rest_param;
        for (size_t j = 1; j < target_list->size(); ++j) {
            const Expr& p = (*target_list)[j];
            if (const auto* p_sym = std::get_if<Symbol>(&p)) {
                if (p_sym->name == ".") {
                    saw_dot = true;
                    continue;
                }
                if (saw_dot) {
                    rest_param = p_sym->name;
                } else {
                    params.push_back(p_sym->name);
                }
            } else {
                return std::unexpected(
                    type_error("define: parameter must be symbol"));
            }
        }

        // Body starts at index 2
        std::vector<Expr> body_exprs;
        body_exprs.reserve(expr.size() - 2);
        for (size_t j = 2; j < expr.size(); ++j) {
            body_exprs.push_back(expr[j]);
        }

        // For now, if we have a rest_param, we include "." and rest_param
        // in the params list so that apply_function can detect it.
        if (saw_dot && !rest_param.empty()) {
            params.push_back(".");
            params.push_back(rest_param);
        }

        auto proc = std::make_shared<Procedure>(
            params,
            make_body_from_vector(body_exprs),
            env,
            *name_opt);
        env->define(*name_opt, Value(proc));
        return Value(nullptr);
    }

    // Variable: (define name expr)
    auto name_opt = extract_symbol_name(target);
    if (!name_opt) {
    {
        std::ostringstream ss;
        ss << expr[1];
        return std::unexpected(
            type_error(std::format("define: expected symbol, got {}", ss.str())));
    }
    }

    auto value = evaluate(expr[2], env);
    if (!value) {
        return std::unexpected(value.error());
    }

    env->define(*name_opt, *value);
    return Value(nullptr);
}

// ── lambda: (lambda (<params>) <body>...) ───────────────────────────────────

Result<Value> eval_lambda(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() < 3) {
        return std::unexpected(
            arity_error(SourceLocation{}, 2,
                        static_cast<int>(expr.size()) - 1));
    }

    const Expr& params_expr = expr[1];
    const auto* params_list = get_list(params_expr);
    if (!params_list) {
        return std::unexpected(
            type_error("lambda: parameters must be a list"));
    }

    // Parse parameters, supporting rest param via '.' pattern
    std::vector<std::string> params;
    bool saw_dot = false;
    std::string rest_param;

    for (const auto& p : *params_list) {
        auto name_opt = extract_symbol_name(p);
        if (!name_opt) {
            return std::unexpected(
                type_error("lambda: parameter must be symbol"));
        }

        if (*name_opt == ".") {
            saw_dot = true;
            continue;
        }

        if (saw_dot) {
            rest_param = *name_opt;
        } else {
            params.push_back(*name_opt);
        }
    }

    // If we have a rest parameter, append "." and the rest param name
    // to the params list so that apply_function can detect them.
    if (saw_dot && !rest_param.empty()) {
        params.push_back(".");
        params.push_back(rest_param);
    }

    // Body expressions start at index 2
    std::vector<Expr> body_exprs;
    body_exprs.reserve(expr.size() - 2);
    for (size_t i = 2; i < expr.size(); ++i) {
        body_exprs.push_back(expr[i]);
    }

    auto proc = std::make_shared<Procedure>(
        params,
        make_body_from_vector(body_exprs),
        env);

    return Value(std::move(proc));
}

// ── let: (let ((name expr) ...) <body>...) — parallel bindings ─────────────

Result<Value> eval_let(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() < 3) {
        return std::unexpected(
            arity_error(SourceLocation{}, 2,
                        static_cast<int>(expr.size()) - 1));
    }

    const Expr& bindings_expr = expr[1];
    const auto* bindings_list = get_list(bindings_expr);
    if (!bindings_list) {
        return std::unexpected(
            type_error("let: bindings must be a list"));
    }

    std::vector<std::string> names;
    std::vector<Value> values;
    names.reserve(bindings_list->size());
    values.reserve(bindings_list->size());

    for (const auto& binding : *bindings_list) {
        const auto* binding_list = get_list(binding);
        if (!binding_list || binding_list->size() != 2) {
            return std::unexpected(
                type_error("let: each binding must be (name expr)"));
        }

        auto name_opt = extract_symbol_name((*binding_list)[0]);
        if (!name_opt) {
            return std::unexpected(
                type_error("let: binding name must be symbol"));
        }

        names.push_back(std::move(*name_opt));

        // All values are evaluated in the OUTER environment (parallel)
        auto val = evaluate((*binding_list)[1], env);
        if (!val) return std::unexpected(val.error());
        values.push_back(std::move(*val));
    }

    // Create child environment and evaluate body
    auto child_env = env->extend(names, values);

    Value result(nullptr);
    for (size_t i = 2; i < expr.size(); ++i) {
        auto val = evaluate(expr[i], child_env);
        if (!val) return std::unexpected(val.error());
        result = std::move(*val);
    }
    return result;
}

// ── let*: (let* ((name expr) ...) <body>...) — sequential bindings ─────────

Result<Value> eval_let_star(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() < 3) {
        return std::unexpected(
            arity_error(SourceLocation{}, 2,
                        static_cast<int>(expr.size()) - 1));
    }

    const Expr& bindings_expr = expr[1];
    const auto* bindings_list = get_list(bindings_expr);
    if (!bindings_list) {
        return std::unexpected(
            type_error("let*: bindings must be a list"));
    }

    // Create child environment
    auto child_env = std::make_shared<Environment>(env);

    for (const auto& binding : *bindings_list) {
        const auto* binding_list = get_list(binding);
        if (!binding_list || binding_list->size() != 2) {
            return std::unexpected(
                type_error("let*: each binding must be (name expr)"));
        }

        auto name_opt = extract_symbol_name((*binding_list)[0]);
        if (!name_opt) {
            return std::unexpected(
                type_error("let*: binding name must be symbol"));
        }

        // Each value is evaluated in the growing child environment
        auto val = evaluate((*binding_list)[1], child_env);
        if (!val) return std::unexpected(val.error());

        child_env->define(*name_opt, *val);
    }

    Value result(nullptr);
    for (size_t i = 2; i < expr.size(); ++i) {
        auto val = evaluate(expr[i], child_env);
        if (!val) return std::unexpected(val.error());
        result = std::move(*val);
    }
    return result;
}

// ── letrec: (letrec ((name expr) ...) <body>...) — recursive bindings ──────

Result<Value> eval_letrec(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() < 3) {
        return std::unexpected(
            arity_error(SourceLocation{}, 2,
                        static_cast<int>(expr.size()) - 1));
    }

    const Expr& bindings_expr = expr[1];
    const auto* bindings_list = get_list(bindings_expr);
    if (!bindings_list) {
        return std::unexpected(
            type_error("letrec: bindings must be a list"));
    }

    // Create child environment with all names bound to placeholder nil
    auto child_env = std::make_shared<Environment>(env);
    std::vector<std::string> names;
    names.reserve(bindings_list->size());

    for (const auto& binding : *bindings_list) {
        const auto* binding_list = get_list(binding);
        if (!binding_list || binding_list->size() != 2) {
            return std::unexpected(
                type_error("letrec: each binding must be (name expr)"));
        }

        auto name_opt = extract_symbol_name((*binding_list)[0]);
        if (!name_opt) {
            return std::unexpected(
                type_error("letrec: binding name must be symbol"));
        }

        names.push_back(*name_opt);
        child_env->define(*name_opt, nullptr);  // placeholder
    }

    // Now evaluate init expressions in the child env (where all names exist)
    for (size_t bi = 0; bi < names.size(); ++bi) {
        const auto& binding = (*bindings_list)[bi];
        const auto* binding_list = get_list(binding);
        auto val = evaluate((*binding_list)[1], child_env);
        if (!val) return std::unexpected(val.error());
        auto set_r = child_env->set(names[bi], *val);
        if (!set_r) return std::unexpected(set_r.error());
    }

    // Evaluate body
    Value result(nullptr);
    for (size_t i = 2; i < expr.size(); ++i) {
        auto val = evaluate(expr[i], child_env);
        if (!val) return std::unexpected(val.error());
        result = std::move(*val);
    }
    return result;
}

// ── begin: (begin <expr> ...) — evaluate sequentially, return last ─────────

Result<Value> eval_begin(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    Value result(nullptr);
    for (size_t i = 1; i < expr.size(); ++i) {
        auto val = evaluate(expr[i], env);
        if (!val) return std::unexpected(val.error());
        result = std::move(*val);
    }
    return result;
}

// ── set!: (set! <name> <expr>) ──────────────────────────────────────────────

Result<Value> eval_set(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() != 3) {
        return std::unexpected(
            arity_error(SourceLocation{}, 2,
                        static_cast<int>(expr.size()) - 1));
    }

    auto name_opt = extract_symbol_name(expr[1]);
    if (!name_opt) {
        return std::unexpected(
            type_error("set!: expected symbol"));
    }

    auto value = evaluate(expr[2], env);
    if (!value) {
        return std::unexpected(value.error());
    }

    auto set_result = env->set(*name_opt, *value);
    if (!set_result) {
        return std::unexpected(set_result.error());
    }

    return Value(nullptr);
}

// ── and: (and <expr> ...) — short-circuit, return last truthy or first falsy ─

Result<Value> eval_and(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    Value result(true);  // Start with true (identity for and)

    for (size_t i = 1; i < expr.size(); ++i) {
        auto val = evaluate(expr[i], env);
        if (!val) return std::unexpected(val.error());

        if (!is_truthy(*val)) {
            return *val;  // Return first falsy value (short-circuit)
        }
        result = std::move(*val);
    }

    return result;  // Return last value (could be #t)
}

// ── or: (or <expr> ...) — short-circuit, return first truthy ────────────────

Result<Value> eval_or(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    for (size_t i = 1; i < expr.size(); ++i) {
        auto val = evaluate(expr[i], env);
        if (!val) return std::unexpected(val.error());

        if (is_truthy(*val)) {
            return *val;  // Return first truthy value (short-circuit)
        }
    }

    return Value(false);  // No truthy value found → #f
}

// ── do: (do ((var init step) ...) (test result...) <body>...) ──────────────

Result<Value> eval_do(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() < 3) {
        return std::unexpected(
            arity_error(SourceLocation{}, 2,
                        static_cast<int>(expr.size()) - 1));
    }

    // Parse variable clauses
    const Expr& var_clauses_expr = expr[1];
    const auto* var_clauses = get_list(var_clauses_expr);
    if (!var_clauses) {
        return std::unexpected(
            type_error("do: variable clauses must be a list"));
    }

    // Parse test clause
    const Expr& test_clause_expr = expr[2];
    const auto* test_clause = get_list(test_clause_expr);
    if (!test_clause || test_clause->empty()) {
        return std::unexpected(
            type_error("do: test clause must be (test [result])"));
    }

    // Parse variable specs
    std::vector<std::string> names;
    std::vector<Expr> inits;
    std::vector<Expr> steps;
    names.reserve(var_clauses->size());
    inits.reserve(var_clauses->size());
    steps.reserve(var_clauses->size());

    for (const auto& clause : *var_clauses) {
        const auto* clause_list = get_list(clause);
        if (!clause_list || clause_list->size() < 2) {
            return std::unexpected(
                type_error("do: each variable clause must be (var init [step])"));
        }

        auto name_opt = extract_symbol_name((*clause_list)[0]);
        if (!name_opt) {
            return std::unexpected(
                type_error("do: variable must be symbol"));
        }

        names.push_back(std::move(*name_opt));
        inits.push_back((*clause_list)[1]);

        // Step is the 3rd element, or the variable name itself (default to identity)
        if (clause_list->size() > 2) {
            steps.push_back((*clause_list)[2]);
        } else {
            steps.push_back((*clause_list)[0]);  // use var name as step
        }
    }

    // Create loop environment
    auto loop_env = std::make_shared<Environment>(env);

    // Initialize variables (evaluate inits in the outer environment)
    for (size_t i = 0; i < names.size(); ++i) {
        auto init_val = evaluate(inits[i], env);
        if (!init_val) return std::unexpected(init_val.error());
        loop_env->define(names[i], *init_val);
    }

    // Loop
    while (true) {
        // Evaluate test
        auto test_val = evaluate((*test_clause)[0], loop_env);
        if (!test_val) return std::unexpected(test_val.error());

        if (is_truthy(*test_val)) {
            // Return result expression (if any)
            if (test_clause->size() > 1) {
                return evaluate((*test_clause)[1], loop_env);
            }
            return Value(nullptr);
        }

        // Execute body
        for (size_t i = 3; i < expr.size(); ++i) {
            auto body_val = evaluate(expr[i], loop_env);
            if (!body_val) return std::unexpected(body_val.error());
        }

        // Update variables (evaluate steps in current env, then assign)
        std::vector<Value> new_values;
        new_values.reserve(steps.size());
        for (const auto& step : steps) {
            auto step_val = evaluate(step, loop_env);
            if (!step_val) return std::unexpected(step_val.error());
            new_values.push_back(std::move(*step_val));
        }

        for (size_t i = 0; i < names.size(); ++i) {
            auto set_r = loop_env->set(names[i], new_values[i]);
            if (!set_r) return std::unexpected(set_r.error());
        }
    }
}

// ── quasiquote: (quasiquote <expr>) — template with unquote interpolation ─

Result<Value> eval_quasiquote(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() != 2) {
        return std::unexpected(
            arity_error(SourceLocation{}, 1,
                        static_cast<int>(expr.size()) - 1));
    }

    // Expand quasiquote to get a new expression, then evaluate it
    Expr expanded = expand_quasiquote(expr[1], env);
    return evaluate(expanded, env);
}

// ── provide: (provide sym1 sym2 ...) — declare module exports ──────────────

Result<Value> eval_provide(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    for (size_t i = 1; i < expr.size(); ++i) {
        auto name_opt = extract_symbol_name(expr[i]);
        if (!name_opt) {
            return std::unexpected(
                type_error("provide: expected symbol"));
        }
        env->exports.insert(*name_opt);
    }
    return Value(nullptr);
}

// ── import: (import "path.pml" [as prefix]) — load a module ────────────────

Result<Value> eval_import(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() < 2) {
        return std::unexpected(
            arity_error(SourceLocation{}, 1,
                        static_cast<int>(expr.size()) - 1));
    }

    // Path must be a string
    const Expr& path_expr = expr[1];
    const auto* path_str = std::get_if<std::string>(&path_expr);
    if (!path_str) {
        return std::unexpected(
            type_error("import: path must be a string"));
    }

    // Parse optional 'as prefix' clause
    std::string prefix;

    if (expr.size() >= 4) {
        // (import "path" as prefix)
        const Expr& as_kw = expr[2];
        if (const auto* as_sym = std::get_if<Symbol>(&as_kw)) {
            if (as_sym->name == "as") {
                auto prefix_opt = extract_symbol_name(expr[3]);
                if (!prefix_opt) {
                    return std::unexpected(
                        type_error("import: prefix must be a symbol"));
                }
                prefix = std::move(*prefix_opt);
            }
        }
    } else if (expr.size() == 3) {
        // (import "path" prefix) — without 'as' keyword
        auto prefix_opt = extract_symbol_name(expr[2]);
        if (prefix_opt) {
            prefix = std::move(*prefix_opt);
        }
    }

    // Default prefix from filename (strip directory and extension)
    if (prefix.empty()) {
        std::string path = *path_str;
        auto last_slash = path.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            path = path.substr(last_slash + 1);
        }
        auto dot = path.find_last_of('.');
        if (dot != std::string::npos) {
            path = path.substr(0, dot);
        }
        prefix = path;
    }

    // Determine the importing file's path for relative resolution
    std::string from_file;
    auto src_file = env->try_lookup("__source_file__");
    if (src_file && std::holds_alternative<std::string>(*src_file)) {
        from_file = std::get<std::string>(*src_file);
    }

    // Get or create the global ModuleLoader
    auto loader = get_global_loader(env);

    // Load the module
    auto mod_result = loader->load(*path_str, from_file);
    if (!mod_result) {
        return std::unexpected(mod_result.error());
    }

    // Bind the Module object to the prefix name
    env->define(prefix, Value(std::move(*mod_result)));

    return Value(nullptr);
}

// ── defmacro: (defmacro name (params) <body>...) ────────────────────────────

Result<Value> eval_defmacro(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() < 4) {
        return std::unexpected(
            arity_error(SourceLocation{}, 3,
                        static_cast<int>(expr.size()) - 1));
    }

    // Macro name
    auto name_opt = extract_symbol_name(expr[1]);
    if (!name_opt) {
        return std::unexpected(
            type_error("defmacro: name must be a symbol"));
    }

    // Parameter list
    const Expr& params_expr = expr[2];
    const auto* params_list = get_list(params_expr);
    if (!params_list) {
        return std::unexpected(
            type_error("defmacro: parameters must be a list"));
    }

    // Parse parameters, supporting rest param via '.' separator
    std::vector<std::string> params;
    std::optional<std::string> rest_param;
    bool saw_dot = false;

    for (const auto& p : *params_list) {
        auto p_name = extract_symbol_name(p);
        if (!p_name) {
            return std::unexpected(
                type_error("defmacro: parameter must be symbol"));
        }

        if (*p_name == ".") {
            saw_dot = true;
            continue;
        }

        if (saw_dot) {
            rest_param = std::move(*p_name);
        } else {
            params.push_back(std::move(*p_name));
        }
    }

    // Body starts at index 3
    std::vector<Expr> body_exprs;
    body_exprs.reserve(expr.size() - 3);
    for (size_t i = 3; i < expr.size(); ++i) {
        body_exprs.push_back(expr[i]);
    }

    auto macro = std::make_shared<Macro>(
        *name_opt, params, body_exprs, env, rest_param);

    env->define(*name_opt, Value(std::move(macro)));
    return Value(nullptr);
}

// ── macroexpand: (macroexpand <form>) — expand macros without evaluating ────

Result<Value> eval_macroexpand(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() != 2) {
        return std::unexpected(
            arity_error(SourceLocation{}, 1,
                        static_cast<int>(expr.size()) - 1));
    }

    // The argument should be a macro call expression
    const Expr& form = expr[1];

    // Try to expand it using the Expander
    // For now, just return the expression as a quoted value
    return expr_to_value(form);
}

// ── assert: (assert <expr>) — evaluate and error if falsy ──────────────────

Result<Value> eval_assert(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() != 2) {
        return std::unexpected(
            arity_error(SourceLocation{}, 1,
                        static_cast<int>(expr.size()) - 1));
    }

    auto val = evaluate(expr[1], env);
    if (!val) {
        return std::unexpected(val.error());
    }

    if (!is_truthy(*val)) {
        std::ostringstream ss;
        ss << expr[1];
        return std::unexpected(assertion_error(
            SourceLocation{},
            std::format("assertion failed: {}", ss.str())));
    }

    return Value(true);  // Return #t on success
}

// ── gensym: (gensym) or (gensym "prefix") — generate a unique symbol ───────

Result<Value> eval_gensym(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> /*env*/) {
    static std::atomic<uint64_t> s_counter{0};

    std::string prefix = "g";
    if (expr.size() >= 2) {
        if (const auto* s = std::get_if<std::string>(&expr[1])) {
            prefix = *s;
        }
    }

    uint64_t id = s_counter.fetch_add(1, std::memory_order_relaxed);
    return Value(Symbol(std::format("{}{}", prefix, id), id));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Special forms dispatch table
// ═══════════════════════════════════════════════════════════════════════════════

std::unordered_map<std::string, SpecialForm>& get_mutable_special_forms() {
    static std::unordered_map<std::string, SpecialForm> forms = {
        {"quote", eval_quote},
        {"if", eval_if},
        {"cond", eval_cond},
        {"define", eval_define},
        {"lambda", eval_lambda},
        {"let", eval_let},
        {"let*", eval_let_star},
        {"letrec", eval_letrec},
        {"begin", eval_begin},
        {"set!", eval_set},
        {"and", eval_and},
        {"or", eval_or},
        {"do", eval_do},
        {"quasiquote", eval_quasiquote},
        {"provide", eval_provide},
        {"import", eval_import},
        {"defmacro", eval_defmacro},
        {"macroexpand", eval_macroexpand},
        {"assert", eval_assert},
        {"gensym", eval_gensym},
    };
    return forms;
}

const std::unordered_map<std::string, SpecialForm>& get_special_forms() {
    return get_mutable_special_forms();
}

void register_special_form(const std::string& name, const SpecialForm& form) {
    get_mutable_special_forms()[name] = form;
}

}  // namespace pml
