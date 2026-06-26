#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Item Components
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Ported from pml/sprites/components/items.py.
// Provides weapon, potion, chest, and generic-item factory functions.
// ==========================================================================================================================================================================================================================================═

#include "pml/graphics/objects.h"
#include "pml/sprites/validator.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace pml {

/// Schema for the weapon component.
ParamSchema weapon_schema();

/// Create a weapon graphic object.
std::shared_ptr<GraphicObject> create_weapon(
    const std::unordered_map<std::string, Value>& kwargs);

/// Schema for the potion component.
ParamSchema potion_schema();

/// Create a potion graphic object.
std::shared_ptr<GraphicObject> create_potion(
    const std::unordered_map<std::string, Value>& kwargs);

/// Schema for the chest component.
ParamSchema chest_schema();

/// Create a treasure chest graphic object.
std::shared_ptr<GraphicObject> create_chest(
    const std::unordered_map<std::string, Value>& kwargs);

/// Schema for the generic-item component.
ParamSchema generic_item_schema();

/// Create a generic item graphic object.
std::shared_ptr<GraphicObject> create_generic_item(
    const std::unordered_map<std::string, Value>& kwargs);

}  // namespace pml
