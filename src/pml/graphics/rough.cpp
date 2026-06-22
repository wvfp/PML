// ═══════════════════════════════════════════════════════════════════════════════
// PML Rough-Style Rendering — Geometry perturbation algorithms
// ───────────────────────────────────────────────────────────────────────────────
// Port of rough.js (rough-stuff/rough) renderer.ts geometry perturbation.
// Provides hand-drawn / sketchy style line, path, and ellipse generation.
// ═══════════════════════════════════════════════════════════════════════════════

#include "rough.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Internal helpers
// ═══════════════════════════════════════════════════════════════════════════════

namespace {
    constexpr double kPi = 3.14159265358979323846;

    // ─────────────────────────────────────────────────────────────────────────
    // _line — generate a single bezier curve for a line segment with random
    //         perturbation.
    //
    // Port of renderer.ts _line (lines 292-361).
    // ─────────────────────────────────────────────────────────────────────────
    std::vector<RoughOp> _line(
        double x1, double y1, double x2, double y2,
        RoughStyleParams& params, RoughRandom& rng,
        bool move, bool overlay)
    {
        // (line 293) squared length
        const double dx = x1 - x2;
        const double dy = y1 - y2;
        const double lengthSq = dx * dx + dy * dy;
        const double length = std::sqrt(lengthSq);

        // (lines 295-302) roughness gain scales down for long lines
        double roughnessGain = 1.0;
        if (length < 200.0) {
            roughnessGain = 1.0;
        } else if (length > 500.0) {
            roughnessGain = 0.4;
        } else {
            roughnessGain = (-0.0016668) * length + 1.233334;
        }

        // (lines 304-308) offset clamping — prevent extreme displacements
        double offset = params.max_randomness_offset;
        if (offset * offset * 100.0 > lengthSq) {
            offset = length / 10.0;
        }
        const double halfOffset = offset / 2.0;

        // (line 309) diverge point — where the bezier control points diverge
        const double divergePoint = 0.2 + rng.next() * 0.2;

        // (lines 310-313) bowing displacement, then perturbed by random offset
        double midDispX = params.bowing * params.max_randomness_offset * (y2 - y1) / 200.0;
        double midDispY = params.bowing * params.max_randomness_offset * (x1 - x2) / 200.0;
        midDispX = rand_offset(midDispX, rng, params.roughness, roughnessGain);
        midDispY = rand_offset(midDispY, rng, params.roughness, roughnessGain);

        const bool preserveVertices = params.preserve_vertices;
        const double rough = params.roughness;

        // (lines 315-316) random offset helpers for this segment
        auto randomHalf = [&]() { return rand_offset(halfOffset, rng, rough, roughnessGain); };
        auto randomFull = [&]() { return rand_offset(offset, rng, rough, roughnessGain); };

        std::vector<RoughOp> ops;
        ops.reserve(2);

        // (lines 318-334) move op
        if (move) {
            RoughOp moveOp;
            moveOp.op = RoughOpType::Move;
            moveOp.data_len = 2;
            if (overlay) {
                moveOp.data[0] = x1 + (preserveVertices ? 0.0 : randomHalf());
                moveOp.data[1] = y1 + (preserveVertices ? 0.0 : randomHalf());
            } else {
                moveOp.data[0] = x1 + (preserveVertices ? 0.0
                    : rand_offset(offset, rng, rough, roughnessGain));
                moveOp.data[1] = y1 + (preserveVertices ? 0.0
                    : rand_offset(offset, rng, rough, roughnessGain));
            }
            ops.push_back(std::move(moveOp));
        }

        // (lines 335-359) bezier curveTo
        RoughOp curveOp;
        curveOp.op = RoughOpType::BCurveTo;
        curveOp.data_len = 6;
        if (overlay) {
            // (lines 336-346) overlay stroke — smaller offsets (halfOffset)
            curveOp.data[0] = midDispX + x1 + (x2 - x1) * divergePoint + randomHalf();
            curveOp.data[1] = midDispY + y1 + (y2 - y1) * divergePoint + randomHalf();
            curveOp.data[2] = midDispX + x1 + 2.0 * (x2 - x1) * divergePoint + randomHalf();
            curveOp.data[3] = midDispY + y1 + 2.0 * (y2 - y1) * divergePoint + randomHalf();
            curveOp.data[4] = x2 + (preserveVertices ? 0.0 : randomHalf());
            curveOp.data[5] = y2 + (preserveVertices ? 0.0 : randomHalf());
        } else {
            // (lines 348-358) primary stroke — full offset
            curveOp.data[0] = midDispX + x1 + (x2 - x1) * divergePoint + randomFull();
            curveOp.data[1] = midDispY + y1 + (y2 - y1) * divergePoint + randomFull();
            curveOp.data[2] = midDispX + x1 + 2.0 * (x2 - x1) * divergePoint + randomFull();
            curveOp.data[3] = midDispY + y1 + 2.0 * (y2 - y1) * divergePoint + randomFull();
            curveOp.data[4] = x2 + (preserveVertices ? 0.0 : randomFull());
            curveOp.data[5] = y2 + (preserveVertices ? 0.0 : randomFull());
        }
        ops.push_back(std::move(curveOp));

        return ops;
    }

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// rough_double_line — two overlapping bezier curves per line segment
// ───────────────────────────────────────────────────────────────────────────────
// Port of renderer.ts _doubleLine (lines 282-290).
//   filling = true  → singleStroke governed by disable_multi_stroke_fill
//   filling = false → singleStroke governed by disable_multi_stroke
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<RoughOp> rough_double_line(
    double x1, double y1, double x2, double y2,
    RoughStyleParams& params, RoughRandom& rng, bool filling)
{
    // (line 283) decide whether to use a single stroke
    const bool singleStroke = filling
        ? params.disable_multi_stroke_fill
        : params.disable_multi_stroke;

    // (line 284) primary stroke: move + full-offset bezier
    std::vector<RoughOp> ops = _line(x1, y1, x2, y2, params, rng, true, false);

    // (lines 285-289) overlay stroke: move + half-offset bezier
    if (!singleStroke) {
        std::vector<RoughOp> overlay = _line(x1, y1, x2, y2, params, rng, true, true);
        ops.insert(ops.end(), overlay.begin(), overlay.end());
    }

    return ops;
}

// ═══════════════════════════════════════════════════════════════════════════════
// rough_linear_path — polyline → overlapping bezier segments
// ───────────────────────────────────────────────────────────────────────────────
// Port of renderer.ts linearPath (lines 25-40).
//   close = true → connect last point back to first.
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<RoughOp> rough_linear_path(
    const std::vector<RoughPoint>& points, bool close,
    RoughStyleParams& params, RoughRandom& rng)
{
    const size_t len = points.size();
    std::vector<RoughOp> ops;

    if (len > 2) {
        // (lines 29-31) each consecutive pair
        ops.reserve((len - 1 + (close ? 1 : 0)) * 2);
        for (size_t i = 0; i < len - 1; i++) {
            std::vector<RoughOp> seg = rough_double_line(
                points[i].x, points[i].y,
                points[i + 1].x, points[i + 1].y,
                params, rng, false);
            ops.insert(ops.end(), seg.begin(), seg.end());
        }
        // (lines 32-34) optional closing segment
        if (close) {
            std::vector<RoughOp> seg = rough_double_line(
                points[len - 1].x, points[len - 1].y,
                points[0].x, points[0].y,
                params, rng, false);
            ops.insert(ops.end(), seg.begin(), seg.end());
        }
    } else if (len == 2) {
        // (lines 36-38) single segment
        return rough_double_line(
            points[0].x, points[0].y,
            points[1].x, points[1].y,
            params, rng, false);
    }

    return ops;
}

// ═══════════════════════════════════════════════════════════════════════════════
// compute_rough_ellipse_points — sample ellipse with jitter for Catmull-Rom
// ───────────────────────────────────────────────────────────────────────────────
// Combines renderer.ts generateEllipseParams (lines 97-107) with
// _computeEllipsePoints (lines 426-484) for the primary stroke (offset=1).
// Returns the allPoints[] array ready for rough_curve().
//
// The overlay stroke (offset=1.5) must be generated by the caller via a
// separate call with a differently-seeded RNG if desired.
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<RoughPoint> compute_rough_ellipse_points(
    double cx, double cy, double rx, double ry,
    RoughStyleParams& params, RoughRandom& rng)
{
    // ── generateEllipseParams (lines 97-107) ───────────────────────────────
    // psq = sqrt(2*PI * sqrt(((rx)^2 + (ry)^2) / 2))
    const double rsq = (rx * rx + ry * ry) / 2.0;
    const double psq = std::sqrt(kPi * 2.0 * std::sqrt(rsq));

    // step count with aspect-ratio-aware scaling
    const int stepCount = static_cast<int>(std::ceil(
        std::max(static_cast<double>(params.curve_step_count),
                 (static_cast<double>(params.curve_step_count) / std::sqrt(200.0)) * psq)));

    double increment = (kPi * 2.0) / static_cast<double>(stepCount);

    // (lines 103-106) apply curve-fitting randomness to radii
    const double curveFitRandomness = 1.0 - params.curve_fitting;
    rx += rand_offset(rx * curveFitRandomness, rng, params.roughness);
    ry += rand_offset(ry * curveFitRandomness, rng, params.roughness);

    // ── _computeEllipsePoints (lines 426-484) ──────────────────────────────
    const bool coreOnly = (params.roughness == 0.0);
    std::vector<RoughPoint> allPoints;

    if (coreOnly) {
        // (lines 431-452) roughness=0: exact ellipse, finer sampling
        increment = increment / 4.0;

        // (lines 433-436) start point: one step before origin
        allPoints.push_back({
            cx + rx * std::cos(-increment),
            cy + ry * std::sin(-increment)
        });

        // (lines 437-444) full circle at exact radius
        for (double angle = 0.0; angle <= kPi * 2.0; angle += increment) {
            allPoints.push_back({
                cx + rx * std::cos(angle),
                cy + ry * std::sin(angle)
            });
        }

        // (lines 445-452) overlap points for seamless closure
        allPoints.push_back({
            cx + rx * std::cos(0.0),
            cy + ry * std::sin(0.0)
        });
        allPoints.push_back({
            cx + rx * std::cos(increment),
            cy + ry * std::sin(increment)
        });
    } else {
        // (lines 453-479) rough case: add jitter and offset
        // (line 454) random radial offset
        const double radOffset = rand_offset(0.5, rng, params.roughness) - (kPi / 2.0);

        // overlap = increment * _offset(0.1, _offset(0.4, 1, o), o)
        // inner: rand_offset_range(0.4, 1.0, ...)
        // outer: rand_offset_range(0.1, inner, ...)
        const double innerOffset = rand_offset_range(0.4, 1.0, rng, params.roughness);
        const double overlap = rand_offset_range(0.1, innerOffset, rng, params.roughness) * increment;

        constexpr double kStrokeOffset = 1.0;  // primary stroke offset magnitude

        // (lines 455-458) start point: slightly before first angle, 0.9 radius
        allPoints.push_back({
            rand_offset(kStrokeOffset, rng, params.roughness) + cx + 0.9 * rx * std::cos(radOffset - increment),
            rand_offset(kStrokeOffset, rng, params.roughness) + cy + 0.9 * ry * std::sin(radOffset - increment)
        });

        // (lines 459-467) body points: full radius with per-point jitter
        // endAngle = 2*PI + radOffset - 0.01 (line 459)
        const double endAngle = kPi * 2.0 + radOffset - 0.01;
        for (double angle = radOffset; angle < endAngle; angle += increment) {
            allPoints.push_back({
                rand_offset(kStrokeOffset, rng, params.roughness) + cx + rx * std::cos(angle),
                rand_offset(kStrokeOffset, rng, params.roughness) + cy + ry * std::sin(angle)
            });
        }

        // (lines 468-479) closure overlap points: three extra samples
        //   overlap * 0.5, full overlap, overlap * 0.5 with decreasing radius
        allPoints.push_back({
            rand_offset(kStrokeOffset, rng, params.roughness) + cx + rx * std::cos(radOffset + kPi * 2.0 + overlap * 0.5),
            rand_offset(kStrokeOffset, rng, params.roughness) + cy + ry * std::sin(radOffset + kPi * 2.0 + overlap * 0.5)
        });
        allPoints.push_back({
            rand_offset(kStrokeOffset, rng, params.roughness) + cx + 0.98 * rx * std::cos(radOffset + overlap),
            rand_offset(kStrokeOffset, rng, params.roughness) + cy + 0.98 * ry * std::sin(radOffset + overlap)
        });
        allPoints.push_back({
            rand_offset(kStrokeOffset, rng, params.roughness) + cx + 0.9 * rx * std::cos(radOffset + overlap * 0.5),
            rand_offset(kStrokeOffset, rng, params.roughness) + cy + 0.9 * ry * std::sin(radOffset + overlap * 0.5)
        });
    }

    return allPoints;
}

// ═══════════════════════════════════════════════════════════════════════════════
// rough_curve — Catmull-Rom spline through points → cubic bezier ops
// ───────────────────────────────────────────────────────────────────────────────
// Port of renderer.ts _curve (lines 391-424).
//
//   points[] — control points (typically from compute_rough_ellipse_points or
//              from a multi-point polyline).
//   close_point — if non-null, emits a final lineTo to this point with
//                 random offset (used for ellipse closure).
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<RoughOp> rough_curve(
    const std::vector<RoughPoint>& points,
    const RoughPoint* close_point,
    RoughStyleParams& params, RoughRandom& rng)
{
    const size_t len = points.size();
    std::vector<RoughOp> ops;

    if (len > 3) {
        // (lines 394-405) Catmull-Rom → cubic Bezier conversion
        // tightness factor (line 396)
        const double s = 1.0 - params.curve_tightness;

        // (line 397) move to second point (first curve start)
        RoughOp moveOp;
        moveOp.op = RoughOpType::Move;
        moveOp.data[0] = points[1].x;
        moveOp.data[1] = points[1].y;
        moveOp.data_len = 2;
        ops.push_back(std::move(moveOp));

        // (lines 398-405) generate bezier segments for i = 1 .. len-3
        for (size_t i = 1; (i + 2) < len; i++) {
            const RoughPoint& p_i   = points[i];
            const RoughPoint& p_im1 = points[i - 1];
            const RoughPoint& p_ip1 = points[i + 1];
            const RoughPoint& p_ip2 = points[i + 2];

            // b[1] = p_i + (s * p_ip1 - s * p_im1) / 6
            // b[2] = p_ip1 + (s * p_i - s * p_ip2) / 6
            // b[3] = p_ip1
            RoughOp curveOp;
            curveOp.op = RoughOpType::BCurveTo;
            curveOp.data_len = 6;
            curveOp.data[0] = p_i.x + (s * p_ip1.x - s * p_im1.x) / 6.0;
            curveOp.data[1] = p_i.y + (s * p_ip1.y - s * p_im1.y) / 6.0;
            curveOp.data[2] = p_ip1.x + (s * p_i.x - s * p_ip2.x) / 6.0;
            curveOp.data[3] = p_ip1.y + (s * p_i.y - s * p_ip2.y) / 6.0;
            curveOp.data[4] = p_ip1.x;
            curveOp.data[5] = p_ip1.y;
            ops.push_back(std::move(curveOp));
        }

        // (lines 406-409) optional close point with random offset
        if (close_point != nullptr) {
            const double ro = params.max_randomness_offset;
            RoughOp closeOp;
            closeOp.op = RoughOpType::LineTo;
            closeOp.data[0] = close_point->x + rand_offset(ro, rng, params.roughness);
            closeOp.data[1] = close_point->y + rand_offset(ro, rng, params.roughness);
            closeOp.data_len = 2;
            ops.push_back(std::move(closeOp));
        }
    } else if (len == 3) {
        // (lines 410-419) three points → single degenerate bezier
        RoughOp moveOp;
        moveOp.op = RoughOpType::Move;
        moveOp.data[0] = points[1].x;
        moveOp.data[1] = points[1].y;
        moveOp.data_len = 2;
        ops.push_back(std::move(moveOp));

        RoughOp curveOp;
        curveOp.op = RoughOpType::BCurveTo;
        curveOp.data_len = 6;
        curveOp.data[0] = points[1].x;
        curveOp.data[1] = points[1].y;
        curveOp.data[2] = points[2].x;
        curveOp.data[3] = points[2].y;
        curveOp.data[4] = points[2].x;
        curveOp.data[5] = points[2].y;
        ops.push_back(std::move(curveOp));
    } else if (len == 2) {
        // (lines 420-422) two points → single line segment with overlay style
        return _line(points[0].x, points[0].y,
                     points[1].x, points[1].y,
                     params, rng, true, true);
    }

    return ops;
}

// ═══════════════════════════════════════════════════════════════════════════════
// generate_fill_pattern — fill pattern ops for a polygon
// ───────────────────────────────────────────────────────────────────────────────
// NOTE: The *implementation* of generate_fill_pattern now lives in
// rough_filler.h as an inline function.  The declaration in rough.h stays
// for forward-reference.  Keep this file's definition removed to avoid ODR
// violations — all callers must include rough_filler.h (which already
// includes rough.h) to get the inline definition.
//
// The individual fillers (hachure, zigzag, cross-hatch, dots, dashed, solid)
// are implemented in rough_filler.cpp.
// ═══════════════════════════════════════════════════════════════════════════════
// (empty — see rough_filler.h / rough_filler.cpp)

} // namespace pml
