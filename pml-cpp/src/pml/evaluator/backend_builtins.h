#pragma once

#include <memory>

namespace pml {

class Environment;

/// Register backend-related builtin procedures into the environment.
///
/// Provides:
///   (list-backends)           → list of (name description) pairs
///   (set-backend! "name")     → set active backend, returns nil
///   (current-backend)         → string name of active backend
///   (backend-capabilities)    → list of capability symbols
///   (backend-available? "name") → bool
void register_backend_builtins(std::shared_ptr<Environment> env);

} // namespace pml
