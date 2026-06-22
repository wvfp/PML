#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Rough-Style Rendering — Types + Seeded PRNG
// ───────────────────────────────────────────────────────────────────────────────
// Port of rough.js (rough-stuff/rough) geometry perturbation algorithms.
// Provides hand-drawn / sketchy style shape rendering.
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/core/types.h"

#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// RoughPoint
// ═══════════════════════════════════════════════════════════════════════════════

struct RoughPoint {
    double x;
    double y;
};

// ═══════════════════════════════════════════════════════════════════════════════
// RoughStyleParams — parsed roughness configuration
// ═══════════════════════════════════════════════════════════════════════════════

/// Mirrors rough.js ResolvedOptions.  Defaults match rough.js v4 defaults.
struct RoughStyleParams {
    double roughness{0.0};              // 0=perfect, 1=rough.js default
    double bowing{1.0};                 // bowing amount
    int seed{0};                        // PRNG seed (0 = auto random)
    double max_randomness_offset{2.0};
    double curve_fitting{0.95};
    double curve_tightness{0.0};
    int curve_step_count{9};
    bool disable_multi_stroke{false};
    bool disable_multi_stroke_fill{false};
    bool preserve_vertices{false};
    double fill_shape_roughness_gain{0.8};
    std::string fill_style{"solid"};    // "solid", "hachure", "cross-hatch",
                                        // "zigzag", "dots", "dashed"

    // --- Hachure / fill pattern parameters (matching rough.js ResolvedOptions) ---
    double hachure_angle{-41.0};        // base angle in degrees (default -41 → net 49°)
    double hachure_gap{-1.0};           // gap between scanlines (-1 = auto, uses default 8.0)
    double fill_weight{-1.0};           // fill line weight (-1 = auto, uses default 2.0)
    double dash_offset{-1.0};           // dash on-segment length (-1 = auto)
    double dash_gap{-1.0};              // gap between dashes (-1 = auto)

    /// Whether the stroke should be rendered with rough perturbation.
    bool has_rough_stroke() const noexcept { return roughness > 0.0; }

    /// Whether the fill should use a rough pattern instead of solid.
    bool has_rough_fill() const noexcept { return fill_style != "solid"; }
};

// ═══════════════════════════════════════════════════════════════════════════════
// RoughRandom — seeded PRNG (matches rough.js math.ts)
// ═══════════════════════════════════════════════════════════════════════════════

/// Deterministic linear congruential generator matching rough.js:
///   next() = ((2^31-1) & (seed = 48271 * seed)) / 2^31
class RoughRandom {
public:
    /// seed=0 uses std::random_device for a nondeterministic seed.
    explicit RoughRandom(int seed = 0)
    {
        if (seed == 0) {
            std::random_device rd;
            seed_ = static_cast<int64_t>(rd());
        } else {
            seed_ = seed;
        }
    }

    /// Returns a pseudo-random double in [0, 1).
    double next()
    {
        seed_ = 48271 * seed_;
        return static_cast<double>(seed_ & 0x7FFFFFFF) / 2147483648.0;
    }

    /// Reseed (useful to get a second independent stream).
    void reseed(int seed) { seed_ = seed; }

private:
    int64_t seed_{0};
};

// ═══════════════════════════════════════════════════════════════════════════════
// Random offset helpers (mirror rough.js renderer.ts helpers)
// ═══════════════════════════════════════════════════════════════════════════════

/// Return random value in [min, max] scaled by roughness.
inline double rand_offset_range(
    double min, double max, RoughRandom& rng,
    double roughness, double roughness_gain = 1.0)
{
    return roughness * roughness_gain * ((rng.next() * (max - min)) + min);
}

/// Return random value in [-x, x] scaled by roughness.
inline double rand_offset(
    double x, RoughRandom& rng,
    double roughness, double roughness_gain = 1.0)
{
    return rand_offset_range(-x, x, rng, roughness, roughness_gain);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Op types (simplified OpSet from rough.js — maps directly to SkPath ops)
// ═══════════════════════════════════════════════════════════════════════════════

enum class RoughOpType { Move, BCurveTo, LineTo };

struct RoughOp {
    RoughOpType op;
    double data[6];   // [x,y] for Move/LineTo, [cx1,cy1,cx2,cy2,x,y] for BCurveTo
    int data_len{0};
};

/// A complete set of drawing operations (stroke or fill path).
struct RoughOpSet {
    std::vector<RoughOp> ops;
};

// ═══════════════════════════════════════════════════════════════════════════════
// Core perturbation algorithms
// ═══════════════════════════════════════════════════════════════════════════════

/// Generate rough line ops: two overlapping bezier curves.
std::vector<RoughOp> rough_double_line(
    double x1, double y1, double x2, double y2,
    RoughStyleParams& params, RoughRandom& rng, bool filling = false);

/// Convert a polyline (list of points) into rough curve ops.
/// close=true connects last point back to first.
std::vector<RoughOp> rough_linear_path(
    const std::vector<RoughPoint>& points, bool close,
    RoughStyleParams& params, RoughRandom& rng);

/// Generate rough ellipse point samples (Catmull-Rom → bezier).
/// Returns point list ready for _curve fitting.
std::vector<RoughPoint> compute_rough_ellipse_points(
    double cx, double cy, double rx, double ry,
    RoughStyleParams& params, RoughRandom& rng);

/// Fit Catmull-Rom spline through points, output as bezier ops.
std::vector<RoughOp> rough_curve(
    const std::vector<RoughPoint>& points,
    const RoughPoint* close_point,
    RoughStyleParams& params, RoughRandom& rng);

// ═══════════════════════════════════════════════════════════════════════════════
// Fill pattern generation
// ═══════════════════════════════════════════════════════════════════════════════

/// Generate fill pattern ops for a polygon.
/// Supports fill_style: "solid", "hachure", "cross-hatch", "zigzag", "dots", "dashed".
std::vector<RoughOp> generate_fill_pattern(
    const std::vector<RoughPoint>& polygon,
    RoughStyleParams& params, RoughRandom& rng);

/// Compute length of line segment.
inline double line_length(double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace pml
