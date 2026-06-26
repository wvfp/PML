// ==========================================================================================================================================================================================================================================═
// PML Built-in Procedures — Arithmetic & Comparison
// ==========================================================================================================================================================================================================================================═

#include "builtins.h"
#include "builtins_helpers.h"
#include "evaluator.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

namespace pml {

void register_arithmetic_builtins(std::shared_ptr<Environment> env) {
    auto def = [&](const std::string& name, BuiltinProcedure::BuiltinFn fn) {
        auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn));
        env->define(name, Value(proc));
    };

    // ==================================================================================================================================================================================================================═
    // Arithmetic
    // ==================================================================================================================================================================================================================═

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
                return Value(-result);
            }
            for (size_t i = 1; i < promoted->floats.size(); ++i) {
                result -= promoted->floats[i];
            }
            return Value(result);
        } else {
            int64_t result = promoted->ints[0];
            if (promoted->ints.size() == 1) {
                return Value(-result);
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
        std::vector<double> doubles;
        doubles.reserve(args.size());
        for (const auto& v : args) {
            if (!is_number(v)) {
                return std::unexpected(type_error("/ expected numeric argument"));
            }
            doubles.push_back(to_double(v));
        }
        if (doubles.size() == 1) {
            return Value(1.0 / doubles[0]);
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

    // ==================================================================================================================================================================================================================═
    // Comparison
    // ==================================================================================================================================================================================================================═

    def("=", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() < 2) {
            return std::unexpected(
                type_error("= requires at least 2 arguments"));
        }
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
        const Value& first = args[0];
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i].tag() != first.tag()) return Value(false);
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

    // ==================================================================================================================================================================================================================═
    // Math extensions (gcd, lcm, log, exp, asin, acos, atan)
    // ==================================================================================================================================================================================================================═

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
}

}  // namespace pml
