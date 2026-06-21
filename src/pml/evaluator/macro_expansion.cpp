// ═══════════════════════════════════════════════════════════════════════════════
// PML Macro Expansion — hygienic and non-hygienic substitution
// ═══════════════════════════════════════════════════════════════════════════════
//
// Extracted from core/types.cpp to separate macro-expansion logic from core
// runtime type definitions.  Macro::expand drives the expansion: it applies
// hygienic renaming to prevent accidental capture (rename_expr), then does
// the non-hygienic substitution of macro arguments (substitute).
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/core/types.h"

#include <atomic>
#include <format>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

namespace {

// ═══════════════════════════════════════════════════════════════════════════════
// Non-hygienic substitution (for macro argument expansion)
// ═══════════════════════════════════════════════════════════════════════════════

/// Recursive non-hygienic substitution for macro expansion.
///
/// For each expression:
///   - Symbol: replace if in bindings, else pass through
///   - (unquote x)  : replace x with its bound value
///   - (unquote-splicing x) : splice the bound list into the parent
///   - list: recursively substitute all elements
///   - atom: pass through unchanged
Expr substitute(
    const Expr& expr,
    const std::unordered_map<std::string, Expr>& bindings) {

    return std::visit(
        [&](const auto& arg) -> Expr {
            using T = std::decay_t<decltype(arg)>;

            // ── Symbol: check bindings ────────────────────────────────
            if constexpr (std::is_same_v<T, Symbol>) {
                auto it = bindings.find(arg.name);
                if (it != bindings.end()) {
                    return it->second;
                }
                return expr;
            }

            // ── List: check for special forms, recurse ────────────────
            if constexpr (std::is_same_v<T, std::shared_ptr<ListExpr>>) {
                const auto& elems = arg->elements;
                if (elems.empty()) {
                    return expr;
                }

                // Check first element for special forms
                const auto* head_sym =
                    std::get_if<Symbol>(&elems[0]);

                // (quasiquote body) — recurse into the body so that
                // unquote / unquote-splicing inside are handled.
                // Return the substituted body directly (without quasiquote wrapper)
                // because we're in macro expansion, not evaluation.
                if (head_sym && head_sym->name == "quasiquote"
                    && elems.size() >= 2) {
                    return substitute(elems[1], bindings);
                }

                // (unquote x) — substitute and return directly
                if (head_sym && head_sym->name == "unquote" && elems.size() >= 2) {
                    const Expr& inner = elems[1];
                    if (const auto* inner_sym =
                            std::get_if<Symbol>(&inner)) {
                        auto it = bindings.find(inner_sym->name);
                        if (it != bindings.end()) {
                            return it->second;
                        }
                    }
                    return substitute(inner, bindings);
                }

                // (unquote-splicing x) — return the spliced list
                if (head_sym && head_sym->name == "unquote-splicing"
                    && elems.size() >= 2) {
                    const Expr& inner = elems[1];
                    if (const auto* inner_sym =
                            std::get_if<Symbol>(&inner)) {
                        auto it = bindings.find(inner_sym->name);
                        if (it != bindings.end()) {
                            const Expr& bound = it->second;
                            // If bound is a list, return its elements as a list
                            // Otherwise, wrap in a single-element list
                            if (const auto* lst =
                                    std::get_if<std::shared_ptr<ListExpr>>(
                                        &bound)) {
                                return make_list((*lst)->elements);
                            }
                            return make_list({bound});
                        }
                    }
                    return substitute(inner, bindings);
                }

                // Regular list: recursively substitute each element,
                // handling splice at the element level
                std::vector<Expr> new_elems;
                new_elems.reserve(elems.size());

                for (const auto& elem : elems) {
                    // Detect (unquote-splicing x) at element level
                    if (const auto* sub_list =
                            std::get_if<std::shared_ptr<ListExpr>>(&elem)) {
                        const auto& sub_elems = sub_list->get()->elements;
                        if (sub_elems.size() == 2) {
                            if (const auto* sub_sym =
                                    std::get_if<Symbol>(&sub_elems[0])) {
                                if (sub_sym->name == "unquote-splicing") {
                                    if (const auto* splice_sym =
                                            std::get_if<Symbol>(
                                                &sub_elems[1])) {
                                        auto it =
                                            bindings.find(splice_sym->name);
                                        if (it != bindings.end()) {
                                            const Expr& bound = it->second;
                                            if (const auto* lst =
                                                    std::get_if<
                                                        std::shared_ptr<
                                                            ListExpr>>(
                                                        &bound)) {
                                                for (const auto& se :
                                                     (*lst)->elements) {
                                                    new_elems.push_back(se);
                                                }
                                            } else {
                                                new_elems.push_back(bound);
                                            }
                                            continue;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    new_elems.push_back(substitute(elem, bindings));
                }

                return make_list(std::move(new_elems));
            }

            // ── Atom: pass through ────────────────────────────────────
            return expr;
        },
        expr);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Hygienic macro expansion helpers
// ═══════════════════════════════════════════════════════════════════════════════

using RenameMap = std::unordered_map<std::string, Symbol>;

Expr rename_expr(const Expr& expr, RenameMap& map);

std::optional<std::string> expr_symbol_name(const Expr& e) {
    if (const auto* s = std::get_if<Symbol>(&e)) {
        return s->name;
    }
    return std::nullopt;
}

Symbol make_gensym(const std::string& base) {
    static std::atomic<uint64_t> counter{0};
    uint64_t id = counter.fetch_add(1, std::memory_order_relaxed);
    return Symbol{std::format("{}#{}", base, id), id};
}

Expr rename_children(const Expr& expr, RenameMap& map) {
    const auto* lst = std::get_if<std::shared_ptr<ListExpr>>(&expr);
    if (!lst) {
        if (const auto* s = std::get_if<Symbol>(&expr)) {
            auto it = map.find(s->name);
            if (it != map.end()) {
                return Expr{it->second};
            }
        }
        return expr;
    }

    const auto& elems = (*lst)->elements;
    std::vector<Expr> new_elems;
    new_elems.reserve(elems.size());
    for (const auto& elem : elems) {
        new_elems.push_back(rename_expr(elem, map));
    }
    return make_list(std::move(new_elems), (*lst)->location);
}

std::vector<Expr> rename_body(
    const std::vector<Expr>& body, RenameMap& map) {
    std::vector<Expr> result;
    result.reserve(body.size());
    for (const auto& expr : body) {
        result.push_back(rename_expr(expr, map));
    }
    return result;
}

Expr rename_lambda(const ArenaExprVector& elems, const SourceLocation& loc, RenameMap& map) {
    std::vector<Expr> new_elems(elems.begin(), elems.end());
    RenameMap local_map = map;

    // Parameter list may be a flat list of symbols or a single variadic symbol.
    if (elems.size() >= 2) {
        const Expr& params_expr = elems[1];
        if (const auto* params_lst = get_list(params_expr)) {
            std::vector<Expr> new_params;
            new_params.reserve(params_lst->size());
            for (const auto& p : *params_lst) {
                auto name = expr_symbol_name(p);
                if (name) {
                    Symbol fresh = make_gensym(*name);
                    local_map.insert_or_assign(*name, fresh);
                    new_params.push_back(Expr{fresh});
                } else {
                    new_params.push_back(p);
                }
            }
            new_elems[1] = make_list(std::move(new_params), get_expr_location(params_expr));
        } else if (auto name = expr_symbol_name(params_expr)) {
            Symbol fresh = make_gensym(*name);
            local_map.insert_or_assign(*name, fresh);
            new_elems[1] = Expr{fresh};
        }
    }

    for (size_t i = 2; i < elems.size(); ++i) {
        new_elems[i] = rename_expr(elems[i], local_map);
    }
    return make_list(std::move(new_elems), loc);
}

Expr rename_let(
    const std::string& form,
    const ArenaExprVector& elems,
    const SourceLocation& loc,
    RenameMap& map) {
    std::vector<Expr> new_elems(elems.begin(), elems.end());
    RenameMap local_map = map;

    if (elems.size() >= 2) {
        const Expr& bindings_expr = elems[1];
        if (const auto* bindings_lst = get_list(bindings_expr)) {
            std::vector<Expr> new_bindings;
            new_bindings.reserve(bindings_lst->size());
            for (const auto& b : *bindings_lst) {
                const auto* pair_lst = get_list(b);
                if (!pair_lst || pair_lst->empty()) {
                    new_bindings.push_back(rename_expr(b, local_map));
                    continue;
                }
                auto name = expr_symbol_name(pair_lst->at(0));
                if (!name) {
                    new_bindings.push_back(rename_expr(b, local_map));
                    continue;
                }

                Symbol fresh = make_gensym(*name);
                std::vector<Expr> new_pair(pair_lst->begin(), pair_lst->end());
                new_pair[0] = Expr{fresh};

                if (form == "let*") {
                    // Each init sees previously renamed bindings.
                    if (new_pair.size() >= 2) {
                        new_pair[1] = rename_expr(pair_lst->at(1), local_map);
                    }
                    local_map.insert_or_assign(*name, fresh);
                } else if (form == "letrec") {
                    // All bindings are mutually visible.
                    local_map.insert_or_assign(*name, fresh);
                    for (size_t i = 1; i < new_pair.size(); ++i) {
                        new_pair[i] = rename_expr(pair_lst->at(i), local_map);
                    }
                } else {
                    // Plain let: inits use the outer map; body uses the new map.
                    if (new_pair.size() >= 2) {
                        new_pair[1] = rename_expr(pair_lst->at(1), map);
                    }
                    local_map.insert_or_assign(*name, fresh);
                }
                new_bindings.push_back(make_list(std::move(new_pair), get_expr_location(b)));
            }
            new_elems[1] = make_list(std::move(new_bindings), get_expr_location(bindings_expr));
        }
    }

    for (size_t i = 2; i < elems.size(); ++i) {
        new_elems[i] = rename_expr(elems[i], local_map);
    }
    return make_list(std::move(new_elems), loc);
}

Expr rename_define(const ArenaExprVector& elems, const SourceLocation& loc, RenameMap& map) {
    std::vector<Expr> new_elems(elems.begin(), elems.end());

    if (elems.size() >= 3) {
        // (define (name params...) body...)
        if (const auto* head_lst = get_list(elems[1])) {
            std::vector<Expr> new_head;
            new_head.reserve(head_lst->size());
            for (size_t i = 0; i < head_lst->size(); ++i) {
                auto name = expr_symbol_name(head_lst->at(i));
                if (i == 0 && name) {
                    // The function name is bound in the surrounding scope.
                    Symbol fresh = make_gensym(*name);
                    map.insert_or_assign(*name, fresh);
                    new_head.push_back(Expr{fresh});
                } else if (name) {
                    new_head.push_back(Expr{make_gensym(*name)});
                } else {
                    new_head.push_back(head_lst->at(i));
                }
            }
            new_elems[1] = make_list(std::move(new_head), get_expr_location(elems[1]));
            for (size_t i = 2; i < elems.size(); ++i) {
                new_elems[i] = rename_expr(elems[i], map);
            }
            return make_list(std::move(new_elems), loc);
        }

        // (define name value)
        auto name = expr_symbol_name(elems[1]);
        if (name) {
            Symbol fresh = make_gensym(*name);
            new_elems[1] = Expr{fresh};
            map.insert_or_assign(*name, fresh);
        }
        new_elems[2] = rename_expr(elems[2], map);
        return make_list(std::move(new_elems), loc);
    }

    return make_list(std::move(new_elems), loc);
}

Expr rename_do(const ArenaExprVector& elems, const SourceLocation& loc, RenameMap& map) {
    std::vector<Expr> new_elems(elems.begin(), elems.end());
    RenameMap local_map = map;

    if (elems.size() >= 2) {
        const Expr& specs_expr = elems[1];
        if (const auto* specs_lst = get_list(specs_expr)) {
            std::vector<Expr> new_specs;
            new_specs.reserve(specs_lst->size());
            for (const auto& s : *specs_lst) {
                const auto* spec_lst = get_list(s);
                if (!spec_lst || spec_lst->empty()) {
                    new_specs.push_back(rename_expr(s, local_map));
                    continue;
                }
                auto name = expr_symbol_name(spec_lst->at(0));
                if (!name) {
                    new_specs.push_back(rename_expr(s, local_map));
                    continue;
                }
                Symbol fresh = make_gensym(*name);
                local_map.insert_or_assign(*name, fresh);
                std::vector<Expr> new_spec(spec_lst->begin(), spec_lst->end());
                new_spec[0] = Expr{fresh};
                // init uses the outer map; step uses the new (iteration) map.
                if (new_spec.size() >= 2) {
                    new_spec[1] = rename_expr(spec_lst->at(1), map);
                }
                for (size_t i = 2; i < new_spec.size(); ++i) {
                    new_spec[i] = rename_expr(spec_lst->at(i), local_map);
                }
                new_specs.push_back(make_list(std::move(new_spec), get_expr_location(s)));
            }
            new_elems[1] = make_list(std::move(new_specs), get_expr_location(specs_expr));
        }
    }

    for (size_t i = 2; i < elems.size(); ++i) {
        new_elems[i] = rename_expr(elems[i], local_map);
    }
    return make_list(std::move(new_elems), loc);
}

Expr rename_expr(const Expr& expr, RenameMap& map) {
    const auto* lst = std::get_if<std::shared_ptr<ListExpr>>(&expr);
    if (!lst) {
        if (const auto* s = std::get_if<Symbol>(&expr)) {
            auto it = map.find(s->name);
            if (it != map.end()) {
                return Expr{it->second};
            }
        }
        return expr;
    }

    const auto& elems = (*lst)->elements;
    const SourceLocation loc = (*lst)->location;
    if (elems.empty()) {
        return expr;
    }

    auto head_name = expr_symbol_name(elems[0]);
    if (!head_name) {
        return rename_children(expr, map);
    }
    const std::string& head = *head_name;

    if (head == "quote" || head == "quasiquote") {
        // Don't rename inside quoted literals.
        return expr;
    }
    if (head == "lambda") {
        return rename_lambda(elems, loc, map);
    }
    if (head == "let" || head == "let*" || head == "letrec") {
        return rename_let(head, elems, loc, map);
    }
    if (head == "do") {
        return rename_do(elems, loc, map);
    }
    if (head == "define") {
        return rename_define(elems, loc, map);
    }

    return rename_children(expr, map);
}

}  // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// Macro::expand
// ═══════════════════════════════════════════════════════════════════════════════

Expr Macro::expand(const std::vector<Expr>& args) {
    // Build parameter bindings
    std::unordered_map<std::string, Expr> bindings;

    for (size_t i = 0; i < params.size(); ++i) {
        if (i < args.size()) {
            bindings[params[i]] = args[i];
        } else {
            bindings[params[i]] = make_list({});  // empty list for missing args
        }
    }

    // Rest parameter collects remaining args — wrap in (list ...) so it
    // evaluates to a PML list
    if (rest_param.has_value()) {
        std::vector<Expr> rest_args;
        for (size_t i = params.size(); i < args.size(); ++i) {
            rest_args.push_back(args[i]);
        }
        // Prepend the `list` symbol so it evaluates to a list
        std::vector<Expr> wrapped;
        wrapped.reserve(1 + rest_args.size());
        wrapped.push_back(Symbol("list"));
        wrapped.insert(wrapped.end(), rest_args.begin(), rest_args.end());
        bindings[*rest_param] = make_list(std::move(wrapped));
    }

    // Apply hygienic renaming: macro-introduced bindings (lambda/let/do/define)
    // are renamed to fresh symbols so they cannot capture variables from the
    // call site or from macro arguments.
    RenameMap rename_map;
    Expr body_list = make_list(body);
    Expr renamed_body_list = rename_expr(body_list, rename_map);
    const auto* renamed_lst = get_list(renamed_body_list);

    // Substitute into body (last expression is the result)
    Expr result = make_nil();
    if (renamed_lst) {
        for (const auto& expr : *renamed_lst) {
            result = substitute(expr, bindings);
        }
    } else {
        for (const auto& expr : body) {
            result = substitute(expr, bindings);
        }
    }

    return result;
}

}  // namespace pml
