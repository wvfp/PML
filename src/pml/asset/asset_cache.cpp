// ═══════════════════════════════════════════════════════════════════════════════
// AssetCache — implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "asset_cache.h"
#include "asset_loader.h"

#include <shared_mutex>
#include <mutex>

namespace pml {

Result<std::shared_ptr<Surface>> AssetCache::load_image(
    RenderBackend& backend,
    const std::string& path,
    const std::string& from_file)
{
    auto resolved = resolve_asset_path(path, from_file, search_roots);
    if (!resolved) {
        return std::unexpected(resolved.error());
    }

    {
        std::shared_lock lock(mutex_);
        if (auto it = cache_.find(*resolved); it != cache_.end()) {
            if (it->second) {
                return it->second;
            }
        }
    }

    auto surface = backend.load_image(*resolved);
    if (!surface) {
        return std::unexpected(surface.error());
    }

    std::shared_ptr<Surface> shared = std::move(*surface);
    {
        std::unique_lock lock(mutex_);
        cache_[*resolved] = shared;
    }
    return shared;
}

void AssetCache::clear() {
    std::unique_lock lock(mutex_);
    cache_.clear();
}

size_t AssetCache::size() const {
    std::shared_lock lock(mutex_);
    return cache_.size();
}

} // namespace pml
