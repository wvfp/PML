// ═══════════════════════════════════════════════════════════════════════════════
// PML Built-in Procedures — IO / Debugging
// ═══════════════════════════════════════════════════════════════════════════════

#include "builtins.h"
#include "builtins_helpers.h"
#include "evaluator.h"

#include <iostream>

namespace pml {

void register_io_builtins(std::shared_ptr<Environment> env) {
    auto def = [&](const std::string& name, BuiltinProcedure::BuiltinFn fn) {
        auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn));
        env->define(name, Value(proc));
    };

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
}

}  // namespace pml
