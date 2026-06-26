#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Polygon Edge Perturbation
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Core algorithm for perturbing polygon edges with Perlin noise, Catmull-Rom
// subdivision, and arc (bezier) corner rounding.
//
// All perturbation happens at construction time — the render backend sees
// the final perturbed vertex list.
// ==========================================================================================================================================================================================================================================═

#include "perlin_noise.h"
#include "rough.h"  // for pml::RoughPoint

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// PerturbConfig — per-edge perturbation parameters
// ==========================================================================================================================================================================================================================================═
//
// All vector fields are indexed by edge: edge i = vertex[i] → vertex[(i+1) % n].
// Scalar values are automatically expanded to vectors of length n.

struct PerturbConfig {
    /// Per-edge noise amplitude as fraction of edge length.  0 = no perturbation.
    /// Scalar: applied uniformly to all edges.
    std::vector<double> edge_noise;

    /// Per-edge mask: true = perturb this edge, false = keep straight.
    /// Scalar bool or vector<bool>.
    std::vector<bool>   edge_mask;

    /// Per-edge subdivision count (number of Catmull-Rom interpolation points
    /// inserted between the original edge endpoints).  0 = no subdivision.
    /// Scalar int or vector<int>.
    std::vector<int>    edge_subdivisions;

    /// Per-corner radius for arc rounding.  0 = sharp corner.
    /// Scalar or vector.  Automatically clamped to ≤ half the shorter adjacent edge.
    std::vector<double> corner_radius;

    /// Per-corner mask: true = round this corner, false = leave sharp.
    std::vector<bool>   corner_mask;

    /// Seed for Perlin noise (ensures deterministic output).
    int seed{0};
};

// ==========================================================================================================================================================================================================================================═
// PerturbResult — the result of perturbing a polygon
// ==========================================================================================================================================================================================================================================═
//
// The perturbed polygon is stored as a list of edges.  Each edge is a list of
// 2D points forming that edge segment.  Concatenating all edges (in order)
// gives the full perturbed polygon outline.

using PerturbResult = std::vector<std::vector<RoughPoint>>;

// ==========================================================================================================================================================================================================================================═
// Helper: expand a scalar vector (size 1) to length n, or return as-is
// ==========================================================================================================================================================================================================================================═

/// Expand a scalar (size-1) config field to match the polygon's edge/corner count.
/// If the input already matches `n`, return it unchanged.
/// Throws std::invalid_argument if size is neither 1 nor n.

std::vector<double> expand_scalar(const std::vector<double>& input, size_t n);
std::vector<bool>   expand_scalar(const std::vector<bool>& input,   size_t n);
std::vector<int>    expand_scalar(const std::vector<int>& input,    size_t n);

// ==========================================================================================================================================================================================================================================═
// Main perturbation function
// ==========================================================================================================================================================================================================================================═
//
// Takes a list of polygon vertices (in order, closed polygon implied) and a
// config, returns a list of perturbed edges (each edge is a list of points).
//
// The caller is responsible for concatenating edges into a flat vertex list
// (e.g. for passing to `polygon`).
//
// Throws std::invalid_argument for degenerate inputs (< 3 vertices).

PerturbResult perturb_polygon(
    const std::vector<RoughPoint>& vertices,
    const PerturbConfig& config,
    const PerlinNoise2D& noise
);

// ==========================================================================================================================================================================================================================================═
// Utility: flatten a PerturbResult into a single vector of points
// ==========================================================================================================================================================================================================================================═

std::vector<RoughPoint> flatten_perturb_result(const PerturbResult& result);

// ==========================================================================================================================================================================================================================================═
// Utility: compute Catmull-Rom interpolation points between p1 and p2
// ==========================================================================================================================================================================================================================================═
//
// Uses p0 and p3 as tangents for the spline (p0 is the vertex before p1,
// p3 is the vertex after p2 in the polygon order).
//
// Returns `count` interpolated points (not including p1 and p2).

std::vector<RoughPoint> catmull_rom_subdivide(
    const RoughPoint& p0, const RoughPoint& p1,
    const RoughPoint& p2, const RoughPoint& p3,
    int count
);

// ==========================================================================================================================================================================================================================================═
// Utility: approximate a circular arc corner with cubic bezier curves
// ==========================================================================================================================================================================================================================================═
//
// Given three consecutive vertices A--B--C (B is the corner vertex), rounds the
// corner with a circular arc of `radius` using cubic bezier approximation.
//
// Returns a list of points forming the arc from the inset point on AB to the
// inset point on BC.

std::vector<RoughPoint> round_corner_bezier(
    const RoughPoint& A, const RoughPoint& B, const RoughPoint& C,
    double radius
);

} // namespace pml
