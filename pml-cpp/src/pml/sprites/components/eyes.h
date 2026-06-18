#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Eyes Component
// ───────────────────────────────────────────────────────────────────────────────
// Ported from pml/sprites/components/eyes.py.
// Generates anime-style eye GraphicObjects in various styles.
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/graphics/objects.h"
#include "pml/sprites/validator.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace pml {

/// Schema for the eyes component.
ParamSchema eyes_schema();

/// Create anime-style eyes.
std::shared_ptr<GraphicObject> create_eyes(
    const std::unordered_map<std::string, Value>& kwargs);

}  // namespace pml
