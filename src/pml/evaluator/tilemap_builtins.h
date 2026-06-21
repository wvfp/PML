#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Tilemap Builtins — Tile-based game map data layer
// ───────────────────────────────────────────────────────────────────────────────
// Registers define-tileset, make-tilemap, tilemap-set! built-in procedures.
// These form the data-management layer for tile-based game maps.
//
// Builtins registered:
//   (define-tileset name :tile-size N :tiles '((id name graphic ...) ...))
//     — register a named tileset with tile types
//   (make-tilemap tileset-name cols rows [:projection 'orthogonal|'isometric] [:layers 1])
//     — create a named tilemap
//   (tilemap-set! tilemap-name layer col row tile-id)
//     — set a tile in the tilemap
// ═══════════════════════════════════════════════════════════════════════════════

#include "environment.h"

#include <memory>

namespace pml {

/// Register tilemap-related built-in procedures into the given environment.
void register_tilemap_builtins(std::shared_ptr<Environment> env);

}  // namespace pml
