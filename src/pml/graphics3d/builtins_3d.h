#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML 3D builtins registration
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/evaluator/environment.h"

#include <memory>

namespace pml {

void register_3d_builtins(std::shared_ptr<Environment> env);

} // namespace pml
