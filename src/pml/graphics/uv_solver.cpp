// ═══════════════════════════════════════════════════════════════════════════════
// PML UV Solvers — Planar, Harmonic, and Explicit UV coordinate generation
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/graphics/uv_solver.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <unordered_map>
#include <vector>

#include "pml/core/types.h"
#include "pml/graphics/cg_solver.h"

namespace pml {

// ── Planar UV ──────────────────────────────────────────────────────────────

UVResult solve_planar_uv(const std::vector<Vec2>& mesh_vertices) {
    if (mesh_vertices.empty()) {
        return {{}, false};
    }

    // Compute bounding box.
    double min_x = std::numeric_limits<double>::max();
    double min_y = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double max_y = std::numeric_limits<double>::lowest();

    for (const auto& v : mesh_vertices) {
        min_x = std::min(min_x, v.x);
        min_y = std::min(min_y, v.y);
        max_x = std::max(max_x, v.x);
        max_y = std::max(max_y, v.y);
    }

    double range_x = max_x - min_x;
    double range_y = max_y - min_y;

    // Degenerate: all vertices at the same point.
    if (range_x < 1e-30 && range_y < 1e-30) {
        return {std::vector<Vec2>(mesh_vertices.size(), Vec2{0.0, 0.0}), true};
    }

    // Handle zero-range in one axis.
    if (range_x < 1e-30) range_x = 1.0;
    if (range_y < 1e-30) range_y = 1.0;

    std::vector<Vec2> uvs;
    uvs.reserve(mesh_vertices.size());
    for (const auto& v : mesh_vertices) {
        uvs.push_back({
            (v.x - min_x) / range_x,
            (v.y - min_y) / range_y
        });
    }

    return {std::move(uvs), true};
}

// ── Harmonic UV ────────────────────────────────────────────────────────────

UVResult solve_harmonic_uv(
    const std::vector<Vec2>& mesh_vertices,
    const std::vector<uint32_t>& indices,
    const std::vector<int>& boundary_vertices
) {
    auto n = static_cast<int>(mesh_vertices.size());
    if (n == 0) return {{}, false};

    // Build set of boundary vertices for fast lookup.
    std::vector<bool> is_boundary(static_cast<size_t>(n), false);
    for (int bv : boundary_vertices) {
        if (bv >= 0 && bv < n)
            is_boundary[static_cast<size_t>(bv)] = true;
    }

    // If all vertices are boundary (no interior), just return planar UVs
    // with the boundary parameterised onto the unit square perimeter.
    bool all_boundary = true;
    for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
        if (!is_boundary[i]) { all_boundary = false; break; }
    }
    if (all_boundary) {
        // Return planar UVs for the all-boundary case.
        // In a full implementation this would arc-length parameterise
        // the boundary to the unit square perimeter.
        return solve_planar_uv(mesh_vertices);
    }

    // Build adjacency from triangle indices.
    // For each vertex i: list of neighbour vertex indices j.
    std::vector<std::vector<int>> adj(static_cast<size_t>(n));

    auto add_edge = [&](uint32_t a, uint32_t b) {
        if (a == b) return;
        auto& na = adj[static_cast<size_t>(a)];
        if (std::find(na.begin(), na.end(), static_cast<int>(b)) == na.end())
            na.push_back(static_cast<int>(b));
        auto& nb = adj[static_cast<size_t>(b)];
        if (std::find(nb.begin(), nb.end(), static_cast<int>(a)) == nb.end())
            nb.push_back(static_cast<int>(a));
    };

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        add_edge(indices[i], indices[i + 1]);
        add_edge(indices[i + 1], indices[i + 2]);
        add_edge(indices[i + 2], indices[i]);
    }

    // Arc-length parameterise boundary vertices to unit square perimeter.
    // Build ordered boundary loop.
    std::vector<int> boundary_order;
    if (!boundary_vertices.empty()) {
        boundary_order = boundary_vertices;
    } else {
        // Fallback: just use all vertices in order.
        for (int i = 0; i < n; ++i)
            boundary_order.push_back(i);
    }

    // Compute chord lengths along boundary.
    double total_length = 0.0;
    std::vector<double> arc_len(static_cast<size_t>(boundary_order.size()), 0.0);
    for (size_t i = 1; i < boundary_order.size(); ++i) {
        int vi = boundary_order[i];
        int vj = boundary_order[i - 1];
        double dx = mesh_vertices[static_cast<size_t>(vi)].x -
                    mesh_vertices[static_cast<size_t>(vj)].x;
        double dy = mesh_vertices[static_cast<size_t>(vi)].y -
                    mesh_vertices[static_cast<size_t>(vj)].y;
        double seg = std::sqrt(dx * dx + dy * dy);
        arc_len[i] = arc_len[i - 1] + seg;
        total_length += seg;
    }

    // Close the loop if it's a closed boundary.
    if (boundary_order.size() >= 3) {
        int vf = boundary_order.front();
        int vb = boundary_order.back();
        double dx = mesh_vertices[static_cast<size_t>(vf)].x -
                    mesh_vertices[static_cast<size_t>(vb)].x;
        double dy = mesh_vertices[static_cast<size_t>(vf)].y -
                    mesh_vertices[static_cast<size_t>(vb)].y;
        total_length += std::sqrt(dx * dx + dy * dy);
    }

    // Map boundary to unit square perimeter proportional to arc length.
    // Unit square perimeter goes: (0,0)→(1,0)→(1,1)→(0,1)→(0,0).
    std::vector<Vec2> fixed_uv(static_cast<size_t>(n), {0.0, 0.0});
    if (total_length > 1e-30) {
        for (size_t i = 0; i < boundary_order.size(); ++i) {
            double t = arc_len[i] / total_length; // [0, 1)
            // Map t to unit square perimeter (4 sides).
            double side = t * 4.0;
            if (side < 1.0) {
                // Bottom edge: (0,0) → (1,0)
                fixed_uv[static_cast<size_t>(boundary_order[i])] = {side, 0.0};
            } else if (side < 2.0) {
                // Right edge: (1,0) → (1,1)
                fixed_uv[static_cast<size_t>(boundary_order[i])] = {1.0, side - 1.0};
            } else if (side < 3.0) {
                // Top edge: (1,1) → (0,1)
                fixed_uv[static_cast<size_t>(boundary_order[i])] = {3.0 - side, 1.0};
            } else {
                // Left edge: (0,1) → (0,0)
                fixed_uv[static_cast<size_t>(boundary_order[i])] = {0.0, 4.0 - side};
            }
        }
    }

    // Build Laplacian system for interior vertices.
    // Use uniform weights (cotangent would require angle computation).
    // For interior i: w_ii = -sum_j w_ij, w_ij = 1 for neighbours.
    std::vector<int> interior;
    std::vector<size_t> interior_map(static_cast<size_t>(n), static_cast<size_t>(-1));
    for (int i = 0; i < n; ++i) {
        if (!is_boundary[static_cast<size_t>(i)]) {
            interior_map[static_cast<size_t>(i)] = interior.size();
            interior.push_back(i);
        }
    }

    int m = static_cast<int>(interior.size());
    if (m == 0) {
        // All vertices are boundary — return the fixed UVs directly.
        std::vector<Vec2> uvs(mesh_vertices.size());
        for (size_t i = 0; i < uvs.size(); ++i)
            uvs[i] = fixed_uv[i];
        return {std::move(uvs), true};
    }

    // Build CSR matrix for the interior Laplacian.
    // For each interior vertex i:
    //   sum_{j neighbour} (u_i - u_j) = 0
    //   degree(i) * u_i - sum_{j neighbour} u_j = 0
    // Move boundary terms to RHS.
    SparseMatrix A;
    A.n = m;
    A.row_ptr.resize(static_cast<size_t>(m) + 1, 0);

    // First pass: count nonzeros.
    int nnz = 0;
    for (int i = 0; i < m; ++i) {
        int vi = interior[static_cast<size_t>(i)];
        int deg = static_cast<int>(adj[static_cast<size_t>(vi)].size());
        nnz += deg + 1; // +1 for diagonal
    }

    A.values.reserve(static_cast<size_t>(nnz));
    A.col_indices.reserve(static_cast<size_t>(nnz));

    int row = 0;
    std::vector<double> b_u(static_cast<size_t>(m), 0.0);
    std::vector<double> b_v(static_cast<size_t>(m), 0.0);

    for (int i = 0; i < m; ++i) {
        A.row_ptr[static_cast<size_t>(i)] = row;
        int vi = interior[static_cast<size_t>(i)];
        double diag = 0.0;

        for (int nj : adj[static_cast<size_t>(vi)]) {
            double w = 1.0; // uniform weight
            if (is_boundary[static_cast<size_t>(nj)]) {
                // Move to RHS.
                b_u[static_cast<size_t>(i)] += w * fixed_uv[static_cast<size_t>(nj)].x;
                b_v[static_cast<size_t>(i)] += w * fixed_uv[static_cast<size_t>(nj)].y;
            } else {
                // Off-diagonal entry.
                A.col_indices.push_back(static_cast<int>(interior_map[static_cast<size_t>(nj)]));
                A.values.push_back(-w);
                ++row;
                diag += w;
            }
        }
        // Diagonal.
        A.col_indices.push_back(i);
        A.values.push_back(diag);
        ++row;
    }
    A.row_ptr[static_cast<size_t>(m)] = row;

    // Solve for u and v coordinates using CG.
    auto result_u = CGSolver{}.solve(A, b_u, nullptr, 1e-8, 1000);
    auto result_v = CGSolver{}.solve(A, b_v, nullptr, 1e-8, 1000);

    // Assemble final UVs.
    std::vector<Vec2> uvs(static_cast<size_t>(n));
    for (int vi = 0; vi < n; ++vi) {
        size_t idx = static_cast<size_t>(vi);
        if (is_boundary[idx]) {
            uvs[idx] = fixed_uv[idx];
        } else {
            size_t ii = interior_map[idx];
            uvs[idx] = {result_u.x[ii], result_v.x[ii]};
        }
    }

    UVResult r;
    r.uvs = std::move(uvs);
    r.valid = result_u.converged && result_v.converged;
    return r;
}

// ── Explicit UV ────────────────────────────────────────────────────────────

UVResult apply_explicit_uv(
    const std::vector<Vec2>& user_uvs,
    const std::vector<Vec2>& mesh_vertices,
    bool wrap
) {
    if (user_uvs.size() > mesh_vertices.size()) {
        return {{}, false}; // Too many UVs provided.
    }

    std::vector<Vec2> uvs;
    uvs.reserve(mesh_vertices.size());

    // Copy provided UVs.
    for (size_t i = 0; i < user_uvs.size(); ++i) {
        Vec2 uv = user_uvs[i];
        if (wrap) {
            uv.x = std::clamp(uv.x, 0.0, 1.0);
            uv.y = std::clamp(uv.y, 0.0, 1.0);
        }
        uvs.push_back(uv);
    }

    // Fallback to planar UV for Steiner/additional vertices.
    if (user_uvs.size() < mesh_vertices.size()) {
        // Compute planar UV for the extra vertices only.
        auto planar = solve_planar_uv(mesh_vertices);
        if (planar.valid) {
            for (size_t i = user_uvs.size(); i < mesh_vertices.size(); ++i) {
                uvs.push_back(planar.uvs[i]);
            }
        } else {
            for (size_t i = user_uvs.size(); i < mesh_vertices.size(); ++i) {
                uvs.push_back({0.0, 0.0});
            }
        }
    }

    UVResult r;
    r.uvs = std::move(uvs);
    r.valid = true;
    return r;
}

} // namespace pml
