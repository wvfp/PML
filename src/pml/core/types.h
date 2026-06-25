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

#include "arena.h"
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
struct GraphicObject;         // pml/graphics/graphic_object.h
class Canvas;                // pml/graphics/canvas.h
struct Animation;            // pml/animation/animation.h
struct SkeletonTemplate;     // pml/skeleton/skeleton.h
class SkeletonInstance;      // pml/skeleton/skeleton.h
class StyleDescriptor;       // pml/sprites/style.h
class Palette;               // pml/sprites/palette.h
struct Timeline;             // pml/animation/timeline.h
struct AffineTransform;      // pml/graphics/transform.h
struct Mesh3D;               // pml/graphics3d/mesh3d.h
struct Transform3D;          // pml/graphics3d/transform3d.h
class Layer;                 // pml/layer/layer.h
class Composition;           // pml/layer/composition.h
class ImageFilter;           // pml/filter/image_filter.h
struct TextureBox;            // pml/core/texture.h

// ═══════════════════════════════════════════════════════════════════════════════
// Symbol — interned / frozen identifier
// ═══════════════════════════════════════════════════════════════════════════════

/// An interned symbol (identifier) in PML.
/// Frozen after construction: name and optional interned ID are immutable.
/// When both symbols have interned IDs, comparison uses the ID (fast path);
/// otherwise falls back to string comparison.
struct Symbol {
    std::string name;
    std::optional<uint64_t> id;

    explicit Symbol(std::string n, std::optional<uint64_t> i = std::nullopt)
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

/// Arena-aware vector alias used by ListExpr.
using ArenaExprVector = std::vector<Expr, Arena::Allocator<Expr>>;
using ArenaLocVector = std::vector<SourceLocation, Arena::Allocator<SourceLocation>>;

/// A cons-like list of expressions. Used only within Expr.
struct ListExpr {
    ArenaExprVector elements;
    SourceLocation location;                           // location of the opening '('
    ArenaLocVector element_locations;                  // location of each element

    ListExpr() = default;

    explicit ListExpr(ArenaExprVector elems)
        : elements(std::move(elems)) {}

    ListExpr(ArenaExprVector elems, SourceLocation loc,
             ArenaLocVector elem_locs = ArenaLocVector())
        : elements(std::move(elems))
        , location(std::move(loc))
        , element_locations(std::move(elem_locs)) {}
};

// ── Expr constructors ─────────────────────────────────────────────────────────

/// Access the arena used for AST/list allocations in the current thread.
[[nodiscard]] inline Arena*& current_arena() noexcept {
    thread_local Arena* arena = nullptr;
    return arena;
}

namespace detail {

[[nodiscard]] inline Arena* active_arena() noexcept {
    return current_arena();
}

}  // namespace detail

/// Create a list Expr from a vector of expressions.
[[nodiscard]] inline Expr make_list(std::vector<Expr> elems) {
    Arena* arena = detail::active_arena();
    if (arena) {
        ArenaExprVector aelems{Arena::Allocator<Expr>(arena)};
        aelems.reserve(elems.size());
        for (auto& e : elems) {
            aelems.push_back(std::move(e));
        }
        return std::allocate_shared<ListExpr>(
            Arena::Allocator<ListExpr>(arena), std::move(aelems));
    }
    ArenaExprVector heap_elems(elems.begin(), elems.end());
    return std::make_shared<ListExpr>(std::move(heap_elems));
}

/// Create a list Expr from an arena-backed vector of expressions.
[[nodiscard]] inline Expr make_list(ArenaExprVector elems) {
    Arena* arena = detail::active_arena();
    if (arena) {
        return std::allocate_shared<ListExpr>(
            Arena::Allocator<ListExpr>(arena), std::move(elems));
    }
    return std::make_shared<ListExpr>(std::move(elems));
}

/// Create a list Expr from a vector of expressions with source locations.
[[nodiscard]] inline Expr make_list(std::vector<Expr> elems,
                                    SourceLocation loc,
                                    std::vector<SourceLocation> elem_locs = {}) {
    Arena* arena = detail::active_arena();
    if (arena) {
        ArenaExprVector aelems{Arena::Allocator<Expr>(arena)};
        aelems.reserve(elems.size());
        for (auto& e : elems) {
            aelems.push_back(std::move(e));
        }
        ArenaLocVector alocs{Arena::Allocator<SourceLocation>(arena)};
        alocs.reserve(elem_locs.size());
        for (auto& l : elem_locs) {
            alocs.push_back(std::move(l));
        }
        return std::allocate_shared<ListExpr>(
            Arena::Allocator<ListExpr>(arena),
            std::move(aelems), std::move(loc), std::move(alocs));
    }
    ArenaExprVector heap_elems(elems.begin(), elems.end());
    ArenaLocVector heap_locs(elem_locs.begin(), elem_locs.end());
    return std::make_shared<ListExpr>(
        std::move(heap_elems), std::move(loc), std::move(heap_locs));
}

/// Create a list Expr from an initializer list.
[[nodiscard]] inline Expr make_list(std::initializer_list<Expr> elems) {
    return make_list(std::vector<Expr>(elems));
}

/// Create a list Expr from an initializer list with source location.
[[nodiscard]] inline Expr make_list(std::initializer_list<Expr> elems,
                                    SourceLocation loc) {
    return make_list(std::vector<Expr>(elems), std::move(loc));
}

/// Create a nil Expr.
[[nodiscard]] inline Expr make_nil() noexcept {
    return nullptr;
}

/// Create a list Expr that is always allocated from the global heap, ignoring
/// any active arena.  Use this for expressions that must outlive the current
/// arena scope (e.g. Procedure bodies, macro bodies).
[[nodiscard]] inline Expr make_list_heap(std::vector<Expr> elems) {
    ArenaExprVector heap_elems(elems.begin(), elems.end());
    return std::make_shared<ListExpr>(std::move(heap_elems));
}

[[nodiscard]] inline Expr make_list_heap(std::vector<Expr> elems,
                                         SourceLocation loc,
                                         std::vector<SourceLocation> elem_locs = {}) {
    ArenaExprVector heap_elems(elems.begin(), elems.end());
    ArenaLocVector heap_locs(elem_locs.begin(), elem_locs.end());
    return std::make_shared<ListExpr>(
        std::move(heap_elems), std::move(loc), std::move(heap_locs));
}

/// Deep-clone an expression tree, forcing all list nodes onto the global heap.
[[nodiscard]] Expr clone_expr_to_heap(const Expr& expr);

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
[[nodiscard]] inline const ArenaExprVector* get_list(const Expr& e) noexcept {
    if (auto* lst = std::get_if<std::shared_ptr<ListExpr>>(&e)) {
        return &(*lst)->elements;
    }
    return nullptr;
}

/// Mutable version.
[[nodiscard]] inline ArenaExprVector* get_list(Expr& e) noexcept {
    if (auto* lst = std::get_if<std::shared_ptr<ListExpr>>(&e)) {
        return &(*lst)->elements;
    }
    return nullptr;
}

/// Extract the source location stored on an Expr, if any.
[[nodiscard]] inline SourceLocation get_expr_location(const Expr& e) noexcept {
    if (const auto* lst = std::get_if<std::shared_ptr<ListExpr>>(&e)) {
        if (*lst) {
            return (*lst)->location;
        }
    }
    return SourceLocation{};
}

/// Stream output for debugging.
std::ostream& operator<<(std::ostream& os, const Expr& expr);

// ═══════════════════════════════════════════════════════════════════════════════
// Forward declarations for Procedure / BuiltinProcedure / Macro / ValueList
// ═══════════════════════════════════════════════════════════════════════════════

class Procedure;
class BuiltinProcedure;
class Macro;

struct ValueList;
struct ValueHashMap;
struct ValueVector;

// ═══════════════════════════════════════════════════════════════════════════════
// Box — heap storage for complex Value alternatives
// ═══════════════════════════════════════════════════════════════════════════════

/// Heap-allocated payload referenced by `Value` for non-inline alternatives.
struct Box {
    enum class Kind {
        String, Symbol, Keyword,
        List, HashMap, Vector,
        Procedure, Builtin, Macro, Module,
        GraphicObject, Canvas, Transform, Mesh3D, Transform3D, Animation,
        SkeletonTemplate, SkeletonInstance,
        Style, Palette, Timeline,
        Layer, Composition,
        ImageFilter,
        Texture
    };

    Kind kind;
    std::variant<
        std::string, Symbol, Keyword,
        std::shared_ptr<ValueList>,
        std::shared_ptr<ValueHashMap>,
        std::shared_ptr<ValueVector>,
        std::shared_ptr<Procedure>,
        std::shared_ptr<BuiltinProcedure>,
        std::shared_ptr<Macro>,
        std::shared_ptr<Module>,
        std::shared_ptr<GraphicObject>,
        std::shared_ptr<Canvas>,
        std::shared_ptr<AffineTransform>,
        std::shared_ptr<Mesh3D>,
        std::shared_ptr<Transform3D>,
        std::shared_ptr<Animation>,
        std::shared_ptr<SkeletonTemplate>,
        std::shared_ptr<SkeletonInstance>,
        std::shared_ptr<StyleDescriptor>,
        std::shared_ptr<Palette>,
        std::shared_ptr<Timeline>,
        std::shared_ptr<Layer>,
        std::shared_ptr<Composition>,
        std::shared_ptr<ImageFilter>,
        std::shared_ptr<TextureBox>
    > data;
};

// ═══════════════════════════════════════════════════════════════════════════════
// Value — compact runtime representation
// ═══════════════════════════════════════════════════════════════════════════════

/// The runtime value type. Small scalar values (nil, int64_t, double, bool) are
/// stored inline in a tagged union. All complex values are stored in a
/// heap-allocated `Box` via `std::shared_ptr`, preserving reference semantics.
class Value {
public:
    enum class Tag : uint8_t { Nil, Int, Double, Bool, Object };

    // ── Constructors ──────────────────────────────────────────────────
    Value() noexcept : tag_(Tag::Nil) {}
    Value(std::nullptr_t) noexcept : Value() {}
    Value(int64_t v) noexcept : tag_(Tag::Int), int_val_(v) {}
    Value(int v) noexcept : Value(static_cast<int64_t>(v)) {}
    Value(double v) noexcept : tag_(Tag::Double), double_val_(v) {}
    Value(bool v) noexcept : tag_(Tag::Bool), bool_val_(v) {}

    Value(std::string v);
    Value(const char* v);
    Value(Symbol v);
    Value(Keyword v);
    Value(std::shared_ptr<ValueList> v);
    Value(std::shared_ptr<ValueHashMap> v);
    Value(std::shared_ptr<ValueVector> v);
    Value(std::shared_ptr<Procedure> v);
    Value(std::shared_ptr<BuiltinProcedure> v);
    Value(std::shared_ptr<Macro> v);
    Value(std::shared_ptr<Module> v);
    Value(std::shared_ptr<GraphicObject> v);
    Value(std::shared_ptr<Canvas> v);
    Value(std::shared_ptr<AffineTransform> v);
    Value(std::shared_ptr<Mesh3D> v);
    Value(std::shared_ptr<Transform3D> v);
    Value(std::shared_ptr<Animation> v);
    Value(std::shared_ptr<SkeletonTemplate> v);
    Value(std::shared_ptr<SkeletonInstance> v);
    Value(std::shared_ptr<StyleDescriptor> v);
    Value(std::shared_ptr<Palette> v);
    Value(std::shared_ptr<Timeline> v);
    Value(std::shared_ptr<Layer> v);
    Value(std::shared_ptr<Composition> v);
    Value(std::shared_ptr<ImageFilter> v);
    Value(std::shared_ptr<TextureBox> v);

    // ── Tag / predicates ──────────────────────────────────────────────
    [[nodiscard]] Tag tag() const noexcept { return tag_; }

    [[nodiscard]] bool is_nil() const noexcept { return tag_ == Tag::Nil; }
    [[nodiscard]] bool is_int() const noexcept { return tag_ == Tag::Int; }
    [[nodiscard]] bool is_double() const noexcept { return tag_ == Tag::Double; }
    [[nodiscard]] bool is_number() const noexcept { return tag_ == Tag::Int || tag_ == Tag::Double; }
    [[nodiscard]] bool is_bool() const noexcept { return tag_ == Tag::Bool; }
    [[nodiscard]] bool is_string() const noexcept;
    [[nodiscard]] bool is_symbol() const noexcept;
    [[nodiscard]] bool is_keyword() const noexcept;
    [[nodiscard]] bool is_list() const noexcept;
    [[nodiscard]] bool is_hash() const noexcept;
    [[nodiscard]] bool is_vector() const noexcept;
    [[nodiscard]] bool is_procedure() const noexcept;
    [[nodiscard]] bool is_builtin() const noexcept;
    [[nodiscard]] bool is_macro() const noexcept;
    [[nodiscard]] bool is_module() const noexcept;
    [[nodiscard]] bool is_graphic_object() const noexcept;
    [[nodiscard]] bool is_canvas() const noexcept;
    [[nodiscard]] bool is_transform() const noexcept;
    [[nodiscard]] bool is_mesh3d() const noexcept;
    [[nodiscard]] bool is_transform3d() const noexcept;
    [[nodiscard]] bool is_animation() const noexcept;
    [[nodiscard]] bool is_skeleton_template() const noexcept;
    [[nodiscard]] bool is_skeleton_instance() const noexcept;
    [[nodiscard]] bool is_style() const noexcept;
    [[nodiscard]] bool is_palette() const noexcept;
    [[nodiscard]] bool is_timeline() const noexcept;
    [[nodiscard]] bool is_layer() const noexcept;
    [[nodiscard]] bool is_composition() const noexcept;
    [[nodiscard]] bool is_image_filter() const noexcept;

    [[nodiscard]] bool is_texture() const noexcept;

    // ── Object kind (valid only when tag() == Tag::Object) ──────────────
    [[nodiscard]] Box::Kind object_kind() const noexcept { return box_kind(); }

    // ── Inline scalar accessors ───────────────────────────────────────
    [[nodiscard]] int64_t int_val() const noexcept { return int_val_; }
    [[nodiscard]] double double_val() const noexcept { return double_val_; }
    [[nodiscard]] bool bool_val() const noexcept { return bool_val_; }

    // ── Object accessors (return nullptr if not the requested kind) ───
    [[nodiscard]] const std::string* as_string() const noexcept;
    [[nodiscard]] const Symbol* as_symbol() const noexcept;
    [[nodiscard]] const Keyword* as_keyword() const noexcept;
    [[nodiscard]] const std::shared_ptr<ValueList>* as_list() const noexcept;
    [[nodiscard]] const std::shared_ptr<ValueHashMap>* as_hash() const noexcept;
    [[nodiscard]] const std::shared_ptr<ValueVector>* as_vector() const noexcept;
    [[nodiscard]] const std::shared_ptr<Procedure>* as_procedure() const noexcept;
    [[nodiscard]] const std::shared_ptr<BuiltinProcedure>* as_builtin() const noexcept;
    [[nodiscard]] const std::shared_ptr<Macro>* as_macro() const noexcept;
    [[nodiscard]] const std::shared_ptr<Module>* as_module() const noexcept;
    [[nodiscard]] const std::shared_ptr<GraphicObject>* as_graphic_object() const noexcept;
    [[nodiscard]] const std::shared_ptr<Canvas>* as_canvas() const noexcept;
    [[nodiscard]] const std::shared_ptr<AffineTransform>* as_transform() const noexcept;
    [[nodiscard]] const std::shared_ptr<Mesh3D>* as_mesh3d() const noexcept;
    [[nodiscard]] const std::shared_ptr<Transform3D>* as_transform3d() const noexcept;
    [[nodiscard]] const std::shared_ptr<Animation>* as_animation() const noexcept;
    [[nodiscard]] const std::shared_ptr<SkeletonTemplate>* as_skeleton_template() const noexcept;
    [[nodiscard]] const std::shared_ptr<SkeletonInstance>* as_skeleton_instance() const noexcept;
    [[nodiscard]] const std::shared_ptr<StyleDescriptor>* as_style() const noexcept;
    [[nodiscard]] const std::shared_ptr<Palette>* as_palette() const noexcept;
    [[nodiscard]] const std::shared_ptr<Timeline>* as_timeline() const noexcept;
    [[nodiscard]] const std::shared_ptr<Layer>* as_layer() const noexcept;
    [[nodiscard]] const std::shared_ptr<Composition>* as_composition() const noexcept;
    [[nodiscard]] const std::shared_ptr<ImageFilter>* as_image_filter() const noexcept;

    [[nodiscard]] const std::shared_ptr<TextureBox>* as_texture() const noexcept;

    // ── Equality (structural for scalars, reference for boxed objects) ─
    bool operator==(const Value& other) const noexcept;
    bool operator!=(const Value& other) const noexcept;

private:
    Tag tag_;
    union {
        int64_t int_val_;
        double double_val_;
        bool bool_val_;
    };
    std::shared_ptr<Box> box_;

    [[nodiscard]] Box::Kind box_kind() const noexcept;
    [[nodiscard]] const Box* box() const noexcept { return box_.get(); }
};

// ── Value predicates (free-function wrappers for backward compatibility) ──────

[[nodiscard]] inline bool is_nil(const Value& v) noexcept { return v.is_nil(); }
[[nodiscard]] inline bool is_integer(const Value& v) noexcept { return v.is_int(); }
[[nodiscard]] inline bool is_float(const Value& v) noexcept { return v.is_double(); }
[[nodiscard]] inline bool is_number(const Value& v) noexcept { return v.is_number(); }
[[nodiscard]] inline bool is_string(const Value& v) noexcept { return v.is_string(); }
[[nodiscard]] inline bool is_bool(const Value& v) noexcept { return v.is_bool(); }
[[nodiscard]] inline bool is_symbol(const Value& v) noexcept { return v.is_symbol(); }
[[nodiscard]] inline bool is_keyword(const Value& v) noexcept { return v.is_keyword(); }
[[nodiscard]] inline bool is_list(const Value& v) noexcept { return v.is_list(); }
[[nodiscard]] inline bool is_hash(const Value& v) noexcept { return v.is_hash(); }
[[nodiscard]] inline bool is_vector(const Value& v) noexcept { return v.is_vector(); }
[[nodiscard]] inline bool is_procedure(const Value& v) noexcept { return v.is_procedure(); }
[[nodiscard]] inline bool is_builtin(const Value& v) noexcept { return v.is_builtin(); }
[[nodiscard]] inline bool is_macro(const Value& v) noexcept { return v.is_macro(); }

[[nodiscard]] inline bool is_texture(const Value& v) noexcept { return v.is_texture(); }


// ═══════════════════════════════════════════════════════════════════════════════
// ValueList / ValueHashMap / ValueVector — reference-counted containers
// ═══════════════════════════════════════════════════════════════════════════════

struct ValueList {
    std::vector<Value> elements;
    ValueList() = default;
    explicit ValueList(std::vector<Value> elems) : elements(std::move(elems)) {}
    ValueList(std::initializer_list<Value> il) : elements(il) {}
};

struct ValueHashMap {
    std::unordered_map<std::string, Value> data;
    ValueHashMap() = default;
    explicit ValueHashMap(std::unordered_map<std::string, Value> d) : data(std::move(d)) {}
};

struct ValueVector {
    std::vector<Value> elements;
    ValueVector() = default;
    explicit ValueVector(std::vector<Value> elems) : elements(std::move(elems)) {}
    explicit ValueVector(size_t n, const Value& init) : elements(n, init) {}
};

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
    Result<Value> call(const std::vector<Value>& args, Environment& env);

    friend std::ostream& operator<<(std::ostream& os, const Procedure& proc);
};

// ═══════════════════════════════════════════════════════════════════════════════
// BuiltinProcedure — native C++ function exposed as PML procedure
// ═══════════════════════════════════════════════════════════════════════════════

/// A built-in function implemented in C++ and registered as a PML procedure.
class BuiltinProcedure {
public:
    /// Legacy signature without source location (kept for backward compatibility).
    using BuiltinFn = std::function<Result<Value>(
        const std::vector<Value>&, Environment&)>;

    /// New signature with source location for precise error reporting.
    using BuiltinFnEx = std::function<Result<Value>(
        const std::vector<Value>&, Environment&, SourceLocation)>;

    std::string name;
    std::variant<BuiltinFn, BuiltinFnEx> fn;
    bool accepts_kwargs;

    BuiltinProcedure(
        std::string name_,
        BuiltinFn fn_,
        bool accepts_kwargs_ = false
    );

    BuiltinProcedure(
        std::string name_,
        BuiltinFnEx fn_,
        bool accepts_kwargs_ = false
    );

    /// Delegate to the stored function, passing the call site location.
    Result<Value> call(
        const std::vector<Value>& args, Environment& env,
        SourceLocation loc = {});

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
    Expr expand(const std::vector<Expr>& args);

    friend std::ostream& operator<<(std::ostream& os, const Macro& macro);
};

// ═══════════════════════════════════════════════════════════════════════════════
// Value utilities
// ═══════════════════════════════════════════════════════════════════════════════

/// Convert a runtime value to its display string.
std::string value_to_string(const Value& v);

/// Stream output for Value (delegates to value_to_string).
std::ostream& operator<<(std::ostream& os, const Value& val);

// ═══════════════════════════════════════════════════════════════════════════════
// Convenience Value factories
// ═══════════════════════════════════════════════════════════════════════════════

[[nodiscard]] inline Value make_nil_value() noexcept {
    return Value{};
}

[[nodiscard]] inline Value make_list_value(std::vector<Value> elems) {
    return std::make_shared<ValueList>(std::move(elems));
}

[[nodiscard]] inline Value make_list_value(std::initializer_list<Value> elems) {
    return std::make_shared<ValueList>(elems);
}

}  // namespace pml
