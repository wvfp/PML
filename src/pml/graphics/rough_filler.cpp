// ═══════════════════════════════════════════════════════════════════════════════
// PML Rough-Style Fill Pattern Generators — Implementation
// ───────────────────────────────────────────────────────────────────────────────
// Port of rough.js fillers (hachure, zigzag, cross-hatch, dots, dashed) plus
// the scanline hachure engine from the 'hachure-fill' npm package v0.5.2.
//
// Reference:
//   https://github.com/rough-stuff/rough/blob/main/src/fillers/
//   https://github.com/rough-stuff/hachure-fill
// ═══════════════════════════════════════════════════════════════════════════════

#include "rough_filler.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Internal helpers — scanline hachure engine
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kBezierKappa = 0.5522847498;  // 4*(sqrt(2)-1)/3

// ─────────────────────────────────────────────────────────────────────────────
// Point rotation helpers (port of hachure-fill rotatePoints / rotateLines)
// ─────────────────────────────────────────────────────────────────────────────

RoughPoint rotate_point(const RoughPoint& p, double cos_a, double sin_a)
{
    return {
        p.x * cos_a - p.y * sin_a,
        p.x * sin_a + p.y * cos_a
    };
}

void rotate_polygon_inplace(std::vector<RoughPoint>& polygon, double angle_deg)
{
    if (angle_deg == 0.0) return;
    const double rad = angle_deg * kPi / 180.0;
    const double cos_a = std::cos(rad);
    const double sin_a = std::sin(rad);
    for (auto& pt : polygon) {
        const double nx = pt.x * cos_a - pt.y * sin_a;
        const double ny = pt.x * sin_a + pt.y * cos_a;
        pt.x = nx;
        pt.y = ny;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Edge table types for the scanline active-edge algorithm
// ─────────────────────────────────────────────────────────────────────────────

struct Edge {
    double ymin;
    double ymax;
    double x;       // current x at the scanline y position
    double islope;  // dx/dy
};

struct ActiveEdge {
    double y_start;  // y at which this edge was activated
    Edge edge;
};

// ─────────────────────────────────────────────────────────────────────────────
// straightHachureLines — scanline fill on axis-aligned (unrotated) polygons
//
// Port of hachure-fill's `straightHachureLines()`.
//   polygons  — closed, non-self-intersecting polygon vertex arrays
//   gap       — spacing between scanlines (≥ 0.1)
//   stepOffset — y increment per iteration (1 = normal, gap = rough skip)
//
// Returns horizontal line segments (x0, y) → (x1, y) with y descending.
// ─────────────────────────────────────────────────────────────────────────────

using LinePair = std::pair<RoughPoint, RoughPoint>;

std::vector<LinePair> straight_hachure_lines(
    const std::vector<std::vector<RoughPoint>>& polygons,
    double gap, double stepOffset)
{
    // ── Build vertex arrays, closing each polygon ──────────────────────────
    std::vector<std::vector<RoughPoint>> vertexArrays;
    for (const auto& poly : polygons) {
        if (poly.size() < 2) continue;  // skip degenerate
        std::vector<RoughPoint> verts = poly;
        // Close polygon if not already closed
        const auto& front = verts.front();
        const auto& back = verts.back();
        if (front.x != back.x || front.y != back.y) {
            verts.push_back(front);
        }
        if (verts.size() > 2) {
            vertexArrays.push_back(std::move(verts));
        }
    }

    // ── Build sorted edge table ────────────────────────────────────────────
    struct SortEdge {
        double ymin;
        double ymax;
        double x;
        double islope;
    };
    std::vector<SortEdge> sortEdges;

    for (const auto& verts : vertexArrays) {
        for (size_t i = 0; i + 1 < verts.size(); ++i) {
            const auto& p1 = verts[i];
            const auto& p2 = verts[i + 1];
            // Skip horizontal edges (no intersection range)
            if (std::abs(p1.y - p2.y) < 1e-12) continue;

            const double ymin = std::min(p1.y, p2.y);
            const double ymax = std::max(p1.y, p2.y);
            const double x_at_ymin = (ymin == p1.y) ? p1.x : p2.x;
            const double islope = (p2.x - p1.x) / (p2.y - p1.y);
            sortEdges.push_back({ymin, ymax, x_at_ymin, islope});
        }
    }

    if (sortEdges.empty()) return {};

    // Sort by ymin ascending, then x ascending, then ymax ascending
    std::sort(sortEdges.begin(), sortEdges.end(),
        [](const SortEdge& a, const SortEdge& b) {
            if (a.ymin < b.ymin) return true;
            if (a.ymin > b.ymin) return false;
            if (a.x < b.x) return true;
            if (a.x > b.x) return false;
            return a.ymax < b.ymax;
        });

    // ── Scanline loop (active edge table) ──────────────────────────────────
    std::vector<ActiveEdge> activeEdges;
    std::vector<LinePair> lines;
    double y = sortEdges[0].ymin;
    size_t edgeIdx = 0;
    int iteration = 0;

    while (edgeIdx < sortEdges.size() || !activeEdges.empty()) {
        // Add edges whose ymin ≤ current y
        while (edgeIdx < sortEdges.size() && sortEdges[edgeIdx].ymin <= y) {
            const auto& se = sortEdges[edgeIdx];
            activeEdges.push_back({y, {se.ymin, se.ymax, se.x, se.islope}});
            ++edgeIdx;
        }

        // Remove edges whose ymax ≤ y
        activeEdges.erase(
            std::remove_if(activeEdges.begin(), activeEdges.end(),
                [y](const ActiveEdge& ae) { return ae.edge.ymax <= y; }),
            activeEdges.end());

        // Sort active edges by x ascending
        std::sort(activeEdges.begin(), activeEdges.end(),
            [](const ActiveEdge& a, const ActiveEdge& b) {
                return a.edge.x < b.edge.x;
            });

        // Emit paired lines (even-odd rule)
        // Port of JS: if (hachureStepOffset !== 1 || iteration % gap === 0)
        const bool emitLine = (stepOffset != 1.0)
            || (std::fmod(static_cast<double>(iteration), gap) < 1e-9);

        if (emitLine && activeEdges.size() >= 2) {
            for (size_t i = 0; i + 1 < activeEdges.size(); i += 2) {
                const auto& ce = activeEdges[i].edge;
                const auto& ne = activeEdges[i + 1].edge;
                lines.push_back({
                    {std::round(ce.x), y},
                    {std::round(ne.x), y}
                });
            }
        }

        // Advance y
        y += stepOffset;
        for (auto& ae : activeEdges) {
            ae.edge.x += stepOffset * ae.edge.islope;
        }
        ++iteration;
    }

    return lines;
}

// ─────────────────────────────────────────────────────────────────────────────
// polygonHachureLines — public scanline entry point
//
// Rotates polygon by angle, runs straight hachure, rotates lines back.
// Port of scan-line-hachure.ts polygonHachureLines().
// ─────────────────────────────────────────────────────────────────────────────

std::vector<LinePair> polygon_hachure_lines(
    const std::vector<RoughPoint>& polygon,
    double angleDeg, double gap, double stepOffset)
{
    gap = std::max(gap, 0.1);
    if (polygon.size() < 3) return {};

    // Rotate polygon
    auto rotated = polygon;
    rotate_polygon_inplace(rotated, angleDeg);

    // Compute scanlines
    std::vector<std::vector<RoughPoint>> polys = {std::move(rotated)};
    auto lines = straight_hachure_lines(polys, gap, stepOffset);

    // Rotate lines back
    if (angleDeg != 0.0 && !lines.empty()) {
        const double rad = -angleDeg * kPi / 180.0;
        const double cos_a = std::cos(rad);
        const double sin_a = std::sin(rad);
        for (auto& line : lines) {
            line.first = rotate_point(line.first, cos_a, sin_a);
            line.second = rotate_point(line.second, cos_a, sin_a);
        }
    }

    return lines;
}

// ─────────────────────────────────────────────────────────────────────────────
// resolve_gap — determine effective scanline gap from params
// ─────────────────────────────────────────────────────────────────────────────

double resolve_gap(RoughStyleParams& params)
{
    double gap = params.hachure_gap;
    if (gap < 0) gap = 8.0;  // default when no strokeWidth is known
    return std::round(std::max(gap, 0.1));
}

// ─────────────────────────────────────────────────────────────────────────────
// resolve_fweight — determine effective fill line weight
// ─────────────────────────────────────────────────────────────────────────────

double resolve_fweight(RoughStyleParams& params)
{
    double fw = params.fill_weight;
    if (fw < 0) fw = 2.0;  // default when no strokeWidth is known
    return std::max(fw, 0.1);
}

// ─────────────────────────────────────────────────────────────────────────────
// resolve_step_offset — decide whether to skip scanlines (roughness ≥ 1)
// ─────────────────────────────────────────────────────────────────────────────

double resolve_step_offset(RoughStyleParams& params, RoughRandom& rng, double gap)
{
    double step = 1.0;
    if (params.roughness >= 1.0 && rng.next() > 0.7) {
        step = gap;
    }
    return step;
}

// ─────────────────────────────────────────────────────────────────────────────
// render_lines — call rough_double_line for each scanline segment
// ─────────────────────────────────────────────────────────────────────────────

std::vector<RoughOp> render_lines(
    const std::vector<LinePair>& lines,
    RoughStyleParams& params, RoughRandom& rng)
{
    std::vector<RoughOp> ops;
    ops.reserve(lines.size() * 2);  // each line → move + bcurve × 2 (rough_double)
    for (const auto& line : lines) {
        auto seg = rough_double_line(
            line.first.x, line.first.y,
            line.second.x, line.second.y,
            params, rng, true);  // filling=true
        ops.insert(ops.end(), seg.begin(), seg.end());
    }
    return ops;
}

// ─────────────────────────────────────────────────────────────────────────────
// render_ellipse_dot — generate a rough ellipse for a single dot
// ─────────────────────────────────────────────────────────────────────────────

std::vector<RoughOp> render_ellipse_dot(
    double cx, double cy, double rx, double ry,
    RoughStyleParams& params, RoughRandom& rng)
{
    auto points = compute_rough_ellipse_points(cx, cy, rx, ry, params, rng);
    if (points.empty()) return {};
    // The ellipse closure point: roughly the "start" of the ellipse
    RoughPoint closePt = {cx - rx, cy};
    return rough_curve(points, &closePt, params, rng);
}

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// HachureFiller
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<RoughOp> hachure_fill_polygon(
    const std::vector<RoughPoint>& polygon,
    RoughStyleParams& params, RoughRandom& rng)
{
    const double gap = resolve_gap(params);
    const double angle = params.hachure_angle + 90.0;
    const double step = resolve_step_offset(params, rng, gap);

    auto lines = polygon_hachure_lines(polygon, angle, gap, step);
    return render_lines(lines, params, rng);
}

// ═══════════════════════════════════════════════════════════════════════════════
// ZigZagFiller — extends Hachure: offset each line by ± dgx,dgy
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<RoughOp> zigzag_fill_polygon(
    const std::vector<RoughPoint>& polygon,
    RoughStyleParams& params, RoughRandom& rng)
{
    const double gap = resolve_gap(params);
    const double angle = params.hachure_angle + 90.0;
    const double step = resolve_step_offset(params, rng, gap);

    auto lines = polygon_hachure_lines(polygon, angle, gap, step);

    // Build zigzag offset
    const double zigRad = params.hachure_angle * kPi / 180.0;
    const double dgx = (gap / 2.0) * std::cos(zigRad);
    const double dgy = (gap / 2.0) * std::sin(zigRad);

    std::vector<LinePair> zigzagLines;
    zigzagLines.reserve(lines.size() * 2);

    for (const auto& line : lines) {
        const double dx = line.first.x - line.second.x;
        const double dy = line.first.y - line.second.y;
        const double lenSq = dx * dx + dy * dy;
        if (lenSq > 1e-12) {
            // Zig 1: p1 offset by (-dgx, +dgy) → p2 (unchanged)
            zigzagLines.push_back({
                {line.first.x - dgx, line.first.y + dgy},
                line.second
            });
            // Zig 2: p1 offset by (+dgx, -dgy) → p2 (unchanged)
            zigzagLines.push_back({
                {line.first.x + dgx, line.first.y - dgy},
                line.second
            });
        }
    }

    return render_lines(zigzagLines, params, rng);
}

// ═══════════════════════════════════════════════════════════════════════════════
// CrossHatchFiller — hachure pass at angle + hachure pass at angle+90
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<RoughOp> cross_hatch_fill_polygon(
    const std::vector<RoughPoint>& polygon,
    RoughStyleParams& params, RoughRandom& rng)
{
    // First pass: normal hachure
    auto ops = hachure_fill_polygon(polygon, params, rng);

    // Second pass: rotate angle by 90 degrees
    const double savedAngle = params.hachure_angle;
    params.hachure_angle = savedAngle + 90.0;
    auto ops2 = hachure_fill_polygon(polygon, params, rng);
    params.hachure_angle = savedAngle;  // restore

    ops.insert(ops.end(), ops2.begin(), ops2.end());
    return ops;
}

// ═══════════════════════════════════════════════════════════════════════════════
// DotFiller — place rough ellipses along hachure scanlines
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<RoughOp> dot_fill_polygon(
    const std::vector<RoughPoint>& polygon,
    RoughStyleParams& params, RoughRandom& rng)
{
    const double gap = resolve_gap(params);
    // Dot filler uses hachureAngle = 0
    const double savedAngle = params.hachure_angle;
    params.hachure_angle = 0.0;
    const double step = resolve_step_offset(params, rng, gap);

    auto lines = polygon_hachure_lines(polygon, 90.0, gap, step);
    params.hachure_angle = savedAngle;  // restore

    if (lines.empty()) return {};

    const double fweight = resolve_fweight(params);
    const double ro = gap / 4.0;

    std::vector<RoughOp> ops;
    // Rough estimate: one dot per gap unit per scanline
    ops.reserve(static_cast<size_t>(lines.size() * (1.0 + 100.0 / gap)));

    for (const auto& line : lines) {
        const double dx = line.first.x - line.second.x;
        const double dy = line.first.y - line.second.y;
        const double length = std::sqrt(dx * dx + dy * dy);
        if (length < 1e-9) continue;

        const double dl = length / gap;
        const int count = static_cast<int>(std::ceil(dl)) - 1;
        if (count <= 0) continue;

        const double offset = length - static_cast<double>(count) * gap;
        const double x_mid = (line.first.x + line.second.x) / 2.0 - gap / 4.0;
        const double y_min = std::min(line.first.y, line.second.y);

        for (int i = 0; i < count; ++i) {
            const double y = y_min + offset + static_cast<double>(i) * gap;
            const double cx = (x_mid - ro) + rng.next() * 2.0 * ro;
            const double cy = (y - ro) + rng.next() * 2.0 * ro;

            auto dot = render_ellipse_dot(cx, cy, fweight, fweight, params, rng);
            ops.insert(ops.end(), dot.begin(), dot.end());
        }
    }

    return ops;
}

// ═══════════════════════════════════════════════════════════════════════════════
// DashedFiller — clip each hachure scanline into dash segments
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<RoughOp> dashed_fill_polygon(
    const std::vector<RoughPoint>& polygon,
    RoughStyleParams& params, RoughRandom& rng)
{
    const double gap = resolve_gap(params);
    const double angle = params.hachure_angle + 90.0;
    const double step = resolve_step_offset(params, rng, gap);

    auto lines = polygon_hachure_lines(polygon, angle, gap, step);
    if (lines.empty()) return {};

    // Resolve dash offset (on-segment length) and dash gap
    const double dashOffset = params.dash_offset >= 0.0
        ? params.dash_offset
        : (params.hachure_gap >= 0.0 ? params.hachure_gap : 8.0);
    const double dashGap = params.dash_gap >= 0.0
        ? params.dash_gap
        : (params.hachure_gap >= 0.0 ? params.hachure_gap : 8.0);

    std::vector<RoughOp> ops;
    ops.reserve(lines.size() * 4);

    for (const auto& line : lines) {
        const double dx = line.first.x - line.second.x;
        const double dy = line.first.y - line.second.y;
        const double length = std::sqrt(dx * dx + dy * dy);
        if (length < 1e-9) continue;

        const double totalDash = dashOffset + dashGap;
        const int count = static_cast<int>(std::floor(length / totalDash));
        if (count <= 0) continue;

        const double startOffset = (length + dashGap
            - static_cast<double>(count) * totalDash) / 2.0;

        // Ensure the line goes left-to-right for consistent angle calc
        const auto& p1 = line.first;
        const auto& p2 = line.second;
        const double alpha = std::atan2(p2.y - p1.y, p2.x - p1.x);
        const double cos_a = std::cos(alpha);
        const double sin_a = std::sin(alpha);

        for (int i = 0; i < count; ++i) {
            const double lstart = static_cast<double>(i) * totalDash + startOffset;
            const double lend = lstart + dashOffset;

            const double sx = p1.x + lstart * cos_a;
            const double sy = p1.y + lstart * sin_a;
            const double ex = p1.x + lend * cos_a;
            const double ey = p1.y + lend * sin_a;

            auto seg = rough_double_line(sx, sy, ex, ey, params, rng, true);
            ops.insert(ops.end(), seg.begin(), seg.end());
        }
    }

    return ops;
}

// ═══════════════════════════════════════════════════════════════════════════════
// SolidFiller — jittered polygon boundary for rough solid fill
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<RoughOp> solid_fill_polygon(
    const std::vector<RoughPoint>& polygon,
    RoughStyleParams& params, RoughRandom& rng)
{
    if (polygon.size() < 3) return {};

    // Port of renderer.ts solidFillPolygon(): jitter each vertex, emit Move + LineTo
    const double offset = params.max_randomness_offset;

    std::vector<RoughOp> ops;
    ops.reserve(polygon.size() + 1);

    // Move to first vertex (jittered)
    RoughOp moveOp;
    moveOp.op = RoughOpType::Move;
    moveOp.data_len = 2;
    moveOp.data[0] = polygon[0].x + rand_offset(offset, rng, params.roughness);
    moveOp.data[1] = polygon[0].y + rand_offset(offset, rng, params.roughness);
    ops.push_back(std::move(moveOp));

    // LineTo each subsequent vertex (jittered)
    for (size_t i = 1; i < polygon.size(); ++i) {
        RoughOp lineOp;
        lineOp.op = RoughOpType::LineTo;
        lineOp.data_len = 2;
        lineOp.data[0] = polygon[i].x + (params.preserve_vertices ? 0.0
            : rand_offset(offset, rng, params.roughness));
        lineOp.data[1] = polygon[i].y + (params.preserve_vertices ? 0.0
            : rand_offset(offset, rng, params.roughness));
        ops.push_back(std::move(lineOp));
    }

    // Close back to first vertex (jittered)
    RoughOp closeOp;
    closeOp.op = RoughOpType::LineTo;
    closeOp.data_len = 2;
    closeOp.data[0] = polygon[0].x + rand_offset(offset, rng, params.roughness);
    closeOp.data[1] = polygon[0].y + rand_offset(offset, rng, params.roughness);
    ops.push_back(std::move(closeOp));

    return ops;
}

} // namespace pml
