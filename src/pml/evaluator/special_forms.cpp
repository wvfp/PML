// ==========================================================================================================================================================================================================================================═
// PML Evaluator — Special Forms Implementation
// ==========================================================================================================================================================================================================================================═
//
// All evaluation special forms extracted from evaluator.cpp for maintainability.
// ==========================================================================================================================================================================================================================================═

#include "evaluator.h"

#include "pml/evaluator/builtins_helpers.h"

#include <atomic>
#include <format>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// Internal helpers
// ==========================================================================================================================================================================================================================================═

namespace {

// ---- expr_to_value: Convert an AST Expr to a runtime Value ------------------------

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

// ---- make_body: Wrap body expression list into a single Expr --------------------

Expr make_body_from_vector(const std::vector<Expr>& body_exprs) {
    if (body_exprs.empty()) {
        return make_nil();
    }
    if (body_exprs.size() == 1) {
        return body_exprs[0];
    }
    std::vector<Expr> begin_form;
    begin_form.reserve(1 + body_exprs.size());
    begin_form.push_back(Symbol("begin"));
    begin_form.insert(begin_form.end(), body_exprs.begin(), body_exprs.end());
    return make_list(std::move(begin_form));
}

// ---- extract_symbol_name: Get param name from Expr for lambda/defmacro ─

std::optional<std::string> extract_symbol_name(const Expr& expr) {
    if (const auto* sym = std::get_if<Symbol>(&expr)) {
        return sym->name;
    }
    return std::nullopt;
}

// ---- Arity checking helpers ------------------------------------------------------------------------------------─

Result<void> check_arity(int expected, int got, std::string_view form_name) {
    if (expected != got) {
        return std::unexpected(arity_error(
            SourceLocation{}, expected, got));
    }
    return {};
}

Result<void> check_min_arity(int min_expected, int got,
                             std::string_view form_name) {
    if (got < min_expected) {
        return std::unexpected(arity_error(
            SourceLocation{}, min_expected, got));
    }
    return {};
}

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
// Special forms
// ==========================================================================================================================================================================================================================================═

// ---- quote: (quote <expr>) → return expr unevaluated ------------------------------------------------─

Result<EvalResult> eval_quote(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> /*env*/) {
    if (expr.size() != 2) {
        return std::unexpected(
            arity_error(SourceLocation{}, 1,
                        static_cast<int>(expr.size()) - 1));
    }
    // Convert AST expression to runtime value (without evaluating it)
    return expr_to_value(expr[1]);
}

// ---- if: (if <cond> <then> [<else>]) --------------------------------------------------------------------------------─

Result<EvalResult> eval_if(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
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
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    for (size_t i = 1; i < expr.size(); ++i) {
        const Expr& clause = expr[i];
        const auto* clause_list = get_list(clause);
        if (!clause_list || clause_list->size() < 2) {
            return std::unexpected(
                type_error("cond clause must be (test expr)"));
        }

        const Expr& test = (*clause_list)[0];

        bool is_else = false;
        if (const auto* test_sym = std::get_if<Symbol>(&test)) {
            if (test_sym->name == "else") {
                is_else = true;
            }
        }

        if (is_else) {
            for (size_t j = 1; j < clause_list->size(); ++j) {
                if (j + 1 == clause_list->size()) {
                    return evaluate((*clause_list)[j], env);
                }
                auto val = eval_to_value((*clause_list)[j], env);
                if (!val) return std::unexpected(val.error());
            }
            return Value(nullptr);
        }

        auto test_val = eval_to_value(test, env);
        if (!test_val) return std::unexpected(test_val.error());

        if (is_truthy(*test_val)) {
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
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() < 3) {
        return std::unexpected(general_error(
            "when expects at least 2 arguments (condition and body)"));
    }

    auto cond = eval_to_value(expr[1], env);
    if (!cond) {
        return std::unexpected(cond.error());
    }

    if (is_truthy(*cond)) {
        for (size_t i = 2; i < expr.size(); ++i) {
            if (i + 1 == expr.size()) {
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
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() < 3) {
        return std::unexpected(general_error(
            "unless expects at least 2 arguments (condition and body)"));
    }

    auto cond = eval_to_value(expr[1], env);
    if (!cond) {
        return std::unexpected(cond.error());
    }

    if (!is_truthy(*cond)) {
        for (size_t i = 2; i < expr.size(); ++i) {
            if (i + 1 == expr.size()) {
                return evaluate(expr[i], env);
            }
            auto val = eval_to_value(expr[i], env);
            if (!val) return std::unexpected(val.error());
        }
    }

    return Value(nullptr);  // condition true → nil
}

// ---- case: (case <key> (<value> <expr>...) ... (else <expr>...)) ----------------─

Result<EvalResult> eval_case(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() < 3) {
        return std::unexpected(general_error(
            "case expects at least 2 arguments (key and clauses)"));
    }

    auto key_val = eval_to_value(expr[1], env);
    if (!key_val) {
        return std::unexpected(key_val.error());
    }

    for (size_t i = 2; i < expr.size(); ++i) {
        const Expr& clause = expr[i];
        const auto* clause_list = get_list(clause);
        if (!clause_list || clause_list->empty()) {
            return std::unexpected(
                type_error("case clause must be a non-empty list"));
        }

        const Expr& first = (*clause_list)[0];

        bool is_else = false;
        if (const auto* first_sym = std::get_if<Symbol>(&first)) {
            if (first_sym->name == "else") {
                is_else = true;
            }
        }

        if (is_else) {
            for (size_t j = 1; j < clause_list->size(); ++j) {
                if (j + 1 == clause_list->size()) {
                    return evaluate((*clause_list)[j], env);
                }
                auto val = eval_to_value((*clause_list)[j], env);
                if (!val) return std::unexpected(val.error());
            }
            return Value(nullptr);
        }

        bool matched = false;
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
            auto val = eval_to_value(first, env);
            if (!val) return std::unexpected(val.error());
            if (deep_equal(*key_val, *val)) {
                matched = true;
            }
        }

        if (matched) {
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

Result<EvalResult> eval_define(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() < 3) {
        return std::unexpected(
            arity_error(SourceLocation{}, 2,
                        static_cast<int>(expr.size()) - 1));
    }

    const Expr& target = expr[1];

    if (const auto* target_list = get_list(target)) {
        if (target_list->empty()) {
            return std::unexpected(
                type_error("define function form requires a name"));
        }

        const Expr& name_expr = (*target_list)[0];
        auto name_opt = extract_symbol_name(name_expr);
        if (!name_opt) {
            return std::unexpected(
                type_error("define: expected symbol for function name"));
        }

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

        std::vector<Expr> body_exprs;
        body_exprs.reserve(expr.size() - 2);
        for (size_t j = 2; j < expr.size(); ++j) {
            body_exprs.push_back(expr[j]);
        }

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

    auto name_opt = extract_symbol_name(target);
    if (!name_opt) {
        std::ostringstream ss;
        ss << expr[1];
        return std::unexpected(
            type_error(std::format("define: expected symbol, got {}", ss.str())));
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

    if (saw_dot && !rest_param.empty()) {
        params.push_back(".");
        params.push_back(rest_param);
    }

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

// ---- let: (let ((name expr) ...) <body>...) — parallel bindings ------------------------─

Result<EvalResult> eval_let(
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

        auto val = eval_to_value((*binding_list)[1], env);
        if (!val) return std::unexpected(val.error());
        values.push_back(std::move(*val));
    }

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

    for (size_t bi = 0; bi < names.size(); ++bi) {
        const auto& binding = (*bindings_list)[bi];
        const auto* binding_list = get_list(binding);
        auto val = eval_to_value((*binding_list)[1], child_env);
        if (!val) return std::unexpected(val.error());
        auto set_r = child_env->set(names[bi], *val);
        if (!set_r) return std::unexpected(set_r.error());
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

// ---- begin: (begin <expr> ...) — evaluate sequentially, return last ----------------─

Result<EvalResult> eval_begin(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
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
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    Value result(true);

    for (size_t i = 1; i < expr.size(); ++i) {
        auto val = eval_to_value(expr[i], env);
        if (!val) return std::unexpected(val.error());

        if (!is_truthy(*val)) {
            return *val;
        }
        result = std::move(*val);
    }

    return result;
}

// ---- or: (or <expr> ...) — short-circuit, return first truthy --------------------------------

Result<EvalResult> eval_or(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    for (size_t i = 1; i < expr.size(); ++i) {
        auto val = eval_to_value(expr[i], env);
        if (!val) return std::unexpected(val.error());

        if (is_truthy(*val)) {
            return *val;
        }
    }

    return Value(false);
}

// ---- do: (do ((var init step) ...) (test result...) <body>...) ----------------------------

Result<EvalResult> eval_do(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() < 3) {
        return std::unexpected(
            arity_error(SourceLocation{}, 2,
                        static_cast<int>(expr.size()) - 1));
    }

    const Expr& var_clauses_expr = expr[1];
    const auto* var_clauses = get_list(var_clauses_expr);
    if (!var_clauses) {
        return std::unexpected(
            type_error("do: variable clauses must be a list"));
    }

    const Expr& test_clause_expr = expr[2];
    const auto* test_clause = get_list(test_clause_expr);
    if (!test_clause || test_clause->empty()) {
        return std::unexpected(
            type_error("do: test clause must be (test [result])"));
    }

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

        if (clause_list->size() > 2) {
            steps.push_back((*clause_list)[2]);
        } else {
            steps.push_back((*clause_list)[0]);
        }
    }

    auto loop_env = std::make_shared<Environment>(env);

    for (size_t i = 0; i < names.size(); ++i) {
        auto init_val = eval_to_value(inits[i], env);
        if (!init_val) return std::unexpected(init_val.error());
        loop_env->define(names[i], *init_val);
    }

    while (true) {
        auto test_val = eval_to_value((*test_clause)[0], loop_env);
        if (!test_val) return std::unexpected(test_val.error());

        if (is_truthy(*test_val)) {
            if (test_clause->size() > 1) {
                return evaluate((*test_clause)[1], loop_env);
            }
            return Value(nullptr);
        }

        for (size_t i = 3; i < expr.size(); ++i) {
            auto body_val = eval_to_value(expr[i], loop_env);
            if (!body_val) return std::unexpected(body_val.error());
        }

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
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() != 2) {
        return std::unexpected(
            arity_error(SourceLocation{}, 1,
                        static_cast<int>(expr.size()) - 1));
    }

    auto result = expand_quasiquote(expr[1], env);
    if (!result) return std::unexpected(result.error());
    return *result;
}

// ---- provide: (provide sym1 sym2 ...) — declare module exports ----------------------------

Result<EvalResult> eval_provide(
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

// ---- import: (import "path.pml" [as prefix]) — load a module --------------------------------

Result<EvalResult> eval_import(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() < 2) {
        return std::unexpected(
            arity_error(SourceLocation{}, 1,
                        static_cast<int>(expr.size()) - 1));
    }

    const Expr& path_expr = expr[1];
    const auto* path_str = std::get_if<std::string>(&path_expr);
    if (!path_str) {
        return std::unexpected(
            type_error("import: path must be a string"));
    }

    std::string prefix;

    if (expr.size() >= 4) {
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
        auto prefix_opt = extract_symbol_name(expr[2]);
        if (prefix_opt) {
            prefix = std::move(*prefix_opt);
        }
    }

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

    std::string from_file;
    auto src_file = env->try_lookup("__source_file__");
    if (src_file && src_file->is_string()) {
        from_file = *src_file->as_string();
    }

    auto loader = get_global_loader(env);

    auto mod_result = loader->load(*path_str, from_file);
    if (!mod_result) {
        return std::unexpected(mod_result.error());
    }

    env->define(prefix, Value(std::move(*mod_result)));

    return Value(nullptr);
}

// ---- defmacro: (defmacro name (params) <body>...) --------------------------------------------------------

Result<EvalResult> eval_defmacro(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() < 4) {
        return std::unexpected(
            arity_error(SourceLocation{}, 3,
                        static_cast<int>(expr.size()) - 1));
    }

    auto name_opt = extract_symbol_name(expr[1]);
    if (!name_opt) {
        return std::unexpected(
            type_error("defmacro: name must be a symbol"));
    }

    const Expr& params_expr = expr[2];
    const auto* params_list = get_list(params_expr);
    if (!params_list) {
        return std::unexpected(
            type_error("defmacro: parameters must be a list"));
    }

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
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() != 2) {
        return std::unexpected(
            arity_error(SourceLocation{}, 1,
                        static_cast<int>(expr.size()) - 1));
    }

    const Expr& form = expr[1];

    // For now, just return the expression as a quoted value
    return expr_to_value(form);
}

// ---- assert: (assert <expr>) — evaluate and error if falsy ------------------------------------

Result<EvalResult> eval_assert(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() != 2) {
        return std::unexpected(
            arity_error(SourceLocation{}, 1,
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
            SourceLocation{},
            std::format("assertion failed: {}", ss.str())));
    }

    return Value(true);  // Return #t on success
}

// ---- gensym: (gensym) or (gensym "prefix") — generate a unique symbol ------------─

Result<EvalResult> eval_gensym(
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

// ---- with-exception-handler: (with-exception-handler <handler> <thunk>) ------------

Result<EvalResult> eval_with_exception_handler(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env) {
    if (expr.size() != 3) {
        return std::unexpected(
            arity_error(SourceLocation{}, 2,
                        static_cast<int>(expr.size()) - 1));
    }

    auto handler_val = eval_to_value(expr[1], env);
    if (!handler_val) return std::unexpected(handler_val.error());

    auto thunk_val = eval_to_value(expr[2], env);
    if (!thunk_val) return std::unexpected(thunk_val.error());

    auto body_result = trampoline(apply_function(*thunk_val, {}, {}, env));
    if (body_result) return Value(*body_result);

    const auto& err = body_result.error();
    std::string type_name(to_string(err.code));
    Value error_value = make_list_value({
        Value(Symbol("error")),
        Value(Symbol(type_name)),
        Value(err.message),
    });

    auto handler_result = trampoline(
        apply_function(*handler_val, {error_value}, {}, env));
    if (handler_result) return Value(*handler_result);
    return std::unexpected(handler_result.error());
}

}  // namespace pml
