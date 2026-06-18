#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Head Component
// ───────────────────────────────────────────────────────────────────────────────
// Ported from pml/sprites/components/head.py.
// Generates a head GraphicObject tree.
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/graphics/objects.h"
#include "pml/sprites/validator.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace pml {

/// Schema for the head component.
ParamSchema head_schema();

/// Create a head graphic object.
std::shared_ptr<GraphicObject> create_head(
    const std::unordered_map<std::string, Value>& kwargs);

}  // namespace pml
