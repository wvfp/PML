#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Tileset — Named tile collections for tile-based game assets
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Data-layer structs + TilesetManager singleton.
// No PML builtins — pure C++ data structures.
// ==========================================================================================================================================================================================================================================═

#include "objects.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// TileType — Single tile variant in a tileset
// ==========================================================================================================================================================================================================================================═

/// A single tile variant within a tileset.
///   - `id` is the tile index (0-based, matches placement data).
///   - `name` is a human-readable label (e.g. "grass", "dirt").
///   - `graphic` is the base GraphicObject (shape/params/transform).
///   - `detail` is an optional overlay GraphicObject (e.g. tuft, crack).
struct TileType {
    int id{};
    std::string name;
    GraphicObject graphic;
    std::optional<GraphicObject> detail;
};

// ==========================================================================================================================================================================================================================================═
// Tileset — Named collection of tile types
// ==========================================================================================================================================================================================================================================═

/// A named tileset with a uniform tile size and a list of tile variants.
/// The tileset is registered with TilesetManager and looked up by name.
struct Tileset {
    std::string name;
    int tile_size{};
    std::vector<TileType> tile_types;
};

// ==========================================================================================================================================================================================================================================═
// TilesetManager — Global named-tileset registry (singleton)
// ==========================================================================================================================================================================================================================================═

/// Singleton registry for named Tilesets.  Follows the same pattern as
/// StyleRegistry / PaletteManager — `instance()` delegates to the current
/// PMLContext, enabling per-runtime isolation.
class TilesetManager {
  public:
    TilesetManager();

    TilesetManager(const TilesetManager&) = delete;
    TilesetManager& operator=(const TilesetManager&) = delete;

    /// Access the registry attached to the current runtime context,
    /// lazily creating it if necessary.
    [[nodiscard]] static TilesetManager& instance();

    /// Register a tileset by name (replaces any existing with the same name).
    /// @param name      Unique tileset name.
    /// @param tiles     Vector of TileType entries (each has id, name, graphic).
    /// @param tile_size Uniform tile size in pixels (width == height).
    void register_tileset(const std::string& name,
                          const std::vector<TileType>& tiles,
                          int tile_size);

    /// Look up a registered tileset by name.
    /// Returns nullptr if no tileset with that name exists.
    [[nodiscard]] const Tileset* lookup_tileset(const std::string& name) const;

    /// Get the base graphic for a specific tile within a named tileset.
    /// Returns nullptr if the tileset or tile_id is unknown.
    /// Callers must handle the null return (clipping / fallback).
    [[nodiscard]] const GraphicObject* get_tile_graphic(const std::string& tileset_name,
                                                        int tile_id) const;

    /// Clear all registered tilesets (for test isolation).
    void reset();

  private:
    std::unordered_map<std::string, Tileset> m_tilesets;
};

} // namespace pml
