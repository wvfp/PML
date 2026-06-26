// ═══════════════════════════════════════════════════════════════════════════════
// PML Textured Draw (Skia backend) — bake GraphicObject → SkImage
// ═══════════════════════════════════════════════════════════════════════════════

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

// ─── bake_graphic_object_to_skimage ────────────────────────────────────────

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

// ── bake_texture ───────────────────────────────────────────────────────────

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

// ── bake_graphic_object ────────────────────────────────────────────────────

std::shared_ptr<::SkImage> bake_graphic_object(
    const GraphicObject& go, int width, int height)
{
    if (width <= 0 || height <= 0) return nullptr;
    return bake_graphic_object_to_skimage(go, width, height);
}

// ── draw_textured_object ──────────────────────────────────────────────────

Result<void> draw_textured_object(SkCanvas* canvas, const GraphicObject& obj)
{
    const Value* uv_val = obj.params.find(ParamKey::uv);
    if (!uv_val) return {};
    const auto* tex_ptr = uv_val->as_texture();
    if (!tex_ptr || !*tex_ptr) return {};
    auto tex = *tex_ptr;

    int uv_mode = 1;
    if (const Value* mode_val = obj.params.find(ParamKey::uv_mode)) {
        if (mode_val->is_number()) {
            uv_mode = mode_val->is_int() ? static_cast<int>(mode_val->int_val())
                                        : static_cast<int>(mode_val->double_val());
        }
    }

    auto value_to_int = [](const Value* v, int default_val) -> int {
        if (!v || !v->is_number()) return default_val;
        return v->is_int() ? static_cast<int>(v->int_val())
                           : static_cast<int>(v->double_val());
    };

    WrapMode wrap_x = tex->wrap_x;
    WrapMode wrap_y = tex->wrap_y;
    FilterMode filter_mode = tex->filter;
    if (const Value* wx = obj.params.find(ParamKey::wrap_x))
        wrap_x = static_cast<WrapMode>(value_to_int(wx, static_cast<int>(wrap_x)));
    if (const Value* wy = obj.params.find(ParamKey::wrap_y))
        wrap_y = static_cast<WrapMode>(value_to_int(wy, static_cast<int>(wrap_y)));
    if (const Value* flt = obj.params.find(ParamKey::filter))
        filter_mode = static_cast<FilterMode>(value_to_int(flt, static_cast<int>(filter_mode)));

    SkTileMode tile_x = (wrap_x == WrapMode::Repeat)  ? SkTileMode::kRepeat
                      : (wrap_x == WrapMode::Mirror)  ? SkTileMode::kMirror : SkTileMode::kClamp;
    SkTileMode tile_y = (wrap_y == WrapMode::Repeat)  ? SkTileMode::kRepeat
                      : (wrap_y == WrapMode::Mirror)  ? SkTileMode::kMirror : SkTileMode::kClamp;
    SkSamplingOptions sampling = (filter_mode == FilterMode::Nearest)
        ? SkSamplingOptions(SkFilterMode::kNearest)
        : SkSamplingOptions(SkFilterMode::kLinear);

    auto mesh_result = triangulate_shape(obj);
    if (!mesh_result) return {};
    const auto& mesh = *mesh_result;
    if (mesh.vertices.empty() || mesh.indices.size() < 3) return {};

    UVResult uv_result;
    switch (uv_mode) {
        case 0:
            uv_result = solve_planar_uv(mesh.vertices);
            break;
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
            uv_result = apply_explicit_uv(explicit_uvs, mesh.vertices);
            break;
        }
        default:
            uv_result = solve_harmonic_uv(mesh.vertices, mesh.indices, mesh.contour_map);
            break;
    }
    if (!uv_result.valid || uv_result.uvs.size() != mesh.vertices.size()) return {};

    auto img = bake_texture(tex);
    if (!img) return {};
    auto shader = img->makeShader(tile_x, tile_y, sampling);
    if (!shader) return {};

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
    auto vertices = SkVertices::MakeCopy(SkVertices::kTriangles_VertexMode,
        static_cast<int>(tri_pos.size()), tri_pos.data(), tri_uv.data(), nullptr);
    if (!vertices) return {};

    SkPaint paint;
    paint.setShader(std::move(shader));
    paint.setBlendMode(SkBlendMode::kSrcOver);
    canvas->drawVertices(vertices.get(), SkBlendMode::kSrcOver, paint);
    return {};
}

}  // namespace pml
