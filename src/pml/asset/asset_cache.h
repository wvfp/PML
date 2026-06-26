// ==========================================================================================================================================================================================================================================═
// AssetCache — per-runtime cache for decoded image assets
// ==========================================================================================================================================================================================================================================═

#pragma once

#include "pml/backend/backend.h"
#include "pml/core/error.h"

#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

/// Per-runtime image asset cache.
///
/// Keeps decoded raster surfaces in memory so that the same image file is only
/// loaded once even if it is referenced by many GraphicObjects / layers.
/// The cache is backend-specific because Surface implementations are backend
/// specific; it must be cleared when the active backend changes.
class AssetCache {
public:
    AssetCache() = default;
    ~AssetCache() = default;

    AssetCache(const AssetCache&) = delete;
    AssetCache& operator=(const AssetCache&) = delete;

    /// Load an image from disk into a Surface, using the cache if available.
    /// @param backend    Active render backend (used for decode).
    /// @param path       User-provided asset path.
    /// @param from_file  Path of the PML source file doing the loading,
    ///                   used for relative resolution.
    /// @return A shared pointer to the decoded Surface, or an error.
    [[nodiscard]] Result<std::shared_ptr<Surface>> load_image(
        RenderBackend& backend,
        const std::string& path,
        const std::string& from_file = "");

    /// Remove all cached entries.
    void clear();

    /// Return the number of cached entries.
    [[nodiscard]] size_t size() const;

    /// Optional extra search roots (e.g. project asset directories).
    std::vector<std::string> search_roots;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Surface>> cache_;
};

} // namespace pml
