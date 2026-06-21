#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Tilemap — Flat int array grid + multi-layer support
// ───────────────────────────────────────────────────────────────────────────────
// Foundation data structure for tile-based game maps.  Wave 1: no rendering
// or PML builtins — pure data.
// ═══════════════════════════════════════════════════════════════════════════════

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Projection — Tilemap Rendering Projection
// ═══════════════════════════════════════════════════════════════════════════════

/// Determines how tile coordinates map to screen space.
enum class Projection {
    Orthogonal,  ///< Standard top-down 2D grid
    Isometric    ///< Diamond-shaped isometric view
};

// ═══════════════════════════════════════════════════════════════════════════════
// TilemapLayer — A Single Layer of Tile Data
// ═══════════════════════════════════════════════════════════════════════════════

/// Flat int-array grid representing one layer of a tilemap.
/// Tile ID 0 means "empty/no tile".
struct TilemapLayer {
    std::vector<int> tiles;  ///< Flat array, row-major: tiles[row * cols + col]
    int cols = 0;            ///< Number of columns
    int rows = 0;            ///< Number of rows
    bool visible = true;     ///< Whether this layer is visible
};

// ═══════════════════════════════════════════════════════════════════════════════
// Tilemap — Full Tilemap Definition
// ═══════════════════════════════════════════════════════════════════════════════

/// A complete tilemap with multi-layer support.
struct Tilemap {
    std::string tileset_name;                 ///< Name of the tileset for rendering
    int cols = 0;                             ///< Grid width in tiles
    int rows = 0;                             ///< Grid height in tiles
    int tile_size = 32;                       ///< Tile size in pixels (default 32)
    std::vector<TilemapLayer> layers;         ///< Ordered layers (bottom to top)
    Projection projection = Projection::Orthogonal;  ///< Rendering projection
};

// ═══════════════════════════════════════════════════════════════════════════════
// Helper Functions
// ═══════════════════════════════════════════════════════════════════════════════

/// Return the tile ID at (col, row) in the given layer.
/// Returns 0 (empty) for out-of-bounds access — no error, just empty.
[[nodiscard]] int tile_at(const TilemapLayer& layer, int col, int row);

/// Set the tile ID at (col, row) in the given layer.
/// Silently does nothing if the coordinates are out of bounds.
void set_tile(TilemapLayer& layer, int col, int row, int tile_id);

/// Create a new layer with the given dimensions, fully zero-initialized.
TilemapLayer add_layer(int cols, int rows, bool visible = true);

// ═══════════════════════════════════════════════════════════════════════════════
// TilemapManager — Global Named Tilemap Registry (Singleton)
// ═══════════════════════════════════════════════════════════════════════════════

/// Registry of named tilemaps.  Tilemaps are created and looked up by name.
/// The global accessor `instance()` delegates to the current PMLContext.
class TilemapManager {
  public:
    TilemapManager() = default;
    TilemapManager(const TilemapManager&) = delete;
    TilemapManager& operator=(const TilemapManager&) = delete;

    /// Access the registry attached to the current runtime context,
    /// lazily creating it if necessary.
    [[nodiscard]] static TilemapManager& instance();

    /// Create a new named tilemap and store it in the registry.
    /// If a tilemap with the same name already exists, it is overwritten.
    /// @param name       Unique name to identify this tilemap
    /// @param tileset_name  Name of the tileset used for rendering
    /// @param cols       Grid width in tiles
    /// @param rows       Grid height in tiles
    /// @param projection Rendering projection (Orthogonal or Isometric)
    /// @param layers     Vector of TilemapLayer objects (may be empty)
    void create_tilemap(const std::string& name,
                        const std::string& tileset_name,
                        int cols, int rows,
                        Projection projection,
                        std::vector<TilemapLayer> layers = {});

    /// Look up a named tilemap by name.
    /// Returns nullptr if no tilemap with that name exists.
    [[nodiscard]] Tilemap* lookup_tilemap(const std::string& name);

    /// Remove all registered tilemaps.
    void reset();

  private:
    std::unordered_map<std::string, Tilemap> m_tilemaps;
};

} // namespace pml
