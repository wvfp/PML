// ══════════════════════════════════════════════════════════════════════
// PML Texture Blend (Skia backend) — Compose multiple texture layers
// ══════════════════════════════════════════════════════════════════════

#include "texture_blend.h"

#include "skia_backend_internal.h"  // draw_object, ShaderLookup
#include "texture_bake.h"            // bake_texture()

#include "include/core/SkBlendMode.h"
#include "include/core/SkImage.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkShader.h"
#include "include/core/SkTileMode.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkImageInfo.h"

#include <utility>

namespace pml {

namespace {

// ── to_sk_tile_mode ───────────────────────────────────────────────────────
SkTileMode to_sk_tile_mode(WrapMode w)
{
    switch (w) {
        case WrapMode::Repeat: return SkTileMode::kRepeat;
        case WrapMode::Mirror: return SkTileMode::kMirror;
        default:              return SkTileMode::kClamp;
    }
}

// ─── to_sk_sampling ────────────────────────────────────────────────────────
SkSamplingOptions to_sk_sampling(FilterMode f)
{
    return SkSamplingOptions(
        f == FilterMode::Linear ? SkFilterMode::kLinear
                                : SkFilterMode::kNearest);
}

// ─── bake_layer_to_sksp ───────────────────────────────────────────────────
// Bake a TextureBox's GraphicObject to an sk_sp<SkImage> via offscreen
// surface.  This avoids the shared_ptr<SkImage> vs sk_sp<SkImage> confusion
// by using Skia's native ref-counting directly.
sk_sp<SkImage> bake_layer_to_sksp(const TextureBox& tex)
{
    int w = tex.width;
    int h = tex.height;
    if (w <= 0 || h <= 0)
        return nullptr;

    // Create offscreen surface with transparent background.
    SkImageInfo info = SkImageInfo::MakeN32Premul(w, h);
    sk_sp<SkSurface> surface = SkSurfaces::Raster(info);
    if (!surface)
        return nullptr;

    SkCanvas* canvas = surface->getCanvas();
    if (!canvas)
        return nullptr;

    canvas->clear(SK_ColorTRANSPARENT);

    // Draw the GraphicObject.  draw_object is declared in
    // skia_backend_internal.h and implemented in skia_backend_draw.cpp.
    auto result = draw_object(canvas, *tex.go, nullptr,
        [](int64_t) -> sk_sp<SkShader> { return nullptr; });

    if (!result) {
        return nullptr;
    }

    // Snapshot to SkImage (returns sk_sp<SkImage>).
    return surface->makeImageSnapshot();
}

// ─── make_layer_shader ─────────────────────────────────────────────────────
// Create a SkShader for one texture layer.
sk_sp<SkShader> make_layer_shader(const TextureBox& layer)
{
    sk_sp<SkImage> img = bake_layer_to_sksp(layer);
    if (!img)
        return nullptr;

    SkTileMode tmx = to_sk_tile_mode(layer.wrap_x);
    SkTileMode tmy = to_sk_tile_mode(layer.wrap_y);
    SkSamplingOptions sampling = to_sk_sampling(layer.filter);

    return SkShaders::Image(std::move(img), tmx, tmy, sampling);
}

}  // anonymous namespace

// ─── compose_texture_layers ───────────────────────────────────────────────
// Given a list of texture layers, return a single SkShader that blends them.
//
// Strategy:
//   1 layer  → SkShaders::Image (direct)
//   2 layers → SkShaders::Blend(mode, bottom, top)
//   3+ layers→ nest SkShaders::Blend (bottom, blend(rest))

sk_sp<SkShader> compose_texture_layers(const std::vector<TextureLayer>& layers)
{
    if (layers.empty())
        return nullptr;

    if (layers.size() == 1) {
        return make_layer_shader(layers.front().texture);
    }

    // Build from bottom layer up.
    sk_sp<SkShader> result = make_layer_shader(layers[0].texture);
    if (!result)
        return nullptr;

    for (size_t i = 1; i < layers.size(); ++i) {
        sk_sp<SkShader> top = make_layer_shader(layers[i].texture);
        if (!top)
            continue;
        result = SkShaders::Blend(layers[i].blend_mode, result, top);
    }

    return result;
}

}  // namespace pml
