#include "primitive_factory.h"
#include "mesh3d.h"

#include <algorithm>
#include <numbers>

namespace pml {

namespace {

void add_quad(Mesh3D& mesh,
              int i0, int i1, int i2, int i3,
              const GraphicObject& material,
              const std::vector<Vec3>& uvs,
              double tex_width,
              double tex_height) {
    Mesh3D::Face face;
    face.indices = {i0, i1, i2, i3};
    face.uvs = uvs;
    face.material = material;
    face.tex_width = tex_width;
    face.tex_height = tex_height;
    face.face_normal = compute_face_normal(face, mesh.vertices);
    mesh.faces.push_back(face);
}

std::shared_ptr<Mesh3D> make_box(double w, double h, double d,
                                 const GraphicObject& front,
                                 const GraphicObject& back,
                                 const GraphicObject& left,
                                 const GraphicObject& right,
                                 const GraphicObject& top,
                                 const GraphicObject& bottom) {
    auto mesh = std::make_shared<Mesh3D>();
    double x = w * 0.5;
    double y = h * 0.5;
    double z = d * 0.5;

    // 0..7 顶点（右手坐标系，+Z 朝前/朝观察者）
    // 左下后、右下后、右下前、左下前、左上后、右上后、右上前、左上前
    mesh->vertices = {
        Mesh3D::Vertex{{-x, -y, -z}},
        Mesh3D::Vertex{{ x, -y, -z}},
        Mesh3D::Vertex{{ x, -y,  z}},
        Mesh3D::Vertex{{-x, -y,  z}},
        Mesh3D::Vertex{{-x,  y, -z}},
        Mesh3D::Vertex{{ x,  y, -z}},
        Mesh3D::Vertex{{ x,  y,  z}},
        Mesh3D::Vertex{{-x,  y,  z}},
    };

    std::vector<Vec3> uv_quad = {
        Vec3(0, 0, 0),
        Vec3(1, 0, 0),
        Vec3(1, 1, 0),
        Vec3(0, 1, 0),
    };

    // +Z front
    add_quad(*mesh, 3, 2, 6, 7, front, uv_quad, w, h);
    // -Z back
    add_quad(*mesh, 1, 0, 4, 5, back, uv_quad, w, h);
    // -X left
    add_quad(*mesh, 0, 3, 7, 4, left, uv_quad, d, h);
    // +X right
    add_quad(*mesh, 2, 1, 5, 6, right, uv_quad, d, h);
    // +Y top
    add_quad(*mesh, 7, 6, 5, 4, top, uv_quad, w, d);
    // -Y bottom
    add_quad(*mesh, 0, 1, 2, 3, bottom, uv_quad, w, d);

    return mesh;
}

} // anonymous namespace

std::shared_ptr<Mesh3D> make_cube(double size,
                                  const GraphicObject& front,
                                  const GraphicObject& back,
                                  const GraphicObject& left,
                                  const GraphicObject& right,
                                  const GraphicObject& top,
                                  const GraphicObject& bottom) {
    return make_box(size, size, size, front, back, left, right, top, bottom);
}

std::shared_ptr<Mesh3D> make_cuboid(double width,
                                    double height,
                                    double depth,
                                    const GraphicObject& front,
                                    const GraphicObject& back,
                                    const GraphicObject& left,
                                    const GraphicObject& right,
                                    const GraphicObject& top,
                                    const GraphicObject& bottom) {
    return make_box(width, height, depth, front, back, left, right, top, bottom);
}

std::shared_ptr<Mesh3D> make_rounded_cuboid(double width,
                                            double height,
                                            double depth,
                                            double radius,
                                            int segments,
                                            const GraphicObject& front,
                                            const GraphicObject& back,
                                            const GraphicObject& left,
                                            const GraphicObject& right,
                                            const GraphicObject& top,
                                            const GraphicObject& bottom) {
    // Initial implementation: fall back to a regular cuboid.
    // Full edge/vertex rounding will be added in a follow-up.
    (void)radius;
    (void)segments;
    return make_box(width, height, depth, front, back, left, right, top, bottom);
}

std::shared_ptr<Mesh3D> make_cone(double radius,
                                  double height,
                                  int segments,
                                  const GraphicObject& side,
                                  const GraphicObject& base) {
    auto mesh = std::make_shared<Mesh3D>();
    const double half_h = height * 0.5;

    // Apex + base ring vertices
    mesh->vertices.push_back(Mesh3D::Vertex{{0.0, half_h, 0.0}});      // 0: apex
    for (int i = 0; i < segments; ++i) {
        double theta = 2.0 * std::numbers::pi_v<double> * i / segments;
        double x = radius * std::cos(theta);
        double z = radius * std::sin(theta);
        mesh->vertices.push_back(Mesh3D::Vertex{{x, -half_h, z}});     // 1..segments
    }
    // Base center
    mesh->vertices.push_back(Mesh3D::Vertex{{0.0, -half_h, 0.0}});     // segments+1

    // Side faces (triangles)
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments + 1;
        int curr = i + 1;
        Mesh3D::Face face;
        face.indices = {0, next, curr};
        double u0 = (i + 0.5) / segments;
        double u1 = (i + 1.5) / segments;
        face.uvs = {
            Vec3((u0 + u1) * 0.5, 1.0, 0.0),
            Vec3(u1, 0.0, 0.0),
            Vec3(u0, 0.0, 0.0),
        };
        face.material = side;
        face.tex_width = 2.0 * std::numbers::pi_v<double> * radius;
        face.tex_height = height;
        face.face_normal = compute_face_normal(face, mesh->vertices);
        mesh->faces.push_back(face);
    }

    // Base faces (triangles fan)
    int center_idx = segments + 1;
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments + 1;
        int curr = i + 1;
        Mesh3D::Face face;
        face.indices = {center_idx, curr, next};
        double u0 = 0.5 + 0.5 * std::cos(2.0 * std::numbers::pi_v<double> * i / segments);
        double v0 = 0.5 + 0.5 * std::sin(2.0 * std::numbers::pi_v<double> * i / segments);
        double u1 = 0.5 + 0.5 * std::cos(2.0 * std::numbers::pi_v<double> * (i + 1) / segments);
        double v1 = 0.5 + 0.5 * std::sin(2.0 * std::numbers::pi_v<double> * (i + 1) / segments);
        face.uvs = {
            Vec3(0.5, 0.5, 0.0),
            Vec3(u0, v0, 0.0),
            Vec3(u1, v1, 0.0),
        };
        face.material = base;
        face.tex_width = 2.0 * radius;
        face.tex_height = 2.0 * radius;
        face.face_normal = compute_face_normal(face, mesh->vertices);
        mesh->faces.push_back(face);
    }

    return mesh;
}

std::shared_ptr<Mesh3D> make_plane(double width,
                                   double depth,
                                   const GraphicObject& material) {
    auto mesh = std::make_shared<Mesh3D>();
    double x = width * 0.5;
    double z = depth * 0.5;

    mesh->vertices = {
        Mesh3D::Vertex{{-x, 0.0, -z}},
        Mesh3D::Vertex{{ x, 0.0, -z}},
        Mesh3D::Vertex{{ x, 0.0,  z}},
        Mesh3D::Vertex{{-x, 0.0,  z}},
    };

    Mesh3D::Face face;
    face.indices = {0, 1, 2, 3};
    face.uvs = {
        Vec3(0, 0, 0),
        Vec3(1, 0, 0),
        Vec3(1, 1, 0),
        Vec3(0, 1, 0),
    };
    face.material = material;
    face.tex_width = width;
    face.tex_height = depth;
    face.face_normal = compute_face_normal(face, mesh->vertices);
    mesh->faces.push_back(face);

    return mesh;
}

std::shared_ptr<Mesh3D> make_sphere(double radius,
                                    int segments,
                                    int rings,
                                    const GraphicObject& material) {
    auto mesh = std::make_shared<Mesh3D>();

    // Generate vertices (lat/long grid)
    for (int ring = 0; ring <= rings; ++ring) {
        double phi = std::numbers::pi_v<double> * ring / rings; // 0..pi
        double y = radius * std::cos(phi);
        double ring_radius = radius * std::sin(phi);
        for (int seg = 0; seg <= segments; ++seg) {
            double theta = 2.0 * std::numbers::pi_v<double> * seg / segments;
            double x = ring_radius * std::cos(theta);
            double z = ring_radius * std::sin(theta);
            mesh->vertices.push_back(Mesh3D::Vertex{{x, y, z}});
        }
    }

    auto index = [segments](int ring, int seg) {
        return ring * (segments + 1) + (seg % (segments + 1));
    };

    for (int ring = 0; ring < rings; ++ring) {
        for (int seg = 0; seg < segments; ++seg) {
            int a = index(ring, seg);
            int b = index(ring, seg + 1);
            int c = index(ring + 1, seg + 1);
            int d = index(ring + 1, seg);

            double u_a = static_cast<double>(seg) / segments;
            double u_b = static_cast<double>(seg + 1) / segments;
            double v_a = static_cast<double>(ring) / rings;
            double v_b = static_cast<double>(ring + 1) / rings;

            Mesh3D::Face face1;
            face1.indices = {a, b, c};
            face1.uvs = {
                Vec3(u_a, v_a, 0.0),
                Vec3(u_b, v_a, 0.0),
                Vec3(u_b, v_b, 0.0),
            };
            face1.material = material;
            face1.tex_width = 2.0 * radius;
            face1.tex_height = radius; // approximate
            face1.face_normal = compute_face_normal(face1, mesh->vertices);
            mesh->faces.push_back(face1);

            Mesh3D::Face face2;
            face2.indices = {a, c, d};
            face2.uvs = {
                Vec3(u_a, v_a, 0.0),
                Vec3(u_b, v_b, 0.0),
                Vec3(u_a, v_b, 0.0),
            };
            face2.material = material;
            face2.tex_width = 2.0 * radius;
            face2.tex_height = radius;
            face2.face_normal = compute_face_normal(face2, mesh->vertices);
            mesh->faces.push_back(face2);
        }
    }

    return mesh;
}

} // namespace pml
