// ==========================================================================================================================================================================================================================================═
// PML Evaluator — Tree-walking Interpreter with Special Forms
// ==========================================================================================================================================================================================================================================═
//
// Matches Python pml/evaluator.py (640 lines).
// Implements 21+ special forms, macro expansion, lexical scoping, and
// function application.
// ==========================================================================================================================================================================================================================================═

#include "evaluator.h"

#include "module_loader.h"

#include "pml/api/context.h"
#include "pml/core/call_stack.h"
#include "pml/evaluator/builtins_helpers.h"

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

// ==========================================================================================================================================================================================================================================═
// Internal helpers
// ==========================================================================================================================================================================================================================================═

namespace {


// ---- expr_to_value: Convert an AST Expr to a runtime Value ----------------------------------------─

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

// ---- make_body: Wrap body expression list into a single Expr ------------------------------------─

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

// ---- extract_symbol_name: Get param name from Expr for lambda/defmacro ----------------

/// Extract a symbol name from an Expr that should be a Symbol.
/// Returns nullopt if the expression is not a Symbol.
std::optional<std::string> extract_symbol_name(const Expr& expr) {
    if (const auto* sym = std::get_if<Symbol>(&expr)) {
        return sym->name;
    }
    return std::nullopt;
}

// ---- check_arity: Validate argument count ------------------------------------------------------------------------─

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

// ==========================================================================================================================================================================================================================================═
// Procedure::call — implemented here to avoid circular dependency
// ==========================================================================================================================================================================================================================================═

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
    return eval_to_value(body, call_env);
}

// ==========================================================================================================================================================================================================================================═
// Macro expansion with depth tracking
// ==========================================================================================================================================================================================================================================═

Result<EvalResult> expand_macro(
    Macro& macro, const std::vector<Expr>& args,
    std::shared_ptr<Environment> env) {

    auto& ctx = PMLContext::current();
    ctx.macro_depth += 1;
    if (ctx.macro_depth > MAX_MACRO_DEPTH) {
        ctx.macro_depth = 0;
        return std::unexpected(macro_expansion_depth_error(
            SourceLocation{}, MAX_MACRO_DEPTH));
    }

    // Expand the macro (substitute args into body)
    Expr expanded = macro.expand(args);

    // Evaluate the expanded expression to a final value.
    auto result = eval_to_value(expanded, std::move(env));

    ctx.macro_depth -= 1;
    if (!result) {
        return std::unexpected(result.error());
    }
    return *result;
}

// ==========================================================================================================================================================================================================================================═
// is_truthy
// ==========================================================================================================================================================================================================================================═

bool is_truthy(const Value& value) noexcept {
    if (value.is_nil()) return false;
    if (value.is_bool()) return value.bool_val();
    if (value.is_int()) return value.int_val() != 0;
    if (value.is_double()) return value.double_val() != 0.0;
    if (value.is_list()) {
        const auto* lst = value.as_list();
        return lst && *lst && !(*lst)->elements.empty();
    }
    return true;  // everything else is truthy
}

// ==========================================================================================================================================================================================================================================═
// trampoline
// ==========================================================================================================================================================================================================================================═

Result<Value> trampoline(Result<EvalResult> result) {
    while (result.has_value() && std::holds_alternative<TailCall>(*result)) {
        const auto& tc = std::get<TailCall>(*result);
        result = evaluate(tc.expr, tc.env);
    }
    if (!result) {
        return std::unexpected(result.error());
    }
    return std::get<Value>(*result);
}

// ==========================================================================================================================================================================================================================================═
// evaluate_arguments
// ==========================================================================================================================================================================================================================================═

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
            auto val = eval_to_value(exprs[i + 1], env);
            if (!val) {
                return std::unexpected(val.error());
            }
            result.keyword[kw->name] = std::move(*val);
            i += 2;
        } else {
            // Positional argument — evaluate normally
            auto val = eval_to_value(current, env);
            if (!val) {
                return std::unexpected(val.error());
            }
            result.positional.push_back(std::move(*val));
            i += 1;
        }
    }

    return result;
}

// ---- apply_with_param_info: ParamInfo-based argument matching (&optional/&key/&rest) ---------─

/// Apply a Procedure that has ParamInfo (&optional/&key/&rest support).
/// Default expressions are lazily evaluated at call time in the
/// incrementally-built environment, so defaults can reference preceding params.
static Result<EvalResult> apply_with_param_info(
    const Procedure& proc,
    const std::vector<Value>& args,
    const std::unordered_map<std::string, Value>& kwargs,
    Environment& env) {

    const auto& info = *proc.param_info;
    size_t arg_pos = 0;

    // Build call environment incrementally so default expressions
    // can reference earlier-bound parameters.
    auto call_env = proc.closure_env;

    // -- Local helper: evaluate a default expression to a Value --
    auto eval_default = [&](const Expr& expr) -> Result<Value> {
        auto result = evaluate(expr, call_env);
        if (!result) return std::unexpected(result.error());
        if (auto* v = std::get_if<Value>(&*result))
            return std::move(*v);
        auto tv = trampoline(std::get<TailCall>(std::move(*result)));
        if (!tv) return std::unexpected(tv.error());
        return std::move(*tv);
    };

    // 1. Required parameters
    {
        std::vector<std::string> rnames;
        std::vector<Value> rvalues;
        rnames.reserve(info.required_count);
        rvalues.reserve(info.required_count);
        for (size_t i = 0; i < info.required_count; ++i, ++arg_pos) {
            if (arg_pos >= args.size()) {
                return std::unexpected(arity_error(
                    SourceLocation{}, static_cast<int>(info.required_count),
                    static_cast<int>(args.size())));
            }
            rnames.push_back(info.names[i]);
            rvalues.push_back(args[arg_pos]);
        }
        if (!rnames.empty())
            call_env = call_env->extend(rnames, rvalues);
    }

    // 2. Optional parameters (lazy defaults evaluated at call time)
    {
        size_t def_idx = 0;
        for (size_t i = 0; i < info.optional_count; ++i) {
            size_t name_idx = info.required_count + i;
            Value val;
            if (arg_pos < args.size()) {
                val = args[arg_pos++];
            } else if (def_idx < info.default_exprs.size()) {
                auto dv = eval_default(info.default_exprs[def_idx]);
                if (!dv) return std::unexpected(dv.error());
                val = std::move(*dv);
            } else {
                val = Value(nullptr);
            }
            call_env = call_env->extend({info.names[name_idx]}, {val});
            def_idx++;
        }
    }

    // 3. Keyword parameters (lazy defaults evaluated at call time)
    {
        size_t key_start = info.required_count + info.optional_count;
        size_t def_idx = info.optional_count;
        for (size_t i = key_start; i < info.names.size(); ++i) {
            if (info.has_rest && i == info.rest_index) continue;
            const std::string& key_name = info.names[i];
            Value val;
            auto it = kwargs.find(key_name);
            if (it != kwargs.end()) {
                val = it->second;
            } else if (def_idx < info.default_exprs.size()) {
                auto dv = eval_default(info.default_exprs[def_idx]);
                if (!dv) return std::unexpected(dv.error());
                val = std::move(*dv);
            } else {
                val = Value(nullptr);
            }
            call_env = call_env->extend({key_name}, {val});
            def_idx++;
        }
    }

    // 4. &rest parameter
    if (info.has_rest) {
        std::vector<Value> rest_values;
        rest_values.reserve(args.size() - arg_pos);
        while (arg_pos < args.size()) {
            rest_values.push_back(args[arg_pos++]);
        }
        call_env = call_env->extend({info.names[info.rest_index]},
                                     {make_list_value(std::move(rest_values))});
    } else if (arg_pos < args.size()) {
        return std::unexpected(arity_error(
            SourceLocation{}, static_cast<int>(info.required_count + info.optional_count),
            static_cast<int>(args.size())));
    }

    return TailCall{proc.body, call_env};
}

// ==========================================================================================================================================================================================================================================═
// apply_function
// ==========================================================================================================================================================================================================================================═

Result<EvalResult> apply_function(
    const Value& func,
    const std::vector<Value>& args,
    const std::unordered_map<std::string, Value>& kwargs,
    std::shared_ptr<Environment> env,
    SourceLocation call_site) {

    // ---- User-defined Procedure ----------------------------------------------------------------─
    if (const auto* proc_ptr = func.as_procedure()) {
        const auto& arg = *proc_ptr;
        if (!arg) {
            return std::unexpected(
                type_error("Cannot apply null procedure"));
        }

        // ParamInfo-based matching (&optional/&key/&rest)
        if (arg->param_info.has_value()) {
            return apply_with_param_info(*arg, args, kwargs, *env);
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

            // Return a tail call so the trampoline can evaluate the body
            // without growing the C++ stack.
            return TailCall{arg->body, call_env};

        } else {
            // Simple parameter matching
            if (expected != got) {
                return std::unexpected(arity_error(
                    SourceLocation{},
                    static_cast<int>(expected),
                    static_cast<int>(got)));
            }

            auto call_env = arg->closure_env->extend(params, args);
            // Return a tail call so the trampoline can evaluate the body
            // without growing the C++ stack.
            return TailCall{arg->body, call_env};
        }
    }

    // ---- BuiltinProcedure ----------------------------------------------------------------------------
    if (const auto* builtin_ptr = func.as_builtin()) {
        const auto& arg = *builtin_ptr;
        if (!arg) {
            return std::unexpected(
                type_error("Cannot apply null builtin procedure"));
        }
        // Push call frame for error reporting.
        auto guard = CallStack::instance().push_guard(arg->name, call_site);
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
            auto result = arg->call(merged, *env);
            if (!result) return std::unexpected(result.error());
            return *result;
        }
        auto result = arg->call(args, *env);
        if (!result) return std::unexpected(result.error());
        return *result;
    }

    // ---- Anything else is not callable ------------------------------------------------─
    return std::unexpected(
        type_error(std::format("Not a procedure: {}",
                               value_to_string(func))));
}

// ==========================================================================================================================================================================================================================================═
// evaluate — main dispatch
// ==========================================================================================================================================================================================================================================═

Result<EvalResult> evaluate(
    const Expr& expr, std::shared_ptr<Environment> env) {

    return std::visit(
        [&](const auto& arg) -> Result<EvalResult> {
            using T = std::decay_t<decltype(arg)>;

            // ---- Self-evaluating atoms --------------------------------------------------------------------
            // nil, int, float, string, bool: return as-is
            if constexpr (std::is_same_v<T, std::nullptr_t> ||
                          std::is_same_v<T, int64_t> ||
                          std::is_same_v<T, double> ||
                          std::is_same_v<T, std::string> ||
                          std::is_same_v<T, bool>) {
                return Value(arg);
            }

            // ---- Symbol lookup ------------------------------------------------------------------------------------
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
                        if (const auto* mod = prefix_val->as_module()) {
                            if (*mod) {
                                auto member_result = (*mod)->get(member);
                                if (!member_result) {
                                    return std::unexpected(member_result.error());
                                }
                                return *member_result;
                            }
                        }
                    }
                }

                return env->lookup(name);
            }

            // ---- Keyword — return as-is (used as parameter markers) --------
            if constexpr (std::is_same_v<T, Keyword>) {
                return Value(arg);
            }

            // ---- List — special form, macro, or function call --------------------─
            if constexpr (std::is_same_v<T, std::shared_ptr<ListExpr>>) {
                const auto& elements = arg->elements;

                if (elements.empty()) {
                    return Value(make_list_value({}));  // empty list
                }

                const Expr& head = elements[0];

                // ---- Special forms ----------------------------------------------------------------------------
                if (const auto* head_sym = std::get_if<Symbol>(&head)) {
                    const auto& forms = get_special_forms();
                    auto it = forms.find(head_sym->name);
                    if (it != forms.end()) {
                        return it->second(arg->elements, env, arg->location);
                    }

                    // ---- Macro expansion (by name) --------------------------------------------
                    auto macro_val = env->try_lookup(head_sym->name);
                    if (macro_val.has_value()) {
                        if (const auto* mac = macro_val->as_macro()) {
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

                // ---- Regular function call ------------------------------------------------------------
                auto func_val = eval_to_value(head, env);
                if (!func_val) {
                    return std::unexpected(func_val.error());
                }

                // Check if the evaluated function is a Macro
                // (for higher-order macro use / immediate macro values)
                if (const auto* mac = func_val->as_macro()) {
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

                // apply_function returns an EvalResult (either a final value
                // or a TailCall).  Because this is the final action of the
                // function-call evaluation, we return it directly.
                return apply_function(
                    *func_val,
                    eval_args->positional,
                    eval_args->keyword,
                    env,
                    arg->location);
            }

            // ---- Unreachable (exhaustive variant) --------------------------------------------
            return std::unexpected(
                type_error("Cannot evaluate: unknown expression type"));
        },
        expr);
}

// ==========================================================================================================================================================================================================================================═
// Quasiquote expansion
// ==========================================================================================================================================================================================================================================═

Result<Value> expand_quasiquote(
    const Expr& tmpl, std::shared_ptr<Environment> env, int depth) {

    const auto* list_ptr = std::get_if<std::shared_ptr<ListExpr>>(&tmpl);
    if (!list_ptr || !*list_ptr) {
        // Plain atom — convert directly to a runtime value.
        return expr_to_value(tmpl);
    }

    const auto& elements = (*list_ptr)->elements;

    // Check for (unquote x), (unquote-splicing x), or (quasiquote x) at top level
    if (elements.size() == 2) {
        if (const auto* head_sym = std::get_if<Symbol>(&elements[0])) {
            if (head_sym->name == "unquote") {
                if (depth == 0) {
                    auto val = eval_to_value(elements[1], env);
                    if (!val) return std::unexpected(val.error());
                    return *val;
                }
                // Escape one level: preserve (unquote <expanded>).
                auto expanded = expand_quasiquote(elements[1], env, depth - 1);
                if (!expanded) return std::unexpected(expanded.error());
                return make_list_value({Value(Symbol("unquote")), std::move(*expanded)});
            }
            if (head_sym->name == "unquote-splicing") {
                if (depth == 0) {
                    return std::unexpected(
                        type_error("unquote-splicing used outside of list context"));
                }
                // Escape one level: preserve (unquote-splicing <expanded>).
                auto expanded = expand_quasiquote(elements[1], env, depth - 1);
                if (!expanded) return std::unexpected(expanded.error());
                return make_list_value({Value(Symbol("unquote-splicing")), std::move(*expanded)});
            }
            if (head_sym->name == "quasiquote") {
                // Increase depth and preserve (quasiquote <expanded>).
                auto expanded = expand_quasiquote(elements[1], env, depth + 1);
                if (!expanded) return std::unexpected(expanded.error());
                return make_list_value({Value(Symbol("quasiquote")), std::move(*expanded)});
            }
        }
    }

    // Recurse into list elements, handling unquote-splicing
    std::vector<Value> new_elements;
    new_elements.reserve(elements.size());

    for (const auto& item : elements) {
        // Check for (unquote-splicing x) at element level
        if (const auto* item_list = std::get_if<std::shared_ptr<ListExpr>>(&item)) {
            const auto& sub = (*item_list)->elements;
            if (sub.size() == 2) {
                if (const auto* sub_sym = std::get_if<Symbol>(&sub[0])) {
                    if (sub_sym->name == "unquote-splicing") {
                        if (depth == 0) {
                            auto val = eval_to_value(sub[1], env);
                            if (!val) return std::unexpected(val.error());
                            const auto* vl = val->as_list();
                            if (!vl || !*vl) {
                                return std::unexpected(
                                    type_error("unquote-splicing: value must be a list"));
                            }
                            for (const auto& v : (*vl)->elements) {
                                new_elements.push_back(v);
                            }
                            continue;
                        }
                        // Escape one level: preserve (unquote-splicing <expanded>).
                        auto expanded = expand_quasiquote(sub[1], env, depth - 1);
                        if (!expanded) return std::unexpected(expanded.error());
                        new_elements.push_back(make_list_value(
                            {Value(Symbol("unquote-splicing")), std::move(*expanded)}));
                        continue;
                    }
                    if (sub_sym->name == "quasiquote") {
                        // Increase depth and preserve nested quasiquote.
                        auto expanded = expand_quasiquote(item, env, depth + 1);
                        if (!expanded) return std::unexpected(expanded.error());
                        new_elements.push_back(std::move(*expanded));
                        continue;
                    }
                }
            }
        }

        // Regular element — recursively quasiquote at the same depth
        auto expanded = expand_quasiquote(item, env, depth);
        if (!expanded) return std::unexpected(expanded.error());
        new_elements.push_back(std::move(*expanded));
    }

    return make_list_value(std::move(new_elements));
}

// ==========================================================================================================================================================================================================================================═
// Special forms
// ==========================================================================================================================================================================================================================================═

// ---- quote: (quote <expr>) → return expr unevaluated ------------------------------------------------─

Result<EvalResult> eval_quote(
    const ArenaExprVector& expr, std::shared_ptr<Environment> /*env*/,
    SourceLocation call_site) {
    if (expr.size() != 2) {
        return std::unexpected(
            arity_error(call_site, 1,
                        static_cast<int>(expr.size()) - 1));
    }
    // Convert AST expression to runtime value (without evaluating it)
    return expr_to_value(expr[1]);
}

// ---- if: (if <cond> <then> [<else>]) --------------------------------------------------------------------------------─

Result<EvalResult> eval_if(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation /*call_site*/) {
    if (expr.size() < 3 || expr.size() > 4) {
        return std::unexpected(general_error(
            "if expects 2 or 3 arguments"));
    }

    auto cond = eval_to_value(expr[1], env);
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

// ---- cond: (cond (<test> <expr>...) ... (else <default>...)) --------------------------------─

Result<EvalResult> eval_cond(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation /*call_site*/) {

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
            for (size_t j = 1; j < clause_list->size(); ++j) {
                if (j + 1 == clause_list->size()) {
                    return evaluate((*clause_list)[j], env);
                }
                auto val = eval_to_value((*clause_list)[j], env);
                if (!val) return std::unexpected(val.error());
            }
            return Value(nullptr);
        }

        // Evaluate test
        auto test_val = eval_to_value(test, env);
        if (!test_val) return std::unexpected(test_val.error());

        if (is_truthy(*test_val)) {
            // Evaluate all expressions in this clause, return last
            for (size_t j = 1; j < clause_list->size(); ++j) {
                if (j + 1 == clause_list->size()) {
                    return evaluate((*clause_list)[j], env);
                }
                auto val = eval_to_value((*clause_list)[j], env);
                if (!val) return std::unexpected(val.error());
            }
            return Value(nullptr);
        }
    }

    return Value(nullptr);  // no clause matched → nil
}

// ---- when: (when <condition> <body>...) ------------------------------------------------------------------------─

Result<EvalResult> eval_when(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation /*call_site*/) {
    if (expr.size() < 3) {
        return std::unexpected(general_error(
            "when expects at least 2 arguments (condition and body)"));
    }

    auto cond = eval_to_value(expr[1], env);
    if (!cond) {
        return std::unexpected(cond.error());
    }

    if (is_truthy(*cond)) {
        // Evaluate all body expressions, return last
        for (size_t i = 2; i < expr.size(); ++i) {
            if (i + 1 == expr.size()) {
                // Last expression — return as EvalResult (may be TailCall)
                return evaluate(expr[i], env);
            }
            auto val = eval_to_value(expr[i], env);
            if (!val) return std::unexpected(val.error());
        }
    }

    return Value(nullptr);  // condition false → nil
}

// ---- unless: (unless <condition> <body>...) ------------------------------------------------------------─

Result<EvalResult> eval_unless(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation /*call_site*/) {
    if (expr.size() < 3) {
        return std::unexpected(general_error(
            "unless expects at least 2 arguments (condition and body)"));
    }

    auto cond = eval_to_value(expr[1], env);
    if (!cond) {
        return std::unexpected(cond.error());
    }

    if (!is_truthy(*cond)) {
        // Evaluate all body expressions, return last
        for (size_t i = 2; i < expr.size(); ++i) {
            if (i + 1 == expr.size()) {
                // Last expression — return as EvalResult (may be TailCall)
                return evaluate(expr[i], env);
            }
            auto val = eval_to_value(expr[i], env);
            if (!val) return std::unexpected(val.error());
        }
    }

    return Value(nullptr);  // condition true → nil
}

// ---- dotimes: (dotimes (var count) body...) ---------------------------------------

Result<EvalResult> eval_dotimes(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation /*call_site*/) {
    if (expr.size() < 3) {
        return std::unexpected(general_error(
            "dotimes expects at least 2 arguments (binding spec and body)"));
    }

    const auto* bindings = get_list(expr[1]);
    if (!bindings || bindings->size() != 2) {
        return std::unexpected(type_error(
            "dotimes: first argument must be a list of (var count)"));
    }

    auto var_name = extract_symbol_name((*bindings)[0]);
    if (!var_name) {
        return std::unexpected(type_error(
            "dotimes: first element of binding spec must be a symbol"));
    }

    auto count_val = eval_to_value((*bindings)[1], env);
    if (!count_val) return std::unexpected(count_val.error());

    int64_t count = 0;
    if (count_val->is_int()) {
        count = count_val->int_val();
    } else if (count_val->is_double()) {
        count = static_cast<int64_t>(count_val->double_val());
    } else {
        return std::unexpected(type_error(
            "dotimes: count must be a number"));
    }

    for (int64_t i = 0; i < count; ++i) {
        auto iter_env = std::make_shared<Environment>(env);
        iter_env->define(*var_name, Value(i));

        for (size_t j = 2; j < expr.size(); ++j) {
            auto val = eval_to_value(expr[j], iter_env);
            if (!val) return std::unexpected(val.error());
        }
    }

    return Value(nullptr);
}

// ---- case: (case <key> (<value> <expr>...) ... (else <expr>...)) ----------------─

Result<EvalResult> eval_case(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation /*call_site*/) {
    if (expr.size() < 3) {
        return std::unexpected(general_error(
            "case expects at least 2 arguments (key and clauses)"));
    }

    // Evaluate the key expression
    auto key_val = eval_to_value(expr[1], env);
    if (!key_val) {
        return std::unexpected(key_val.error());
    }

    // Iterate over clauses (expr[2:])
    for (size_t i = 2; i < expr.size(); ++i) {
        const Expr& clause = expr[i];

        // Clause must be a list: (<value(s)> <expr>...)
        const auto* clause_list = get_list(clause);
        if (!clause_list || clause_list->empty()) {
            return std::unexpected(
                type_error("case clause must be a non-empty list"));
        }

        const Expr& first = (*clause_list)[0];

        // Check for else clause
        bool is_else = false;
        if (const auto* first_sym = std::get_if<Symbol>(&first)) {
            if (first_sym->name == "else") {
                is_else = true;
            }
        }

        if (is_else) {
            // Evaluate all expressions in the else clause, return last
            for (size_t j = 1; j < clause_list->size(); ++j) {
                if (j + 1 == clause_list->size()) {
                    return evaluate((*clause_list)[j], env);
                }
                auto val = eval_to_value((*clause_list)[j], env);
                if (!val) return std::unexpected(val.error());
            }
            return Value(nullptr);
        }

        // Check if key matches any value in the clause
        bool matched = false;

        // If first element is a list, check each element
        if (const auto* values_list = get_list(first)) {
            for (const auto& val_expr : *values_list) {
                auto val = eval_to_value(val_expr, env);
                if (!val) return std::unexpected(val.error());
                if (deep_equal(*key_val, *val)) {
                    matched = true;
                    break;
                }
            }
        } else {
            // Single value — evaluate it and compare
            auto val = eval_to_value(first, env);
            if (!val) return std::unexpected(val.error());
            if (deep_equal(*key_val, *val)) {
                matched = true;
            }
        }

        if (matched) {
            // Evaluate all expressions in this clause, return last
            for (size_t j = 1; j < clause_list->size(); ++j) {
                if (j + 1 == clause_list->size()) {
                    return evaluate((*clause_list)[j], env);
                }
                auto val = eval_to_value((*clause_list)[j], env);
                if (!val) return std::unexpected(val.error());
            }
            return Value(nullptr);
        }
    }

    return Value(nullptr);  // no clause matched → nil
}

// ---- define: (define <name> <expr>) or (define (<name> <params>) <body>...) ----

struct ParseParamsResult {
    std::vector<std::string> params;
    std::string dot_rest_name;
    bool saw_dot = false;
    bool saw_ampersand = false;
    ParamInfo pinfo;
};

/// Parse a list of parameter expressions (from lambda or define shorthand).
/// Supports simple symbols, '. rest' syntax, and &optional/&key/&rest.
static Result<ParseParamsResult> parse_lambda_params(
    const std::vector<Expr>& param_exprs) {

    std::vector<std::string> params;
    bool saw_dot = false;
    std::string dot_rest_name;
    bool saw_ampersand = false;

    enum class ParamSection { Required, Optional, Key, Rest };
    ParamSection section = ParamSection::Required;

    ParamInfo pinfo;

    for (const auto& p : param_exprs) {
        auto name_opt = extract_symbol_name(p);
        if (name_opt) {
            const std::string& n = *name_opt;
            if (n == ".") { saw_dot = true; continue; }
            if (n == "&optional") {
                if (section == ParamSection::Optional || section == ParamSection::Key || section == ParamSection::Rest)
                    return std::unexpected(type_error("&optional: duplicate or misplaced"));
                section = ParamSection::Optional;
                saw_ampersand = true;
                continue;
            }
            if (n == "&key") {
                if (section == ParamSection::Key || section == ParamSection::Rest)
                    return std::unexpected(type_error("&key: duplicate or misplaced"));
                section = ParamSection::Key;
                pinfo.has_keys = true;
                saw_ampersand = true;
                continue;
            }
            if (n == "&rest") {
                if (section == ParamSection::Rest)
                    return std::unexpected(type_error("&rest: duplicate"));
                section = ParamSection::Rest;
                saw_ampersand = true;
                continue;
            }
            if (saw_dot) {
                dot_rest_name = n;
                continue;
            }
            if (section == ParamSection::Rest) {
                pinfo.has_rest = true;
                pinfo.rest_index = params.size();
                params.push_back(n);
                pinfo.names.push_back(n);
                continue;
            }
            // Simple symbol param
            params.push_back(n);
            pinfo.names.push_back(n);
            if (section == ParamSection::Optional) {
                pinfo.optional_count++;
            } else if (section == ParamSection::Key) {
                pinfo.key_count++;
            }
            continue;
        }

        // (name default) form for &optional or &key
        const auto* sub = get_list(p);
        if (sub && sub->size() >= 2) {
            auto sub_name = extract_symbol_name((*sub)[0]);
            if (!sub_name) {
                return std::unexpected(type_error("expected symbol as parameter name"));
            }
            if (section != ParamSection::Optional && section != ParamSection::Key) {
                return std::unexpected(type_error(
                    "parameter list form only allowed after &optional or &key"));
            }
            // Reject (x default extra) — only 2 elements allowed
            if (sub->size() > 2) {
                return std::unexpected(type_error(
                    "invalid parameter specification"));
            }

            params.push_back(*sub_name);
            pinfo.names.push_back(*sub_name);

            // Store default expression for lazy evaluation at call time
            pinfo.default_exprs.push_back((*sub)[1]);

            if (section == ParamSection::Optional) {
                pinfo.optional_count++;
            }
            if (section == ParamSection::Key) {
                pinfo.key_count++;
            }
            continue;
        }

        return std::unexpected(type_error("parameter must be symbol"));
    }

    // Reject mixing old-style ". rest" with &optional/&key/&rest
    if (saw_dot && saw_ampersand) {
        return std::unexpected(type_error(
            "cannot mix '.' rest syntax with &optional/&key/&rest"));
    }

    // For traditional ". rest" syntax, append "." and rest param name
    if (saw_dot && !dot_rest_name.empty()) {
        params.push_back(".");
        params.push_back(dot_rest_name);
    }

    if (saw_ampersand) {
        pinfo.required_count = pinfo.names.size() - pinfo.optional_count
                             - pinfo.key_count
                             - (pinfo.has_rest ? 1 : 0);
    }

    return ParseParamsResult{
        std::move(params),
        std::move(dot_rest_name),
        saw_dot,
        saw_ampersand,
        std::move(pinfo)
    };
}

Result<EvalResult> eval_define(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation call_site) {
    if (expr.size() < 3) {
        return std::unexpected(
            arity_error(call_site, 2,
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

        // Remaining elements are parameter names (delegate to shared parser)
        std::vector<Expr> param_exprs;
        for (size_t j = 1; j < target_list->size(); ++j) {
            param_exprs.push_back((*target_list)[j]);
        }
        auto parsed = parse_lambda_params(param_exprs);
        if (!parsed) return std::unexpected(parsed.error());

        // Body starts at index 2
        std::vector<Expr> body_exprs;
        body_exprs.reserve(expr.size() - 2);
        for (size_t j = 2; j < expr.size(); ++j) {
            body_exprs.push_back(expr[j]);
        }

        std::optional<ParamInfo> param_info_opt;
        if (parsed->saw_ampersand) {
            param_info_opt = std::move(parsed->pinfo);
        }

        auto proc = std::make_shared<Procedure>(
            parsed->params,
            make_body_from_vector(body_exprs),
            env,
            *name_opt,
            std::move(param_info_opt));
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

    auto value = eval_to_value(expr[2], env);
    if (!value) {
        return std::unexpected(value.error());
    }

    env->define(*name_opt, *value);
    return Value(nullptr);
}

// ---- lambda: (lambda (<params>) <body>...) --------------------------------------------------------------------─

Result<EvalResult> eval_lambda(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation call_site) {
    if (expr.size() < 3) {
        return std::unexpected(
            arity_error(call_site, 2,
                        static_cast<int>(expr.size()) - 1));
    }

    const Expr& params_expr = expr[1];
    const auto* params_list = get_list(params_expr);
    if (!params_list) {
        return std::unexpected(
            type_error("lambda: parameters must be a list"));
    }

    // Parse parameters using shared helper
    std::vector<Expr> param_exprs(params_list->begin(), params_list->end());
    auto parsed = parse_lambda_params(param_exprs);
    if (!parsed) return std::unexpected(parsed.error());

    std::optional<ParamInfo> param_info_opt;
    if (parsed->saw_ampersand) {
        param_info_opt = std::move(parsed->pinfo);
    }

    // Body expressions start at index 2
    std::vector<Expr> body_exprs;
    body_exprs.reserve(expr.size() - 2);
    for (size_t i = 2; i < expr.size(); ++i) {
        body_exprs.push_back(expr[i]);
    }

    auto proc = std::make_shared<Procedure>(
        parsed->params,
        make_body_from_vector(body_exprs),
        env,
        std::nullopt,          // name
        std::move(param_info_opt));

    return Value(std::move(proc));
}

// ---- let: (let ((name expr) ...) <body>...) — parallel bindings ------------------------─

Result<EvalResult> eval_let_par(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation call_site) {
    if (expr.size() < 3) {
        return std::unexpected(
            arity_error(call_site, 2,
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
        auto val = eval_to_value((*binding_list)[1], env);
        if (!val) return std::unexpected(val.error());
        values.push_back(std::move(*val));
    }

    // Create child environment and evaluate body
    auto child_env = env->extend(names, values);

    for (size_t i = 2; i < expr.size(); ++i) {
        if (i + 1 == expr.size()) {
            return evaluate(expr[i], child_env);
        }
        auto val = eval_to_value(expr[i], child_env);
        if (!val) return std::unexpected(val.error());
    }
    return Value(nullptr);
}

// ---- let*: (let* ((name expr) ...) <body>...) — sequential bindings ----------------─

Result<EvalResult> eval_let_star(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation call_site) {
    if (expr.size() < 3) {
        return std::unexpected(
            arity_error(call_site, 2,
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
        auto val = eval_to_value((*binding_list)[1], child_env);
        if (!val) return std::unexpected(val.error());

        child_env->define(*name_opt, *val);
    }

    for (size_t i = 2; i < expr.size(); ++i) {
        if (i + 1 == expr.size()) {
            return evaluate(expr[i], child_env);
        }
        auto val = eval_to_value(expr[i], child_env);
        if (!val) return std::unexpected(val.error());
    }
    return Value(nullptr);
}

// ---- letrec: (letrec ((name expr) ...) <body>...) — recursive bindings ------------

Result<EvalResult> eval_letrec(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation call_site) {
    if (expr.size() < 3) {
        return std::unexpected(
            arity_error(call_site, 2,
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
        auto val = eval_to_value((*binding_list)[1], child_env);
        if (!val) return std::unexpected(val.error());
        auto set_r = child_env->set(names[bi], *val);
        if (!set_r) return std::unexpected(set_r.error());
    }

    // Evaluate body
    for (size_t i = 2; i < expr.size(); ++i) {
        if (i + 1 == expr.size()) {
            return evaluate(expr[i], child_env);
        }
        auto val = eval_to_value(expr[i], child_env);
        if (!val) return std::unexpected(val.error());
    }
    return Value(nullptr);
}

// ---- begin: (begin <expr> ...) — evaluate sequentially, return last ----------------─

Result<EvalResult> eval_begin(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation /*call_site*/) {
    for (size_t i = 1; i < expr.size(); ++i) {
        if (i + 1 == expr.size()) {
            return evaluate(expr[i], env);
        }
        auto val = eval_to_value(expr[i], env);
        if (!val) return std::unexpected(val.error());
    }
    return Value(nullptr);
}

// ---- set!: (set! <name> <expr>) --------------------------------------------------------------------------------------------

Result<EvalResult> eval_set(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation call_site) {
    if (expr.size() != 3) {
        return std::unexpected(
            arity_error(call_site, 2,
                        static_cast<int>(expr.size()) - 1));
    }

    auto name_opt = extract_symbol_name(expr[1]);
    if (!name_opt) {
        return std::unexpected(
            type_error("set!: expected symbol"));
    }

    auto value = eval_to_value(expr[2], env);
    if (!value) {
        return std::unexpected(value.error());
    }

    auto set_result = env->set(*name_opt, *value);
    if (!set_result) {
        return std::unexpected(set_result.error());
    }

    return Value(nullptr);
}

// ---- and: (and <expr> ...) — short-circuit, return last truthy or first falsy ─

Result<EvalResult> eval_and(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation /*call_site*/) {
    Value result(true);  // Start with true (identity for and)

    for (size_t i = 1; i < expr.size(); ++i) {
        auto val = eval_to_value(expr[i], env);
        if (!val) return std::unexpected(val.error());

        if (!is_truthy(*val)) {
            return *val;  // Return first falsy value (short-circuit)
        }
        result = std::move(*val);
    }

    return result;  // Return last value (could be #t)
}

// ---- or: (or <expr> ...) — short-circuit, return first truthy --------------------------------

Result<EvalResult> eval_or(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation /*call_site*/) {
    for (size_t i = 1; i < expr.size(); ++i) {
        auto val = eval_to_value(expr[i], env);
        if (!val) return std::unexpected(val.error());

        if (is_truthy(*val)) {
            return *val;  // Return first truthy value (short-circuit)
        }
    }

    return Value(false);  // No truthy value found → #f
}

// ---- do: (do ((var init step) ...) (test result...) <body>...) ----------------------------

Result<EvalResult> eval_do(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation call_site) {
    if (expr.size() < 3) {
        return std::unexpected(
            arity_error(call_site, 2,
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
        auto init_val = eval_to_value(inits[i], env);
        if (!init_val) return std::unexpected(init_val.error());
        loop_env->define(names[i], *init_val);
    }

    // Loop
    while (true) {
        // Evaluate test
        auto test_val = eval_to_value((*test_clause)[0], loop_env);
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
            auto body_val = eval_to_value(expr[i], loop_env);
            if (!body_val) return std::unexpected(body_val.error());
        }

        // Update variables (evaluate steps in current env, then assign)
        std::vector<Value> new_values;
        new_values.reserve(steps.size());
        for (const auto& step : steps) {
            auto step_val = eval_to_value(step, loop_env);
            if (!step_val) return std::unexpected(step_val.error());
            new_values.push_back(std::move(*step_val));
        }

        for (size_t i = 0; i < names.size(); ++i) {
            auto set_r = loop_env->set(names[i], new_values[i]);
            if (!set_r) return std::unexpected(set_r.error());
        }
    }
}

// ---- quasiquote: (quasiquote <expr>) — template with unquote interpolation ─

Result<EvalResult> eval_quasiquote(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation call_site) {
    if (expr.size() != 2) {
        return std::unexpected(
            arity_error(call_site, 1,
                        static_cast<int>(expr.size()) - 1));
    }

    // Expand quasiquote directly into a runtime value.
    auto result = expand_quasiquote(expr[1], env);
    if (!result) return std::unexpected(result.error());
    return *result;
}

// ---- provide: (provide sym1 sym2 ...) — declare module exports ----------------------------

Result<EvalResult> eval_provide(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation /*call_site*/) {
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

// ---- import: (import "path.pml" [as prefix]) — load a module --------------------------------

Result<EvalResult> eval_import(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation call_site) {
    if (expr.size() < 2) {
        return std::unexpected(
            arity_error(call_site, 1,
                        static_cast<int>(expr.size()) - 1));
    }

    // Path must be a string
    const Expr& path_expr = expr[1];
    const auto* path_str = std::get_if<std::string>(&path_expr);
    if (!path_str) {
        return std::unexpected(
            type_error("import: path must be a string"));
    }

    // Parse optional prefix clause:
    //   (import "path" prefix)
    //   (import "path" as prefix)
    //   (import "path" :prefix prefix)
    std::string prefix;

    if (expr.size() >= 4) {
        const Expr& kw = expr[2];
        // (import "path" as prefix) or (import "path" :prefix prefix)
        if (const auto* as_sym = std::get_if<Symbol>(&kw)) {
            if (as_sym->name == "as") {
                auto prefix_opt = extract_symbol_name(expr[3]);
                if (!prefix_opt) {
                    return std::unexpected(
                        type_error("import: prefix must be a symbol"));
                }
                prefix = std::move(*prefix_opt);
            }
        } else if (const auto* kw_sym = std::get_if<Keyword>(&kw)) {
            if (kw_sym->name == "prefix") {
                auto prefix_opt = extract_symbol_name(expr[3]);
                if (!prefix_opt) {
                    return std::unexpected(
                        type_error("import: prefix must be a symbol"));
                }
                prefix = std::move(*prefix_opt);
            }
        }
    } else if (expr.size() == 3) {
        // (import "path" prefix) — without any keyword
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
    if (src_file && src_file->is_string()) {
        from_file = *src_file->as_string();
    }

    // Get a ModuleLoader — prefer the per-runtime instance from PMLContext,
    // fall back to the global accessor for backward compatibility.
    std::shared_ptr<ModuleLoader> loader;
    auto* ctx = PMLContext::current_ptr();
    if (ctx && ctx->module_loader) {
        loader = ctx->module_loader;
    } else {
        loader = get_global_loader(env);
    }

    // Load the module
    auto mod_result = loader->load(*path_str, from_file);
    if (!mod_result) {
        auto err = mod_result.error();
        if (call_site.line > 0) {
            err.call_stack.insert(err.call_stack.begin(),
                CallFrame{std::format("import \"{}\"", *path_str), call_site});
        }
        return std::unexpected(std::move(err));
    }

    // Bind the Module object to the prefix name
    env->define(prefix, Value(std::move(*mod_result)));

    return Value(nullptr);
}

// ---- defmacro: (defmacro name (params) <body>...) --------------------------------------------------------

Result<EvalResult> eval_defmacro(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation call_site) {
    if (expr.size() < 4) {
        return std::unexpected(
            arity_error(call_site, 3,
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

    // Body starts at index 3.  Clone to the global heap so the macro
    // survives resets of the per-evaluation arena.
    std::vector<Expr> body_exprs;
    body_exprs.reserve(expr.size() - 3);
    for (size_t i = 3; i < expr.size(); ++i) {
        body_exprs.push_back(clone_expr_to_heap(expr[i]));
    }

    auto macro = std::make_shared<Macro>(
        *name_opt, params, std::move(body_exprs), env, rest_param);

    env->define(*name_opt, Value(std::move(macro)));
    return Value(nullptr);
}

// ---- macroexpand: (macroexpand <form>) — expand macros without evaluating --------

Result<EvalResult> eval_macroexpand(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation call_site) {
    if (expr.size() != 2) {
        return std::unexpected(
            arity_error(call_site, 1,
                        static_cast<int>(expr.size()) - 1));
    }

    // The argument should be a macro call expression
    const Expr& form = expr[1];

    // Try to expand it using the Expander
    // For now, just return the expression as a quoted value
    return expr_to_value(form);
}

// ---- assert: (assert <expr>) — evaluate and error if falsy ------------------------------------

Result<EvalResult> eval_assert(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation call_site) {
    if (expr.size() != 2) {
        return std::unexpected(
            arity_error(call_site, 1,
                        static_cast<int>(expr.size()) - 1));
    }

    auto val = eval_to_value(expr[1], env);
    if (!val) {
        return std::unexpected(val.error());
    }

    if (!is_truthy(*val)) {
        std::ostringstream ss;
        ss << expr[1];
        return std::unexpected(assertion_error(
            call_site,
            std::format("assertion failed: {}", ss.str())));
    }

    return Value(true);  // Return #t on success
}

// ---- gensym: (gensym) or (gensym "prefix") — generate a unique symbol ------------

Result<EvalResult> eval_gensym(
    const ArenaExprVector& expr, std::shared_ptr<Environment> /*env*/,
    SourceLocation /*call_site*/) {
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

// ---- with-exception-handler: (with-exception-handler <handler> <thunk>) ------------
//
// Evaluates <thunk>; if it raises an error, calls <handler> with the error
// converted to a Value (list: (error <ErrorType> <message>)) and returns the
// handler's result.  If <thunk> succeeds, returns its value directly.

Result<EvalResult> eval_with_exception_handler(
    const ArenaExprVector& expr, std::shared_ptr<Environment> env,
    SourceLocation call_site) {
    if (expr.size() != 3) {
        return std::unexpected(
            arity_error(call_site, 2,
                        static_cast<int>(expr.size()) - 1));
    }

    // Evaluate the handler so we can call it on error.
    auto handler_val = eval_to_value(expr[1], env);
    if (!handler_val) return std::unexpected(handler_val.error());

    // Evaluate the thunk to get a procedure, then call it with no args.
    auto thunk_val = eval_to_value(expr[2], env);
    if (!thunk_val) return std::unexpected(thunk_val.error());

    // Fully resolve the thunk call via trampoline so that any error from
    // the thunk's body (e.g. (error "boom")) is caught here, not deferred
    // as a TailCall.
    auto body_result = trampoline(apply_function(*thunk_val, {}, {}, env));
    if (body_result) return Value(*body_result);

    // Error occurred — convert PMLException to a Value.
    const auto& err = body_result.error();
    std::string type_name(to_string(err.code));
    Value error_value = make_list_value({
        Value(Symbol("error")),
        Value(Symbol(type_name)),
        Value(err.message),
    });

    // Fully resolve the handler call too.
    auto handler_result = trampoline(
        apply_function(*handler_val, {error_value}, {}, env));
    if (handler_result) return Value(*handler_result);
    return std::unexpected(handler_result.error());
}

// ==========================================================================================================================================================================================================================================═
// Special forms dispatch table
// ==========================================================================================================================================================================================================================================═

std::unordered_map<std::string, SpecialForm>& get_mutable_special_forms() {
    static std::unordered_map<std::string, SpecialForm> forms = {
        {"quote", eval_quote},
        {"if", eval_if},
        {"cond", eval_cond},
        {"define", eval_define},
        {"lambda", eval_lambda},
        {"let", eval_let_star}, // sequential (was parallel before Wave 3)
        {"let*", eval_let_star}, // sequential (alias for let)
        {"let-par", eval_let_par}, // parallel (was let before Wave 3)
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
        {"when", eval_when},
        {"unless", eval_unless},
        {"dotimes", eval_dotimes},
        {"case", eval_case},
        {"with-exception-handler", eval_with_exception_handler},
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
