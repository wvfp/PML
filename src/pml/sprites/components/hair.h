#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Hair Component
// ───────────────────────────────────────────────────────────────────────────────
// Ported from pml/sprites/components/hair.py.
// Generates hairstyle GraphicObjects.
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/graphics/objects.h"
#include "pml/sprites/validator.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace pml {

/// Schema for the hair component.
ParamSchema hair_schema();

/// Create a hairstyle graphic object.
std::shared_ptr<GraphicObject> create_hair(
    const std::unordered_map<std::string, Value>& kwargs);

}  // namespace pml
