#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Environment — Lexical Scope Chain
// ═══════════════════════════════════════════════════════════════════════════════

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
class Environment : public std::enable_shared_from_this<Environment> {
public:
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
        , bindings(std::move(bindings)) {}

    /// Look up a symbol, searching outward through parent scopes.
    /// Returns an UnboundVariableError if not found anywhere.
    [[nodiscard]] Result<Value> lookup(const std::string& name) const noexcept;

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
};

}  // namespace pml
