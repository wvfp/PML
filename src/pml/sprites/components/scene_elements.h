#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Scene Element Components
// ───────────────────────────────────────────────────────────────────────────────
// Ported from pml/sprites/components/scene_elements.py.
// Provides tile, decoration, and background factory functions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/graphics/objects.h"
#include "pml/sprites/validator.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace pml {

/// Schema for the tile component.
ParamSchema tile_schema();

/// Create a terrain tile graphic object.
std::shared_ptr<GraphicObject> create_tile(
    const std::unordered_map<std::string, Value>& kwargs);

/// Schema for the decoration component.
ParamSchema decoration_schema();

/// Create a scene decoration graphic object.
std::shared_ptr<GraphicObject> create_decoration(
    const std::unordered_map<std::string, Value>& kwargs);

/// Schema for the background component.
ParamSchema background_schema();

/// Create a scene background graphic object.
std::shared_ptr<GraphicObject> create_background(
    const std::unordered_map<std::string, Value>& kwargs);

}  // namespace pml
