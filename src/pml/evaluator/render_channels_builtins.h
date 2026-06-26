#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Render Channels Builtins — multi-channel sprite export
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Registers the (render-channels ...) builtin for exporting albedo, specular,
// and normal channels from a GraphicObject.
//
// Builtins registered:
//   (render-channels graphic :output "path" [:channels '(albedo specular normal)]
//                     [:width 64] [:height 64] [:bg "transparent"])
//
// Channels:
//   albedo   — Render GraphicObject normally (original colours).
//   specular — Replace all fill/stroke colours with their luminance value
//              L = 0.299R + 0.587G + 0.114B (greyscale output).
//   normal   — Replace all fill/stroke colours with default normal vector
//              RGB(128, 128, 255) (Z-normal for 2D shapes). Alpha preserved.
// ==========================================================================================================================================================================================================================================═

#include "environment.h"

#include <memory>

namespace pml {

/// Register render-channels builtin into the given environment.
/// @param env The environment to define symbols in.
void register_render_channels(std::shared_ptr<Environment> env);

}  // namespace pml
