#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Polygon Perturbation Builtins
//
// Implements:
//   perturb-polygon — perturb polygon vertices with Perlin noise, subdivision,
//                     and corner rounding
// ═══════════════════════════════════════════════════════════════════════════════

#include "environment.h"

#include <memory>

namespace pml {

/// Register polygon perturbation builtins (perturb-polygon).
void register_perturb_builtins(std::shared_ptr<Environment> env);

}  // namespace pml
