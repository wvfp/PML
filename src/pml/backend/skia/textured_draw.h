#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Textured Draw (Skia backend) — bake + UV-mapped shape rendering
// ═══════════════════════════════════════════════════════════════════════════════

#include <memory>

#include "pml/core/error.h"  // Result<void>

// SkImage and SkCanvas live in the global namespace (see include/core/*.h).
class SkCanvas;
class SkImage;

namespace pml {

class GraphicObject;

// ─── bake_graphic_object_to_skimage ─────────────────────────────────────
std::shared_ptr<::SkImage> bake_graphic_object_to_skimage(
    const GraphicObject& go, int width, int height);

// ─── draw_textured_object ──────────────────────────────────────────────
Result<void> draw_textured_object(
    SkCanvas* canvas, const GraphicObject& obj);

}  // namespace pml
