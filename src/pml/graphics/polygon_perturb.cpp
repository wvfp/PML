// ==========================================================================================================================================================================================================================================═
// PML Polygon Edge Perturbation — Implementation
// ==========================================================================================================================================================================================================================================═

#include "polygon_perturb.h"

#include "perlin_noise.h"
#include "rough.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// Helpers
// ==========================================================================================================================================================================================================================================═

static double vec_len(double x, double y) noexcept {
    return std::sqrt(x * x + y * y);
}

static double dot(double ax, double ay, double bx, double by) noexcept {
    return ax * bx + ay * by;
}

// ==========================================================================================================================================================================================================================================═
// Scalar expansion
// ==========================================================================================================================================================================================================================================═

std::vector<double> expand_scalar(const std::vector<double>& input, size_t n) {
    if (input.size() == n) return input;
    if (input.size() == 1) return std::vector<double>(n, input[0]);
    throw std::invalid_argument(
        "expand_scalar(double): expected size 1 or " + std::to_string(n)
        + ", got " + std::to_string(input.size()));
}

std::vector<bool> expand_scalar(const std::vector<bool>& input, size_t n) {
    if (input.size() == n) return input;
    if (input.size() == 1) return std::vector<bool>(n, input[0]);
    throw std::invalid_argument(
        "expand_scalar(bool): expected size 1 or " + std::to_string(n)
        + ", got " + std::to_string(input.size()));
}

std::vector<int> expand_scalar(const std::vector<int>& input, size_t n) {
    if (input.size() == n) return input;
    if (input.size() == 1) return std::vector<int>(n, input[0]);
    throw std::invalid_argument(
        "expand_scalar(int): expected size 1 or " + std::to_string(n)
        + ", got " + std::to_string(input.size()));
}

// ==========================================================================================================================================================================================================================================═
// Catmull-Rom subdivision
// ==========================================================================================================================================================================================================================================═
//
// Standard uniform Catmull-Rom with τ = 0.5 (centripetal).
// q(t) = 0.5 * ((2*P1) + (-P0+P2)*t + (2*P0-5*P1+4*P2-P3)*t² + (-P0+3*P1-3*P2+P3)*t³)
// P0..P3 are the four control points; the curve runs from P1 to P2.

std::vector<RoughPoint> catmull_rom_subdivide(
    const RoughPoint& p0, const RoughPoint& p1,
    const RoughPoint& p2, const RoughPoint& p3,
    int count)
{
    if (count <= 0) return {};

    std::vector<RoughPoint> result;
    result.reserve(count);

    for (int i = 1; i <= count; ++i) {
        double t = static_cast<double>(i) / static_cast<double>(count + 1);

        double t2 = t * t;
        double t3 = t2 * t;

        double x = 0.5 * (
            (2.0 * p1.x) +
            (-p0.x + p2.x) * t +
            (2.0 * p0.x - 5.0 * p1.x + 4.0 * p2.x - p3.x) * t2 +
            (-p0.x + 3.0 * p1.x - 3.0 * p2.x + p3.x) * t3
        );
        double y = 0.5 * (
            (2.0 * p1.y) +
            (-p0.y + p2.y) * t +
            (2.0 * p0.y - 5.0 * p1.y + 4.0 * p2.y - p3.y) * t2 +
            (-p0.y + 3.0 * p1.y - 3.0 * p2.y + p3.y) * t3
        );

        result.push_back({x, y});
    }

    return result;
}

// ==========================================================================================================================================================================================================================================═
// Bezier corner rounding
// ==========================================================================================================================================================================================================================================═
//
// Approximate a circular arc fillet at vertex B using a single cubic Bezier.
//   A--B--C: three consecutive polygon vertices.  The arc replaces the sharp
//   corner, touching the two edges at inset points.
//
// Arc center lies on the angle bisector.  Tangent distance from B:
//   d = r / tan(θ/2)  where θ = angle between BA and BC (interior angle).
//
// Cubic Bezier control point distance from the tangent points:
//   k = (4/3) * r * tan(θ/4)
//
// Clamps radius to ≤ min(|AB|, |BC|) / 2 so the arc stays within both edges.

std::vector<RoughPoint> round_corner_bezier(
    const RoughPoint& A, const RoughPoint& B, const RoughPoint& C,
    double radius)
{
    if (radius <= 0.0) return {};

    // Edge vectors (pointing TOWARD B).
    double v1x = A.x - B.x;
    double v1y = A.y - B.y;
    double v2x = C.x - B.x;
    double v2y = C.y - B.y;

    double len1 = vec_len(v1x, v1y);
    double len2 = vec_len(v2x, v2y);

    if (len1 < 1e-12 || len2 < 1e-12) return {};

    // Normalized edge directions (from B toward the adjacent vertices).
    double u1x = v1x / len1;  // from B toward A
    double u1y = v1y / len1;
    double u2x = v2x / len2;  // from B toward C
    double u2y = v2y / len2;

    // Interior angle at B.
    double cos_theta = dot(u1x, u1y, u2x, u2y);
    cos_theta = std::clamp(cos_theta, -1.0, 1.0);
    double theta = std::acos(cos_theta);

    // Clamp radius to half the shorter adjacent edge.
    double max_r = std::min(len1, len2) * 0.5;
    double r = std::min(radius, max_r);

    // For nearly-straight or fully-straight angles, skip rounding.
    if (theta < 0.01 || theta > std::numbers::pi_v<double> - 0.01) return {};

    // Distance from B to tangent point along each edge.
    // d = r / tan(θ/2)
    double half_theta = theta * 0.5;
    double d = r / std::tan(half_theta);

    // Tangent points on each edge (inset from B).
    double t1x = B.x + u1x * d;
    double t1y = B.y + u1y * d;
    double t2x = B.x + u2x * d;
    double t2y = B.y + u2y * d;

    // Cubic Bezier approximation of a circular arc of angle θ.
    // Control point offset distance from the tangent points:
    //   k = (4/3) * r * tan(θ/4)
    double quarter_theta = theta * 0.25;
    double k = (4.0 / 3.0) * r * std::tan(quarter_theta);

    // Arc center direction: unit angle bisector from B pointing into the interior.
    // u1 and u2 point outward from B toward the adjacent vertices; their sum
    // points outward, so the interior direction is the negative bisector.
    double bis_x = u1x + u2x;
    double bis_y = u1y + u2y;
    double bis_len = vec_len(bis_x, bis_y);
    if (bis_len < 1e-12) return {};
    bis_x /= bis_len;
    bis_y /= bis_len;

    // Control points: offset from each tangent point opposite to the outward
    // bisector (toward the polygon interior / arc center).
    double c1x = t1x - bis_x * k;
    double c1y = t1y - bis_y * k;
    double c2x = t2x - bis_x * k;
    double c2y = t2y - bis_y * k;

    // Generate points along the cubic Bezier t1 → c1 → c2 → t2.
    // Use NumSteps = ceil(θ * 4 / π) + 1  (~8 for a right angle, ~16 for a straight angle).
    int num_steps = std::max(4, static_cast<int>(std::ceil(theta * 4.0 / std::numbers::pi_v<double>)) + 1);

    std::vector<RoughPoint> arc_points;
    arc_points.reserve(num_steps + 1);

    for (int i = 0; i <= num_steps; ++i) {
        double t = static_cast<double>(i) / static_cast<double>(num_steps);
        double t2 = t * t;
        double t3 = t2 * t;
        double mt = 1.0 - t;
        double mt2 = mt * mt;
        double mt3 = mt2 * mt;

        // Cubic Bezier: B(t) = (1-t)³·P0 + 3(1-t)²·t·P1 + 3(1-t)·t²·P2 + t³·P3
        double px = mt3 * t1x + 3.0 * mt2 * t * c1x + 3.0 * mt * t2 * c2x + t3 * t2x;
        double py = mt3 * t1y + 3.0 * mt2 * t * c1y + 3.0 * mt * t2 * c2y + t3 * t2y;

        arc_points.push_back({px, py});
    }

    return arc_points;
}

// ==========================================================================================================================================================================================================================================═
// Main perturbation function
// ==========================================================================================================================================================================================================================================═

PerturbResult perturb_polygon(
    const std::vector<RoughPoint>& vertices,
    const PerturbConfig& config,
    const PerlinNoise2D& noise)
{
    size_t n = vertices.size();
    if (n < 3) {
        throw std::invalid_argument(
            "perturb_polygon: at least 3 vertices required, got " + std::to_string(n));
    }

    // ---- Expand all scalar config fields --------------------------------------------------------------------

    auto edge_noise       = expand_scalar(config.edge_noise,       n);
    auto edge_mask        = expand_scalar(config.edge_mask,        n);
    auto edge_subdiv      = expand_scalar(config.edge_subdivisions, n);
    auto corner_radius    = expand_scalar(config.corner_radius,    n);
    auto corner_mask      = expand_scalar(config.corner_mask,      n);

    // ---- Pre-compute edge lengths --------------------------------------------------------------------------------

    std::vector<double> edge_lengths(n);
    for (size_t i = 0; i < n; ++i) {
        size_t j = (i + 1) % n;
        double dx = vertices[j].x - vertices[i].x;
        double dy = vertices[j].y - vertices[i].y;
        edge_lengths[i] = vec_len(dx, dy);
    }

    // ---- Process each edge ------------------------------------------------------------------------------------------------
    //
    // For each edge i (v[i] → v[(i+1)%n]):
    //   1. If edge_mask[i] is false: emit just the two endpoints.
    //   2. If edge_mask[i] is true:
    //      a. Subdivide using Catmull-Rom through adjacent vertices.
    //      b. Displace each subdivision point by Perlin noise along the
    //         edge normal.  Displacement amplitude = edge_noise[i] * edge_length[i].

    PerturbResult edges(n);

    for (size_t i = 0; i < n; ++i) {
        size_t j = (i + 1) % n;

        // Four control points for Catmull-Rom: p0, p1, p2, p3.
        size_t i_prev = (i == 0) ? n - 1 : i - 1;
        size_t j_next = (j + 1) % n;

        const RoughPoint& p0 = vertices[i_prev];
        const RoughPoint& p1 = vertices[i];
        const RoughPoint& p2 = vertices[j];
        const RoughPoint& p3 = vertices[j_next];

        // Edge direction and normal.
        double dx = p2.x - p1.x;
        double dy = p2.y - p1.y;
        double len = edge_lengths[i];

        if (len < 1e-12) {
            // Degenerate edge: emit just the vertex.
            edges[i] = {{p1.x, p1.y}};
            continue;
        }

        // Unit normal (perpendicular to edge, rotated 90° CCW).
        double nx = -dy / len;
        double ny =  dx / len;

        bool do_perturb = edge_mask[i] && edge_noise[i] > 0.0;

        // Adaptive subdivision: when edge_subdiv[i] is 0 or negative,
        // auto-compute subdivision count proportional to edge length
        // (~1 subdivision per 8 pixels) so that long edges get enough
        // sampling points for smooth noise perturbation.
        int subdiv = edge_subdiv[i];
        if (subdiv <= 0 && (do_perturb || edge_subdiv[i] < 0)) {
            subdiv = std::max(1, static_cast<int>(std::ceil(len / 8.0)));
        }

        if (!do_perturb && subdiv <= 0) {
            // No perturbation, no subdivision: emit just endpoints.
            edges[i].push_back(p1);
            edges[i].push_back(p2);
            continue;
        }

        // Start with the first endpoint.
        std::vector<RoughPoint> seg_points;
        seg_points.push_back(p1);  // v[i]

        if (subdiv > 0) {
            // Catmull-Rom subdivision points between p1 and p2.
            auto sub_points = catmull_rom_subdivide(p0, p1, p2, p3, subdiv);
            seg_points.insert(seg_points.end(), sub_points.begin(), sub_points.end());
        }

        seg_points.push_back(p2);  // v[i+1]

        // Apply noise displacement.
        if (do_perturb) {
            double amplitude = edge_noise[i] * len;
            for (auto& pt : seg_points) {
                double displacement = noise.noise(pt.x, pt.y);
                // Map from [-1, 1] to [-amplitude, amplitude].
                pt.x += displacement * amplitude * nx;
                pt.y += displacement * amplitude * ny;
            }
        }

        edges[i] = std::move(seg_points);
    }

    // ---- Corner rounding ----------------------------------------------------------------------------------------------------
    //
    // For each vertex i (corner between edge[i-1] and edge[i]):
    //   1. If corner_mask[i] is false: keep the original corner point.
    //   2. If corner_mask[i] is true and corner_radius[i] > 0:
    //      a. Use the original vertices (i-1, i, i+1) to compute a circular
    //         arc that replaces the sharp corner.
    //      b. The arc starts on edge[i-1] near v[i] and ends on edge[i]
    //         near v[i], staying inside the polygon.
    //      c. Replace the overlap region in both edges with the arc points.

    for (size_t i = 0; i < n; ++i) {
        if (!corner_mask[i] || corner_radius[i] <= 0.0) continue;

        size_t i_prev = (i == 0) ? n - 1 : i - 1;
        size_t i_next = (i + 1) % n;

        auto& prev_edge = edges[i_prev];
        auto& curr_edge = edges[i];

        if (prev_edge.empty() || curr_edge.empty()) continue;

        // Use original vertices to compute the arc geometry.
        const RoughPoint& A = vertices[i_prev];
        const RoughPoint& B = vertices[i];
        const RoughPoint& C = vertices[i_next];

        // corner_radius is an absolute length (not a fraction of edge length).
        // Clamp to half the shorter adjacent edge so the arc stays inside.
        double max_adj_len = std::min(edge_lengths[i_prev], edge_lengths[i]);
        double arc_radius = std::min(corner_radius[i], max_adj_len * 0.5);

        auto arc = round_corner_bezier(A, B, C, arc_radius);

        if (arc.empty()) continue;

        // The arc replaces the portion of both edges near the corner vertex.
        // Remove the last point of prev_edge (at/near v[i]) and the first
        // point of curr_edge (also at/near v[i]), then insert the arc.
        prev_edge.pop_back();
        curr_edge.erase(curr_edge.begin());

        // If prev_edge now ends at the same point arc starts with, skip
        // the first arc point to avoid a degenerate segment.
        size_t arc_start = 0;
        if (!prev_edge.empty() && arc.size() > 1) {
            double dx = prev_edge.back().x - arc.front().x;
            double dy = prev_edge.back().y - arc.front().y;
            if (dx * dx + dy * dy < 1e-12)
                arc_start = 1;
        }

        // Append remaining arc points to prev_edge.
        for (size_t ai = arc_start; ai < arc.size(); ++ai)
            prev_edge.push_back(arc[ai]);
    }

    return edges;
}

// ==========================================================================================================================================================================================================================================═
// Flatten result
// ==========================================================================================================================================================================================================================================═

std::vector<RoughPoint> flatten_perturb_result(const PerturbResult& result) {
    std::vector<RoughPoint> flat;
    for (const auto& edge : result) {
        flat.insert(flat.end(), edge.begin(), edge.end());
    }
    return flat;
}

} // namespace pml
