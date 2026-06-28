#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Built-in Procedures — Shared helpers
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Shared helper functions used by all builtin registration modules.
// ==========================================================================================================================================================================================================================================═

#include "types.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace pml {

// ---- Type checks for arithmetic ----------------------------------------------------------------------------------------─

/// Return true if any argument is a double (prompts float promotion).
inline bool has_float(const std::vector<Value>& args) noexcept {
    for (const auto& v : args) {
        if (v.is_double()) return true;
    }
    return false;
}

/// Convert a numeric Value to double. Assumes is_number(v) is true.
inline double to_double(const Value& v) {
    return v.to_double();
}

/// @deprecated Use to_double() instead. Kept for compatibility.
inline double value_to_double(const Value& v) {
    return to_double(v);
}

/// Convert a numeric Value to int (for dimensions). Assumes is_number(v).
inline int value_to_int(const Value& v) {
    return static_cast<int>(v.to_double());
}

/// Convert a numeric Value to int64_t. Assumes is_number(v) is true.
inline int64_t to_int64(const Value& v) {
    return static_cast<int64_t>(v.to_double());
}

// ---- Arity checking helpers ------------------------------------------------------------------------------------------------─

/// Expect exactly N args.
inline Result<void> expect_arity(size_t expected, const std::vector<Value>& args,
                                  std::string_view name) {
    if (args.size() != expected) {
        return std::unexpected(arity_error(
            SourceLocation{},
            static_cast<int>(expected),
            static_cast<int>(args.size())));
    }
    return {};
}

/// Expect at least min_args args.
inline Result<void> expect_min_arity(size_t min_args, const std::vector<Value>& args,
                                     std::string_view name) {
    if (args.size() < min_args) {
        return std::unexpected(arity_error(
            SourceLocation{},
            static_cast<int>(min_args),
            static_cast<int>(args.size())));
    }
    return {};
}

/// Expect N or M args.
inline Result<void> expect_arity_one_of(const std::vector<size_t>& allowed,
                                         const std::vector<Value>& args,
                                         std::string_view name) {
    for (size_t a : allowed) {
        if (args.size() == a) return {};
    }
    std::string allowed_str;
    for (size_t i = 0; i < allowed.size(); ++i) {
        if (i > 0) allowed_str += " or ";
        allowed_str += std::to_string(allowed[i]);
    }
    return std::unexpected(general_error(
        std::format("{}: expected {} argument(s), got {}", name, allowed_str,
                     args.size())));
}

// ---- typeof helper --------------------------------------------------------------------------------------------------------------------

/// Return the PML type name as a Symbol for a runtime value.
inline Symbol typeof_value(const Value& v) {
    if (v.is_nil()) return Symbol("nil");
    if (v.is_int()) return Symbol("integer");
    if (v.is_double()) return Symbol("float");
    if (v.is_string()) return Symbol("string");
    if (v.is_bool()) return Symbol("boolean");
    if (v.is_symbol()) return Symbol("symbol");
    if (v.is_keyword()) return Symbol("keyword");
    if (v.is_list()) return Symbol("list");
    if (v.is_hash()) return Symbol("hash");
    if (v.is_vector()) return Symbol("vector");
    if (v.is_procedure() || v.is_builtin()) return Symbol("procedure");
    if (v.is_macro()) return Symbol("macro");
    if (v.is_module()) return Symbol("module");
    if (v.is_graphic_object()) return Symbol("graphic-object");
    if (v.is_canvas()) return Symbol("canvas");
    if (v.is_transform()) return Symbol("matrix");
    if (v.is_animation()) return Symbol("animation");
    if (v.is_skeleton_template() || v.is_skeleton_instance())
        return Symbol("skeleton");
    if (v.is_style()) return Symbol("style");
    if (v.is_palette()) return Symbol("palette");
    if (v.is_timeline()) return Symbol("timeline");
    if (v.is_layer()) return Symbol("layer");
    if (v.is_composition()) return Symbol("composition");
    if (v.is_image_filter()) return Symbol("filter");
    return Symbol("unknown");
}

// ---- Structural equality (deep compare) ------------------------------------------------------------------------─

/// Recursive structural equality check.
inline bool deep_equal(const Value& a, const Value& b) {
    if (a.tag() != b.tag()) return false;
    if (a.is_hash()) {
        const auto* ha = a.as_hash();
        const auto* hb = b.as_hash();
        if (!ha || !hb || !*ha || !*hb) return false;
        const auto& da = (*ha)->data;
        const auto& db = (*hb)->data;
        if (da.size() != db.size()) return false;
        for (const auto& [k, v] : da) {
            auto it = db.find(k);
            if (it == db.end() || !deep_equal(v, it->second)) return false;
        }
        return true;
    }
    if (a.is_vector()) {
        const auto* va = a.as_vector();
        const auto* vb = b.as_vector();
        if (!va || !vb || !*va || !*vb) return false;
        const auto& ea = (*va)->elements;
        const auto& eb = (*vb)->elements;
        if (ea.size() != eb.size()) return false;
        for (size_t i = 0; i < ea.size(); ++i) {
            if (!deep_equal(ea[i], eb[i])) return false;
        }
        return true;
    }
    if (!a.is_list()) return a == b;

    const auto* la = a.as_list();
    const auto* lb = b.as_list();
    if (!la || !lb) return false;
    if (!*la || !*lb) return !*la && !*lb;
    if ((*la)->elements.size() != (*lb)->elements.size()) return false;
    for (size_t i = 0; i < (*la)->elements.size(); ++i) {
        if (!deep_equal((*la)->elements[i], (*lb)->elements[i])) return false;
    }
    return true;
}

// ---- Numeric type promotion for arithmetic --------------------------------------------------------------------

/// Result of promoting mixed int/float args for arithmetic.
struct PromotedArgs {
    bool is_float;
    std::vector<double> floats;   // populated when is_float = true
    std::vector<int64_t> ints;    // populated when is_float = false
};

/// Promote all args to the widest type (int → double if any arg is float).
/// Returns an error if any arg is non-numeric.
inline Result<PromotedArgs> promote_numeric(const std::vector<Value>& args) {
    PromotedArgs result{false, {}, {}};
    if (args.empty()) return result;

    // Check for floats first
    for (const auto& v : args) {
        if (!is_number(v)) {
            return std::unexpected(type_error(
                "expected numeric argument"));
        }
        if (v.is_double()) {
            result.is_float = true;
        }
    }

    if (result.is_float) {
        result.floats.reserve(args.size());
        for (const auto& v : args) {
            result.floats.push_back(to_double(v));
        }
    } else {
        result.ints.reserve(args.size());
        for (const auto& v : args) {
            result.ints.push_back(to_int64(v));
        }
    }

    return result;
}

// ---- Builtin registration lambda factory ------------------------------------------------------------------------

/// Creates a def lambda for registering builtins in an environment.
/// Usage: auto def = make_def(env);  def("name", fn);
inline auto make_def(std::shared_ptr<Environment> env) {
    return [env = std::move(env)](const std::string& name,
                                   BuiltinProcedure::BuiltinFn fn) {
        auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn));
        env->define(name, Value(proc));
    };
}

}  // namespace pml
