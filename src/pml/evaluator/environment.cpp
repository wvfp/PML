// ==========================================================================================================================================================================================================================================═
// PML Environment — Lexical Scope Chain Implementation
// ==========================================================================================================================================================================================================================================═

#include "environment.h"

#include <format>
#include <utility>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// lookup — search outward through parent scopes
// ==========================================================================================================================================================================================================================================═

Result<Value> Environment::lookup(const std::string& name) const noexcept {
    // Fast path: use a previously resolved lexical address.
    auto cache_it = varref_cache_.find(name);
    if (cache_it != varref_cache_.end()) {
        return lookup(cache_it->second);
    }

    // Resolve and cache the lexical address for subsequent lookups.
    VarRef ref;
    if (auto err = resolve_varref(name, ref)) {
        return std::unexpected(*err);
    }
    varref_cache_[name] = ref;
    return lookup(ref);
}

Result<Value> Environment::lookup(VarRef ref) const noexcept {
    const Environment* env = this;
    for (size_t i = 0; i < ref.depth; ++i) {
        if (!env->parent) {
            return std::unexpected(PMLException{
                ErrorCode::UnboundVariableError,
                std::nullopt,
                "lookup: lexical address out of range",
                std::nullopt
            });
        }
        env = env->parent.get();
    }
    if (ref.index >= env->indexed_values_.size()) {
        return std::unexpected(PMLException{
            ErrorCode::UnboundVariableError,
            std::nullopt,
            "lookup: lexical index out of range",
            std::nullopt
        });
    }
    return env->indexed_values_[ref.index];
}

// ==========================================================================================================================================================================================================================================═
// resolve_varref — walk parent scopes and compute (depth, index)
// ==========================================================================================================================================================================================================================================═

std::optional<PMLException> Environment::resolve_varref(
    const std::string& name, VarRef& out) const noexcept {
    const Environment* env = this;
    size_t depth = 0;
    while (env) {
        auto it = env->name_to_index_.find(name);
        if (it != env->name_to_index_.end()) {
            out = VarRef{depth, it->second};
            return std::nullopt;
        }
        env = env->parent.get();
        ++depth;
    }
    return PMLException{
        ErrorCode::UnboundVariableError,
        std::nullopt,
        std::format("unbound variable: {}", name),
        std::nullopt
    };
}

// ==========================================================================================================================================================================================================================================═
// define — bind in the current scope (overwrites existing local binding)
// ==========================================================================================================================================================================================================================================═

void Environment::define(const std::string& name, const Value& value) {
    auto it = name_to_index_.find(name);
    if (it != name_to_index_.end()) {
        // Shadowing / redefining in the same scope: update value, keep index.
        bindings[name] = value;
        indexed_values_[it->second] = value;
    } else {
        // New binding: append to indexed storage.
        size_t idx = indexed_values_.size();
        indexed_values_.push_back(value);
        name_to_index_[name] = idx;
        bindings[name] = value;
    }
    // A previous cache entry from an outer scope is now shadowed.
    varref_cache_.erase(name);
}

// ==========================================================================================================================================================================================================================================═
// set — mutate an existing binding, searching outward
// ==========================================================================================================================================================================================================================================═

Result<void> Environment::set(const std::string& name, const Value& value) {
    auto it = name_to_index_.find(name);
    if (it != name_to_index_.end()) {
        bindings[name] = value;
        indexed_values_[it->second] = value;
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

// ==========================================================================================================================================================================================================================================═
// extend — create a child environment with new bindings (for function calls)
// ==========================================================================================================================================================================================================================================═

std::shared_ptr<Environment> Environment::extend(
    const std::vector<std::string>& names,
    const std::vector<Value>& values) {
    auto child = std::make_shared<Environment>(shared_from_this());
    const size_t count = std::min(names.size(), values.size());
    for (size_t i = 0; i < count; ++i) {
        child->define(names[i], values[i]);
    }
    return child;
}

// ==========================================================================================================================================================================================================================================═
// try_lookup — safe lookup returning nullopt instead of error
// ==========================================================================================================================================================================================================================================═

std::optional<Value> Environment::try_lookup(const std::string& name) const noexcept {
    VarRef ref;
    if (auto err = resolve_varref(name, ref)) {
        (void)err;
        return std::nullopt;
    }
    auto result = lookup(ref);
    if (!result) return std::nullopt;
    return *result;
}

// ==========================================================================================================================================================================================================================================═
// has — check if name exists in current scope only
// ==========================================================================================================================================================================================================================================═

bool Environment::has(const std::string& name) const noexcept {
    return name_to_index_.find(name) != name_to_index_.end();
}

// ==========================================================================================================================================================================================================================================═
// current_bindings — access current scope's bindings (debugging)
// ==========================================================================================================================================================================================================================================═

const std::unordered_map<std::string, Value>& Environment::current_bindings() const noexcept {
    return bindings;
}

// ==========================================================================================================================================================================================================================================═
// rebuild_index — sync dense storage with the bindings map
// ==========================================================================================================================================================================================================================================═

void Environment::rebuild_index() {
    indexed_values_.clear();
    name_to_index_.clear();
    indexed_values_.reserve(bindings.size());
    for (const auto& [name, value] : bindings) {
        size_t idx = indexed_values_.size();
        indexed_values_.push_back(value);
        name_to_index_[name] = idx;
    }
}

// ==========================================================================================================================================================================================================================================═
// operator<< — stream output for debugging
// ==========================================================================================================================================================================================================================================═

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
