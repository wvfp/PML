// ═══════════════════════════════════════════════════════════════════════════════
// PML Skia — Pre-rasterization atlas utility implementation
// ═══════════════════════════════════════════════════════════════════════════════
//
// Implements rasterize_graphic_to_image() and build_tile_atlas() declared in
// skia_backend_atlas.h.
//
// Rasterization happens through the existing draw_object() dispatch defined in
// skia_backend_draw.cpp; no new draw logic lives here.
// ═══════════════════════════════════════════════════════════════════════════════

#include "skia_backend_atlas.h"
#include "skia_backend_internal.h"

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// rasterize_graphic_to_image
// ═══════════════════════════════════════════════════════════════════════════════

sk_sp<SkImage> rasterize_graphic_to_image(
    const GraphicObject& go,
    int width, int height,
    SkColorType ct)
{
    if (width <= 0 || height <= 0) return nullptr;

    SkImageInfo info = SkImageInfo::Make(
        width, height, ct, kPremul_SkAlphaType);

    auto surface = SkSurfaces::Raster(info);
    if (!surface) return nullptr;

    auto* canvas = surface->getCanvas();
    canvas->clear(SK_ColorTRANSPARENT);

    // Pre-rasterization does not resolve shader handles; pass a no-op lookup.
    ShaderLookup noop_lookup = [](int64_t) -> sk_sp<SkShader> { return nullptr; };
    auto result = draw_object(canvas, go, nullptr, noop_lookup);
    if (!result) return nullptr;

    return surface->makeImageSnapshot();
}

// ═══════════════════════════════════════════════════════════════════════════════
// build_tile_atlas
// ═══════════════════════════════════════════════════════════════════════════════

RenderAtlas build_tile_atlas(
    const Tileset& ts,
    const std::unordered_set<int>& used_ids)
{
    RenderAtlas atlas;
    atlas.tile_size = ts.tile_size;

    for (int id : used_ids) {
        auto it = ts.tiles.find(id);
        if (it == ts.tiles.end()) {
            // Unknown tile id → emit a fully transparent image of correct size.
            auto img = rasterize_graphic_to_image(
                GraphicObject{}, ts.tile_size, ts.tile_size);
            if (img) {
                atlas.tiles[id] = std::move(img);
            }
            continue;
        }

        const auto& def = it->second;

        if (!def.detail.has_value()) {
            // Simple tile: rasterise the base shape directly.
            auto img = rasterize_graphic_to_image(
                def.base, ts.tile_size, ts.tile_size);
            if (img) {
                atlas.tiles[id] = std::move(img);
            }
        } else {
            // Composite tile: draw base first, then detail overlay on top,
            // producing a single combined SkImage.
            SkImageInfo info = SkImageInfo::Make(
                ts.tile_size, ts.tile_size,
                kN32_SkColorType, kPremul_SkAlphaType);

            auto surface = SkSurfaces::Raster(info);
            if (!surface) continue;

            auto* canvas = surface->getCanvas();
            canvas->clear(SK_ColorTRANSPARENT);

            ShaderLookup noop_lookup = [](int64_t) -> sk_sp<SkShader> { return nullptr; };

            // Layer 1: base graphic.
            auto r1 = draw_object(canvas, def.base, nullptr, noop_lookup);
            if (!r1) continue;

            // Layer 2: detail graphic composited on top.
            auto r2 = draw_object(canvas, *def.detail, nullptr, noop_lookup);
            if (!r2) continue;

            auto img = surface->makeImageSnapshot();
            if (img) {
                atlas.tiles[id] = std::move(img);
            }
        }
    }

    return atlas;
}

}  // namespace pml
