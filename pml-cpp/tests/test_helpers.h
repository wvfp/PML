#pragma once

// Test helper: evaluates PML source and returns the last expression's value.

#include "pml/core/types.h"
#include "pml/core/error.h"
#include "pml/evaluator/environment.h"
#include "pml/evaluator/evaluator.h"
#include "pml/evaluator/builtins.h"
#include "pml/frontend/lexer.h"
#include "pml/frontend/parser.h"
#include "pml/frontend/expander.h"

#include <memory>
#include <string>
#include <vector>

namespace pml::test {

inline std::shared_ptr<Environment> make_env() {
    auto env = std::make_shared<Environment>();
    register_builtins(env);
    return env;
}

inline Result<std::vector<Expr>> parse(const std::string& source) {
    auto tokens = Lexer(source, "<test>").tokenize();
    if (!tokens) return std::unexpected(tokens.error());
    return Parser(std::move(*tokens), "<test>").parse();
}

inline Result<Value> eval(const std::string& source,
                          std::shared_ptr<Environment> env = nullptr) {
    if (!env) env = make_env();

    auto tokens = Lexer(source, "<test>").tokenize();
    if (!tokens) return std::unexpected(tokens.error());

    auto ast = Parser(std::move(*tokens), "<test>").parse();
    if (!ast) return std::unexpected(ast.error());

    Expander expander(env);
    auto expanded = expander.expand_all(*ast);
    if (!expanded) return std::unexpected(expanded.error());

    Value last = make_nil_value();
    for (const auto& expr : *expanded) {
        auto result = evaluate(expr, env);
        if (!result) return result;
        last = *result;
    }
    return last;
}

inline int64_t eval_int(const std::string& source,
                        std::shared_ptr<Environment> env = nullptr) {
    auto r = eval(source, env);
    if (!r) throw std::runtime_error("eval failed: " + r.error().message);
    return std::get<int64_t>(*r);
}

inline double eval_double(const std::string& source,
                          std::shared_ptr<Environment> env = nullptr) {
    auto r = eval(source, env);
    if (!r) throw std::runtime_error("eval failed: " + r.error().message);
    if (std::holds_alternative<int64_t>(*r))
        return static_cast<double>(std::get<int64_t>(*r));
    return std::get<double>(*r);
}

inline std::string eval_string(const std::string& source,
                               std::shared_ptr<Environment> env = nullptr) {
    auto r = eval(source, env);
    if (!r) throw std::runtime_error("eval failed: " + r.error().message);
    return std::get<std::string>(*r);
}

inline bool eval_bool(const std::string& source,
                      std::shared_ptr<Environment> env = nullptr) {
    auto r = eval(source, env);
    if (!r) throw std::runtime_error("eval failed: " + r.error().message);
    return std::get<bool>(*r);
}

} // namespace pml::test
