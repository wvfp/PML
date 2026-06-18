#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Transform Builtins Registration
//
// Exposes AffineTransform operations as PML builtins:
//   translate, rotate, scale, shear, identity-matrix, compose,
//   matrix-inverse, matrix-apply
// ═══════════════════════════════════════════════════════════════════════════════

#include "environment.h"

#include <memory>

namespace pml {

/// Register transform builtins into the given environment.
void register_transform_builtins(std::shared_ptr<Environment> env);

}  // namespace pml
