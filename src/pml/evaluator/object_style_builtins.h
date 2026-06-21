#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Object Style + Transform-Object Builtins Registration
//
// Object style and transform-object builtins extracted from canvas_builtins.cpp:
//   fill, stroke, no-fill, no-stroke, stroke-width,
//   with-transform, translate-object, rotate-object, scale-object
// ═══════════════════════════════════════════════════════════════════════════════

#include "environment.h"

#include <memory>

namespace pml {

/// Register object style + transform-object builtins.
void register_object_style_builtins(std::shared_ptr<Environment> env);

}  // namespace pml
