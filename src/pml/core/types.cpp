// ═══════════════════════════════════════════════════════════════════════════════
// PML Core Runtime Types — Implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "types.h"

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

Result<Value> BuiltinProcedure::call(
    const std::vector<Value>& args,
    Environment& env) {
    return fn(args, env);
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

    // Substitute into body (last expression is the result)
    Expr result = make_nil();
    for (const auto& expr : body) {
        result = substitute(expr, bindings);
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
// Value — stream output & string conversion
// ═══════════════════════════════════════════════════════════════════════════════

std::string value_to_string(const Value& v) {
    return std::visit(
        [](const auto& arg) -> std::string {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                return "nil";
            } else if constexpr (std::is_same_v<T, int64_t>) {
                return std::to_string(arg);
            } else if constexpr (std::is_same_v<T, double>) {
                std::string s = std::to_string(arg);
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
            } else if constexpr (std::is_same_v<T, std::string>) {
                return arg;  // displayed without surrounding quotes
            } else if constexpr (std::is_same_v<T, bool>) {
                return arg ? "#t" : "#f";
            } else if constexpr (std::is_same_v<T, Symbol>) {
                return arg.name;
            } else if constexpr (std::is_same_v<T, Keyword>) {
                return ':' + arg.name;
            } else if constexpr (std::is_same_v<T,
                                     std::shared_ptr<ValueList>>) {
                if (!arg) return "nil";
                std::string result = "(";
                for (size_t i = 0; i < arg->elements.size(); ++i) {
                    if (i > 0) result += ' ';
                    result += value_to_string(arg->elements[i]);
                }
                result += ')';
                return result;
            } else if constexpr (std::is_same_v<T,
                                     std::shared_ptr<Procedure>>) {
                if (!arg) return "<null-procedure>";
                const std::string& label =
                    arg->name.value_or("lambda");
                std::string result = "<Procedure " + label + " (";
                for (size_t i = 0; i < arg->params.size(); ++i) {
                    if (i > 0) result += ' ';
                    result += arg->params[i];
                }
                result += ")>";
                return result;
            } else if constexpr (std::is_same_v<T,
                                     std::shared_ptr<BuiltinProcedure>>) {
                if (!arg) return "<null-builtin>";
                return "<BuiltinProcedure " + arg->name + ">";
            } else if constexpr (std::is_same_v<T,
                                     std::shared_ptr<Macro>>) {
                if (!arg) return "<null-macro>";
                return "<Macro " + arg->name + ">";
            } else if constexpr (std::is_same_v<T,
                                     std::shared_ptr<Module>>) {
                return arg ? "<Module>" : "<null-module>";
            } else if constexpr (std::is_same_v<T,
                                     std::shared_ptr<GraphicObject>>) {
                return arg ? "<GraphicObject>" : "<null-graphic>";
            } else if constexpr (std::is_same_v<T,
                                     std::shared_ptr<Canvas>>) {
                return arg ? "<Canvas>" : "<null-canvas>";
            } else if constexpr (std::is_same_v<T,
                                     std::shared_ptr<AffineTransform>>) {
                return arg ? "<AffineTransform>" : "<null-transform>";
            } else if constexpr (std::is_same_v<T,
                                     std::shared_ptr<Animation>>) {
                return arg ? "<Animation>" : "<null-animation>";
            } else if constexpr (std::is_same_v<T,
                                     std::shared_ptr<SkeletonInstance>>) {
                return arg ? "<SkeletonInstance>" : "<null-skeleton>";
            } else if constexpr (std::is_same_v<T,
                                     std::shared_ptr<StyleDescriptor>>) {
                return arg ? "<StyleDescriptor>" : "<null-style>";
            } else if constexpr (std::is_same_v<T,
                                     std::shared_ptr<Palette>>) {
                return arg ? "<Palette>" : "<null-palette>";
            } else if constexpr (std::is_same_v<T,
                                     std::shared_ptr<Timeline>>) {
                return arg ? "<Timeline>" : "<null-timeline>";
            } else {
                return "<unknown>";
            }
        },
        v);
}

std::ostream& operator<<(std::ostream& os, const Value& val) {
    os << value_to_string(val);
    return os;
}

}  // namespace pml
