#pragma once

// ==================================================================================================================================================================================================================═
// PML Texture Bake — Rasterise a GraphicObject into an SkImage via offscreen
// SkSurface, with automatic LRU caching.
// ==================================================================================================================================================================================================================═

#include <memory>

// SkImage lives in the global namespace (see include/core/SkImage.h).
// Forward-declare it here so the shared_ptr<SkImage> return type compiles
// without pulling in the full Skia header.
class SkImage;

namespace pml {

struct TextureBox;

/// Bake a TextureBox into an SkImage (as shared_ptr).
/// Uses the offscreen SkSurface pattern (existing render pipeline).
/// Results are cached in TextureCache, keyed by stable_id.
/// @param texture  The PML texture to rasterise.
/// @param width    Override raster width (default: texture's own width).
/// @param height   Override raster height (default: texture's own height).
/// @return shared_ptr<SkImage> (nullptr on failure).
std::shared_ptr<::SkImage> bake_texture(
    std::shared_ptr<TextureBox> texture,
    int width = 0,
    int height = 0
);

/// Bake a bare GraphicObject into an SkImage without TextureBox wrapping.
/// This is a lower-level entry used internally by texture-map rendering.
/// Not cached (caller should cache if desired).
std::shared_ptr<::SkImage> bake_graphic_object(
    const class GraphicObject& go,
    int width = 512,
    int height = 512
);

}  // namespace pml
