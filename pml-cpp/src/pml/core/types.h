#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Core Runtime Types
// ───────────────────────────────────────────────────────────────────────────────
// Foundation type layer: Symbol, Keyword, Expr (AST), Value (runtime),
// Procedure, BuiltinProcedure, Macro.
//
// All public types are in the global namespace (matching existing code style).
// Forward declarations are used for cross-module types that will be defined
// in later tasks (Module, GraphicObject, Canvas, Animation, etc.).
// ═══════════════════════════════════════════════════════════════════════════════

#include "error.h"

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Forward declarations — types defined in other modules (later tasks)
// ═══════════════════════════════════════════════════════════════════════════════

class Environment;           // pml/evaluator/environment.h
class Module;                // pml/module/module_loader.h
class GraphicObject;         // pml/graphics/graphic_object.h
class Canvas;                // pml/graphics/canvas.h
struct Animation;            // pml/animation/animation.h
struct SkeletonTemplate;     // pml/skeleton/skeleton.h
class SkeletonInstance;      // pml/skeleton/skeleton.h
class StyleDescriptor;       // pml/sprites/style.h
class Palette;               // pml/sprites/palette.h
struct Timeline;             // pml/animation/timeline.h
struct AffineTransform;      // pml/graphics/transform.h

// ═══════════════════════════════════════════════════════════════════════════════
// Symbol — interned / frozen identifier
// ═══════════════════════════════════════════════════════════════════════════════

/// An interned symbol (identifier) in PML.
/// Frozen after construction: name and optional interned ID are immutable.
/// When both symbols have interned IDs, comparison uses the ID (fast path);
/// otherwise falls back to string comparison.
struct Symbol {
    std::string name;
    std::optional<uint32_t> id;

    explicit Symbol(std::string n, std::optional<uint32_t> i = std::nullopt)
        : name(std::move(n)), id(i) {}

    // ── Equality ──────────────────────────────────────────────────────
    bool operator==(const Symbol& other) const noexcept {
        if (id.has_value() && other.id.has_value()) {
            return *id == *other.id;  // fast path: interned ID
        }
        return name == other.name;    // fallback: string compare
    }

    bool operator!=(const Symbol& other) const noexcept {
        return !(*this == other);
    }

    // ── Ordering ──────────────────────────────────────────────────────
    bool operator<(const Symbol& other) const noexcept {
        return name < other.name;
    }

    // ── Streaming ─────────────────────────────────────────────────────
    friend std::ostream& operator<<(std::ostream& os, const Symbol& sym);
};

// ═══════════════════════════════════════════════════════════════════════════════
// Keyword — prefixed with : in source, stored without leading colon
// ═══════════════════════════════════════════════════════════════════════════════

/// A keyword like :fill, :stroke. Stores the name without the leading colon.
/// Frozen after construction.
struct Keyword {
    std::string name;

    explicit Keyword(std::string n) : name(std::move(n)) {}

    bool operator==(const Keyword& other) const noexcept {
        return name == other.name;
    }

    bool operator!=(const Keyword& other) const noexcept {
        return !(*this == other);
    }

    friend std::ostream& operator<<(std::ostream& os, const Keyword& kw);
};

// ═══════════════════════════════════════════════════════════════════════════════
// Expr — AST (parse tree) representation
// ═══════════════════════════════════════════════════════════════════════════════

struct ListExpr;

/// An S-expression in the PML AST.
/// Recursive via shared_ptr<ListExpr> to keep the variant copyable.
using Expr = std::variant<
    std::nullptr_t,                  // nil
    int64_t,                         // integer literal
    double,                          // float literal
    std::string,                     // string literal
    bool,                            // boolean literal
    Symbol,                          // symbol (identifier)
    Keyword,                         // keyword (:key)
    std::shared_ptr<ListExpr>        // list (S-expression)
>;

/// A cons-like list of expressions. Used only within Expr.
struct ListExpr {
    std::vector<Expr> elements;

    ListExpr() = default;
    explicit ListExpr(std::vector<Expr> elems) : elements(std::move(elems)) {}
};

// ── Expr constructors ─────────────────────────────────────────────────────────

/// Create a list Expr from a vector of expressions.
[[nodiscard]] inline Expr make_list(std::vector<Expr> elems) {
    return std::make_shared<ListExpr>(std::move(elems));
}

/// Create a list Expr from an initializer list.
[[nodiscard]] inline Expr make_list(std::initializer_list<Expr> elems) {
    return std::make_shared<ListExpr>(std::vector<Expr>(elems));
}

/// Create a nil Expr.
[[nodiscard]] inline Expr make_nil() noexcept {
    return nullptr;
}

// ── Expr predicates (constexpr-compatible) ────────────────────────────────────

[[nodiscard]] constexpr bool is_nil(const Expr& e) noexcept {
    return std::holds_alternative<std::nullptr_t>(e);
}
[[nodiscard]] constexpr bool is_integer(const Expr& e) noexcept {
    return std::holds_alternative<int64_t>(e);
}
[[nodiscard]] constexpr bool is_float(const Expr& e) noexcept {
    return std::holds_alternative<double>(e);
}
[[nodiscard]] constexpr bool is_number(const Expr& e) noexcept {
    return is_integer(e) || is_float(e);
}
[[nodiscard]] constexpr bool is_string(const Expr& e) noexcept {
    return std::holds_alternative<std::string>(e);
}
[[nodiscard]] constexpr bool is_bool(const Expr& e) noexcept {
    return std::holds_alternative<bool>(e);
}
[[nodiscard]] constexpr bool is_symbol(const Expr& e) noexcept {
    return std::holds_alternative<Symbol>(e);
}
[[nodiscard]] constexpr bool is_keyword(const Expr& e) noexcept {
    return std::holds_alternative<Keyword>(e);
}
[[nodiscard]] constexpr bool is_list(const Expr& e) noexcept {
    return std::holds_alternative<std::shared_ptr<ListExpr>>(e);
}

// ── Expr accessors ────────────────────────────────────────────────────────────

/// Get pointer to list elements (or nullptr if not a list).
[[nodiscard]] inline const std::vector<Expr>* get_list(const Expr& e) noexcept {
    if (auto* lst = std::get_if<std::shared_ptr<ListExpr>>(&e)) {
        return &(*lst)->elements;
    }
    return nullptr;
}

/// Mutable version.
[[nodiscard]] inline std::vector<Expr>* get_list(Expr& e) noexcept {
    if (auto* lst = std::get_if<std::shared_ptr<ListExpr>>(&e)) {
        return &(*lst)->elements;
    }
    return nullptr;
}

/// Stream output for debugging.
std::ostream& operator<<(std::ostream& os, const Expr& expr);

// ═══════════════════════════════════════════════════════════════════════════════
// Forward declarations for Procedure / BuiltinProcedure / Macro
// (needed by Value variant — Value stores them via shared_ptr for reference
//  semantics, matching Python's object model.)
// ═══════════════════════════════════════════════════════════════════════════════

class Procedure;
class BuiltinProcedure;
class Macro;

/// Forward-declared so the Value variant can refer to lists without
/// self-referential completeness issues.
struct ValueList;

// ═══════════════════════════════════════════════════════════════════════════════
// Value — runtime representation
// ═══════════════════════════════════════════════════════════════════════════════

/// The runtime value type. An S-expression evaluated to a value.
/// Complex types (Procedure, Module, etc.) are stored via shared_ptr for
/// reference semantics, mirroring Python's object model.
using Value = std::variant<
    std::nullptr_t,                          // nil
    int64_t,                                 // integer
    double,                                  // float
    std::string,                             // string
    bool,                                    // boolean
    Symbol,                                  // symbol
    Keyword,                                 // keyword
    std::shared_ptr<ValueList>,              // list at runtime (see below)
    std::shared_ptr<Procedure>,              // user-defined function
    std::shared_ptr<BuiltinProcedure>,       // built-in function
    std::shared_ptr<Macro>,                  // macro
    std::shared_ptr<Module>,                 // module
    std::shared_ptr<GraphicObject>,          // graphic primitive / object
    std::shared_ptr<Canvas>,                 // drawing canvas
    std::shared_ptr<AffineTransform>,        // 2D affine transform
    std::shared_ptr<Animation>,              // animation object
    std::shared_ptr<SkeletonTemplate>,       // skeleton template
    std::shared_ptr<SkeletonInstance>,       // skeleton instance
    std::shared_ptr<StyleDescriptor>,        // styling information
    std::shared_ptr<Palette>,                // color palette
    std::shared_ptr<Timeline>                // animation timeline
>;

/// ValueList — a reference-counted vector of Values.
/// Defined out-of-line so the Value variant can be complete first.
struct ValueList {
    std::vector<Value> elements;
    ValueList() = default;
    explicit ValueList(std::vector<Value> elems) : elements(std::move(elems)) {}
    ValueList(std::initializer_list<Value> il) : elements(il) {}
};

// ── Value predicates ──────────────────────────────────────────────────────────

[[nodiscard]] constexpr bool is_nil(const Value& v) noexcept {
    return std::holds_alternative<std::nullptr_t>(v);
}
[[nodiscard]] constexpr bool is_integer(const Value& v) noexcept {
    return std::holds_alternative<int64_t>(v);
}
[[nodiscard]] constexpr bool is_float(const Value& v) noexcept {
    return std::holds_alternative<double>(v);
}
[[nodiscard]] constexpr bool is_number(const Value& v) noexcept {
    return is_integer(v) || is_float(v);
}
[[nodiscard]] constexpr bool is_string(const Value& v) noexcept {
    return std::holds_alternative<std::string>(v);
}
[[nodiscard]] constexpr bool is_bool(const Value& v) noexcept {
    return std::holds_alternative<bool>(v);
}
[[nodiscard]] constexpr bool is_symbol(const Value& v) noexcept {
    return std::holds_alternative<Symbol>(v);
}
[[nodiscard]] constexpr bool is_keyword(const Value& v) noexcept {
    return std::holds_alternative<Keyword>(v);
}
[[nodiscard]] constexpr bool is_list(const Value& v) noexcept {
    return std::holds_alternative<std::shared_ptr<ValueList>>(v);
}
[[nodiscard]] constexpr bool is_procedure(const Value& v) noexcept {
    return std::holds_alternative<std::shared_ptr<Procedure>>(v);
}
[[nodiscard]] constexpr bool is_builtin(const Value& v) noexcept {
    return std::holds_alternative<std::shared_ptr<BuiltinProcedure>>(v);
}
[[nodiscard]] constexpr bool is_macro(const Value& v) noexcept {
    return std::holds_alternative<std::shared_ptr<Macro>>(v);
}

// ── Value utilities ───────────────────────────────────────────────────────────

/// Convert a runtime value to its display string.
/// Matches PML printer semantics:
///   - strings: printed as-is (no surrounding quotes in display)
///   - nil: "nil"
///   - bool: "#t" / "#f"
///   - lists: "(el1 el2 ...)"
std::string value_to_string(const Value& v);

/// Stream output for Value (delegates to value_to_string).
std::ostream& operator<<(std::ostream& os, const Value& val);

// ═══════════════════════════════════════════════════════════════════════════════
// Procedure — user-defined PML function (closure)
// ═══════════════════════════════════════════════════════════════════════════════

/// A user-defined PML function with lexical closure.
class Procedure {
public:
    std::vector<std::string> params;
    Expr body;
    std::shared_ptr<Environment> closure_env;
    std::optional<std::string> name;

    Procedure(
        std::vector<std::string> params_,
        Expr body_,
        std::shared_ptr<Environment> closure_env_,
        std::optional<std::string> name_ = std::nullopt
    );

    /// Call the procedure with the given arguments in the given environment.
    /// Delegates to the evaluator (implementation in types.cpp, may be stubbed
    /// until the evaluator is available).
    Result<Value> call(const std::vector<Value>& args, Environment& env);

    friend std::ostream& operator<<(std::ostream& os, const Procedure& proc);
};

// ═══════════════════════════════════════════════════════════════════════════════
// BuiltinProcedure — native C++ function exposed as PML procedure
// ═══════════════════════════════════════════════════════════════════════════════

/// A built-in function implemented in C++ and registered as a PML procedure.
class BuiltinProcedure {
public:
    using BuiltinFn = std::function<Result<Value>(
        const std::vector<Value>&, Environment&)>;

    std::string name;
    BuiltinFn fn;
    bool accepts_kwargs;

    BuiltinProcedure(
        std::string name_,
        BuiltinFn fn_,
        bool accepts_kwargs_ = false
    );

    /// Delegate to the stored function.
    Result<Value> call(const std::vector<Value>& args, Environment& env);

    friend std::ostream& operator<<(std::ostream& os, const BuiltinProcedure& bp);
};

// ═══════════════════════════════════════════════════════════════════════════════
// Macro — PML macro (non-hygienic, expanded before evaluation)
// ═══════════════════════════════════════════════════════════════════════════════

/// A user-defined PML macro. Performs non-hygienic substitution of
/// parameters with unevaluated argument expressions.
class Macro {
public:
    std::string name;
    std::vector<std::string> params;
    std::optional<std::string> rest_param;
    std::vector<Expr> body;
    std::shared_ptr<Environment> closure_env;

    Macro(
        std::string name_,
        std::vector<std::string> params_,
        std::vector<Expr> body_,
        std::shared_ptr<Environment> closure_env_,
        std::optional<std::string> rest_param_ = std::nullopt
    );

    /// Expand the macro by substituting parameters with unevaluated args.
    /// Supports (unquote x) and (unquote-splicing x) patterns within the body.
    Expr expand(const std::vector<Expr>& args);

    friend std::ostream& operator<<(std::ostream& os, const Macro& macro);
};

// ═══════════════════════════════════════════════════════════════════════════════
// Convenience Value factories
// ═══════════════════════════════════════════════════════════════════════════════

[[nodiscard]] inline Value make_nil_value() noexcept {
    return nullptr;
}

[[nodiscard]] inline Value make_list_value(std::vector<Value> elems) {
    return std::make_shared<ValueList>(std::move(elems));
}

[[nodiscard]] inline Value make_list_value(std::initializer_list<Value> elems) {
    return std::make_shared<ValueList>(elems);
}

}  // namespace pml
