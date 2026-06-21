// ═══════════════════════════════════════════════════════════════════════════════
// PML Built-in Procedures — Aggregation
// ───────────────────────────────────────────────────────────────────────────────
// Thin file that delegates each builtin category to its own registration
// function.  Add new categories by including the header and calling the
// registration function here.
// ═══════════════════════════════════════════════════════════════════════════════

#include "builtins.h"
#include "evaluator.h"

#include <algorithm>

// ── Sorting ─────────────────────────────────────────────────────────────────
// Register sort builtin (needs evaluator + trampoline, kept here for now).
// TODO: extract to builtins_sort.cpp if sort grows more variants.

namespace pml {

void register_builtins(std::shared_ptr<Environment> env) {
    register_arithmetic_builtins(env);
    register_io_builtins(env);
    register_list_builtins(env);
    register_string_builtins(env);
    register_predicates_builtins(env);
    register_container_builtins(env);

    // ── sort ──────────────────────────────────────────────────────────────
    // Needs trampoline access from evaluator; registered inline for now.
    auto def = [&](const std::string& name, BuiltinProcedure::BuiltinFn fn) {
        auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn));
        env->define(name, Value(proc));
    };

    def("sort", [](const std::vector<Value>& args, Environment& env) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[0].as_list();
        if (!lst || !*lst) {
            return std::unexpected(type_error("sort: first argument must be a list"));
        }
        std::vector<Value> elements = (*lst)->elements;
        std::sort(elements.begin(), elements.end(),
            [&](const Value& a, const Value& b) {
                auto r = trampoline(apply_function(args[1], {a, b}, {}, env.shared_from_this()));
                return r.has_value() && is_truthy(*r);
            });
        return Value(std::make_shared<ValueList>(std::move(elements)));
    });
}

void register_builtins(Environment& env) {
    register_builtins(env.shared_from_this());
}

}  // namespace pml
