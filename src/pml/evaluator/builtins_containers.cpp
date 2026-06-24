// ═══════════════════════════════════════════════════════════════════════════════
// PML Built-in Procedures — Hash Tables & Vectors
// ═══════════════════════════════════════════════════════════════════════════════

#include "builtins.h"
#include "builtins_helpers.h"

#include <cstdint>
#include <format>
#include <string>

namespace pml {

void register_container_builtins(std::shared_ptr<Environment> env) {
    auto def = [&](const std::string& name, BuiltinProcedure::BuiltinFn fn) {
        auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn));
        env->define(name, Value(proc));
    };

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

}  // namespace pml
