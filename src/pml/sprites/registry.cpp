// ==========================================================================================================================================================================================================================================═
// PML Component Registry — Implementation
// ==========================================================================================================================================================================================================================================═

#include "registry.h"

#include "error.h"
#include "objects.h"

#include "components/body.h"
#include "components/head.h"
#include "components/eyes.h"
#include "components/mouth.h"
#include "components/hair.h"
#include "components/character.h"
#include "components/outfit.h"
#include "components/items.h"
#include "components/ui_widgets.h"
#include "components/scene_elements.h"

#include <algorithm>
#include <format>
#include <set>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// Singleton
// ==========================================================================================================================================================================================================================================═

ComponentRegistry& ComponentRegistry::instance() {
    static ComponentRegistry s_instance;
    return s_instance;
}

// ==========================================================================================================================================================================================================================================═
// Registration / lookup
// ==========================================================================================================================================================================================================================================═

void ComponentRegistry::register_(const std::string& name,
                                  ComponentFactory factory,
                                  std::optional<ParamSchema> schema,
                                  std::optional<std::string> category) {
    Entry entry;
    entry.factory = std::move(factory);
    entry.schema = std::move(schema);
    entry.category = std::move(category);
    m_components[name] = std::move(entry);
}

Value ComponentRegistry::create(
    const std::string& name,
    const std::unordered_map<std::string, Value>& kwargs) const {
    auto it = m_components.find(name);
    if (it == m_components.end()) {
        throw std::runtime_error(std::format("Unknown component: {}", name));
    }
    return it->second.factory(kwargs);
}

bool ComponentRegistry::has(const std::string& name) const {
    return m_components.find(name) != m_components.end();
}

std::vector<std::string> ComponentRegistry::list_components() const {
    std::vector<std::string> names;
    names.reserve(m_components.size());
    for (const auto& [name, _] : m_components) {
        names.push_back(name);
    }
    return names;
}

std::optional<ParamSchema> ComponentRegistry::get_schema(
    const std::string& name) const {
    auto it = m_components.find(name);
    if (it == m_components.end()) return std::nullopt;
    return it->second.schema;
}

std::optional<std::string> ComponentRegistry::get_category(
    const std::string& name) const {
    auto it = m_components.find(name);
    if (it == m_components.end()) return std::nullopt;
    return it->second.category;
}

std::vector<std::string> ComponentRegistry::list_by_category(
    const std::string& category) const {
    std::vector<std::string> names;
    for (const auto& [name, entry] : m_components) {
        if (entry.category == category) {
            names.push_back(name);
        }
    }
    return names;
}

std::vector<std::string> ComponentRegistry::categories() const {
    std::set<std::string> cats;
    for (const auto& [_, entry] : m_components) {
        if (entry.category) {
            cats.insert(*entry.category);
        }
    }
    return std::vector<std::string>(cats.begin(), cats.end());
}

// ==========================================================================================================================================================================================================================================═
// Component builtin registration
// ==========================================================================================================================================================================================================================================═

namespace {

/// Parse flat keyword arguments from a vector of Values.
[[nodiscard]] std::unordered_map<std::string, Value> parse_component_kwargs(
    const std::vector<Value>& args) {
    std::unordered_map<std::string, Value> result;
    for (size_t i = 0; i + 1 < args.size(); i += 2) {
        if (const auto* kw = args[i].as_keyword()) {
            result[kw->name] = args[i + 1];
        } else if (const auto* sym = args[i].as_symbol()) {
            result[sym->name] = args[i + 1];
        } else {
            break;
        }
    }
    return result;
}

/// Wrap a component factory into a BuiltinProcedure function.
[[nodiscard]] BuiltinProcedure::BuiltinFn wrap_factory(
    ComponentFactory factory) {
    return [factory = std::move(factory)](const std::vector<Value>& args,
                                          Environment&) -> Result<Value> {
        auto kwargs = parse_component_kwargs(args);
        return factory(kwargs);
    };
}

}  // anonymous namespace

void register_components(std::shared_ptr<Environment> env) {
    if (!env) return;

    auto& registry = ComponentRegistry::instance();

    // Helper to register a component both in the registry and as a builtin.
    auto reg = [&](const std::string& name,
                   ComponentFactory factory,
                   ParamSchema schema,
                   const std::string& category) {
        registry.register_(name, factory, schema, category);
        env->define(name,
            Value(std::make_shared<BuiltinProcedure>(
                name, wrap_factory(factory), true)));
    };

    // Character components
    reg("body", create_body, body_schema(), "character");
    reg("head", create_head, head_schema(), "character");
    reg("anime-eyes", create_eyes, eyes_schema(), "character");
    reg("mouth", create_mouth, mouth_schema(), "character");
    reg("hair", create_hair, hair_schema(), "character");
    reg("character", create_character, character_schema(), "character");
    reg("outfit", create_outfit, outfit_schema(), "character");

    // Items
    reg("weapon", create_weapon, weapon_schema(), "items");
    reg("potion", create_potion, potion_schema(), "items");
    reg("chest", create_chest, chest_schema(), "items");
    reg("generic-item", create_generic_item, generic_item_schema(), "items");

    // UI widgets
    reg("button", create_button, button_schema(), "ui");
    reg("panel", create_panel, panel_schema(), "ui");
    reg("health-bar", create_health_bar, health_bar_schema(), "ui");
    reg("icon", create_icon, icon_schema(), "ui");

    // Scene elements
    reg("tile", create_tile, tile_schema(), "scene");
    reg("decoration", create_decoration, decoration_schema(), "scene");
    reg("background", create_background, background_schema(), "scene");
}

}  // namespace pml
