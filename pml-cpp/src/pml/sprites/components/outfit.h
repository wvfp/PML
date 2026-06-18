#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Outfit Component
// ───────────────────────────────────────────────────────────────────────────────
// Ported from pml/sprites/components/outfit.py.
// Generates clothing (top / bottom / shoes) as a group of GraphicObjects.
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/graphics/objects.h"
#include "pml/sprites/validator.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace pml {

/// Schema for the outfit component.
ParamSchema outfit_schema();

/// Create a clothing overlay for a character body.
std::shared_ptr<GraphicObject> create_outfit(
    const std::unordered_map<std::string, Value>& kwargs);

}  // namespace pml
