#include "mesh3d.h"

namespace pml {

Vec3 compute_face_normal(
    const Mesh3D::Face& face,
    const std::vector<Mesh3D::Vertex>& vertices) noexcept {
    if (face.indices.size() < 3) return Vec3(0, 0, 0);

    const Vec3& a = vertices[face.indices[0]].position;
    const Vec3& b = vertices[face.indices[1]].position;
    const Vec3& c = vertices[face.indices[2]].position;

    return (b - a).cross(c - a).normalized();
}

} // namespace pml
