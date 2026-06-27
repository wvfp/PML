#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Name Registry Builtins — name, find, remove, set-property
// ==========================================================================================================================================================================================================================================═

#include <memory>

namespace pml {

class Environment;

/// Register name-related builtins (name, find, remove, etc.) into the
/// given environment.
void register_name_builtins(std::shared_ptr<Environment> env);

} // namespace pml
