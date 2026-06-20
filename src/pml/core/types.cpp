// ═══════════════════════════════════════════════════════════════════════════════
// PML Core Runtime Types — Implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "types.h"

#include <atomic>
#include <format>
#include <type_traits>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Symbol
// ═══════════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const Symbol& sym) {
    os << sym.name;
    return os;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Keyword
// ═══════════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const Keyword& kw) {
    os << ':' << kw.name;
    return os;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Expr — stream output & clone
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

/// Recursive helper for Expr streaming.
void expr_to_stream(std::ostream& os, const Expr& expr) {
    std::visit(
        [&os](const auto& arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                os << "nil";
            } else if constexpr (std::is_same_v<T, int64_t>) {
                os << arg;
            } else if constexpr (std::is_same_v<T, double>) {
                // Format: remove trailing zeros, keep decimal point
                std::string s = std::to_string(arg);
                auto dot = s.find('.');
                if (dot != std::string::npos) {
                    auto last = s.find_last_not_of('0');
                    if (last > dot) {
                        s.erase(last + 1);
                    } else {
                        s.erase(dot + 2);  // keep at least ".0"
                    }
                }
                os << s;
            } else if constexpr (std::is_same_v<T, std::string>) {
                os << '"' << arg << '"';
            } else if constexpr (std::is_same_v<T, bool>) {
                os << (arg ? "#t" : "#f");
            } else if constexpr (std::is_same_v<T, Symbol>) {
                os << arg;  // uses Symbol::operator<<
            } else if constexpr (std::is_same_v<T, Keyword>) {
                os << arg;  // uses Keyword::operator<<
            } else if constexpr (std::is_same_v<T, std::shared_ptr<ListExpr>>) {
                os << '(';
                const auto& elems = arg->elements;
                for (size_t i = 0; i < elems.size(); ++i) {
                    if (i > 0) os << ' ';
                    expr_to_stream(os, elems[i]);
                }
                os << ')';
            } else {
                os << "<unknown>";
            }
        },
        expr);
}

}  // anonymous namespace

std::ostream& operator<<(std::ostream& os, const Expr& expr) {
    expr_to_stream(os, expr);
    return os;
}

namespace {

/// Recursive helper for clone_expr_to_heap.
Expr clone_expr_to_heap_impl(const Expr& expr) {
    if (const auto* lst = std::get_if<std::shared_ptr<ListExpr>>(&expr)) {
        if (!*lst) return expr;
        std::vector<Expr> cloned;
        cloned.reserve((*lst)->elements.size());
        for (const auto& elem : (*lst)->elements) {
            cloned.push_back(clone_expr_to_heap_impl(elem));
        }
        std::vector<SourceLocation> cloned_locs;
        cloned_locs.reserve((*lst)->element_locations.size());
        for (const auto& loc : (*lst)->element_locations) {
            cloned_locs.push_back(loc);
        }
        return make_list_heap(
            std::move(cloned), (*lst)->location, std::move(cloned_locs));
    }
    return expr;  // atoms are immutable and cheap to copy
}

}  // anonymous namespace

/// Deep-clone an Expr tree, forcing every list node onto the global heap.
Expr clone_expr_to_heap(const Expr& expr) {
    return clone_expr_to_heap_impl(expr);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Procedure
// ═══════════════════════════════════════════════════════════════════════════════

Procedure::Procedure(
    std::vector<std::string> params_,
    Expr body_,
    std::shared_ptr<Environment> closure_env_,
    std::optional<std::string> name_)
    : params(std::move(params_))
    , body(std::move(body_))
    , closure_env(std::move(closure_env_))
    , name(std::move(name_)) {}

std::ostream& operator<<(std::ostream& os, const Procedure& proc) {
    const std::string& label = proc.name.value_or("lambda");
    os << "<Procedure " << label << " (";
    for (size_t i = 0; i < proc.params.size(); ++i) {
        if (i > 0) os << ' ';
        os << proc.params[i];
    }
    os << ")>";
    return os;
}

// ═══════════════════════════════════════════════════════════════════════════════
// BuiltinProcedure
// ═══════════════════════════════════════════════════════════════════════════════

BuiltinProcedure::BuiltinProcedure(
    std::string name_,
    BuiltinFn fn_,
    bool accepts_kwargs_)
    : name(std::move(name_))
    , fn(std::move(fn_))
    , accepts_kwargs(accepts_kwargs_) {}

BuiltinProcedure::BuiltinProcedure(
    std::string name_,
    BuiltinFnEx fn_,
    bool accepts_kwargs_)
    : name(std::move(name_))
    , fn(std::move(fn_))
    , accepts_kwargs(accepts_kwargs_) {}

Result<Value> BuiltinProcedure::call(
    const std::vector<Value>& args,
    Environment& env,
    SourceLocation loc) {
    return std::visit(
        [&](auto&& fn) -> Result<Value> {
            using Fn = std::decay_t<decltype(fn)>;
            if constexpr (std::is_same_v<Fn, BuiltinFnEx>) {
                return fn(args, env, loc);
            } else {
                return fn(args, env);
            }
        },
        fn);
}

std::ostream& operator<<(std::ostream& os, const BuiltinProcedure& bp) {
    os << "<BuiltinProcedure " << bp.name << '>';
    return os;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Macro — expand (non-hygienic substitution)
// ═══════════════════════════════════════════════════════════════════════════════

Macro::Macro(
    std::string name_,
    std::vector<std::string> params_,
    std::vector<Expr> body_,
    std::shared_ptr<Environment> closure_env_,
    std::optional<std::string> rest_param_)
    : name(std::move(name_))
    , params(std::move(params_))
    , rest_param(std::move(rest_param_))
    , body(std::move(body_))
    , closure_env(std::move(closure_env_)) {}

namespace {

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

}  // anonymous namespace

namespace {

// ── Hygienic macro expansion helpers ─────────────────────────────────────────

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

std::ostream& operator<<(std::ostream& os, const Macro& macro) {
    os << "<Macro " << macro.name << " (";
    for (size_t i = 0; i < macro.params.size(); ++i) {
        if (i > 0) os << ' ';
        os << macro.params[i];
    }
    if (macro.rest_param.has_value()) {
        os << " . " << *macro.rest_param;
    }
    os << ")>";
    return os;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Value — constructors, accessors, equality, string conversion
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

// Helper to format a double with the same rules as the original variant visitor.
std::string format_double(double v) {
    std::string s = std::to_string(v);
    auto dot = s.find('.');
    if (dot != std::string::npos) {
        auto last = s.find_last_not_of('0');
        if (last > dot) {
            s.erase(last + 1);
        } else {
            s.erase(dot + 2);  // keep ".0"
        }
    }
    return s;
}

}  // anonymous namespace

// ── Object constructors ───────────────────────────────────────────────────────

Value::Value(std::string v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::String, std::move(v)})) {}

Value::Value(const char* v) : Value(std::string(v)) {}

Value::Value(Symbol v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Symbol, std::move(v)})) {}

Value::Value(Keyword v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Keyword, std::move(v)})) {}

Value::Value(std::shared_ptr<ValueList> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::List, std::move(v)})) {}

Value::Value(std::shared_ptr<ValueHashMap> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::HashMap, std::move(v)})) {}

Value::Value(std::shared_ptr<ValueVector> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Vector, std::move(v)})) {}

Value::Value(std::shared_ptr<Procedure> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Procedure, std::move(v)})) {}

Value::Value(std::shared_ptr<BuiltinProcedure> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Builtin, std::move(v)})) {}

Value::Value(std::shared_ptr<Macro> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Macro, std::move(v)})) {}

Value::Value(std::shared_ptr<Module> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Module, std::move(v)})) {}

Value::Value(std::shared_ptr<GraphicObject> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::GraphicObject, std::move(v)})) {}

Value::Value(std::shared_ptr<Canvas> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Canvas, std::move(v)})) {}

Value::Value(std::shared_ptr<AffineTransform> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Transform, std::move(v)})) {}

Value::Value(std::shared_ptr<Mesh3D> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Mesh3D, std::move(v)})) {}

Value::Value(std::shared_ptr<Transform3D> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Transform3D, std::move(v)})) {}

Value::Value(std::shared_ptr<Animation> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Animation, std::move(v)})) {}

Value::Value(std::shared_ptr<SkeletonTemplate> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::SkeletonTemplate, std::move(v)})) {}

Value::Value(std::shared_ptr<SkeletonInstance> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::SkeletonInstance, std::move(v)})) {}

Value::Value(std::shared_ptr<StyleDescriptor> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Style, std::move(v)})) {}

Value::Value(std::shared_ptr<Palette> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Palette, std::move(v)})) {}

Value::Value(std::shared_ptr<Timeline> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Timeline, std::move(v)})) {}

Value::Value(std::shared_ptr<Layer> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Layer, std::move(v)})) {}

Value::Value(std::shared_ptr<Composition> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Composition, std::move(v)})) {}

Value::Value(std::shared_ptr<ImageFilter> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::ImageFilter, std::move(v)})) {}

// ── Kind helpers ──────────────────────────────────────────────────────────────

Box::Kind Value::box_kind() const noexcept {
    return box_ ? box_->kind : Box::Kind{};
}

// ── Object predicates ─────────────────────────────────────────────────────────

bool Value::is_string() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::String;
}
bool Value::is_symbol() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Symbol;
}
bool Value::is_keyword() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Keyword;
}
bool Value::is_list() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::List;
}
bool Value::is_hash() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::HashMap;
}
bool Value::is_vector() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Vector;
}
bool Value::is_procedure() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Procedure;
}
bool Value::is_builtin() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Builtin;
}
bool Value::is_macro() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Macro;
}
bool Value::is_module() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Module;
}
bool Value::is_graphic_object() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::GraphicObject;
}
bool Value::is_canvas() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Canvas;
}
bool Value::is_transform() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Transform;
}
bool Value::is_mesh3d() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Mesh3D;
}
bool Value::is_transform3d() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Transform3D;
}
bool Value::is_animation() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Animation;
}
bool Value::is_skeleton_template() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::SkeletonTemplate;
}
bool Value::is_skeleton_instance() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::SkeletonInstance;
}
bool Value::is_style() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Style;
}
bool Value::is_palette() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Palette;
}
bool Value::is_timeline() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Timeline;
}
bool Value::is_layer() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Layer;
}
bool Value::is_composition() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Composition;
}
bool Value::is_image_filter() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::ImageFilter;
}

// ── Object accessors ──────────────────────────────────────────────────────────

const std::string* Value::as_string() const noexcept {
    if (!is_string()) return nullptr;
    return std::get_if<std::string>(&box_->data);
}
const Symbol* Value::as_symbol() const noexcept {
    if (!is_symbol()) return nullptr;
    return std::get_if<Symbol>(&box_->data);
}
const Keyword* Value::as_keyword() const noexcept {
    if (!is_keyword()) return nullptr;
    return std::get_if<Keyword>(&box_->data);
}
const std::shared_ptr<ValueList>* Value::as_list() const noexcept {
    if (!is_list()) return nullptr;
    return std::get_if<std::shared_ptr<ValueList>>(&box_->data);
}
const std::shared_ptr<ValueHashMap>* Value::as_hash() const noexcept {
    if (!is_hash()) return nullptr;
    return std::get_if<std::shared_ptr<ValueHashMap>>(&box_->data);
}
const std::shared_ptr<ValueVector>* Value::as_vector() const noexcept {
    if (!is_vector()) return nullptr;
    return std::get_if<std::shared_ptr<ValueVector>>(&box_->data);
}
const std::shared_ptr<Procedure>* Value::as_procedure() const noexcept {
    if (!is_procedure()) return nullptr;
    return std::get_if<std::shared_ptr<Procedure>>(&box_->data);
}
const std::shared_ptr<BuiltinProcedure>* Value::as_builtin() const noexcept {
    if (!is_builtin()) return nullptr;
    return std::get_if<std::shared_ptr<BuiltinProcedure>>(&box_->data);
}
const std::shared_ptr<Macro>* Value::as_macro() const noexcept {
    if (!is_macro()) return nullptr;
    return std::get_if<std::shared_ptr<Macro>>(&box_->data);
}
const std::shared_ptr<Module>* Value::as_module() const noexcept {
    if (!is_module()) return nullptr;
    return std::get_if<std::shared_ptr<Module>>(&box_->data);
}
const std::shared_ptr<GraphicObject>* Value::as_graphic_object() const noexcept {
    if (!is_graphic_object()) return nullptr;
    return std::get_if<std::shared_ptr<GraphicObject>>(&box_->data);
}
const std::shared_ptr<Canvas>* Value::as_canvas() const noexcept {
    if (!is_canvas()) return nullptr;
    return std::get_if<std::shared_ptr<Canvas>>(&box_->data);
}
const std::shared_ptr<AffineTransform>* Value::as_transform() const noexcept {
    if (!is_transform()) return nullptr;
    return std::get_if<std::shared_ptr<AffineTransform>>(&box_->data);
}
const std::shared_ptr<Mesh3D>* Value::as_mesh3d() const noexcept {
    if (!is_mesh3d()) return nullptr;
    return std::get_if<std::shared_ptr<Mesh3D>>(&box_->data);
}
const std::shared_ptr<Transform3D>* Value::as_transform3d() const noexcept {
    if (!is_transform3d()) return nullptr;
    return std::get_if<std::shared_ptr<Transform3D>>(&box_->data);
}
const std::shared_ptr<Animation>* Value::as_animation() const noexcept {
    if (!is_animation()) return nullptr;
    return std::get_if<std::shared_ptr<Animation>>(&box_->data);
}
const std::shared_ptr<SkeletonTemplate>* Value::as_skeleton_template() const noexcept {
    if (!is_skeleton_template()) return nullptr;
    return std::get_if<std::shared_ptr<SkeletonTemplate>>(&box_->data);
}
const std::shared_ptr<SkeletonInstance>* Value::as_skeleton_instance() const noexcept {
    if (!is_skeleton_instance()) return nullptr;
    return std::get_if<std::shared_ptr<SkeletonInstance>>(&box_->data);
}
const std::shared_ptr<StyleDescriptor>* Value::as_style() const noexcept {
    if (!is_style()) return nullptr;
    return std::get_if<std::shared_ptr<StyleDescriptor>>(&box_->data);
}
const std::shared_ptr<Palette>* Value::as_palette() const noexcept {
    if (!is_palette()) return nullptr;
    return std::get_if<std::shared_ptr<Palette>>(&box_->data);
}
const std::shared_ptr<Timeline>* Value::as_timeline() const noexcept {
    if (!is_timeline()) return nullptr;
    return std::get_if<std::shared_ptr<Timeline>>(&box_->data);
}
const std::shared_ptr<Layer>* Value::as_layer() const noexcept {
    if (!is_layer()) return nullptr;
    return std::get_if<std::shared_ptr<Layer>>(&box_->data);
}
const std::shared_ptr<Composition>* Value::as_composition() const noexcept {
    if (!is_composition()) return nullptr;
    return std::get_if<std::shared_ptr<Composition>>(&box_->data);
}
const std::shared_ptr<ImageFilter>* Value::as_image_filter() const noexcept {
    if (!is_image_filter()) return nullptr;
    return std::get_if<std::shared_ptr<ImageFilter>>(&box_->data);
}

// ── Equality ──────────────────────────────────────────────────────────────────

bool Value::operator==(const Value& other) const noexcept {
    if (tag_ != other.tag_) return false;
    switch (tag_) {
        case Tag::Nil: return true;
        case Tag::Int: return int_val_ == other.int_val_;
        case Tag::Double: return double_val_ == other.double_val_;
        case Tag::Bool: return bool_val_ == other.bool_val_;
        case Tag::Object:
            if (box_kind() != other.box_kind()) return false;
            return box_->data == other.box_->data;
    }
    return false;
}

bool Value::operator!=(const Value& other) const noexcept {
    return !(*this == other);
}

// ── String conversion ─────────────────────────────────────────────────────────

std::string value_to_string(const Value& v) {
    if (v.is_nil()) return "nil";
    if (v.is_int()) return std::to_string(v.int_val());
    if (v.is_double()) return format_double(v.double_val());
    if (v.is_bool()) return v.bool_val() ? "#t" : "#f";

    if (const auto* s = v.as_string()) return *s;
    if (const auto* sym = v.as_symbol()) return sym->name;
    if (const auto* kw = v.as_keyword()) return ':' + kw->name;

    if (const auto* lst = v.as_list()) {
        if (!*lst) return "nil";
        std::string result = "(";
        for (size_t i = 0; i < (*lst)->elements.size(); ++i) {
            if (i > 0) result += ' ';
            result += value_to_string((*lst)->elements[i]);
        }
        result += ')';
        return result;
    }
    if (const auto* h = v.as_hash()) {
        if (!*h) return "<null-hash>";
        std::string result = "{";
        bool first = true;
        for (const auto& [k, val] : (*h)->data) {
            if (!first) result += ' ';
            first = false;
            result += '"';
            result += k;
            result += "\" ";
            result += value_to_string(val);
        }
        result += '}';
        return result;
    }
    if (const auto* vec = v.as_vector()) {
        if (!*vec) return "<null-vector>";
        std::string result = "#(";
        for (size_t i = 0; i < (*vec)->elements.size(); ++i) {
            if (i > 0) result += ' ';
            result += value_to_string((*vec)->elements[i]);
        }
        result += ')';
        return result;
    }
    if (const auto* proc = v.as_procedure()) {
        if (!*proc) return "<null-procedure>";
        const std::string& label = (*proc)->name.value_or("lambda");
        std::string result = "<Procedure " + label + " (";
        for (size_t i = 0; i < (*proc)->params.size(); ++i) {
            if (i > 0) result += ' ';
            result += (*proc)->params[i];
        }
        result += ")>";
        return result;
    }
    if (const auto* bp = v.as_builtin()) {
        if (!*bp) return "<null-builtin>";
        return "<BuiltinProcedure " + (*bp)->name + ">";
    }
    if (const auto* m = v.as_macro()) {
        if (!*m) return "<null-macro>";
        return "<Macro " + (*m)->name + ">";
    }
    if (const auto* mod = v.as_module()) {
        return *mod ? "<Module>" : "<null-module>";
    }
    if (const auto* g = v.as_graphic_object()) {
        return *g ? "<GraphicObject>" : "<null-graphic>";
    }
    if (const auto* c = v.as_canvas()) {
        return *c ? "<Canvas>" : "<null-canvas>";
    }
    if (const auto* t = v.as_transform()) {
        return *t ? "<AffineTransform>" : "<null-transform>";
    }
    if (const auto* a = v.as_animation()) {
        return *a ? "<Animation>" : "<null-animation>";
    }
    if (const auto* sk = v.as_skeleton_instance()) {
        return *sk ? "<SkeletonInstance>" : "<null-skeleton>";
    }
    if (const auto* st = v.as_style()) {
        return *st ? "<StyleDescriptor>" : "<null-style>";
    }
    if (const auto* pal = v.as_palette()) {
        return *pal ? "<Palette>" : "<null-palette>";
    }
    if (const auto* tl = v.as_timeline()) {
        return *tl ? "<Timeline>" : "<null-timeline>";
    }
    if (v.is_layer()) return "<layer>";
    if (v.is_composition()) return "<composition>";
    if (v.is_image_filter()) return "<filter>";

    return "<unknown>";
}

std::ostream& operator<<(std::ostream& os, const Value& val) {
    os << value_to_string(val);
    return os;
}

}  // namespace pml
