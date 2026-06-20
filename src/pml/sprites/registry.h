#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Component Registry
// ───────────────────────────────────────────────────────────────────────────────
// Ported from pml/sprites/registry.py.
// Central registry for semantic sprite components (body, head, eyes, etc.).
// Each component is a factory function that returns a GraphicObject.
// ═══════════════════════════════════════════════════════════════════════════════

#include "environment.h"
#include "pml/core/types.h"
#include "validator.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// ComponentFactory — signature for sprite component constructors
// ═══════════════════════════════════════════════════════════════════════════════

/// A sprite component factory receives cleaned keyword arguments and returns
/// a GraphicObject wrapped in a shared_ptr.
using ComponentFactory = std::function<Value(
    const std::unordered_map<std::string, Value>& kwargs)>;

// ═══════════════════════════════════════════════════════════════════════════════
// ComponentRegistry — singleton registry
// ═══════════════════════════════════════════════════════════════════════════════

/// Singleton registry of named sprite components with optional schemas and
/// categories.
class ComponentRegistry {
public:
    /// Access the singleton instance.
    static ComponentRegistry& instance();

    /// Register a component.
    void register_(const std::string& name,
                   ComponentFactory factory,
                   std::optional<ParamSchema> schema = std::nullopt,
                   std::optional<std::string> category = std::nullopt);

    /// Create a component by name.
    [[nodiscard]] Value create(
        const std::string& name,
        const std::unordered_map<std::string, Value>& kwargs) const;

    /// Check whether a component is registered.
    [[nodiscard]] bool has(const std::string& name) const;

    /// List all registered component names.
    [[nodiscard]] std::vector<std::string> list_components() const;

    /// Return the schema for a component, or nullopt if none.
    [[nodiscard]] std::optional<ParamSchema> get_schema(
        const std::string& name) const;

    /// Return the category for a component, or nullopt if none.
    [[nodiscard]] std::optional<std::string> get_category(
        const std::string& name) const;

    /// List components in a category.
    [[nodiscard]] std::vector<std::string> list_by_category(
        const std::string& category) const;

    /// List all categories.
    [[nodiscard]] std::vector<std::string> categories() const;

private:
    ComponentRegistry() = default;
    ComponentRegistry(const ComponentRegistry&) = delete;
    ComponentRegistry& operator=(const ComponentRegistry&) = delete;

    struct Entry {
        ComponentFactory factory;
        std::optional<ParamSchema> schema;
        std::optional<std::string> category;
    };

    std::unordered_map<std::string, Entry> m_components;
};

// ═══════════════════════════════════════════════════════════════════════════════
// Builtin registration
// ═══════════════════════════════════════════════════════════════════════════════

/// Register all sprite component builtins into the environment.
void register_components(std::shared_ptr<Environment> env);

}  // namespace pml
