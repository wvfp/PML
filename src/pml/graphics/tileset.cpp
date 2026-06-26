// ==========================================================================================================================================================================================================================================═
// PML Tileset — Implementation
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Singleton TilesetManager.
// ==========================================================================================================================================================================================================================================═

#include "tileset.h"

#include "pml/api/context.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// TilesetManager
// ==========================================================================================================================================================================================================================================═

TilesetManager::TilesetManager() = default;

TilesetManager& TilesetManager::instance() {
    auto& ctx = PMLContext::current();
    if (!ctx.tilesets) {
        ctx.tilesets = std::make_unique<TilesetManager>();
    }
    return *ctx.tilesets;
}

void TilesetManager::register_tileset(const std::string& name,
                                      const std::vector<TileType>& tiles,
                                      int tile_size) {
    Tileset ts;
    ts.name = name;
    ts.tile_size = tile_size;
    ts.tile_types = tiles;
    m_tilesets[name] = std::move(ts);
}

const Tileset* TilesetManager::lookup_tileset(const std::string& name) const {
    auto it = m_tilesets.find(name);
    if (it == m_tilesets.end()) {
        std::cerr << "Warning: unknown tileset '" << name << "', returning nullptr" << std::endl;
        return nullptr;
    }
    return &it->second;
}

const GraphicObject* TilesetManager::get_tile_graphic(const std::string& tileset_name,
                                                       int tile_id) const {
    const Tileset* ts = lookup_tileset(tileset_name);
    if (!ts) {
        return nullptr;
    }
    for (const auto& tile : ts->tile_types) {
        if (tile.id == tile_id) {
            return &tile.graphic;
        }
    }
    std::cerr << "Warning: tile id " << tile_id << " not found in tileset '"
              << tileset_name << "', returning nullptr" << std::endl;
    return nullptr;
}

void TilesetManager::reset() {
    m_tilesets.clear();
}

} // namespace pml
