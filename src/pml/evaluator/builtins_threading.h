#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Threading Built-ins (→ and ->>) — Registration
// ═══════════════════════════════════════════════════════════════════════════════
//
// These are registered as special forms (not as env-bound builtins) so they
// receive their arguments unevaluated, allowing the thread-first/thread-last
// logic to work correctly.

#include "environment.h"
#include "evaluator.h"

#include <memory>

namespace pml {

/// Register thread-first (->) and thread-last (->>) as special forms.
/// @param env  Ignored (special forms are globally registered). Kept for
///             consistency with other registration functions.
void register_threading_builtins(std::shared_ptr<Environment> env);

} // namespace pml
