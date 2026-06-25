// ═══════════════════════════════════════════════════════════════════════════════
// PML Textured Draw (Skia backend) — bake + UV-mapped shape rendering
// ═══════════════════════════════════════════════════════════════════════════════
//
// This file consolidates:
//   1. `bake_graphic_object_to_skimage` — rasterise a GraphicObject → SkImage
//   2. `draw_textured_object`          — triangulate + UV + bake + drawVertices
//
// Both are called from `draw_object()` in skia_backend_draw.cpp when a shape
// has ParamKey::uv set (via texture-map).
//
// Keeping everything in one .cpp avoids cross-TU linker issues with MSVC's
// static library resolution order.
// ═══════════════════════════════════════════════════════════════════════════════

#include "textured_draw.h"

#include "skia_backend_internal.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkImage.h"
#include "include/core/SkSurface.h"
#include "include/core/SkVertices.h"

#include <memory>
#include <utility>

#include "pml/core/texture.h"
#include "pml/core/texture_cache.h"
#include "pml/graphics/objects.h"
#include "pml/graphics/texture_bake.h"

namespace pml {

// ─── bake_graphic_object_to_skimage ────────────────────────────────────────
// Rasterise a GraphicObject into a shared SkImage at the given dimensions.

std::shared_ptr<::SkImage> bake_graphic_object_to_skimage(
    const GraphicObject& go, int width, int height)
{
    if (width <= 0 || height <= 0)
        return nullptr;

    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
    sk_sp<SkSurface> surface = SkSurfaces::Raster(info);
    if (!surface) return nullptr;

    SkCanvas* canvas = surface->getCanvas();
    if (!canvas) return nullptr;

    canvas->clear(SK_ColorTRANSPARENT);

    auto result = draw_object(canvas, go, nullptr, [](int64_t) -> sk_sp<SkShader> {
        return nullptr;
    });
    if (!result) return nullptr;

    sk_sp<::SkImage> img = surface->makeImageSnapshot();
    if (!img) return nullptr;

    // Adopt sk_sp into std::shared_ptr with proper Skia ref-counting.
    return std::shared_ptr<::SkImage>(
        img.get(), [img](::SkImage*) mutable { img.reset(); });
}



// ── bake_texture ───────────────────────────────────────────────────────────

std::shared_ptr<::SkImage> bake_texture(
    std::shared_ptr<TextureBox> texture,
    int width,
    int height
) {
    if (!texture || !texture->go)
        return nullptr;
    int w = (width > 0) ? width : texture->width;
    int h = (height > 0) ? height : texture->height;
    if (w <= 0 || h <= 0)
        return nullptr;
    auto& cache = TextureCache::instance();
    if (auto cached = cache.lookup(texture->stable_id))
        return cached;
    auto img = bake_graphic_object_to_skimage(*texture->go, w, h);
    if (img) cache.insert(texture->stable_id, img);
    return img;
}

std::shared_ptr<::SkImage> bake_graphic_object(
    const GraphicObject& go,
    int width,
    int height
) {
    if (width <= 0 || height <= 0)
        return nullptr;
    return bake_graphic_object_to_skimage(go, width, height);
}

}  // namespace pml