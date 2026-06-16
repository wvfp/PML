#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Shader Builtins — stub registration (Skia not available)
// ───────────────────────────────────────────────────────────────────────────────
// Registers shader-related built-in procedures that return clear errors
// indicating Skia backend support is required but not available in this build.
//
// Builtins registered:
//   (shader "...glsl...")              — compile a GLSL shader
//   (apply-shader! graphic shader)     — apply a shader to a graphic object
// ═══════════════════════════════════════════════════════════════════════════════

#include "environment.h"

#include <memory>

namespace pml {

/// Register shader-related built-in procedures into the given environment.
/// Since Skia is not available in this build, all shader builtins return
/// a ResourceError with a descriptive message.
void register_shader_builtins(std::shared_ptr<Environment> env);

}  // namespace pml
