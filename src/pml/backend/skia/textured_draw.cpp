// ==========================================================================================================================================================================================================================================═
// PML Textured Draw (Skia backend) — bake GraphicObject → SkImage
// ==========================================================================================================================================================================================================================================═

#include "textured_draw.h"

#include "skia_backend_internal.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkImage.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkSurface.h"


#include <memory>
#include <utility>

#include "pml/core/texture.h"
#include "pml/core/texture_cache.h"
#include "pml/graphics/objects.h"
#include "pml/graphics/texture_bake.h"
#include "pml/graphics/triangulation.h"
#include "pml/graphics/uv_solver.h"

namespace pml {

// ----─ bake_graphic_object_to_skimage --------------------------------------------------------------------------------

std::shared_ptr<::SkImage> bake_graphic_object_to_skimage(
    const GraphicObject& go, int width, int height)
{
    if (width <= 0 || height <= 0) return nullptr;
    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
    sk_sp<SkSurface> surface = SkSurfaces::Raster(info);
    if (!surface) return nullptr;
    SkCanvas* canvas = surface->getCanvas();
    if (!canvas) return nullptr;
    canvas->clear(SK_ColorTRANSPARENT);
    auto result = draw_object(canvas, go, nullptr,
        [](int64_t) -> sk_sp<SkShader> { return nullptr; });
    if (!result) return nullptr;
    sk_sp<::SkImage> img = surface->makeImageSnapshot();
    if (!img) return nullptr;
    return std::shared_ptr<::SkImage>(
        img.get(), [img](::SkImage*) mutable { img.reset(); });
}

// ---- bake_texture --------------------------------------------------------------------------------------------------------------------─

std::shared_ptr<::SkImage> bake_texture(
    std::shared_ptr<TextureBox> texture, int width, int height)
{
    if (!texture || !texture->go) return nullptr;
    int w = (width > 0) ? width : texture->width;
    int h = (height > 0) ? height : texture->height;
    if (w <= 0 || h <= 0) return nullptr;
    auto& cache = TextureCache::instance();
    if (auto cached = cache.lookup(texture->stable_id)) return cached;
    auto img = bake_graphic_object_to_skimage(*texture->go, w, h);
    if (img) cache.insert(texture->stable_id, img);
    return img;
}

// ---- bake_graphic_object --------------------------------------------------------------------------------------------------------

std::shared_ptr<::SkImage> bake_graphic_object(
    const GraphicObject& go, int width, int height)
{
    if (width <= 0 || height <= 0) return nullptr;
    return bake_graphic_object_to_skimage(go, width, height);
}

// ==========================================================================================================================================================================================================================================═
// Pipeline stages (internal, for testability)
// ==========================================================================================================================================================================================================================================═

namespace {

/// Stage 1: extract texture and UV-mode params from the GraphicObject.
struct UVParams {
    std::shared_ptr<TextureBox> texture;
    int uv_mode{1};               // 0=planar, 1=harmonic, 2=explicit
    SkTileMode tile_x{SkTileMode::kClamp};
    SkTileMode tile_y{SkTileMode::kClamp};
    SkSamplingOptions sampling{SkFilterMode::kLinear};
};

UVParams stage_extract(const GraphicObject& obj) {
    UVParams p;
    const Value* uv_val = obj.params.find(ParamKey::uv);
    if (!uv_val) return p;
    const auto* tex_ptr = uv_val->as_texture();
    if (!tex_ptr || !*tex_ptr) return p;
    p.texture = *tex_ptr;

    if (const Value* mode_val = obj.params.find(ParamKey::uv_mode)) {
        if (mode_val->is_number()) {
            p.uv_mode = mode_val->is_int()
                ? static_cast<int>(mode_val->int_val())
                : static_cast<int>(mode_val->double_val());
        }
    }

    auto val_to_int = [](const Value* v, int def) -> int {
        if (!v || !v->is_number()) return def;
        return v->is_int() ? static_cast<int>(v->int_val())
                           : static_cast<int>(v->double_val());
    };

    WrapMode wx = p.texture->wrap_x;
    WrapMode wy = p.texture->wrap_y;
    FilterMode fm = p.texture->filter;
    if (const Value* v = obj.params.find(ParamKey::wrap_x))
        wx = static_cast<WrapMode>(val_to_int(v, static_cast<int>(wx)));
    if (const Value* v = obj.params.find(ParamKey::wrap_y))
        wy = static_cast<WrapMode>(val_to_int(v, static_cast<int>(wy)));
    if (const Value* v = obj.params.find(ParamKey::filter))
        fm = static_cast<FilterMode>(val_to_int(v, static_cast<int>(fm)));

    p.tile_x = (wx == WrapMode::Repeat)  ? SkTileMode::kRepeat
              : (wx == WrapMode::Mirror)  ? SkTileMode::kMirror : SkTileMode::kClamp;
    p.tile_y = (wy == WrapMode::Repeat)  ? SkTileMode::kRepeat
              : (wy == WrapMode::Mirror)  ? SkTileMode::kMirror : SkTileMode::kClamp;
    p.sampling = (fm == FilterMode::Nearest)
        ? SkSamplingOptions(SkFilterMode::kNearest)
        : SkSamplingOptions(SkFilterMode::kLinear);
    return p;
}

/// Stage 2: triangulate shape → mesh.
Result<TriangulatedMesh> stage_triangulate(const GraphicObject& obj) {
    return triangulate_shape(obj);
}

/// Stage 3: solve UV coordinates from mesh.
UVResult stage_solve_uv(const TriangulatedMesh& mesh, int uv_mode,
                        const GraphicObject& obj) {
    switch (uv_mode) {
        case 0:
            return solve_planar_uv(mesh.vertices);
        case 2: {
            std::vector<Vec2> explicit_uvs;
            if (const Value* uvs_val = obj.params.find(ParamKey::uv_vertices)) {
                const auto* lst = uvs_val->as_list();
                if (lst && *lst) {
                    const auto& elems = (*lst)->elements;
                    for (size_t i = 0; i + 1 < elems.size(); i += 2) {
                        double u = elems[i].is_int()   ? static_cast<double>(elems[i].int_val())
                                  : elems[i].is_double() ? elems[i].double_val() : 0.0;
                        double v = elems[i+1].is_int() ? static_cast<double>(elems[i+1].int_val())
                                  : elems[i+1].is_double() ? elems[i+1].double_val() : 0.0;
                        explicit_uvs.push_back({u, v});
                    }
                }
            }
            return apply_explicit_uv(explicit_uvs, mesh.vertices);
        }
        default:
            return solve_harmonic_uv(mesh.vertices, mesh.indices, mesh.contour_map);
    }
}

/// Stage 4: bake texture → SkImage.
std::shared_ptr<::SkImage> stage_bake(const std::shared_ptr<TextureBox>& tex) {
    return bake_texture(tex);
}

/// Stage 5: build SkVertices from positions + UVs.
sk_sp<SkVertices> stage_build_vertices(const TriangulatedMesh& mesh,
                                        const UVResult& uv_result,
                                        const SkImage* img) {
    const size_t n_tri = mesh.indices.size() / 3;
    std::vector<SkPoint> tri_pos; tri_pos.reserve(n_tri * 3);
    std::vector<SkPoint> tri_uv;  tri_uv.reserve(n_tri * 3);
    const SkScalar tex_w = static_cast<SkScalar>(img->width());
    const SkScalar tex_h = static_cast<SkScalar>(img->height());
    for (size_t t = 0; t < n_tri; ++t) {
        for (int k = 0; k < 3; ++k) {
            uint32_t idx = mesh.indices[t * 3 + k];
            tri_pos.push_back({static_cast<SkScalar>(mesh.vertices[idx].x),
                               static_cast<SkScalar>(mesh.vertices[idx].y)});
            double u = uv_result.uvs[idx].x * tex_w;
            double v = uv_result.uvs[idx].y * tex_h;
            tri_uv.push_back({static_cast<SkScalar>(u), static_cast<SkScalar>(v)});
        }
    }
    return SkVertices::MakeCopy(SkVertices::kTriangles_VertexMode,
        static_cast<int>(tri_pos.size()), tri_pos.data(), tri_uv.data(), nullptr);
}

/// Stage 6: draw SkVertices with texture shader.
Result<void> stage_draw(SkCanvas* canvas, const sk_sp<SkVertices>& verts,
                         const sk_sp<SkShader>& shader) {
    if (!verts || !shader) return {};
    SkPaint paint;
    paint.setShader(shader);
    paint.setBlendMode(SkBlendMode::kSrcOver);
    canvas->drawVertices(verts.get(), SkBlendMode::kSrcOver, paint);
    return {};
}

/// Perspective correction for explicit UV mode.
///
/// Uses SkMatrix::setPolyToPoly() to compute a perspective homography from
/// the shape's 4 boundary corners to the user-provided UV corners, then
/// pre-warps every mesh vertex's UV coordinate.  The resulting affine
/// interpolation within each triangle approximates perspective-correct
/// texture mapping.
///
/// @return  true when correction was successfully applied.
static bool correct_uv_for_perspective(
    UVResult& uv_result,
    const std::vector<Vec2>& mesh_vertices,
    const GraphicObject& obj)
{
    // We need the shape's boundary corners and the user's UV corners.
    // Both come from params: "points" for polygon shapes, "uv-vertices" for UVs.
    const Value* pts_val = obj.params.find(ParamKey::points);
    const Value* uvs_val = obj.params.find(ParamKey::uv_vertices);
    if (!pts_val || !uvs_val) return false;

    // Parse "points" — get first 4 vertices (the quad corners).
    auto read_pts = [](const Value& v, SkPoint out[4]) -> int {
        const auto* lst = v.as_list();
        if (!lst || !*lst) return 0;
        const auto& elems = (*lst)->elements;
        bool flat = !elems.empty() && !elems[0].as_list();
        int count = 0;
        auto push = [&](double x, double y) {
            if (count < 4) out[count++] = {SkScalar(x), SkScalar(y)};
        };
        if (flat) {
            for (size_t i = 0; i + 1 < elems.size() && count < 4; i += 2) {
                double x = elems[i].is_number()
                    ? (elems[i].is_int() ? double(elems[i].int_val()) : elems[i].double_val())
                    : 0.0;
                double y = elems[i+1].is_number()
                    ? (elems[i+1].is_int() ? double(elems[i+1].int_val()) : elems[i+1].double_val())
                    : 0.0;
                push(x, y);
            }
        } else {
            for (size_t i = 0; i < elems.size() && count < 4; ++i) {
                const auto* pair = elems[i].as_list();
                if (!pair || !*pair || (*pair)->elements.size() < 2) continue;
                double x = (*pair)->elements[0].is_number()
                    ? ((*pair)->elements[0].is_int() ? double((*pair)->elements[0].int_val())
                       : (*pair)->elements[0].double_val()) : 0.0;
                double y = (*pair)->elements[1].is_number()
                    ? ((*pair)->elements[1].is_int() ? double((*pair)->elements[1].int_val())
                       : (*pair)->elements[1].double_val()) : 0.0;
                push(x, y);
            }
        }
        return count;
    };

    SkPoint shape_corners[4];
    if (read_pts(*pts_val, shape_corners) != 4) return false;

    // Parse "uv-vertices" — get first 4 UV corners.
    SkPoint uv_corners[4];
    if (read_pts(*uvs_val, uv_corners) != 4) return false;

    // Compute perspective matrix: shape_position → UV.
    // PolyToPoly is a static method that returns std::optional<SkMatrix>.
    auto persp = SkMatrix::PolyToPoly(SkSpan<const SkPoint>(shape_corners, 4),
                                       SkSpan<const SkPoint>(uv_corners, 4));
    if (!persp.has_value()) return false;

    // Map every mesh vertex position through the perspective matrix to get
    // the corrected UV.  This pre-warps UVs so that affine interpolation
    // within each triangle approximates perspective-correct texturing.
    std::vector<Vec2> corrected;
    corrected.reserve(mesh_vertices.size());
    for (const auto& v : mesh_vertices) {
        SkPoint src{SkScalar(v.x), SkScalar(v.y)};
        SkPoint dst;
        persp->mapPoints(SkSpan<SkPoint>(&dst, 1), SkSpan<const SkPoint>(&src, 1));
        corrected.push_back({dst.x(), dst.y()});
    }
    uv_result.uvs = std::move(corrected);
    return true;
}

} // anonymous namespace

// ---- draw_textured_object — stage orchestrator --------------------------------------------------------

Result<void> draw_textured_object(SkCanvas* canvas, const GraphicObject& obj)
{
    // Stage 1: extract params
    auto uv = stage_extract(obj);
    if (!uv.texture) return {};

    // Stage 2: triangulate
    auto mesh = stage_triangulate(obj);
    if (!mesh || mesh->vertices.empty() || mesh->indices.size() < 3) return {};

    // Stage 3: solve UVs
    auto uv_result = stage_solve_uv(*mesh, uv.uv_mode, obj);
    if (!uv_result.valid || uv_result.uvs.size() != mesh->vertices.size()) return {};

    // Stage 3b: perspective correction (explicit UV mode only)
    if (uv.uv_mode == 2 && obj.params.find(ParamKey::perspective_correction)) {
        correct_uv_for_perspective(uv_result, mesh->vertices, obj);
    }

    // Stage 4: bake texture
    auto img = stage_bake(uv.texture);
    if (!img) return {};

    // Stage 5: build vertices
    auto verts = stage_build_vertices(*mesh, uv_result, img.get());
    if (!verts) return {};

    // Stage 6: create shader and draw
    auto shader = img->makeShader(uv.tile_x, uv.tile_y, uv.sampling);
    if (!shader) return {};

    return stage_draw(canvas, verts, shader);
}

}  // namespace pml
