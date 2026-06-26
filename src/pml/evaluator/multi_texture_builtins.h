#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Multi-Texture Shader Builtins — bind SkImages to `uniform shader` slots
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Registers the (bind-textures ...) builtin that binds GraphicObject-rendered
// textures to a compiled SkSL shader's child shader slots.
//
// Builtins registered:
//   (bind-textures shader-handle :textures '((slot-name graphic-expr) ...))
//     — Bind texture GraphicObjects to `uniform shader <slot-name>` slots
//     — Returns a new shader handle with children bound
// ==========================================================================================================================================================================================================================================═

#include "environment.h"

#include <memory>

namespace pml {

/// Register multi-texture shader binding builtins into the given environment.
void register_multi_texture_builtins(std::shared_ptr<Environment> env);

}  // namespace pml
