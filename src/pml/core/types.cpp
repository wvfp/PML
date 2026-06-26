// ==========================================================================================================================================================================================================================================═
// PML Core Runtime Types — Implementation
// ==========================================================================================================================================================================================================================================═

#include "types.h"

#include <atomic>
#include <format>
#include <type_traits>

#include "texture.h"  // for TextureBox

namespace pml {

// ==========================================================================================================================================================================================================================================═
// Symbol
// ==========================================================================================================================================================================================================================================═

std::ostream& operator<<(std::ostream& os, const Symbol& sym) {
    os << sym.name;
    return os;
}

// ==========================================================================================================================================================================================================================================═
// Keyword
// ==========================================================================================================================================================================================================================================═

std::ostream& operator<<(std::ostream& os, const Keyword& kw) {
    os << ':' << kw.name;
    return os;
}

// ==========================================================================================================================================================================================================================================═
// Expr — stream output & clone
// ==========================================================================================================================================================================================================================================═

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

// ==========================================================================================================================================================================================================================================═
// Procedure
// ==========================================================================================================================================================================================================================================═

Procedure::Procedure(
    std::vector<std::string> params_,
    Expr body_,
    std::shared_ptr<Environment> closure_env_,
    std::optional<std::string> name_,
    std::optional<ParamInfo> param_info_)
    : params(std::move(params_))
    , body(std::move(body_))
    , closure_env(std::move(closure_env_))
    , name(std::move(name_))
    , param_info(std::move(param_info_)) {}

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

// ==========================================================================================================================================================================================================================================═
// BuiltinProcedure
// ==========================================================================================================================================================================================================================================═

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

// ==========================================================================================================================================================================================================================================═
// Macro — expand (non-hygienic substitution)
// ==========================================================================================================================================================================================================================================═

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

// Macro expansion (substitute, rename_expr, Macro::expand) has been moved
// to src/pml/evaluator/macro_expansion.cpp.

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

// ==========================================================================================================================================================================================================================================═
// Value — constructors, accessors, equality, string conversion
// ==========================================================================================================================================================================================================================================═

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

// ---- Object constructors ------------------------------------------------------------------------------------------------------------─

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

Value::Value(std::shared_ptr<TextureBox> v)
    : tag_(Tag::Object)
    , box_(std::make_shared<Box>(Box{Box::Kind::Texture, std::move(v)})) {}

// ---- Kind helpers ----------------------------------------------------------------------------------------------------------------------------

Box::Kind Value::box_kind() const noexcept {
    return box_ ? box_->kind : Box::Kind{};
}

// ---- Object predicates ----------------------------------------------------------------------------------------------------------------─

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

bool Value::is_texture() const noexcept {
    return tag_ == Tag::Object && box_kind() == Box::Kind::Texture;
}

// ---- Object accessors --------------------------------------------------------------------------------------------------------------------

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

const std::shared_ptr<TextureBox>* Value::as_texture() const noexcept {
    if (!is_texture()) return nullptr;
    return std::get_if<std::shared_ptr<TextureBox>>(&box_->data);
}

// ---- Equality ------------------------------------------------------------------------------------------------------------------------------------

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

// ---- String conversion ----------------------------------------------------------------------------------------------------------------─

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
