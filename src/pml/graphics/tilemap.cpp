// ═══════════════════════════════════════════════════════════════════════════════
// PML Tilemap — Implementation
// ───────────────────────────────────────────────────────────────────────────────
// Foundation data structure for tile-based game maps.
// ═══════════════════════════════════════════════════════════════════════════════

#include "tilemap.h"

#include "pml/api/context.h"

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Helper Functions
// ═══════════════════════════════════════════════════════════════════════════════

int tile_at(const TilemapLayer& layer, int col, int row) {
    // Bounds check — return 0 (empty) for out-of-bounds
    if (col < 0 || col >= layer.cols || row < 0 || row >= layer.rows) {
        return 0;
    }
    return layer.tiles[static_cast<size_t>(row) * static_cast<size_t>(layer.cols)
                       + static_cast<size_t>(col)];
}

void set_tile(TilemapLayer& layer, int col, int row, int tile_id) {
    // Bounds check — silently do nothing
    if (col < 0 || col >= layer.cols || row < 0 || row >= layer.rows) {
        return;
    }
    layer.tiles[static_cast<size_t>(row) * static_cast<size_t>(layer.cols)
                + static_cast<size_t>(col)] = tile_id;
}

TilemapLayer add_layer(int cols, int rows, bool visible) {
    TilemapLayer layer;
    layer.cols = cols;
    layer.rows = rows;
    layer.visible = visible;
    layer.tiles.resize(static_cast<size_t>(cols) * static_cast<size_t>(rows), 0);
    return layer;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TilemapManager
// ═══════════════════════════════════════════════════════════════════════════════

TilemapManager& TilemapManager::instance() {
    auto& ctx = PMLContext::current();
    if (!ctx.tilemaps) {
        ctx.tilemaps = std::make_unique<TilemapManager>();
    }
    return *ctx.tilemaps;
}

void TilemapManager::create_tilemap(const std::string& name,
                                     const std::string& tileset_name,
                                     int cols, int rows,
                                     Projection projection,
                                     std::vector<TilemapLayer> layers,
                                     int tile_size) {
    Tilemap tm;
    tm.tileset_name = tileset_name;
    tm.cols = cols;
    tm.rows = rows;
    tm.tile_size = tile_size;
    tm.layers = std::move(layers);
    tm.projection = projection;
    m_tilemaps[name] = std::move(tm);
}

Tilemap* TilemapManager::lookup_tilemap(const std::string& name) {
    auto it = m_tilemaps.find(name);
    if (it != m_tilemaps.end()) {
        return &it->second;
    }
    return nullptr;
}

void TilemapManager::reset() {
    m_tilemaps.clear();
}

} // namespace pml
