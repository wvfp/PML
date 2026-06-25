#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Triangulation — poly2tri-based constraint Delaunay triangulation
// ═══════════════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <vector>

#include "pml/core/error.h"
#include "pml/core/types.h"
#include "pml/graphics/graphics_types.h"  // for Vec2

namespace pml {

struct GraphicObject;

/// Result of triangulating a 2D polygon contour.
struct TriangulatedMesh {
    std::vector<Vec2>   vertices;    ///< [x, y] pairs, all mesh vertices
    std::vector<uint32_t> indices;    ///< Triangle indices (3 per triangle, CCW)
    std::vector<int>    contour_map; ///< Maps mesh-vertex-index → original contour
                                     ///< vertex index, or -1 for Steiner points.
};

/// Triangulate a simple polygon (outer contour) with optional holes.
/// Uses poly2tri for constraint Delaunay triangulation.
Result<TriangulatedMesh> triangulate_polygon(
    const std::vector<Vec2>& outer_contour,
    const std::vector<std::vector<Vec2>>& holes = {}
);

/// Triangulate the outline of any closed shape (polygon, path, rect, …)
/// by first extracting its contour points.
Result<TriangulatedMesh> triangulate_shape(const GraphicObject& obj);

} // namespace pml
