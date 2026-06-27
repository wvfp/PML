// ==========================================================================================================================================================================================================================================═
// PML Skia render backend — 3D mesh drawing
// ==========================================================================================================================================================================================================================================═
//
// 3D mesh rendering extracted from skia_backend_draw.cpp for maintainability.
// ==========================================================================================================================================================================================================================================═

#include "pml/core/types.h"

#include "skia_backend_internal.h"

#include <algorithm>
#include <vector>

namespace pml {

namespace {

Result<void> draw_mesh3d_impl(SkCanvas* canvas, const GraphicObject& obj,
                              const ShaderLookup& lookup)
{
    const Value* mesh_val = obj.params.find("mesh");
    const Value* transform_val = obj.params.find("transform");
    if (!mesh_val || !transform_val) return {};

    const auto* mesh_ptr = mesh_val->as_mesh3d();
    const auto* transform_ptr = transform_val->as_transform3d();
    if (!mesh_ptr || !transform_ptr) return {};

    const auto& mesh = **mesh_ptr;
    const auto& transform = **transform_ptr;
    const Camera3D& cam = current_camera();
    const Mat4 vp = cam.projection_matrix() * cam.view_matrix();

    const int canvas_width = canvas->getBaseLayerSize().width();
    const int canvas_height = canvas->getBaseLayerSize().height();
    if (canvas_width <= 0 || canvas_height <= 0) return {};

    struct ProjectedFace {
        const Mesh3D::Face* face;
        std::vector<SkPoint> points;
        std::vector<SkPoint> uvs;
        double depth;
    };
    std::vector<ProjectedFace> visible;
    visible.reserve(mesh.faces.size());

    for (const auto& face : mesh.faces) {
        if (face.indices.size() < 3) continue;

        ProjectedFace pf;
        pf.face = &face;
        pf.points.reserve(face.indices.size());
        pf.uvs.reserve(face.indices.size());

        Vec3 world_sum(0, 0, 0);
        bool clipped = false;
        for (int idx : face.indices) {
            Vec3 world = transform.apply(mesh.vertices[idx].position);
            world_sum = world_sum + world;
            Vec3 ndc = vp.transform_point(world);

            // Simple clipping: skip faces entirely behind the near plane.
            if (ndc.z < -1.0 || ndc.z > 1.0) {
                clipped = true;
                break;
            }

            SkPoint p{
                static_cast<SkScalar>((ndc.x + 1.0) * 0.5 * canvas_width),
                static_cast<SkScalar>((1.0 - ndc.y) * 0.5 * canvas_height)};
            pf.points.push_back(p);
        }
        if (clipped || pf.points.size() < 3) continue;

        // Backface culling using signed screen-space area.
        if (cam.backface_culling) {
            double signed_area = 0.0;
            for (size_t i = 0; i < pf.points.size(); ++i) {
                const SkPoint& a = pf.points[i];
                const SkPoint& b = pf.points[(i + 1) % pf.points.size()];
                signed_area += static_cast<double>(a.x()) * b.y() -
                               static_cast<double>(b.x()) * a.y();
            }
            if (signed_area < 0.0) continue;
        }

        Vec3 center = world_sum / static_cast<double>(face.indices.size());
        Vec3 center_ndc = vp.transform_point(center);
        pf.depth = center_ndc.z;

        for (const auto& uv : face.uvs) {
            pf.uvs.push_back({static_cast<SkScalar>(uv.x),
                              static_cast<SkScalar>(uv.y)});
        }
        visible.push_back(std::move(pf));
    }

    // Painter's algorithm: draw from far to near.
    std::sort(visible.begin(), visible.end(),
              [](const ProjectedFace& a, const ProjectedFace& b) {
                  return a.depth > b.depth;
              });

    // Temporary surface shared across all face rasterizations.
    for (const auto& pf : visible) {
        const Mesh3D::Face& face = *pf.face;
        const int tex_w = std::max(1, static_cast<int>(std::ceil(face.tex_width)));
        const int tex_h = std::max(1, static_cast<int>(std::ceil(face.tex_height)));

        // Rasterize the material to a temporary surface.
        SkiaSurface mat_surface(tex_w, tex_h, 0x00000000);
        auto mat_result = draw_object(mat_surface.surface->getCanvas(),
                                      face.material, nullptr, lookup);
        if (!mat_result) return mat_result;

        sk_sp<SkShader> shader = mat_surface.bitmap.asImage()->makeShader(
            SkTileMode::kClamp, SkTileMode::kClamp, SkSamplingOptions());
        if (!shader) continue;

        // Triangulate: fan for arbitrary n-gons; our factories use quads.
        const size_t n = pf.points.size();
        if (n < 3) continue;

        std::vector<SkPoint> tri_positions;
        std::vector<SkPoint> tri_uvs;
        tri_positions.reserve((n - 2) * 3);
        tri_uvs.reserve((n - 2) * 3);

        for (size_t i = 1; i + 1 < n; ++i) {
            tri_positions.push_back(pf.points[0]);
            tri_positions.push_back(pf.points[i]);
            tri_positions.push_back(pf.points[i + 1]);

            tri_uvs.push_back({pf.uvs[0].x() * tex_w, pf.uvs[0].y() * tex_h});
            tri_uvs.push_back({pf.uvs[i].x() * tex_w, pf.uvs[i].y() * tex_h});
            tri_uvs.push_back({pf.uvs[i + 1].x() * tex_w,
                               pf.uvs[i + 1].y() * tex_h});
        }

        auto vertices = SkVertices::MakeCopy(
            SkVertices::kTriangles_VertexMode,
            static_cast<int>(tri_positions.size()),
            tri_positions.data(),
            tri_uvs.data(),
            nullptr);

        SkPaint paint;
        paint.setShader(std::move(shader));
        paint.setBlendMode(SkBlendMode::kSrcOver);
        canvas->drawVertices(vertices.get(), SkBlendMode::kSrcOver, paint);
    }

    return {};
}

} // anonymous namespace

// ---- Public API --------------------------------------------------------------------------------------------------------

Result<void> draw_mesh3d(SkCanvas* canvas, const GraphicObject& obj,
                         const ShaderLookup& lookup)
{
    return draw_mesh3d_impl(canvas, obj, lookup);
}

} // namespace pml
