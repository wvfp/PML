#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Gradient Builtins Registration
//
// Builtins for constructing gradient fill descriptors:
//   linear, radial
// ==========================================================================================================================================================================================================================================═

#include "environment.h"

#include <memory>

namespace pml {

/// Register gradient builtins (linear, radial).
void register_gradient_builtins(std::shared_ptr<Environment> env);

}  // namespace pml
