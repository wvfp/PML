#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Skia — Pre-rasterization atlas utility
// ═══════════════════════════════════════════════════════════════════════════════
//
// Free functions and types for rasterising GraphicObjects into SkImages for
// tile-atlas use (e.g. drawAtlas spritesheet).  Independent of tilemap.h.
//
// TileDef       — one tile definition (base graphic + optional detail overlay)
// Tileset       — collection of tile definitions keyed by integer id
// RenderAtlas   — result map from tile id → pre-rasterized SkImage
//
// rasterize_graphic_to_image() — render any GraphicObject into an SkImage
// build_tile_atlas()           — bulk-rasterize a tileset subset into an atlas
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/graphics/objects.h"

#include <skia.h>

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// TileDef — one tile in a tileset
// ═══════════════════════════════════════════════════════════════════════════════

/// A single tile definition.
/// The optional @p detail graphic is composited on top of @p base when the
/// atlas is built, so the final tile image contains both layers.
struct TileDef {
    GraphicObject base;
    std::optional<GraphicObject> detail;
};

// ═══════════════════════════════════════════════════════════════════════════════
// Tileset — named collection of tile definitions
// ═══════════════════════════════════════════════════════════════════════════════

/// A collection of tile definitions keyed by numeric id.
struct Tileset {
    /// Width (and height) of every tile in pixels.
    int tile_size{16};

    /// Tile definitions accessible by id.
    std::unordered_map<int, TileDef> tiles;
};

// ═══════════════════════════════════════════════════════════════════════════════
// RenderAtlas — result of pre-rasterization
// ═══════════════════════════════════════════════════════════════════════════════

/// The output of build_tile_atlas(): a map from tile id to a pre-rasterised
/// SkImage of size @p tile_size × @p tile_size.
struct RenderAtlas {
    std::unordered_map<int, sk_sp<SkImage>> tiles;
    int tile_size{};
};

// ═══════════════════════════════════════════════════════════════════════════════
// rasterize_graphic_to_image  —  single GraphicObject → SkImage
// ═══════════════════════════════════════════════════════════════════════════════

/// Render a GraphicObject into a standalone SkImage of the given dimensions.
///
/// Creates a temporary SkSurface with the specified colour type (defaulting to
/// the platform-native 32-bit type), fills it with transparent, dispatches
/// draw_object(), and returns makeImageSnapshot().
///
/// @param go       The GraphicObject tree to render.
/// @param width    Output image width in pixels.
/// @param height   Output image height in pixels.
/// @param ct       Skia colour type (default kN32_SkColorType).
/// @return         SkImage snapshot, or nullptr on failure.
[[nodiscard]] sk_sp<SkImage> rasterize_graphic_to_image(
    const GraphicObject& go,
    int width, int height,
    SkColorType ct = kN32_SkColorType);

// ═══════════════════════════════════════════════════════════════════════════════
// build_tile_atlas  —  bulk rasterize a Tileset subset
// ═══════════════════════════════════════════════════════════════════════════════

/// Build a RenderAtlas containing only the tiles whose ids appear in @p
/// used_ids.
///
/// For tiles that have no @p detail the base GraphicObject is rasterised
/// directly.  For tiles with a detail overlay the base is drawn first, then
/// the detail is composited on top in a single surface, producing a single
/// combined SkImage per tile.  Unknown ids produce a fully transparent image
/// of tile_size × tile_size.
///
/// @param ts         The tileset containing tile definitions.
/// @param used_ids   Subset of tile ids to pre-rasterise.
/// @return           RenderAtlas with the requested tiles.
[[nodiscard]] RenderAtlas build_tile_atlas(
    const Tileset& ts,
    const std::unordered_set<int>& used_ids);

}  // namespace pml
