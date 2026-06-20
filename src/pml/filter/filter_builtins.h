#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// Filter builtins — PML-facing constructors for image filters and filter chains
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/evaluator/environment.h"

namespace pml {

/// Register filter builtins in the global environment.
/// Includes: color-adjust, levels, curves, threshold, posterize,
///           blur, sharpen, edge-detect, emboss, convolution,
///           drop-shadow, inner-shadow, outer-glow, inner-glow, bevel-emboss,
///           filter-chain, filter?
void register_filter_builtins(std::shared_ptr<Environment> env);

} // namespace pml
