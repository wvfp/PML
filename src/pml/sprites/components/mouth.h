#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Mouth Component
// ───────────────────────────────────────────────────────────────────────────────
// Ported from pml/sprites/components/mouth.py.
// Generates mouth GraphicObjects in various styles.
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/graphics/objects.h"
#include "pml/sprites/validator.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace pml {

/// Schema for the mouth component.
ParamSchema mouth_schema();

/// Create a mouth graphic object.
std::shared_ptr<GraphicObject> create_mouth(
    const std::unordered_map<std::string, Value>& kwargs);

}  // namespace pml
