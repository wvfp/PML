#pragma once

// ==========================================================================================================================================================================================================================================═
// PML UV Solvers — Planar, Harmonic, and Explicit UV coordinate generation
// ==========================================================================================================================================================================================================================================═

#include <cstdint>
#include <vector>

#include "pml/core/types.h"
#include "pml/graphics/graphics_types.h"

namespace pml {

/// Result of a UV projection solver.
struct UVResult {
    std::vector<Vec2> uvs;  ///< Per-vertex UV coordinates, same count as input vertices.
    bool              valid = false;
};

/// Planar UV: simple AABB-based projection.
/// Maps each vertex's position to [0,1]² based on the bounding box.
UVResult solve_planar_uv(const std::vector<Vec2>& mesh_vertices);

/// Harmonic UV: solves the Laplace equation with cotangent Laplacian weights.
/// Boundary vertices are fixed via arc-length parameterisation of the boundary
/// to the unit square perimeter.
/// @param mesh_vertices   All mesh vertices (including Steiner points).
/// @param indices         Triangle indices (3 per triangle, CCW).
/// @param boundary_vertices  Indices into mesh_vertices for boundary vertices.
UVResult solve_harmonic_uv(
    const std::vector<Vec2>& mesh_vertices,
    const std::vector<uint32_t>& indices,
    const std::vector<int>& boundary_vertices
);

/// Explicit UV: use user-provided per-vertex UVs.
/// If user_uvs.size() < mesh_vertices.size(), the extra vertices (Steiner
/// points) get planar-fallback UVs.
/// @param user_uvs    User-provided UV coordinates.
/// @param mesh_vertices  Mesh vertices (for dimension checking / fallback).
/// @param wrap        Clamp UVs to [0,1] when true.
UVResult apply_explicit_uv(
    const std::vector<Vec2>& user_uvs,
    const std::vector<Vec2>& mesh_vertices,
    bool wrap = true
);

} // namespace pml
