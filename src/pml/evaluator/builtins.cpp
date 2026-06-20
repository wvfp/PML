// ═══════════════════════════════════════════════════════════════════════════════
// PML Built-in Procedures — Implementation
// ───────────────────────────────────────────────────────────────────────────────
// All builtin groups in one compilation unit:
//   Arithmetic | Comparison | IO | List | String | Type predicates
//
// Every builtin is a std::function<Result<Value>(const std::vector<Value>&,
// Environment&)> wrapped in a BuiltinProcedure and registered via
// Environment::define().
// ═══════════════════════════════════════════════════════════════════════════════

#include "builtins.h"

#include "error.h"
#include "evaluator.h"
#include "module_loader.h"
#include "types.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Internal helpers (anonymous namespace)
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

// ── Type checks for arithmetic ─────────────────────────────────────────────

/// Return true if any argument is a double (prompts float promotion).
bool has_float(const std::vector<Value>& args) noexcept {
    for (const auto& v : args) {
        if (v.is_double()) return true;
    }
    return false;
}

/// Convert a numeric Value to double. Assumes is_number(v) is true.
double to_double(const Value& v) {
    if (v.is_int()) return static_cast<double>(v.int_val());
    if (v.is_double()) return v.double_val();
    return 0.0;  // unreachable for valid numeric values
}

/// Convert a numeric Value to int64_t. Assumes is_number(v) is true.
int64_t to_int64(const Value& v) {
    if (v.is_int()) return v.int_val();
    if (v.is_double()) return static_cast<int64_t>(v.double_val());
    return 0;  // unreachable for valid numeric values
}

// ── Arity checking helpers ─────────────────────────────────────────────────

/// Expect exactly N args.
Result<void> expect_arity(size_t expected, const std::vector<Value>& args,
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
Result<void> expect_min_arity(size_t min_args, const std::vector<Value>& args,
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
Result<void> expect_arity_one_of(const std::vector<size_t>& allowed,
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

// ── typeof helper ──────────────────────────────────────────────────────────

/// Return the PML type name as a Symbol for a runtime value.
Symbol typeof_value(const Value& v) {
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

// ── Structural equality (deep compare) ─────────────────────────────────────

/// Recursive structural equality check.
bool deep_equal(const Value& a, const Value& b) {
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

// ── Numeric type promotion for arithmetic ──────────────────────────────────

/// Result of promoting mixed int/float args for arithmetic.
struct PromotedArgs {
    bool is_float;
    std::vector<double> floats;   // populated when is_float = true
    std::vector<int64_t> ints;    // populated when is_float = false
};

/// Promote all args to the widest type (int → double if any arg is float).
/// Returns an error if any arg is non-numeric.
Result<PromotedArgs> promote_numeric(const std::vector<Value>& args) {
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

}  // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// register_builtins — register all built-in procedures
// ═══════════════════════════════════════════════════════════════════════════════

void register_builtins(std::shared_ptr<Environment> env) {
    // ── Helper to register a builtin ────────────────────────────────────
    auto def = [&](const std::string& name, BuiltinProcedure::BuiltinFn fn) {
        auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn));
        env->define(name, Value(proc));
    };

    // ═══════════════════════════════════════════════════════════════════════
    // Arithmetic
    // ═══════════════════════════════════════════════════════════════════════

    def("+", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.empty()) return Value(static_cast<int64_t>(0));
        auto promoted = promote_numeric(args);
        if (!promoted) return std::unexpected(promoted.error());
        if (promoted->is_float) {
            double sum = 0.0;
            for (double d : promoted->floats) sum += d;
            return Value(sum);
        } else {
            int64_t sum = 0;
            for (int64_t i : promoted->ints) sum += i;
            return Value(sum);
        }
    });

    def("-", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.empty()) {
            return std::unexpected(
                type_error("- requires at least 1 argument"));
        }
        auto promoted = promote_numeric(args);
        if (!promoted) return std::unexpected(promoted.error());
        if (promoted->is_float) {
            double result = promoted->floats[0];
            if (promoted->floats.size() == 1) {
                return Value(-result);  // negation
            }
            for (size_t i = 1; i < promoted->floats.size(); ++i) {
                result -= promoted->floats[i];
            }
            return Value(result);
        } else {
            int64_t result = promoted->ints[0];
            if (promoted->ints.size() == 1) {
                return Value(-result);  // negation
            }
            for (size_t i = 1; i < promoted->ints.size(); ++i) {
                result -= promoted->ints[i];
            }
            return Value(result);
        }
    });

    def("*", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.empty()) return Value(static_cast<int64_t>(1));
        auto promoted = promote_numeric(args);
        if (!promoted) return std::unexpected(promoted.error());
        if (promoted->is_float) {
            double product = 1.0;
            for (double d : promoted->floats) product *= d;
            return Value(product);
        } else {
            int64_t product = 1;
            for (int64_t i : promoted->ints) product *= i;
            return Value(product);
        }
    });

    def("/", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.empty()) {
            return std::unexpected(
                type_error("/ requires at least 1 argument"));
        }
        // Division always returns double (matches Python PML behavior)
        std::vector<double> doubles;
        doubles.reserve(args.size());
        for (const auto& v : args) {
            if (!is_number(v)) {
                return std::unexpected(type_error("/ expected numeric argument"));
            }
            doubles.push_back(to_double(v));
        }
        if (doubles.size() == 1) {
            return Value(1.0 / doubles[0]);  // reciprocal
        }
        double result = doubles[0];
        for (size_t i = 1; i < doubles.size(); ++i) {
            result /= doubles[i];
        }
        return Value(result);
    });

    def("%", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        if (!is_number(args[0]) || !is_number(args[1])) {
            return std::unexpected(type_error("% expected numeric arguments"));
        }
        if (has_float(args)) {
            double a = to_double(args[0]);
            double b = to_double(args[1]);
            return Value(std::fmod(a, b));
        } else {
            int64_t a = to_int64(args[0]);
            int64_t b = to_int64(args[1]);
            if (b == 0) {
                return std::unexpected(
                    general_error("%: division by zero"));
            }
            return Value(a % b);
        }
    });

    def("modulo", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        if (!is_number(args[0]) || !is_number(args[1])) {
            return std::unexpected(type_error("modulo expected numeric arguments"));
        }
        if (has_float(args)) {
            double a = to_double(args[0]);
            double b = to_double(args[1]);
            if (b == 0) {
                return std::unexpected(general_error("modulo: division by zero"));
            }
            double r = std::fmod(a, b);
            if (r != 0 && ((r < 0) != (b < 0))) {
                r += b;
            }
            return Value(r);
        } else {
            int64_t a = to_int64(args[0]);
            int64_t b = to_int64(args[1]);
            if (b == 0) {
                return std::unexpected(general_error("modulo: division by zero"));
            }
            int64_t r = a % b;
            if (r != 0 && ((r < 0) != (b < 0))) {
                r += b;
            }
            return Value(r);
        }
    });

    def("remainder", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        if (!is_number(args[0]) || !is_number(args[1])) {
            return std::unexpected(type_error("remainder expected numeric arguments"));
        }
        if (has_float(args)) {
            double a = to_double(args[0]);
            double b = to_double(args[1]);
            if (b == 0) {
                return std::unexpected(general_error("remainder: division by zero"));
            }
            return Value(std::fmod(a, b));
        } else {
            int64_t a = to_int64(args[0]);
            int64_t b = to_int64(args[1]);
            if (b == 0) {
                return std::unexpected(general_error("remainder: division by zero"));
            }
            return Value(a % b);
        }
    });

    def("quotient", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        if (!is_number(args[0]) || !is_number(args[1])) {
            return std::unexpected(type_error("quotient expected numeric arguments"));
        }
        double a = to_double(args[0]);
        double b = to_double(args[1]);
        if (b == 0) {
            return std::unexpected(general_error("quotient: division by zero"));
        }
        return Value(static_cast<int64_t>(a / b));
    });

    def("abs", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("abs expected numeric argument"));
        }
        if (args[0].is_double()) {
            return Value(std::abs(to_double(args[0])));
        } else {
            int64_t v = to_int64(args[0]);
            return Value(v < 0 ? -v : v);
        }
    });

    def("min", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.empty()) {
            return std::unexpected(type_error("min requires at least 1 argument"));
        }
        auto promoted = promote_numeric(args);
        if (!promoted) return std::unexpected(promoted.error());
        if (promoted->is_float) {
            double result = promoted->floats[0];
            for (double d : promoted->floats) result = std::min(result, d);
            return Value(result);
        } else {
            int64_t result = promoted->ints[0];
            for (int64_t i : promoted->ints) result = std::min(result, i);
            return Value(result);
        }
    });

    def("max", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.empty()) {
            return std::unexpected(type_error("max requires at least 1 argument"));
        }
        auto promoted = promote_numeric(args);
        if (!promoted) return std::unexpected(promoted.error());
        if (promoted->is_float) {
            double result = promoted->floats[0];
            for (double d : promoted->floats) result = std::max(result, d);
            return Value(result);
        } else {
            int64_t result = promoted->ints[0];
            for (int64_t i : promoted->ints) result = std::max(result, i);
            return Value(result);
        }
    });

    def("floor", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("floor expected numeric argument"));
        }
        return Value(std::floor(to_double(args[0])));
    });

    def("ceil", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("ceil expected numeric argument"));
        }
        return Value(std::ceil(to_double(args[0])));
    });

    def("round", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("round expected numeric argument"));
        }
        return Value(std::round(to_double(args[0])));
    });

    def("even?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("even?: expected numeric argument"));
        }
        int64_t v = to_int64(args[0]);
        return Value((v % 2) == 0);
    });

    def("odd?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("odd?: expected numeric argument"));
        }
        int64_t v = to_int64(args[0]);
        return Value((v % 2) != 0);
    });

    def("zero?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("zero?: expected numeric argument"));
        }
        return Value(to_double(args[0]) == 0.0);
    });

    def("positive?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("positive?: expected numeric argument"));
        }
        return Value(to_double(args[0]) > 0.0);
    });

    def("negative?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("negative?: expected numeric argument"));
        }
        return Value(to_double(args[0]) < 0.0);
    });

    def("sqrt", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("sqrt expected numeric argument"));
        }
        return Value(std::sqrt(to_double(args[0])));
    });

    def("pow", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        if (!is_number(args[0]) || !is_number(args[1])) {
            return std::unexpected(type_error("pow expected numeric arguments"));
        }
        return Value(std::pow(to_double(args[0]), to_double(args[1])));
    });

    def("expt", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        if (!is_number(args[0]) || !is_number(args[1])) {
            return std::unexpected(type_error("expt expected numeric arguments"));
        }
        return Value(std::pow(to_double(args[0]), to_double(args[1])));
    });

    def("sin", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("sin expected numeric argument"));
        }
        return Value(std::sin(to_double(args[0])));
    });

    def("cos", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("cos expected numeric argument"));
        }
        return Value(std::cos(to_double(args[0])));
    });

    def("tan", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("tan expected numeric argument"));
        }
        return Value(std::tan(to_double(args[0])));
    });

    def("atan2", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        if (!is_number(args[0]) || !is_number(args[1])) {
            return std::unexpected(type_error("atan2 expected numeric arguments"));
        }
        return Value(std::atan2(to_double(args[0]), to_double(args[1])));
    });

    // ═══════════════════════════════════════════════════════════════════════
    // Comparison
    // ═══════════════════════════════════════════════════════════════════════

    def("=", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() < 2) {
            return std::unexpected(
                type_error("= requires at least 2 arguments"));
        }
        // All numeric — promote if any float
        auto promoted = promote_numeric(args);
        if (!promoted) return std::unexpected(promoted.error());
        if (promoted->is_float) {
            double ref = promoted->floats[0];
            for (size_t i = 1; i < promoted->floats.size(); ++i) {
                if (promoted->floats[i] != ref) return Value(false);
            }
        } else {
            int64_t ref = promoted->ints[0];
            for (size_t i = 1; i < promoted->ints.size(); ++i) {
                if (promoted->ints[i] != ref) return Value(false);
            }
        }
        return Value(true);
    });

    def("<", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() < 2) {
            return std::unexpected(
                type_error("< requires at least 2 arguments"));
        }
        auto promoted = promote_numeric(args);
        if (!promoted) return std::unexpected(promoted.error());
        if (promoted->is_float) {
            for (size_t i = 1; i < promoted->floats.size(); ++i) {
                if (!(promoted->floats[i - 1] < promoted->floats[i]))
                    return Value(false);
            }
        } else {
            for (size_t i = 1; i < promoted->ints.size(); ++i) {
                if (!(promoted->ints[i - 1] < promoted->ints[i]))
                    return Value(false);
            }
        }
        return Value(true);
    });

    def(">", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() < 2) {
            return std::unexpected(
                type_error("> requires at least 2 arguments"));
        }
        auto promoted = promote_numeric(args);
        if (!promoted) return std::unexpected(promoted.error());
        if (promoted->is_float) {
            for (size_t i = 1; i < promoted->floats.size(); ++i) {
                if (!(promoted->floats[i - 1] > promoted->floats[i]))
                    return Value(false);
            }
        } else {
            for (size_t i = 1; i < promoted->ints.size(); ++i) {
                if (!(promoted->ints[i - 1] > promoted->ints[i]))
                    return Value(false);
            }
        }
        return Value(true);
    });

    def("<=", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() < 2) {
            return std::unexpected(
                type_error("<= requires at least 2 arguments"));
        }
        auto promoted = promote_numeric(args);
        if (!promoted) return std::unexpected(promoted.error());
        if (promoted->is_float) {
            for (size_t i = 1; i < promoted->floats.size(); ++i) {
                if (!(promoted->floats[i - 1] <= promoted->floats[i]))
                    return Value(false);
            }
        } else {
            for (size_t i = 1; i < promoted->ints.size(); ++i) {
                if (!(promoted->ints[i - 1] <= promoted->ints[i]))
                    return Value(false);
            }
        }
        return Value(true);
    });

    def(">=", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() < 2) {
            return std::unexpected(
                type_error(">= requires at least 2 arguments"));
        }
        auto promoted = promote_numeric(args);
        if (!promoted) return std::unexpected(promoted.error());
        if (promoted->is_float) {
            for (size_t i = 1; i < promoted->floats.size(); ++i) {
                if (!(promoted->floats[i - 1] >= promoted->floats[i]))
                    return Value(false);
            }
        } else {
            for (size_t i = 1; i < promoted->ints.size(); ++i) {
                if (!(promoted->ints[i - 1] >= promoted->ints[i]))
                    return Value(false);
            }
        }
        return Value(true);
    });

    def("eq?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() < 2) {
            return std::unexpected(
                type_error("eq? requires at least 2 arguments"));
        }
        // Reference equality: same tag AND same value/pointer
        const Value& first = args[0];
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i].tag() != first.tag()) return Value(false);
            // Compare the values directly
            if (first != args[i]) return Value(false);
        }
        return Value(true);
    });

    def("equal?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() < 2) {
            return std::unexpected(
                type_error("equal? requires at least 2 arguments"));
        }
        const Value& first = args[0];
        for (size_t i = 1; i < args.size(); ++i) {
            if (!deep_equal(first, args[i])) return Value(false);
        }
        return Value(true);
    });

    def("not", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        return Value(!is_truthy(args[0]));
    });

    // ═══════════════════════════════════════════════════════════════════════
    // IO / Debugging
    // ═══════════════════════════════════════════════════════════════════════

    def("print", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) std::cout << ' ';
            std::cout << value_to_string(args[i]);
        }
        std::cout << std::flush;
        return make_nil_value();
    });

    def("println", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) std::cout << ' ';
            std::cout << value_to_string(args[i]);
        }
        std::cout << '\n' << std::flush;
        return make_nil_value();
    });

    def("error", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        std::string msg = value_to_string(args[0]);
        return std::unexpected(general_error(std::move(msg)));
    });

    def("typeof", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        return Value(typeof_value(args[0]));
    });

    def("assert", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() < 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_truthy(args[0])) {
            std::string msg = "Assertion failed";
            if (args.size() >= 2) {
                msg = value_to_string(args[1]);
            }
            return std::unexpected(assertion_error(std::move(msg)));
        }
        return make_nil_value();
    });

    // ═══════════════════════════════════════════════════════════════════════
    // List operations
    // ═══════════════════════════════════════════════════════════════════════

    def("car", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        const auto* lst = args[0].as_list();
        if (!lst || !*lst || (*lst)->elements.empty()) {
            return std::unexpected(
                type_error("car: expected non-empty list"));
        }
        return (*lst)->elements[0];
    });

    def("cdr", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        const auto* lst = args[0].as_list();
        if (!lst || !*lst || (*lst)->elements.empty()) {
            return std::unexpected(
                type_error("cdr: expected non-empty list"));
        }
        // Return rest as a new list
        std::vector<Value> rest((*lst)->elements.begin() + 1,
                                (*lst)->elements.end());
        return make_list_value(std::move(rest));
    });

    def("cons", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const Value& a = args[0];
        const Value& b = args[1];

        // If second arg is a list, prepend a
        if (const auto* lst = b.as_list()) {
            if (*lst) {
                std::vector<Value> result;
                result.reserve(1 + (*lst)->elements.size());
                result.push_back(a);
                result.insert(result.end(),
                              (*lst)->elements.begin(),
                              (*lst)->elements.end());
                return make_list_value(std::move(result));
            }
        }
        // Otherwise make a pair (list of two elements)
        return make_list_value({a, b});
    });

    def("list", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        return make_list_value(args);
    });

    def("length", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        const auto* lst = args[0].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("length: expected list"));
        }
        return Value(static_cast<int64_t>((*lst)->elements.size()));
    });

    def("append", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        std::vector<Value> result;
        for (const auto& arg : args) {
            const auto* lst = arg.as_list();
            if (!lst || !*lst) {
                return std::unexpected(
                    type_error("append: expected list arguments"));
            }
            result.insert(result.end(),
                          (*lst)->elements.begin(),
                          (*lst)->elements.end());
        }
        return make_list_value(std::move(result));
    });

    def("reverse", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        const auto* lst = args[0].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("reverse: expected list"));
        }
        auto reversed = (*lst)->elements;
        std::reverse(reversed.begin(), reversed.end());
        return make_list_value(std::move(reversed));
    });

    def("nth", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[0].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("nth: first argument must be list"));
        }
        int64_t index = to_int64(args[1]);
        if (index < 0 || static_cast<size_t>(index) >= (*lst)->elements.size()) {
            return std::unexpected(
                general_error(std::format("nth: index {} out of range for list of "
                                           "length {}", index,
                                           (*lst)->elements.size())));
        }
        return (*lst)->elements[static_cast<size_t>(index)];
    });

    def("list-ref", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[0].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("list-ref: first argument must be list"));
        }
        int64_t index = to_int64(args[1]);
        if (index < 0 || static_cast<size_t>(index) >= (*lst)->elements.size()) {
            return std::unexpected(
                general_error(std::format("list-ref: index {} out of range for list of "
                                           "length {}", index,
                                           (*lst)->elements.size())));
        }
        return (*lst)->elements[static_cast<size_t>(index)];
    });

    def("list-tail", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[0].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("list-tail: first argument must be list"));
        }
        int64_t index = to_int64(args[1]);
        if (index < 0 || static_cast<size_t>(index) > (*lst)->elements.size()) {
            return std::unexpected(
                general_error(std::format("list-tail: index {} out of range for list of "
                                           "length {}", index,
                                           (*lst)->elements.size())));
        }
        std::vector<Value> tail((*lst)->elements.begin() + static_cast<size_t>(index),
                                (*lst)->elements.end());
        return make_list_value(std::move(tail));
    });

    def("member", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[1].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("member: second argument must be list"));
        }
        const auto& elems = (*lst)->elements;
        for (size_t i = 0; i < elems.size(); ++i) {
            if (deep_equal(elems[i], args[0])) {
                std::vector<Value> tail(elems.begin() + i, elems.end());
                return make_list_value(std::move(tail));
            }
        }
        return Value(false);
    });

    def("assoc", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[1].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("assoc: second argument must be list"));
        }
        for (const auto& item : (*lst)->elements) {
            const auto* pair = item.as_list();
            if (pair && *pair && !(*pair)->elements.empty()) {
                if (deep_equal((*pair)->elements[0], args[0])) {
                    return item;
                }
            }
        }
        return Value(false);
    });

    // ── memq, assq ─────────────────────────────────────────────────────

    def("memq", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[1].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("memq: second argument must be list"));
        }
        for (size_t i = 0; i < (*lst)->elements.size(); ++i) {
            if ((*lst)->elements[i] == args[0]) {
                // Return the tail starting from the matching element
                std::vector<Value> tail;
                tail.reserve((*lst)->elements.size() - i);
                for (size_t j = i; j < (*lst)->elements.size(); ++j) {
                    tail.push_back((*lst)->elements[j]);
                }
                return make_list_value(std::move(tail));
            }
        }
        return Value(false);
    });

    def("assq", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[1].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("assq: second argument must be list"));
        }
        for (const auto& item : (*lst)->elements) {
            const auto* pair = item.as_list();
            if (pair && *pair && !(*pair)->elements.empty()) {
                if ((*pair)->elements[0] == args[0]) {
                    return item;
                }
            }
        }
        return Value(false);
    });

    def("range", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() < 2 || args.size() > 3) {
            return std::unexpected(
                type_error("range: expects 2 or 3 arguments (start end [step])"));
        }
        int64_t start = to_int64(args[0]);
        int64_t end = to_int64(args[1]);
        int64_t step = (args.size() == 3) ? to_int64(args[2]) : 1;

        if (step == 0) {
            return std::unexpected(
                general_error("range: step cannot be zero"));
        }

        std::vector<Value> result;
        if (step > 0) {
            for (int64_t i = start; i < end; i += step) {
                result.push_back(Value(i));
            }
        } else {
            for (int64_t i = start; i > end; i += step) {
                result.push_back(Value(i));
            }
        }
        return make_list_value(std::move(result));
    });

    // Higher-order list functions (need evaluator access to apply fn)
    def("map", [](const std::vector<Value>& args, Environment& env) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[1].as_list();
        if (!lst || !*lst) {
            return std::unexpected(type_error("map: second argument must be a list"));
        }
        std::vector<Value> out;
        out.reserve((*lst)->elements.size());
        for (const auto& item : (*lst)->elements) {
            auto r = trampoline(apply_function(args[0], {item}, {}, env.shared_from_this()));
            if (!r) return std::unexpected(std::move(r.error()));
            out.push_back(std::move(*r));
        }
        return make_list_value(std::move(out));
    });

    def("filter", [](const std::vector<Value>& args, Environment& env) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[1].as_list();
        if (!lst || !*lst) {
            return std::unexpected(type_error("filter: second argument must be a list"));
        }
        std::vector<Value> out;
        for (const auto& item : (*lst)->elements) {
            auto r = trampoline(apply_function(args[0], {item}, {}, env.shared_from_this()));
            if (!r) return std::unexpected(std::move(r.error()));
            if (is_truthy(*r)) out.push_back(item);
        }
        return make_list_value(std::move(out));
    });

    def("reduce", [](const std::vector<Value>& args, Environment& env) -> Result<Value> {
        if (args.size() != 3) {
            return std::unexpected(
                arity_error(SourceLocation{}, 3, static_cast<int>(args.size())));
        }
        const auto* lst = args[2].as_list();
        if (!lst || !*lst) {
            return std::unexpected(type_error("reduce: third argument must be a list"));
        }
        Value acc = args[1];
        for (const auto& item : (*lst)->elements) {
            auto r = trampoline(apply_function(args[0], {acc, item}, {}, env.shared_from_this()));
            if (!r) return std::unexpected(std::move(r.error()));
            acc = std::move(*r);
        }
        return acc;
    });

    def("for-each", [](const std::vector<Value>& args, Environment& env) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[1].as_list();
        if (!lst || !*lst) {
            return std::unexpected(type_error("for-each: second argument must be a list"));
        }
        for (const auto& item : (*lst)->elements) {
            auto r = trampoline(apply_function(args[0], {item}, {}, env.shared_from_this()));
            if (!r) return std::unexpected(std::move(r.error()));
        }
        return make_nil_value();
    });

    def("apply", [](const std::vector<Value>& args, Environment& env) -> Result<Value> {
        if (args.size() < 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        std::vector<Value> call_args;
        for (size_t i = 1; i + 1 < args.size(); ++i) {
            call_args.push_back(args[i]);
        }
        const auto* lst = args.back().as_list();
        if (!lst || !*lst) {
            return std::unexpected(type_error("apply: last argument must be a list"));
        }
        call_args.insert(call_args.end(), (*lst)->elements.begin(), (*lst)->elements.end());
        return trampoline(apply_function(args[0], call_args, {}, env.shared_from_this()));
    });

    def("null?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        // nil or empty list
        if (is_nil(args[0])) return Value(true);
        if (const auto* lst = args[0].as_list()) {
            if (*lst && (*lst)->elements.empty()) return Value(true);
        }
        return Value(false);
    });

    // list? is also registered below in type predicates
    // (single define for both)

    // pair? is registered below in type predicates

    // ═══════════════════════════════════════════════════════════════════════
    // String operations
    // ═══════════════════════════════════════════════════════════════════════

    def("string-append",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            std::string result;
            for (const auto& arg : args) {
                const auto* s = arg.as_string();
                if (!s) {
                    return std::unexpected(
                        type_error("string-append: expected string arguments"));
                }
                result += *s;
            }
            return Value(std::move(result));
        });

    def("string-length",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* s = args[0].as_string();
            if (!s) {
                return std::unexpected(
                    type_error("string-length: expected string"));
            }
            return Value(static_cast<int64_t>(s->size()));
        });

    def("substring",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 3) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 3, static_cast<int>(args.size())));
            }
            const auto* s = args[0].as_string();
            if (!s) {
                return std::unexpected(
                    type_error("substring: expected string"));
            }
            int64_t start = to_int64(args[1]);
            int64_t end = to_int64(args[2]);

            // Clamp indices
            if (start < 0) start = 0;
            if (end > static_cast<int64_t>(s->size())) end = static_cast<int64_t>(s->size());
            if (start >= end) return Value(std::string(""));

            return Value(s->substr(static_cast<size_t>(start),
                                    static_cast<size_t>(end - start)));
        });

    // ── string-copy, make-string ──────────────────────────────

    def("string-copy", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        const auto* s = args[0].as_string();
        if (!s) {
            return std::unexpected(
                type_error("string-copy: expected string"));
        }
        return Value(*s);  // return a copy
    });

    def("make-string", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() < 1 || args.size() > 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_integer(args[0]) || to_int64(args[0]) < 0) {
            return std::unexpected(
                type_error("make-string: expected non-negative integer"));
        }
        int64_t len = to_int64(args[0]);
        char fill = ' ';
        if (args.size() == 2) {
            const auto* s = args[1].as_string();
            if (!s || s->size() != 1) {
                return std::unexpected(
                    type_error("make-string: fill must be a single character string"));
            }
            fill = (*s)[0];
        }
        return Value(std::string(static_cast<size_t>(len), fill));
    });

    def("string-ref",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 2) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 2, static_cast<int>(args.size())));
            }
            const auto* s = args[0].as_string();
            if (!s) {
                return std::unexpected(
                    type_error("string-ref: expected string"));
            }
            int64_t index = to_int64(args[1]);
            if (index < 0 || static_cast<size_t>(index) >= s->size()) {
                return std::unexpected(
                    general_error(std::format("string-ref: index {} out of range "
                                               "for string of length {}",
                                               index, s->size())));
            }
            return Value(std::string(1, (*s)[static_cast<size_t>(index)]));
        });

    def("number->string",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            if (!is_number(args[0])) {
                return std::unexpected(
                    type_error("number->string: expected numeric argument"));
            }
            if (args[0].is_double()) {
                // Use value_to_string's formatting (removes trailing zeros)
                return Value(value_to_string(args[0]));
            }
            return Value(std::to_string(to_int64(args[0])));
        });

    def("string->number",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* s = args[0].as_string();
            if (!s) {
                return std::unexpected(
                    type_error("string->number: expected string"));
            }
            try {
                size_t idx = 0;
                // Check for decimal point or exponent → parse as double
                if (s->find('.') != std::string::npos ||
                    s->find('e') != std::string::npos ||
                    s->find('E') != std::string::npos) {
                    double val = std::stod(*s, &idx);
                    if (idx != s->size()) {
                        return std::unexpected(type_error(
                            std::format("string->number: cannot convert '{}'",
                                        *s)));
                    }
                    return Value(val);
                }
                // Otherwise parse as integer
                int64_t val = std::stoll(*s, &idx);
                if (idx != s->size()) {
                    return std::unexpected(type_error(
                        std::format("string->number: cannot convert '{}'",
                                    *s)));
                }
                return Value(val);
            } catch (const std::exception&) {
                return std::unexpected(
                    type_error(std::format("string->number: cannot convert '{}'",
                                            *s)));
            }
        });

    def("format",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.empty()) {
                return std::unexpected(
                    type_error("format: expected at least 1 argument"));
            }
            const auto* fmt = args[0].as_string();
            if (!fmt) {
                return std::unexpected(
                    type_error("format: first argument must be string"));
            }
            std::string result = *fmt;
            for (size_t i = 1; i < args.size(); ++i) {
                size_t pos = result.find("~a");
                if (pos == std::string::npos) break;
                result.replace(pos, 2, value_to_string(args[i]));
            }
            return Value(std::move(result));
        });

    def("string->symbol",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* s = args[0].as_string();
            if (!s) {
                return std::unexpected(
                    type_error("string->symbol: expected string"));
            }
            return Value(Symbol(*s));
        });

    def("symbol->string",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* sym = args[0].as_symbol();
            if (!sym) {
                return std::unexpected(
                    type_error("symbol->string: expected symbol"));
            }
            return Value(std::string(sym->name));
        });

    def("string->list",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* s = args[0].as_string();
            if (!s) {
                return std::unexpected(
                    type_error("string->list: expected string"));
            }
            std::vector<Value> chars;
            for (char c : *s) {
                chars.push_back(Value(std::string(1, c)));
            }
            return make_list_value(std::move(chars));
        });

    def("list->string",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* lst = args[0].as_list();
            if (!lst || !*lst) {
                return std::unexpected(
                    type_error("list->string: expected list of strings"));
            }
            std::string result;
            for (const auto& v : (*lst)->elements) {
                const auto* s = v.as_string();
                if (!s) {
                    return std::unexpected(
                        type_error("list->string: list elements must be strings"));
                }
                result += *s;
            }
            return Value(std::move(result));
        });

    def("string=?",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() < 2) {
                return std::unexpected(
                    type_error("string=? requires at least 2 arguments"));
            }
            const auto* first = args[0].as_string();
            if (!first) {
                return std::unexpected(
                    type_error("string=?: expected string arguments"));
            }
            for (size_t i = 1; i < args.size(); ++i) {
                const auto* s = args[i].as_string();
                if (!s || *s != *first) return Value(false);
            }
            return Value(true);
        });

    def("string<?",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() < 2) {
                return std::unexpected(
                    type_error("string<? requires at least 2 arguments"));
            }
            std::vector<const std::string*> strs;
            for (const auto& arg : args) {
                const auto* s = arg.as_string();
                if (!s) {
                    return std::unexpected(
                        type_error("string<?: expected string arguments"));
                }
                strs.push_back(s);
            }
            for (size_t i = 1; i < strs.size(); ++i) {
                if (!(*strs[i - 1] < *strs[i])) return Value(false);
            }
            return Value(true);
        });

    def("string>?",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() < 2) {
                return std::unexpected(
                    type_error("string>? requires at least 2 arguments"));
            }
            std::vector<const std::string*> strs;
            for (const auto& arg : args) {
                const auto* s = arg.as_string();
                if (!s) {
                    return std::unexpected(
                        type_error("string>?: expected string arguments"));
                }
                strs.push_back(s);
            }
            for (size_t i = 1; i < strs.size(); ++i) {
                if (!(*strs[i - 1] > *strs[i])) return Value(false);
            }
            return Value(true);
        });

    def("string<=?",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() < 2) {
                return std::unexpected(
                    type_error("string<=? requires at least 2 arguments"));
            }
            std::vector<const std::string*> strs;
            for (const auto& arg : args) {
                const auto* s = arg.as_string();
                if (!s) {
                    return std::unexpected(
                        type_error("string<=?: expected string arguments"));
                }
                strs.push_back(s);
            }
            for (size_t i = 1; i < strs.size(); ++i) {
                if (!(*strs[i - 1] <= *strs[i])) return Value(false);
            }
            return Value(true);
        });

    def("string>=?",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() < 2) {
                return std::unexpected(
                    type_error("string>=? requires at least 2 arguments"));
            }
            std::vector<const std::string*> strs;
            for (const auto& arg : args) {
                const auto* s = arg.as_string();
                if (!s) {
                    return std::unexpected(
                        type_error("string>=?: expected string arguments"));
                }
                strs.push_back(s);
            }
            for (size_t i = 1; i < strs.size(); ++i) {
                if (!(*strs[i - 1] >= *strs[i])) return Value(false);
            }
            return Value(true);
        });

    // ═══════════════════════════════════════════════════════════════════════
    // Type predicates
    // ═══════════════════════════════════════════════════════════════════════

    def("number?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        return Value(is_number(args[0]));
    });

    def("integer?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        return Value(is_integer(args[0]));
    });

    def("float?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        return Value(is_float(args[0]));
    });

    def("string?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        return Value(is_string(args[0]));
    });

    def("symbol?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        return Value(is_symbol(args[0]));
    });

    def("boolean?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        return Value(is_bool(args[0]));
    });

    def("procedure?",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            return Value(is_procedure(args[0]) || is_builtin(args[0]));
        });

    def("keyword?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        return Value(is_keyword(args[0]));
    });

    def("list?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        // nil is NOT a list in PML (though it's an empty list in some Schemes)
        // In the Python PML, list? returns true for actual list values
        return Value(is_list(args[0]));
    });

    // null? is already registered in list ops above

    def("pair?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        // A pair is a non-empty list
        if (const auto* lst = args[0].as_list()) {
            if (*lst && !(*lst)->elements.empty()) return Value(true);
        }
        return Value(false);
    });

    // ═══════════════════════════════════════════════════════════════════════
    // Hash Tables
    // ═══════════════════════════════════════════════════════════════════════

    def("make-hash",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            auto hash = std::make_shared<ValueHashMap>();
            if (!args.empty()) {
                if (args.size() != 1) {
                    return std::unexpected(arity_error(
                        SourceLocation{}, 1, static_cast<int>(args.size())));
                }
                const auto* lst = args[0].as_list();
                if (!lst || !*lst) {
                    return std::unexpected(type_error(
                        "make-hash: expected list of key-value pairs"));
                }
                for (const auto& pair : (*lst)->elements) {
                    const auto* pair_lst = pair.as_list();
                    if (!pair_lst || !*pair_lst ||
                        (*pair_lst)->elements.size() != 2) {
                        return std::unexpected(type_error(
                            "make-hash: each entry must be a key-value pair"));
                    }
                    const auto* key = (*pair_lst)->elements[0].as_string();
                    if (!key) {
                        return std::unexpected(type_error(
                            "make-hash: key must be a string"));
                    }
                    hash->data[*key] = (*pair_lst)->elements[1];
                }
            }
            return Value(hash);
        });

    def("hash-ref",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 2 && args.size() != 3) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 2, static_cast<int>(args.size())));
            }
            const auto* hash = args[0].as_hash();
            if (!hash || !*hash) {
                return std::unexpected(
                    type_error("hash-ref: expected hash table"));
            }
            const auto* key = args[1].as_string();
            if (!key) {
                return std::unexpected(
                    type_error("hash-ref: key must be a string"));
            }
            auto it = (*hash)->data.find(*key);
            if (it == (*hash)->data.end()) {
                if (args.size() == 3) return args[2];
                return std::unexpected(access_error(
                    std::format("hash-ref: key not found: '{}'", *key)));
            }
            return it->second;
        });

    def("hash-set!",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 3) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 3, static_cast<int>(args.size())));
            }
            const auto* hash = args[0].as_hash();
            if (!hash || !*hash) {
                return std::unexpected(
                    type_error("hash-set!: expected hash table"));
            }
            const auto* key = args[1].as_string();
            if (!key) {
                return std::unexpected(
                    type_error("hash-set!: key must be a string"));
            }
            (*hash)->data[*key] = args[2];
            return Value(true);
        });

    def("hash-delete!",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 2) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 2, static_cast<int>(args.size())));
            }
            const auto* hash = args[0].as_hash();
            if (!hash || !*hash) {
                return std::unexpected(
                    type_error("hash-delete!: expected hash table"));
            }
            const auto* key = args[1].as_string();
            if (!key) {
                return std::unexpected(
                    type_error("hash-delete!: key must be a string"));
            }
            (*hash)->data.erase(*key);
            return Value(true);
        });

    def("hash-keys",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* hash = args[0].as_hash();
            if (!hash || !*hash) {
                return std::unexpected(
                    type_error("hash-keys: expected hash table"));
            }
            std::vector<Value> keys;
            keys.reserve((*hash)->data.size());
            for (const auto& [k, _] : (*hash)->data) {
                keys.emplace_back(k);
            }
            return Value(std::make_shared<ValueList>(std::move(keys)));
        });

    def("hash-values",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* hash = args[0].as_hash();
            if (!hash || !*hash) {
                return std::unexpected(
                    type_error("hash-values: expected hash table"));
            }
            std::vector<Value> values;
            values.reserve((*hash)->data.size());
            for (const auto& [_, v] : (*hash)->data) {
                values.push_back(v);
            }
            return Value(std::make_shared<ValueList>(std::move(values)));
        });

    def("hash?",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            return Value(is_hash(args[0]));
        });

    // ═══════════════════════════════════════════════════════════════════════
    // Vectors
    // ═══════════════════════════════════════════════════════════════════════

    def("make-vector",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1 && args.size() != 2) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 2, static_cast<int>(args.size())));
            }
            if (!args[0].is_int()) {
                return std::unexpected(type_error(
                    "make-vector: size must be an integer"));
            }
            size_t n = static_cast<size_t>(args[0].int_val());
            Value init = args.size() == 2 ? args[1] : Value();
            return Value(std::make_shared<ValueVector>(n, init));
        });

    def("vector-ref",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 2) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 2, static_cast<int>(args.size())));
            }
            const auto* vec = args[0].as_vector();
            if (!vec || !*vec) {
                return std::unexpected(
                    type_error("vector-ref: expected vector"));
            }
            if (!args[1].is_int()) {
                return std::unexpected(type_error(
                    "vector-ref: index must be an integer"));
            }
            size_t idx = static_cast<size_t>(args[1].int_val());
            if (idx >= (*vec)->elements.size()) {
                return std::unexpected(general_error(
                    std::format("vector-ref: index {} out of bounds", idx)));
            }
            return (*vec)->elements[idx];
        });

    def("vector-set!",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 3) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 3, static_cast<int>(args.size())));
            }
            const auto* vec = args[0].as_vector();
            if (!vec || !*vec) {
                return std::unexpected(
                    type_error("vector-set!: expected vector"));
            }
            if (!args[1].is_int()) {
                return std::unexpected(type_error(
                    "vector-set!: index must be an integer"));
            }
            size_t idx = static_cast<size_t>(args[1].int_val());
            if (idx >= (*vec)->elements.size()) {
                return std::unexpected(general_error(
                    std::format("vector-set!: index {} out of bounds", idx)));
            }
            (*vec)->elements[idx] = args[2];
            return Value(true);
        });

    def("vector-length",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* vec = args[0].as_vector();
            if (!vec || !*vec) {
                return std::unexpected(type_error(
                    "vector-length: expected vector"));
            }
            return Value(static_cast<int64_t>((*vec)->elements.size()));
        });

    def("vector->list",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* vec = args[0].as_vector();
            if (!vec || !*vec) {
                return std::unexpected(type_error(
                    "vector->list: expected vector"));
            }
            return Value(std::make_shared<ValueList>((*vec)->elements));
        });

    def("list->vector",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* lst = args[0].as_list();
            if (!lst || !*lst) {
                return std::unexpected(
                    type_error("list->vector: expected list"));
            }
            return Value(std::make_shared<ValueVector>((*lst)->elements));
        });

    def("vector?",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            return Value(is_vector(args[0]));
        });

    // ═══════════════════════════════════════════════════════════════════════
    // Module Introspection
    // ═══════════════════════════════════════════════════════════════════════

    auto current_source_file = [](const Environment& env) -> std::string {
        auto v = env.try_lookup("__source_file__");
        if (v && v->is_string()) {
            const auto* s = v->as_string();
            if (s) return *s;
        }
        return "";
    };

    def("module-available?",
        [&current_source_file](const std::vector<Value>& args,
                               Environment& env) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* path = args[0].as_string();
            if (!path) {
                return std::unexpected(
                    type_error("module-available?: expected string path"));
            }
            auto loader = get_global_loader(env.shared_from_this());
            return Value(loader->is_available(*path, current_source_file(env)));
        });

    def("module-list",
        [&current_source_file](const std::vector<Value>& args,
                               Environment& env) -> Result<Value> {
            if (!args.empty()) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 0, static_cast<int>(args.size())));
            }
            auto loader = get_global_loader(env.shared_from_this());
            auto modules = loader->loaded_modules();
            std::vector<Value> names;
            names.reserve(modules.size());
            for (const auto& mod : modules) {
                names.emplace_back(mod->name);
            }
            return Value(std::make_shared<ValueList>(std::move(names)));
        });

    def("module-exports",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* mod = args[0].as_module();
            if (!mod || !*mod) {
                return std::unexpected(
                    type_error("module-exports: expected module"));
            }
            std::vector<std::string> sorted((*mod)->exports.begin(),
                                            (*mod)->exports.end());
            std::sort(sorted.begin(), sorted.end());
            std::vector<Value> symbols;
            symbols.reserve(sorted.size());
            for (const auto& name : sorted) {
                symbols.emplace_back(Symbol(name));
            }
            return Value(std::make_shared<ValueList>(std::move(symbols)));
        });

    // ═══════════════════════════════════════════════════════════════════════
    // Math extensions (gcd, lcm, log, exp, asin, acos, atan)
    // ═══════════════════════════════════════════════════════════════════════

    def("gcd", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.empty()) return Value(static_cast<int64_t>(0));
        int64_t result = 0;
        for (const auto& arg : args) {
            if (!is_integer(arg)) {
                return std::unexpected(type_error("gcd: expected integer arguments"));
            }
            int64_t n = to_int64(arg);
            while (n != 0) {
                int64_t t = n;
                n = result % n;
                result = t;
            }
        }
        return Value(result < 0 ? -result : result);
    });

    def("lcm", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.empty()) return Value(static_cast<int64_t>(1));
        int64_t result = 1;
        for (const auto& arg : args) {
            if (!is_integer(arg)) {
                return std::unexpected(type_error("lcm: expected integer arguments"));
            }
            int64_t n = to_int64(arg);
            if (n == 0) return Value(static_cast<int64_t>(0));
            // gcd of result and n
            int64_t a = result < 0 ? -result : result;
            int64_t b = n < 0 ? -n : n;
            while (b != 0) {
                int64_t t = b;
                b = a % b;
                a = t;
            }
            result = (result / a) * n;
        }
        return Value(result < 0 ? -result : result);
    });

    def("log", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("log: expected numeric argument"));
        }
        return Value(std::log(to_double(args[0])));
    });

    def("exp", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("exp: expected numeric argument"));
        }
        return Value(std::exp(to_double(args[0])));
    });

    def("asin", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("asin: expected numeric argument"));
        }
        return Value(std::asin(to_double(args[0])));
    });

    def("acos", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("acos: expected numeric argument"));
        }
        return Value(std::acos(to_double(args[0])));
    });

    def("atan", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("atan: expected numeric argument"));
        }
        return Value(std::atan(to_double(args[0])));
    });

    def("random", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("random: expected numeric argument"));
        }
        static std::mt19937_64 s_rng(std::random_device{}());
        int64_t limit = to_int64(args[0]);
        if (limit <= 0) {
            return std::unexpected(type_error("random: limit must be positive"));
        }
        if (limit == 1) return Value(static_cast<int64_t>(0));
        std::uniform_int_distribution<int64_t> dist(0, limit - 1);
        return Value(dist(s_rng));
    });

    def("sort", [](const std::vector<Value>& args, Environment& env) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[0].as_list();
        if (!lst || !*lst) {
            return std::unexpected(type_error("sort: first argument must be a list"));
        }
        std::vector<Value> elements = (*lst)->elements;
        // Use the less-than predicate to sort
        std::sort(elements.begin(), elements.end(),
            [&](const Value& a, const Value& b) {
                auto r = trampoline(apply_function(args[1], {a, b}, {}, env.shared_from_this()));
                return r.has_value() && is_truthy(*r);
            });
        return Value(std::make_shared<ValueList>(std::move(elements)));
    });

    // ═══════════════════════════════════════════════════════════════════════
    // Vector extensions (vector-fill!, vector-copy)
    // ═══════════════════════════════════════════════════════════════════════

    def("vector-fill!", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 3) {
            return std::unexpected(arity_error(SourceLocation{}, 3, static_cast<int>(args.size())));
        }
        const auto* vec = args[0].as_vector();
        if (!vec || !*vec) {
            return std::unexpected(type_error("vector-fill!: expected vector"));
        }
        if (!args[1].is_int()) {
            return std::unexpected(type_error("vector-fill!: index must be an integer"));
        }
        size_t idx = static_cast<size_t>(args[1].int_val());
        if (idx >= (*vec)->elements.size()) {
            return std::unexpected(general_error(
                std::format("vector-fill!: index {} out of bounds", idx)));
        }
        (*vec)->elements[idx] = args[2];
        return Value(true);
    });

    def("vector-copy", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() < 1 || args.size() > 3) {
            return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        const auto* vec = args[0].as_vector();
        if (!vec || !*vec) {
            return std::unexpected(type_error("vector-copy: expected vector"));
        }
        size_t start = 0;
        size_t end = (*vec)->elements.size();
        if (args.size() >= 2) {
            if (!args[1].is_int()) {
                return std::unexpected(type_error("vector-copy: start must be an integer"));
            }
            start = static_cast<size_t>(args[1].int_val());
        }
        if (args.size() >= 3) {
            if (!args[2].is_int()) {
                return std::unexpected(type_error("vector-copy: end must be an integer"));
            }
            end = static_cast<size_t>(args[2].int_val());
        }
        if (start > (*vec)->elements.size() || end > (*vec)->elements.size() || start > end) {
            return std::unexpected(general_error("vector-copy: invalid range"));
        }
        std::vector<Value> copied((*vec)->elements.begin() + start,
                                   (*vec)->elements.begin() + end);
        return Value(std::make_shared<ValueVector>(std::move(copied)));
    });
}

void register_builtins(Environment& env) {
    register_builtins(env.shared_from_this());
}

}  // namespace pml
