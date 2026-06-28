// ==========================================================================================================================================================================================================================================═
// PML Runtime Context — Implementation
// ==========================================================================================================================================================================================================================================═

#include "pml/api/context.h"

#include "pml/asset/asset_cache.h"
#include "pml/graphics/canvas.h"
#include "pml/animation/timeline.h"
#include "pml/graphics/name_registry.h"
#include "pml/sprites/style.h"
#include "pml/sprites/palette.h"
#include "pml/sprites/registry.h"
#include "pml/graphics/tilemap.h"
#include "pml/graphics/tileset.h"
#include "pml/core/call_stack.h"
#include "pml/core/source_manager.h"
#include "pml/api/texture_cache.h"
#include "pml/graphics3d/camera3d.h"
#include "pml/module/module_loader.h"

namespace pml {

// ==========================================================================================================================================================================================================================================═
// Thread-local current context
// ==========================================================================================================================================================================================================================================═

thread_local PMLContext* PMLContext::s_current = nullptr;

// ==========================================================================================================================================================================================================================================═
// Construction / destruction
// ==========================================================================================================================================================================================================================================═

PMLContext::PMLContext()
    : assets(std::make_unique<AssetCache>())
    , call_stack(std::make_unique<CallStack>())
    , source_manager(std::make_unique<SourceManager>())
    , camera(std::make_unique<Camera3D>())
    , name_registry(std::make_unique<NameRegistry>()) {}

PMLContext::~PMLContext() = default;

// ==========================================================================================================================================================================================================================================═
// Lifecycle
// ==========================================================================================================================================================================================================================================═

void PMLContext::reset() {
    current_canvas.reset();
    timeline.reset();
    styles = std::make_unique<StyleRegistry>();
    palettes = std::make_unique<PaletteManager>();
    tilemaps = std::make_unique<TilemapManager>();
    tilesets = std::make_unique<TilesetManager>();
    assets = std::make_unique<AssetCache>();
    texture_cache.reset();
    compositions.clear();
    output_files.clear();
    source_dir.clear();
    output_dir.clear();
    call_stack = std::make_unique<CallStack>();
    source_manager = std::make_unique<SourceManager>();
    camera = std::make_unique<Camera3D>();
    module_loader.reset();
    macro_depth = 0;
    name_registry = std::make_unique<NameRegistry>();
}

// ==========================================================================================================================================================================================================================================═
// Current-context access
// ==========================================================================================================================================================================================================================================═

PMLContext& PMLContext::current() {
    if (s_current) {
        return *s_current;
    }
    static PMLContext default_context;
    return default_context;
}

PMLContext* PMLContext::current_ptr() noexcept {
    return s_current;
}

void PMLContext::set_current(PMLContext* ctx) noexcept {
    s_current = ctx;
}

} // namespace pml
