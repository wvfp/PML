#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Style Engine — StyleDescriptor + StyleRegistry + Builtins
// ───────────────────────────────────────────────────────────────────────────────
// Ported from pml/sprites/style.py. Three predefined named styles:
//   "cel"   – thick outlines, cel shading, highlights
//   "pixel" – pixel art style, no anti-alias, larger pixel size
//   "flat"  – no outlines, flat shading, no highlights
// ═══════════════════════════════════════════════════════════════════════════════

#include "environment.h"
#include "types.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// StyleDescriptor — Visual Style Parameters
// ═══════════════════════════════════════════════════════════════════════════════

/// Data-only struct holding visual style parameters that propagate to
/// child sprite components.  Matches Python StyleDescriptor exactly.
struct StyleDescriptor {
    float outline_width = 2.0f;
    std::string outline_color = "#000000";
    std::string outline_style = "solid";
    std::string shading = "flat";
    bool shadow = false;
    bool highlight = true;
    int pixel_size = 1;
    bool anti_alias = true;
    float corner_radius = 0.0f;

    /// Convert all fields to a flat keyword-argument map for graphic primitives.
    /// Keys are hyphenated (e.g. "outline-width") matching PML keyword convention.
    [[nodiscard]] std::unordered_map<std::string, Value> to_kwargs() const;

    /// Return a new StyleDescriptor with fields overridden by the kwargs map.
    /// Only keys present in the map are changed; all others keep their current value.
    [[nodiscard]] StyleDescriptor merge(
        const std::unordered_map<std::string, Value>& overrides) const;

    /// Return a new StyleDescriptor with fields overridden by another descriptor.
    /// All non-nil fields in `overrides` replace the corresponding field.
    [[nodiscard]] StyleDescriptor merge(const StyleDescriptor& overrides) const;
};

// ═══════════════════════════════════════════════════════════════════════════════
// StyleRegistry — Global Named Style Registry (Singleton)
// ═══════════════════════════════════════════════════════════════════════════════

/// Singleton registry of named styles.  Pre-populated with "cel", "pixel",
/// and "flat".  User-defined styles are added via `define-style` at the
/// PML level (which calls StyleRegistry::define()).
class StyleRegistry {
public:
    static StyleRegistry& instance();

    /// Define or override a named style.
    void define(const std::string& name, const StyleDescriptor& style);

    /// Retrieve a named style.  Falls back to "cel" if not found.
    StyleDescriptor get(const std::string& name) const;

    /// Check whether a name is registered.
    bool has(const std::string& name) const;

private:
    StyleRegistry();  // singleton — private constructor
    std::unordered_map<std::string, StyleDescriptor> m_styles;
};

// ═══════════════════════════════════════════════════════════════════════════════
// Free functions
// ═══════════════════════════════════════════════════════════════════════════════

/// Resolve a style value (Symbol, string, or shared_ptr<StyleDescriptor>)
/// to a concrete StyleDescriptor.  Falls back to "cel" on unknown names.
StyleDescriptor resolve_style(const Value& style_val);

/// Register `define-style` and `use-style` builtins into the given environment.
void register_style(std::shared_ptr<Environment> env);

}  // namespace pml
