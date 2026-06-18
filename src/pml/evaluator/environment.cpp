// ═══════════════════════════════════════════════════════════════════════════════
// PML Environment — Lexical Scope Chain Implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "environment.h"

#include <format>
#include <utility>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// lookup — search outward through parent scopes
// ═══════════════════════════════════════════════════════════════════════════════

Result<Value> Environment::lookup(const std::string& name) const noexcept {
    auto it = bindings.find(name);
    if (it != bindings.end()) {
        return it->second;
    }
    if (parent) {
        return parent->lookup(name);
    }
    return std::unexpected(PMLException{
        ErrorCode::UnboundVariableError,
        std::nullopt,
        std::format("unbound variable: {}", name),
        std::nullopt
    });
}

// ═══════════════════════════════════════════════════════════════════════════════
// define — bind in the current scope (overwrites existing local binding)
// ═══════════════════════════════════════════════════════════════════════════════

void Environment::define(const std::string& name, const Value& value) {
    bindings[name] = value;
}

// ═══════════════════════════════════════════════════════════════════════════════
// set — mutate an existing binding, searching outward
// ═══════════════════════════════════════════════════════════════════════════════

Result<void> Environment::set(const std::string& name, const Value& value) {
    auto it = bindings.find(name);
    if (it != bindings.end()) {
        it->second = value;
        return {};
    }
    if (parent) {
        return parent->set(name, value);
    }
    return std::unexpected(PMLException{
        ErrorCode::UnboundVariableError,
        std::nullopt,
        std::format("cannot set! unbound variable: {}", name),
        std::nullopt
    });
}

// ═══════════════════════════════════════════════════════════════════════════════
// extend — create a child environment with new bindings (for function calls)
// ═══════════════════════════════════════════════════════════════════════════════

std::shared_ptr<Environment> Environment::extend(
    const std::vector<std::string>& names,
    const std::vector<Value>& values) {
    auto child = std::make_shared<Environment>(shared_from_this());
    const size_t count = std::min(names.size(), values.size());
    for (size_t i = 0; i < count; ++i) {
        child->bindings[names[i]] = values[i];
    }
    return child;
}

// ═══════════════════════════════════════════════════════════════════════════════
// try_lookup — safe lookup returning nullopt instead of error
// ═══════════════════════════════════════════════════════════════════════════════

std::optional<Value> Environment::try_lookup(const std::string& name) const noexcept {
    auto it = bindings.find(name);
    if (it != bindings.end()) {
        return it->second;
    }
    if (parent) {
        return parent->try_lookup(name);
    }
    return std::nullopt;
}

// ═══════════════════════════════════════════════════════════════════════════════
// has — check if name exists in current scope only
// ═══════════════════════════════════════════════════════════════════════════════

bool Environment::has(const std::string& name) const noexcept {
    return bindings.find(name) != bindings.end();
}

// ═══════════════════════════════════════════════════════════════════════════════
// current_bindings — access current scope's bindings (debugging)
// ═══════════════════════════════════════════════════════════════════════════════

const std::unordered_map<std::string, Value>& Environment::current_bindings() const noexcept {
    return bindings;
}

// ═══════════════════════════════════════════════════════════════════════════════
// operator<< — stream output for debugging
// ═══════════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const Environment& env) {
    os << "<Environment bindings=[";
    bool first = true;
    for (const auto& [name, _] : env.bindings) {
        if (!first) os << ", ";
        os << name;
        first = false;
    }
    os << "]>";
    return os;
}

}  // namespace pml
