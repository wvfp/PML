#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Texture Builtins — define-texture, texture?, texture-width, etc.
// ==========================================================================================================================================================================================================================================═

#include <memory>

namespace pml {

class Environment;

/// Register texture builtins (define-texture, texture?, texture-width,
/// texture-height, texture-id) in the given environment.
void register_texture_builtins(std::shared_ptr<Environment> env);

} // namespace pml
