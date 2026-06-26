#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Color Helper Builtins Registration
//
// Color helper builtins extracted from canvas_builtins.cpp:
//   color/rgb, color/rgba
// ==========================================================================================================================================================================================================================================═

#include "environment.h"

#include <memory>

namespace pml {

/// Register color helper builtins (color/rgb, color/rgba).
void register_color_builtins(std::shared_ptr<Environment> env);

}  // namespace pml
