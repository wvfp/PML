// ==========================================================================================================================================================================================================================================═
// AssetLoader — path resolution helpers for external resources
// ==========================================================================================================================================================================================================================================═

#pragma once

#include "pml/core/error.h"

#include <string>
#include <vector>

namespace pml {

/// Resolve an asset path against a set of search roots.
/// Tries in order:
///   1. path as an absolute path
///   2. relative to from_file's directory (if provided)
///   3. relative to each search root
/// Returns the first existing absolute path, or a ResourceError.
[[nodiscard]] Result<std::string> resolve_asset_path(
    const std::string& path,
    const std::string& from_file = "",
    const std::vector<std::string>& search_roots = {});

} // namespace pml
