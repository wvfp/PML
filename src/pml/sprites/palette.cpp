// ═══════════════════════════════════════════════════════════════════════════════
// PML Palette System — Implementation
// ───────────────────────────────────────────────────────────────────────────────
// Ported from pml/sprites/palette.py.
//
//   - Predefined palettes ("dark-hero", "warm-skin") registered at startup
//   - PaletteManager singleton for global registry + active palette
//   - define-palette and palette-ref builtins registered via register_palette()
// ═══════════════════════════════════════════════════════════════════════════════

#include "palette.h"

#include "error.h"
#include "types.h"

#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Palette
// ═══════════════════════════════════════════════════════════════════════════════

Palette::Palette(std::string n, std::unordered_map<std::string, std::string> c)
    : name(std::move(n))
    , colors(std::move(c)) {}

std::string Palette::get(const std::string& key) const {
    auto it = colors.find(key);
    if (it == colors.end()) {
        std::cerr << "Warning: palette '" << name << "' has no color '" << key << "', using #808080"
                  << std::endl;
        return "#808080";
    }
    return it->second;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Predefined palettes (matching pml/sprites/palette.py exactly)
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

std::unordered_map<std::string, std::shared_ptr<Palette>> make_predefined_palettes() {
    std::unordered_map<std::string, std::shared_ptr<Palette>> palettes;

    palettes["dark-hero"] = std::make_shared<Palette>("dark-hero",
                                                      std::unordered_map<std::string, std::string>{
                                                          {"hair", "#2c2c2c"},
                                                          {"skin", "#fce4c8"},
                                                          {"skin-shadow", "#d4a574"},
                                                          {"eyes", "#4a90d9"},
                                                          {"outfit-primary", "#1a1a2e"},
                                                          {"outfit-secondary", "#16213e"},
                                                          {"outline", "#0a0a0a"},
                                                          {"highlight", "#ffffff"},
                                                      });

    palettes["warm-skin"] = std::make_shared<Palette>("warm-skin",
                                                      std::unordered_map<std::string, std::string>{
                                                          {"skin", "#f5d0a9"},
                                                          {"skin-shadow", "#d4a574"},
                                                          {"skin-highlight", "#ffe8cc"},
                                                      });

    return palettes;
}

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// PaletteManager — global singleton
// ═══════════════════════════════════════════════════════════════════════════════

PaletteManager& PaletteManager::instance() {
    auto& ctx = PMLContext::current();
    if (!ctx.palettes) {
        ctx.palettes = std::make_unique<PaletteManager>();
    }
    return *ctx.palettes;
}

PaletteManager::PaletteManager()
    : m_palettes(make_predefined_palettes())
    , m_active(nullptr) {}

void PaletteManager::define(const std::string& name, std::shared_ptr<Palette> palette) {
    m_palettes[name] = std::move(palette);
}

std::shared_ptr<Palette> PaletteManager::get(const std::string& name) const {
    auto it = m_palettes.find(name);
    if (it == m_palettes.end()) {
        std::cerr << "Warning: unknown palette '" << name << "', returning nullptr" << std::endl;
        return nullptr;
    }
    return it->second;
}

void PaletteManager::set_active(std::shared_ptr<Palette> palette) {
    m_active = std::move(palette);
}

std::shared_ptr<Palette> PaletteManager::active() const {
    return m_active;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Builtin: define-palette
// ═══════════════════════════════════════════════════════════════════════════════

/// (define-palette "name" (list (list "key" "#color") ...))
///
/// Registers a new palette in the global registry.
/// The first argument is the palette name (string).
/// The second argument is a list of [key, color] pairs.
static Result<Value> builtin_define_palette(const std::vector<Value>& args, Environment&) {

    if (args.size() != 2) {
        return std::unexpected(arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
    }

    // Extract palette name
    const auto* name_str = args[0].as_string();
    if (!name_str) {
        return std::unexpected(type_error("define-palette: first argument must be a string"));
    }
    std::string palette_name = *name_str;

    // Extract color list
    const auto* color_list = args[1].as_list();
    if (!color_list || !*color_list) {
        return std::unexpected(type_error("define-palette: second argument must be a list"));
    }

    std::unordered_map<std::string, std::string> colors;
    for (const auto& pair_val : (*color_list)->elements) {
        const auto* pair_list = pair_val.as_list();
        if (!pair_list || !*pair_list || (*pair_list)->elements.size() < 2) {
            return std::unexpected(type_error("define-palette: each entry must be a list of "
                                              "(key color)"));
        }
        const auto& elements = (*pair_list)->elements;

        const auto* key = elements[0].as_string();
        const auto* color = elements[1].as_string();
        if (!key || !color) {
            return std::unexpected(type_error("define-palette: each entry must be a list of "
                                              "(string string)"));
        }
        colors[*key] = *color;
    }

    auto palette = std::make_shared<Palette>(palette_name, std::move(colors));
    PaletteManager::instance().define(palette_name, std::move(palette));

    return make_nil_value();
}

// ═══════════════════════════════════════════════════════════════════════════════
// Builtin: palette-ref
// ═══════════════════════════════════════════════════════════════════════════════

/// (palette-ref "key") → look up in active palette.
///
/// Returns the color string from the active palette, or "#808080" if
/// no active palette is set or the key is not found.
static Result<Value> builtin_palette_ref(const std::vector<Value>& args, Environment&) {

    if (args.size() != 1) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }

    const auto* key = args[0].as_string();
    if (!key) {
        return std::unexpected(type_error("palette-ref: argument must be a string"));
    }

    auto active = PaletteManager::instance().active();
    if (!active) {
        std::cerr << "Warning: no active palette, returning #808080 for '" << *key << "'"
                  << std::endl;
        return Value(std::string("#808080"));
    }

    return Value(active->get(*key));
}

// ═══════════════════════════════════════════════════════════════════════════════
// register_palette — register builtins into environment
// ═══════════════════════════════════════════════════════════════════════════════

void register_palette(std::shared_ptr<Environment> env) {
    auto define_proc = std::make_shared<BuiltinProcedure>("define-palette", builtin_define_palette);
    auto ref_proc = std::make_shared<BuiltinProcedure>("palette-ref", builtin_palette_ref);

    env->define("define-palette", Value(define_proc));
    env->define("palette-ref", Value(ref_proc));
}

} // namespace pml
