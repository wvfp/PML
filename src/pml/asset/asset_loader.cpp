// ==========================================================================================================================================================================================================================================═
// AssetLoader — path resolution implementation
// ==========================================================================================================================================================================================================================================═

#include "asset_loader.h"

#include <filesystem>
#include <format>
#include <fstream>

namespace pml {

namespace fs = std::filesystem;

Result<std::string> resolve_asset_path(
    const std::string& path,
    const std::string& from_file,
    const std::vector<std::string>& search_roots)
{
    if (path.empty()) {
        return std::unexpected(resource_error( "asset path is empty"));
    }

    // 1. Absolute path as-is.
    fs::path p = path;
    if (p.is_absolute()) {
        std::error_code ec;
        if (fs::exists(p, ec) && fs::is_regular_file(p, ec)) {
            return fs::canonical(p, ec).string();
        }
        return std::unexpected(resource_error(
            std::format("asset not found: {}", path)));
    }

    std::vector<std::string> candidates;

    // 2. Relative to the source file doing the loading.
    if (!from_file.empty()) {
        fs::path base = fs::path(from_file).parent_path();
        if (!base.empty()) {
            candidates.push_back((base / p).string());
        }
    }

    // 3. Relative to each search root.
    for (const auto& root : search_roots) {
        if (!root.empty()) {
            candidates.push_back((fs::path(root) / p).string());
        }
    }

    // 4. Relative to current working directory.
    candidates.push_back(p.string());

    std::error_code ec;
    for (const auto& candidate : candidates) {
        fs::path cp = candidate;
        if (fs::exists(cp, ec) && fs::is_regular_file(cp, ec)) {
            return fs::canonical(cp, ec).string();
        }
    }

    return std::unexpected(resource_error(
        std::format("asset not found: {} (tried {} locations)",
                    path, candidates.size())));
}

} // namespace pml
