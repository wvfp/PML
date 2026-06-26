#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Rough-Style Fill Pattern Generators
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Port of rough.js fillers: hachure, zigzag, cross-hatch, dots, dashed, solid.
// Uses the scanline hachure engine from the 'hachure-fill' npm package v0.5.2.
// ==========================================================================================================================================================================================================================================═

#include "rough.h"

#include <vector>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// Individual fill pattern generators
// ==========================================================================================================================================================================================================================================═

/// Generate hachure (parallel line) fill for a polygon.
/// Returns rough ops for the fill lines.
std::vector<RoughOp> hachure_fill_polygon(
    const std::vector<RoughPoint>& polygon,
    RoughStyleParams& params, RoughRandom& rng);

/// Generate zigzag fill (hachure + zigzag offset on each line).
std::vector<RoughOp> zigzag_fill_polygon(
    const std::vector<RoughPoint>& polygon,
    RoughStyleParams& params, RoughRandom& rng);

/// Generate cross-hatch fill (hachure at angle + hachure at angle+90).
std::vector<RoughOp> cross_hatch_fill_polygon(
    const std::vector<RoughPoint>& polygon,
    RoughStyleParams& params, RoughRandom& rng);

/// Generate dot fill (dots along hachure scanlines).
std::vector<RoughOp> dot_fill_polygon(
    const std::vector<RoughPoint>& polygon,
    RoughStyleParams& params, RoughRandom& rng);

/// Generate dashed line fill.
std::vector<RoughOp> dashed_fill_polygon(
    const std::vector<RoughPoint>& polygon,
    RoughStyleParams& params, RoughRandom& rng);

/// Generate solid fill outline (jittered polygon boundary, no pattern).
std::vector<RoughOp> solid_fill_polygon(
    const std::vector<RoughPoint>& polygon,
    RoughStyleParams& params, RoughRandom& rng);

// ==========================================================================================================================================================================================================================================═
// Dispatcher — delegates to the correct filler based on params.fill_style
// ==========================================================================================================================================================================================================================================═

/// Select and invoke the appropriate fill-pattern generator.
/// Returns empty ops for "solid" (handled natively by the renderer).
inline std::vector<RoughOp> generate_fill_pattern(
    const std::vector<RoughPoint>& polygon,
    RoughStyleParams& params, RoughRandom& rng)
{
    if (params.fill_style == "hachure")
        return hachure_fill_polygon(polygon, params, rng);
    if (params.fill_style == "zigzag")
        return zigzag_fill_polygon(polygon, params, rng);
    if (params.fill_style == "cross-hatch")
        return cross_hatch_fill_polygon(polygon, params, rng);
    if (params.fill_style == "dots")
        return dot_fill_polygon(polygon, params, rng);
    if (params.fill_style == "dashed")
        return dashed_fill_polygon(polygon, params, rng);
    // "solid" or unknown — fall back to solid (returns empty, renderer handles natively)
    return solid_fill_polygon(polygon, params, rng);
}

} // namespace pml
