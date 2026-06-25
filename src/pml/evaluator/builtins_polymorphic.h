#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Polymorphic Access Built-ins (get / set!) — Registration
// ═══════════════════════════════════════════════════════════════════════════════

#include "environment.h"

#include <memory>

namespace pml {

/// Register polymorphic get / set! builtins.
void register_polymorphic_builtins(std::shared_ptr<Environment> env);

} // namespace pml
