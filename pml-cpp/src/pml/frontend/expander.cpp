// ═══════════════════════════════════════════════════════════════════════════════
// PML Expander — Macro Expansion Pass Implementation
// ═══════════════════════════════════════════════════════════════════════════════
//
// Matches the Python PML expander (pml/expander.py) exactly.
// ═══════════════════════════════════════════════════════════════════════════════

#include "expander.h"
#include "environment.h"  // from pml/evaluator — Environment with try_lookup()

#include <string>
#include <utility>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════════

Expander::Expander(std::shared_ptr<Environment> env_, int max_depth_)
    : env(std::move(env_))
    , max_depth(max_depth_) {}

// ═══════════════════════════════════════════════════════════════════════════════
// expand — expand a single expression recursively
// ═══════════════════════════════════════════════════════════════════════════════

Result<Expr> Expander::expand(const Expr& expr, int depth) const {
    // 1. Depth limit
    if (depth > max_depth) {
        return std::unexpected(
            macro_expansion_depth_error(SourceLocation{}, max_depth));
    }

    // 2. Atoms and empty lists pass through unchanged
    const auto* list = get_list(expr);
    if (!list || list->empty()) {
        return expr;
    }

    const Expr& head = (*list)[0];

    // 3. Check if head is a macro
    if (is_symbol(head)) {
        const auto& sym = std::get<Symbol>(head);
        auto val = env->try_lookup(sym.name);
        if (val.has_value() && is_macro(*val)) {
            const auto& macro_ptr =
                *std::get_if<std::shared_ptr<Macro>>(&*val);

            // Get args: all elements after head (expr[1:])
            std::vector<Expr> args;
            if (list->size() > 1) {
                args.reserve(list->size() - 1);
                for (size_t i = 1; i < list->size(); ++i) {
                    args.push_back((*list)[i]);
                }
            }

            // Expand: substitute unevaluated args into macro body
            Expr expanded = macro_ptr->expand(args);

            // Re-expand the result (macros can produce more macro calls)
            return this->expand(expanded, depth + 1);
        }
    }

    // 4. Not a macro call: recursively expand sub-expressions
    //    But preserve special forms — don't expand inside quote/quasiquote
    if (is_symbol(head)) {
        const auto& sym = std::get<Symbol>(head);

        if (sym.name == "quote") {
            return expr;  // Don't expand inside quotes
        }

        if (sym.name == "lambda" || sym.name == "defmacro") {
            // Expand body (expr[2:]) but not parameter list (expr[1])
            std::vector<Expr> result;
            result.reserve(list->size());
            result.push_back((*list)[0]);  // head (lambda/defmacro)
            result.push_back((*list)[1]);  // parameter list (unexpanded)
            for (size_t i = 2; i < list->size(); ++i) {
                auto expanded = expand((*list)[i], depth);
                if (!expanded) {
                    return std::unexpected(expanded.error());
                }
                result.push_back(std::move(*expanded));
            }
            return make_list(std::move(result));
        }

        if (sym.name == "define") {
            // For (define (name params) body...), expand body only
            if (list->size() >= 3 && is_list((*list)[1])) {
                std::vector<Expr> result;
                result.reserve(list->size());
                result.push_back((*list)[0]);  // define
                result.push_back((*list)[1]);  // function form (unexpanded)
                for (size_t i = 2; i < list->size(); ++i) {
                    auto expanded = expand((*list)[i], depth);
                    if (!expanded) {
                        return std::unexpected(expanded.error());
                    }
                    result.push_back(std::move(*expanded));
                }
                return make_list(std::move(result));
            }
            // Variable form (define name value) falls through to default
        }
    }

    // 5. Default: recursively expand all sub-expressions
    std::vector<Expr> result;
    result.reserve(list->size());
    for (const auto& elem : *list) {
        auto expanded = expand(elem, depth);
        if (!expanded) {
            return std::unexpected(expanded.error());
        }
        result.push_back(std::move(*expanded));
    }
    return make_list(std::move(result));
}

// ═══════════════════════════════════════════════════════════════════════════════
// expand_all — expand macros in a list of top-level expressions
// ═══════════════════════════════════════════════════════════════════════════════

Result<std::vector<Expr>> Expander::expand_all(
    const std::vector<Expr>& ast) const {
    std::vector<Expr> result;
    result.reserve(ast.size());
    for (const auto& expr : ast) {
        auto expanded = expand(expr);
        if (!expanded) {
            return std::unexpected(expanded.error());
        }
        result.push_back(std::move(*expanded));
    }
    return result;
}

}  // namespace pml
