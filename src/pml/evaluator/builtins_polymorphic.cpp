// ═══════════════════════════════════════════════════════════════════════════════
// PML Polymorphic Access Built-ins (get / set!)
// ───────────────────────────────────────────────────────────────────────────────
// Unified access interface dispatching on container type:
//
//   (get container key)      →  list-ref | vector-ref | hash-ref
//   (set! container key val) →  list-set | vector-set! | hash-set!
//
// The polymorphic dispatch selects the appropriate builtin based on the type of
// the first argument (list, vector, or hash table).
// ═══════════════════════════════════════════════════════════════════════════════

#include "builtins.h"
#include "environment.h"
#include "error.h"
#include "types.h"

#include <format>
#include <string>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// get
// ═══════════════════════════════════════════════════════════════════════════════

static Result<Value> builtin_get(
    const std::vector<Value>& args,
    Environment& env)
{
    if (args.size() != 2) {
        return std::unexpected(
            arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
    }

    // Dispatch based on container type
    if (args[0].is_list()) {
        // (get list index) → list-ref
        const auto* lst = args[0].as_list();
        if (!lst || !*lst || !args[1].is_int()) {
            return std::unexpected(type_error("get: for list, key must be an integer index"));
        }
        size_t idx = static_cast<size_t>(args[1].int_val());
        if (idx >= (*lst)->elements.size()) {
            return std::unexpected(
                general_error(std::format("get: index {} out of range for list of size {}", idx, (*lst)->elements.size())));
        }
        return (*lst)->elements[idx];
    }

    if (args[0].is_vector()) {
        // (get vector index) → vector-ref
        const auto* vec = args[0].as_vector();
        if (!vec || !*vec || !args[1].is_int()) {
            return std::unexpected(type_error("get: for vector, key must be an integer index"));
        }
        size_t idx = static_cast<size_t>(args[1].int_val());
        if (idx >= (*vec)->elements.size()) {
            return std::unexpected(
                general_error(std::format("get: index {} out of bounds for vector of size {}", idx, (*vec)->elements.size())));
        }
        return (*vec)->elements[idx];
    }

    if (args[0].is_hash()) {
        // (get hash key) → hash-ref
        const auto* hash = args[0].as_hash();
        if (!hash || !*hash) {
            return std::unexpected(type_error("get: expected hash table"));
        }
        const auto* key = args[1].as_string();
        if (!key) {
            return std::unexpected(type_error("get: for hash table, key must be a string"));
        }
        auto it = (*hash)->data.find(*key);
        if (it == (*hash)->data.end()) {
            return std::unexpected(
                access_error(std::format("get: key not found in hash table: '{}'", *key)));
        }
        return it->second;
    }

    return std::unexpected(
        type_error("get: expected list, vector, or hash table"));
}

// ═══════════════════════════════════════════════════════════════════════════════
// set!
// ═══════════════════════════════════════════════════════════════════════════════

static Result<Value> builtin_set_bang(
    const std::vector<Value>& args,
    Environment& env)
{
    if (args.size() != 3) {
        return std::unexpected(
            arity_error(SourceLocation{}, 3, static_cast<int>(args.size())));
    }

    // Dispatch based on container type
    if (args[0].is_list()) {
        // (set! list index value) → list-set (returns new list)
        const auto* lst = args[0].as_list();
        if (!lst || !*lst || !args[1].is_int()) {
            return std::unexpected(type_error("set!: for list, key must be an integer index"));
        }
        size_t idx = static_cast<size_t>(args[1].int_val());
        if (idx >= (*lst)->elements.size()) {
            return std::unexpected(
                general_error(std::format("set!: index {} out of range for list of size {}", idx, (*lst)->elements.size())));
        }
        // Create a new list with the element replaced
        auto new_list = std::make_shared<ValueList>((*lst)->elements);
        new_list->elements[idx] = args[2];
        return Value(std::move(new_list));
    }

    if (args[0].is_vector()) {
        // (set! vector index value) → vector-set!
        const auto* vec = args[0].as_vector();
        if (!vec || !*vec || !args[1].is_int()) {
            return std::unexpected(type_error("set!: for vector, key must be an integer index"));
        }
        size_t idx = static_cast<size_t>(args[1].int_val());
        if (idx >= (*vec)->elements.size()) {
            return std::unexpected(
                general_error(std::format("set!: index {} out of bounds for vector of size {}", idx, (*vec)->elements.size())));
        }
        (*vec)->elements[idx] = args[2];
        return Value(true);
    }

    if (args[0].is_hash()) {
        // (set! hash key value) → hash-set!
        const auto* hash = args[0].as_hash();
        if (!hash || !*hash) {
            return std::unexpected(type_error("set!: expected hash table"));
        }
        const auto* key = args[1].as_string();
        if (!key) {
            return std::unexpected(type_error("set!: for hash table, key must be a string"));
        }
        (*hash)->data[*key] = args[2];
        return Value(true);
    }

    return std::unexpected(
        type_error("set!: expected list, vector, or hash table"));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Registration
// ═══════════════════════════════════════════════════════════════════════════════

void register_polymorphic_builtins(std::shared_ptr<Environment> env) {
    auto def = [&](const std::string& name, auto fn) {
        auto proc = std::make_shared<BuiltinProcedure>(name, fn);
        env->define(name, Value(proc));
    };

    def("get",  builtin_get);
    def("set-at!", builtin_set_bang);
}

} // namespace pml
