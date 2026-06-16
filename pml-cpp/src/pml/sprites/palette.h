#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Palette System
// ───────────────────────────────────────────────────────────────────────────────
// Named color palettes with global registry + active palette.
// Ported from pml/sprites/palette.py.
//
//   - Palette: a named collection of color key→hex mappings
//   - PaletteManager: singleton registry + active palette
//   - register_palette(env): exposes define-palette, palette-ref builtins
// ═══════════════════════════════════════════════════════════════════════════════

#include "environment.h"
#include "types.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Palette — named collection of colors
// ═══════════════════════════════════════════════════════════════════════════════

/// A named collection of color key→hex mappings.
/// Ported from Python PML's Palette dataclass.
/// Lookups that miss fall back to "#808080".
class Palette {
public:
    std::string name;
    std::unordered_map<std::string, std::string> colors;

    Palette(std::string n, std::unordered_map<std::string, std::string> c);

    /// Look up a color by key. Warns on miss and returns "#808080".
    [[nodiscard]] std::string get(const std::string& key) const;
};

// ═══════════════════════════════════════════════════════════════════════════════
// PaletteManager — global palette registry + active palette
// ═══════════════════════════════════════════════════════════════════════════════

/// Singleton managing the global palette registry and active palette.
/// Matches Python's _palette_registry dict + _active_palette list.
class PaletteManager {
public:
    /// Access the singleton instance.
    static PaletteManager& instance();

    /// Register a palette by name (replaces any existing with the same name).
    void define(const std::string& name, std::shared_ptr<Palette> palette);

    /// Retrieve a palette by name, or nullptr if unknown.
    [[nodiscard]] std::shared_ptr<Palette> get(const std::string& name) const;

    /// Set the currently active palette.
    void set_active(std::shared_ptr<Palette> palette);

    /// Get the currently active palette (may be nullptr).
    [[nodiscard]] std::shared_ptr<Palette> active() const;

private:
    PaletteManager();  // singleton — use instance()
    PaletteManager(const PaletteManager&) = delete;
    PaletteManager& operator=(const PaletteManager&) = delete;

    std::unordered_map<std::string, std::shared_ptr<Palette>> m_palettes;
    std::shared_ptr<Palette> m_active;
};

// ═══════════════════════════════════════════════════════════════════════════════
// Builtin registration
// ═══════════════════════════════════════════════════════════════════════════════

/// Register palette-related builtin procedures into the given environment.
/// Defines:
///   (define-palette "name" (list (list "key" "#color") ...)) — define a palette
///   (palette-ref "key") — look up color key in active palette
void register_palette(std::shared_ptr<Environment> env);

}  // namespace pml
