// ═══════════════════════════════════════════════════════════════════════
// PML Textured Draw (Skia backend) — bake GraphicObject → SkImage
// ═══════════════════════════════════════════════════════════════════════

#include "textured_draw.h"

#include "skia_backend_internal.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkImage.h"
#include "include/core/SkSurface.h"

#include <memory>
#include <utility>

namespace pml {

// ─── bake_graphic_object_to_skimage ─────────────────────────────────────
// Rasterise a GraphicObject into a shared SkImage at the given dimensions.
// Creates an offscreen SkSurface, draws the object, and snapshots to SkImage.
// Returns nullptr on failure.

std::shared_ptr<::SkImage> bake_graphic_object_to_skimage(
    const GraphicObject& go, int width, int height)
{
    if (width <= 0 || height <= 0)
        return nullptr;

    // Create an offscreen surface with transparent background.
    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
    sk_sp<SkSurface> surface = SkSurfaces::Raster(info);
    if (!surface)
        return nullptr;

    SkCanvas* canvas = surface->getCanvas();
    if (!canvas)
        return nullptr;

    // Clear to transparent.
    canvas->clear(SK_ColorTRANSPARENT);

    // Draw the GraphicObject.
    // draw_object is declared in skia_backend_internal.h and implemented
    // in skia_backend_draw.cpp.
    auto result = draw_object(canvas, go, nullptr, [](int64_t) -> sk_sp<SkShader> {
        return nullptr;  // no shader lookup needed for baking
    });

    if (!result) {
        return nullptr;
    }

    // Snapshot to SkImage.
    sk_sp<::SkImage> img = surface->makeImageSnapshot();
    if (!img)
        return nullptr;

    // Adopt the sk_sp into a std::shared_ptr (keep the sk_sp alive
    // via the custom deleter to preserve the Skia ref-count).
    return std::shared_ptr<::SkImage>(
        img.get(), [img](::SkImage*) mutable { img.reset(); });
}

}  // namespace pml
