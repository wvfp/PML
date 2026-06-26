#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Environment — Lexical Scope Chain
// ==========================================================================================================================================================================================================================================═

#include "types.h"
#include "error.h"

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pml {

/// A lexical environment mapping symbol names to runtime values,
/// with parent pointer for lexical scoping (search outward).
///
/// Matches Python PML's Environment semantics:
///   - `lookup`  searches outward through parent scopes
///   - `define`  binds in the current scope (shadows outer)
///   - `set`     mutates an existing binding, searching outward
///   - `extend`  creates a child frame for function calls
///
/// Additionally, this environment supports lexical-indexed lookup: once a
/// variable's lexical address (depth, index) has been resolved, subsequent
/// lookups use O(1) array access instead of hashing the name on every access.
class Environment : public std::enable_shared_from_this<Environment> {
public:
    /// A resolved lexical address: `depth` frames outward, `index` slot in that
    /// frame's indexed storage.
    struct VarRef {
        size_t depth = 0;
        size_t index = 0;
    };

    /// The parent environment (nullptr for the root/global scope).
    std::shared_ptr<Environment> parent;

    /// Bindings in this scope only.
    std::unordered_map<std::string, Value> bindings;

    /// Names exported via `provide` (for module system).
    std::unordered_set<std::string> exports;

    /// Construct an environment with an optional parent and initial bindings.
    explicit Environment(
        std::shared_ptr<Environment> parent = nullptr,
        std::unordered_map<std::string, Value> bindings = {})
        : parent(std::move(parent))
        , bindings(std::move(bindings)) {
        rebuild_index();
    }

    /// Look up a symbol, searching outward through parent scopes.
    /// Returns an UnboundVariableError if not found anywhere.
    [[nodiscard]] Result<Value> lookup(const std::string& name) const noexcept;

    /// O(1) lookup using a previously resolved lexical address.
    [[nodiscard]] Result<Value> lookup(VarRef ref) const noexcept;

    /// Bind a name to a value in the current scope (shadows any outer binding).
    void define(const std::string& name, const Value& value);

    /// Mutate an existing binding, searching outward through parent scopes.
    /// Returns an UnboundVariableError if the name is not bound anywhere.
    [[nodiscard]] Result<void> set(const std::string& name, const Value& value);

    /// Create a child environment with new bindings (for function calls).
    /// The child's parent is `this` (via shared_from_this).
    [[nodiscard]] std::shared_ptr<Environment> extend(
        const std::vector<std::string>& names,
        const std::vector<Value>& values);

    /// Look up a symbol, returning std::nullopt instead of an error.
    [[nodiscard]] std::optional<Value> try_lookup(const std::string& name) const noexcept;

    /// Check if name is bound in the current scope only (no parent search).
    [[nodiscard]] bool has(const std::string& name) const noexcept;

    /// Access current scope's bindings (for debugging/inspection).
    [[nodiscard]] const std::unordered_map<std::string, Value>& current_bindings() const noexcept;

    /// Stream output: `<Environment bindings=[name1, name2, ...]>`
    friend std::ostream& operator<<(std::ostream& os, const Environment& env);

private:
    /// Dense, index-addressable storage for this scope's values. Kept in sync
    /// with `bindings` so that a resolved VarRef can access values in O(1).
    std::vector<Value> indexed_values_;

    /// Map from name to its index in `indexed_values_` for this scope.
    std::unordered_map<std::string, size_t> name_to_index_;

    /// Cache of resolved lexical addresses. Mutable because lookup() is const.
    mutable std::unordered_map<std::string, VarRef> varref_cache_;

    /// Rebuild `indexed_values_` and `name_to_index_` from `bindings`.
    void rebuild_index();

    /// Resolve a name to its lexical address, walking parent scopes.
    /// Returns nullptr on success and writes to `out`; returns an error result
    /// if the name is unbound.
    [[nodiscard]] std::optional<PMLException> resolve_varref(
        const std::string& name, VarRef& out) const noexcept;
};

}  // namespace pml
