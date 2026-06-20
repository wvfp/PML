#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// Mesh3D — simple indexed mesh with per-face PML 2D material
// ═══════════════════════════════════════════════════════════════════════════════

#include "vec3.h"
#include "pml/graphics/objects.h"

#include <memory>
#include <vector>

namespace pml {

struct Mesh3D {
    struct Vertex {
        Vec3 position;
        Vec3 normal;
    };

    struct Face {
        std::vector<int> indices;      // 3 or 4 vertex indices
        std::vector<Vec3> uvs;         // UV per vertex (z unused), [0,1] range
        GraphicObject material;        // 2D material rendered onto this face
        Vec3 face_normal;              // Local-space face normal
        double tex_width{1.0};         // Material surface width in PML coords
        double tex_height{1.0};        // Material surface height in PML coords
    };

    std::vector<Vertex> vertices;
    std::vector<Face> faces;
};

/// Compute the normal of a face from its vertices.
[[nodiscard]] Vec3 compute_face_normal(
    const Mesh3D::Face& face,
    const std::vector<Mesh3D::Vertex>& vertices) noexcept;

} // namespace pml
