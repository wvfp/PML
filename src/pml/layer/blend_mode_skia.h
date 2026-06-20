#pragma once

#include "blend_mode.h"
#include <include/core/SkBlendMode.h>

namespace pml {

[[nodiscard]] SkBlendMode to_skia_blend_mode(BlendMode mode) noexcept;

} // namespace pml
