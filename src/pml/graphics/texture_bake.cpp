// ═══════════════════════════════════════════════════════════════════════════════
// PML Texture Bake — Rasterise GraphicObject → SkImage via offscreen SkSurface
// ═══════════════════════════════════════════════════════════════════════════════
//
// `bake_texture` rasterises a TextureBox's source GraphicObject into an
// SkImage at the texture's own width/height and caches the result in
// TextureCache.  The actual rasterisation is performed by the Skia backend
// (SkiaBackend::draw_to_new_surface), and the resulting SkiaSurface is
// converted into a SkImage for caching.
//
// On non-Skia builds the function returns nullptr and does not cache.
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/graphics/texture_bake.h"

#include <cstddef>
#include <cstdint>
#include <memory>

#include "pml/core/texture.h"
#include "pml/core/texture_cache.h"
#include "pml/graphics/objects.h"

namespace pml {

// Skia-specific bake is implemented in the Skia backend (textured_draw.cpp).
// The graphics layer exposes a thin wrapper that just delegates.

#ifdef PML_USE_SKIA

// Forward declaration: implemented in backend/skia/textured_draw.cpp.
// Returns sk_sp<::SkImage> (the native Skia smart pointer).
sk_sp<::SkImage> bake_graphic_object_to_skimage(
    const GraphicObject& go, int width, int height);

#endif

// ── bake_texture ───────────────────────────────────────────────────────────

sk_sp<::SkImage> bake_texture(
    std::shared_ptr<TextureBox> texture,
    int width,
    int height
) {
    if (!texture || !texture->go)
        return nullptr;

    // Resolve dimensions.
    int w = (width > 0) ? width : texture->width;
    int h = (height > 0) ? height : texture->height;
    if (w <= 0 || h <= 0)
        return nullptr;

    // Check cache first.
    auto& cache = TextureCache::instance();
    if (auto cached = cache.lookup_as_sksp(texture->stable_id)) {
        return cached;
    }

#ifdef PML_USE_SKIA
    // Delegate the actual rasterisation to the Skia backend.
    sk_sp<::SkImage> img = bake_graphic_object_to_skimage(*texture->go, w, h);
    if (img) {
        cache.insert(texture->stable_id, img);
    }
    return img;
#else
    (void)cache;
    return nullptr;
#endif
}

// ── bake_graphic_object ────────────────────────────────────────────────────

sk_sp<::SkImage> bake_graphic_object(
    const GraphicObject& go,
    int width,
    int height
) {
    if (width <= 0 || height <= 0)
        return nullptr;

#ifdef PML_USE_SKIA
    return bake_graphic_object_to_skimage(go, width, height);
#else
    (void)go;
    return nullptr;
#endif
}

}  // namespace pml
