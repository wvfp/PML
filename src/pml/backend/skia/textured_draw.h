#pragma once

// ══════════════════════════════════════════════════════════════════════
// PML Textured Draw (Skia backend) — Rasterise GraphicObject → SkImage
// ══════════════════════════════════════════════════════════════════════

#include <memory>

// SkImage lives in the global namespace (see include/core/SkImage.h).
class SkImage;

namespace pml {

class GraphicObject;

// ─── bake_graphic_object_to_skimage ─────────────────────────────────────
// Rasterise a GraphicObject into an SkImage at the given dimensions.
// Implemented in textured_draw.cpp (Skia backend only).
// Returns nullptr on failure or when Skia is not available.
//
// Returns std::shared_ptr<::SkImage> (global namespace), NOT pml::SkImage.
// ───────────────────────────────────────────────────────────────────────────
std::shared_ptr<::SkImage> bake_graphic_object_to_skimage(
    const GraphicObject& go, int width, int height);

}  // namespace pml
