#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Character Component
// ───────────────────────────────────────────────────────────────────────────────
// Ported from pml/sprites/components/character.py.
// Assembles a complete character sprite from body, head, eyes, mouth, hair.
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/graphics/objects.h"
#include "pml/sprites/validator.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace pml {

/// Schema for the character component.
ParamSchema character_schema();

/// Assemble a complete character sprite from components.
std::shared_ptr<GraphicObject> create_character(
    const std::unordered_map<std::string, Value>& kwargs);

}  // namespace pml
