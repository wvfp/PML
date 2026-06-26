// ==========================================================================================================================================================================================================================================═
// PML Built-in Procedures — Type Predicates & Module Introspection
// ==========================================================================================================================================================================================================================================═

#include "builtins.h"
#include "builtins_helpers.h"

#include "evaluator.h"
#include "module_loader.h"

#include <algorithm>

namespace pml {

void register_predicates_builtins(std::shared_ptr<Environment> env) {
    auto def = [&](const std::string& name, BuiltinProcedure::BuiltinFn fn) {
        auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn));
        env->define(name, Value(proc));
    };

    // ==================================================================================================================================================================================================================═
    // Type predicates
    // ==================================================================================================================================================================================================================═

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
    def("proc?",  // alias for procedure?
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

    def("graphic-object?",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            return Value(args[0].is_graphic_object());
        });

    def("list?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        return Value(is_list(args[0]));
    });

    def("pair?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (const auto* lst = args[0].as_list()) {
            if (*lst && !(*lst)->elements.empty()) return Value(true);
        }
        return Value(false);
    });

    // ==================================================================================================================================================================================================================═
    // Module Introspection
    // ==================================================================================================================================================================================================================═

    auto current_source_file = [](const Environment& env) -> std::string {
        auto v = env.try_lookup("__source_file__");
        if (v && v->is_string()) {
            const auto* s = v->as_string();
            if (s) return *s;
        }
        return "";
    };

    def("module-available?",
        [current_source_file](const std::vector<Value>& args,
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
        [current_source_file](const std::vector<Value>& args,
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
}

}  // namespace pml
