#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Texture-Map Builtins — texture-map, multi-texture blend
// ==========================================================================================================================================================================================================================================═

#include <memory>

namespace pml {

class Environment;

/// Register texture-map builtins (texture-map, and integration with :fill
/// shorthand) in the given environment.
void register_texture_map_builtins(std::shared_ptr<Environment> env);

} // namespace pml
