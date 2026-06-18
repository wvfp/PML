#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Body Component
// ───────────────────────────────────────────────────────────────────────────────
// Ported from pml/sprites/components/body.py.
// Generates a torso GraphicObject tree.
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/graphics/objects.h"
#include "pml/sprites/validator.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace pml {

/// Schema for the body component.
extern ParamSchema body_schema();

/// Create a body (torso) graphic object.
///
/// Args:
///   :height — total character height in pixels (default 160)
///   :build — 'slim | 'average | 'muscular | 'chubby
///   :skin — skin color
///   :proportions — 'realistic | 'anime | 'chibi
std::shared_ptr<GraphicObject> create_body(
    const std::unordered_map<std::string, Value>& kwargs);

}  // namespace pml
