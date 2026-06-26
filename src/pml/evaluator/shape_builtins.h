#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Shape Factory Builtins Registration
//
// Shape factory builtins extracted from canvas_builtins.cpp:
//   circle, rect, ellipse, line, polygon, text, group
// ==========================================================================================================================================================================================================================================═

#include "environment.h"

#include <memory>

namespace pml {

/// Register shape factory builtins (circle, rect, ellipse, line, polygon, text, group).
void register_shape_builtins(std::shared_ptr<Environment> env);

}  // namespace pml
