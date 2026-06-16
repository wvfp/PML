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
#include "types.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <iostream>
#include <memory>
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
        if (std::holds_alternative<double>(v)) return true;
    }
    return false;
}

/// Convert a numeric Value to double. Assumes is_number(v) is true.
double to_double(const Value& v) {
    if (const auto* i = std::get_if<int64_t>(&v)) return static_cast<double>(*i);
    if (const auto* d = std::get_if<double>(&v)) return *d;
    return 0.0;  // unreachable for valid numeric values
}

/// Convert a numeric Value to int64_t. Assumes is_number(v) is true.
int64_t to_int64(const Value& v) {
    if (const auto* i = std::get_if<int64_t>(&v)) return *i;
    if (const auto* d = std::get_if<double>(&v)) return static_cast<int64_t>(*d);
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
    return std::visit(
        [](const auto& arg) -> Symbol {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                return Symbol("nil");
            } else if constexpr (std::is_same_v<T, int64_t>) {
                return Symbol("integer");
            } else if constexpr (std::is_same_v<T, double>) {
                return Symbol("float");
            } else if constexpr (std::is_same_v<T, std::string>) {
                return Symbol("string");
            } else if constexpr (std::is_same_v<T, bool>) {
                return Symbol("boolean");
            } else if constexpr (std::is_same_v<T, Symbol>) {
                return Symbol("symbol");
            } else if constexpr (std::is_same_v<T, Keyword>) {
                return Symbol("keyword");
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<ValueList>>) {
                return Symbol("list");
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<Procedure>>) {
                return Symbol("procedure");
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<BuiltinProcedure>>) {
                return Symbol("procedure");
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<Macro>>) {
                return Symbol("macro");
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<Module>>) {
                return Symbol("module");
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<GraphicObject>>) {
                return Symbol("graphic-object");
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<Canvas>>) {
                return Symbol("canvas");
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<AffineTransform>>) {
                return Symbol("matrix");
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<Animation>>) {
                return Symbol("animation");
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<SkeletonInstance>>) {
                return Symbol("skeleton");
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<StyleDescriptor>>) {
                return Symbol("style");
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<Palette>>) {
                return Symbol("palette");
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<Timeline>>) {
                return Symbol("timeline");
            } else {
                return Symbol("unknown");
            }
        },
        v);
}

// ── Structural equality (deep compare) ─────────────────────────────────────

/// Recursive structural equality check.
bool deep_equal(const Value& a, const Value& b) {
    // Different variant alternatives → not equal
    if (a.index() != b.index()) return false;

    return std::visit(
        [&](const auto& arg) -> bool {
            using T = std::decay_t<decltype(arg)>;

            const auto& other = std::get<T>(b);

            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                return true;  // both nil
            } else if constexpr (std::is_same_v<T, int64_t>) {
                return arg == other;
            } else if constexpr (std::is_same_v<T, double>) {
                return arg == other;
            } else if constexpr (std::is_same_v<T, std::string>) {
                return arg == other;
            } else if constexpr (std::is_same_v<T, bool>) {
                return arg == other;
            } else if constexpr (std::is_same_v<T, Symbol>) {
                return arg.name == other.name;
            } else if constexpr (std::is_same_v<T, Keyword>) {
                return arg.name == other.name;
            } else if constexpr (std::is_same_v<T,
                                      std::shared_ptr<ValueList>>) {
                // Both are shared_ptr<ValueList> — compare element by element
                if (!arg && !other) return true;
                if (!arg || !other) return false;
                if (arg->elements.size() != other->elements.size()) return false;
                for (size_t i = 0; i < arg->elements.size(); ++i) {
                    if (!deep_equal(arg->elements[i], other->elements[i])) {
                        return false;
                    }
                }
                return true;
            } else {
                // For all shared_ptr complex types: reference equality
                return arg == other;
            }
        },
        a);
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
        if (std::holds_alternative<double>(v)) {
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

    def("abs", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_number(args[0])) {
            return std::unexpected(type_error("abs expected numeric argument"));
        }
        if (std::holds_alternative<double>(args[0])) {
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
        // Reference equality: same variant alternative AND same value/pointer
        const Value& first = args[0];
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i].index() != first.index()) return Value(false);
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
        const auto* lst = std::get_if<std::shared_ptr<ValueList>>(&args[0]);
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
        const auto* lst = std::get_if<std::shared_ptr<ValueList>>(&args[0]);
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
        if (const auto* lst = std::get_if<std::shared_ptr<ValueList>>(&b)) {
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
        const auto* lst = std::get_if<std::shared_ptr<ValueList>>(&args[0]);
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("length: expected list"));
        }
        return Value(static_cast<int64_t>((*lst)->elements.size()));
    });

    def("append", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        std::vector<Value> result;
        for (const auto& arg : args) {
            const auto* lst = std::get_if<std::shared_ptr<ValueList>>(&arg);
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
        const auto* lst = std::get_if<std::shared_ptr<ValueList>>(&args[0]);
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
        const auto* lst = std::get_if<std::shared_ptr<ValueList>>(&args[0]);
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

    def("null?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        // nil or empty list
        if (is_nil(args[0])) return Value(true);
        if (const auto* lst = std::get_if<std::shared_ptr<ValueList>>(&args[0])) {
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
                const auto* s = std::get_if<std::string>(&arg);
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
            const auto* s = std::get_if<std::string>(&args[0]);
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
            const auto* s = std::get_if<std::string>(&args[0]);
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

    def("string-ref",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 2) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 2, static_cast<int>(args.size())));
            }
            const auto* s = std::get_if<std::string>(&args[0]);
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
            if (std::holds_alternative<double>(args[0])) {
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
            const auto* s = std::get_if<std::string>(&args[0]);
            if (!s) {
                return std::unexpected(
                    type_error("string->number: expected string"));
            }
            try {
                // Check for decimal point or exponent → parse as double
                if (s->find('.') != std::string::npos ||
                    s->find('e') != std::string::npos ||
                    s->find('E') != std::string::npos) {
                    return Value(std::stod(*s));
                }
                // Otherwise parse as integer
                return Value(static_cast<int64_t>(std::stoll(*s)));
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
            const auto* fmt = std::get_if<std::string>(&args[0]);
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
        if (const auto* lst = std::get_if<std::shared_ptr<ValueList>>(&args[0])) {
            if (*lst && !(*lst)->elements.empty()) return Value(true);
        }
        return Value(false);
    });
}

void register_builtins(Environment& env) {
    register_builtins(env.shared_from_this());
}

}  // namespace pml
