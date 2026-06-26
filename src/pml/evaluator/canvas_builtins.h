#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Canvas Builtins Registration
//
// Exposes Canvas, shape factory, style, group/transform, and color operations
// as PML builtins:
//
//   Canvas:        canvas, sprite-canvas, add
//   Shapes:        circle, rect, ellipse, line, polygon, text
//   Styles:        fill, stroke, no-fill, no-stroke, stroke-width, group
//   Transform obj: with-transform, translate-object, rotate-object, scale-object
//   Colors:        color/rgb, color/rgba
// ==========================================================================================================================================================================================================================================═

#include "environment.h"

#include <memory>

namespace pml {

/// Register canvas/shape/style/transform-object/color builtins into
/// the given environment.
void register_canvas_builtins(std::shared_ptr<Environment> env);

}  // namespace pml
