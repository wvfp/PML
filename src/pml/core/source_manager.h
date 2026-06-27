#pragma once

// ==========================================================================================================================================================================================================================================═
// Source Manager — cache source files for error reporting
// ==========================================================================================================================================================================================================================================═

#include "error.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

/// Caches file contents split into lines (1-based indexing) so that error
/// formatters can display source snippets without re-reading disk.
class SourceManager {
public:
    SourceManager() = default;

    /// Load a file from disk and split it into lines.
    /// Returns false if the file cannot be read.
    [[nodiscard]] bool load_file(const std::string& filename);

    /// Load source from an in-memory string (used for REPL input).
    /// Replaces any existing cache entry for `filename`.
    void load_source(const std::string& filename, const std::string& source);

    /// Ensure a file is loaded. If not already cached, reads it from disk.
    [[nodiscard]] bool ensure_loaded(const std::string& filename);

    /// Return the 1-based `line` from `filename`, or an empty string if
    /// the file is not cached or the line number is out of range.
    [[nodiscard]] std::string get_line(const std::string& filename, int line) const;

    /// Return true if `filename` is currently cached.
    [[nodiscard]] bool is_loaded(const std::string& filename) const noexcept;

private:
    std::unordered_map<std::string, std::vector<std::string>> file_cache_;
};

/// Return a reference to the global SourceManager singleton.
/// All code (CLI, evaluator, module loader) shares this instance so that
/// source snippets work for errors in both main files and imported modules.
[[nodiscard]] SourceManager& get_global_source_manager();

/// Format a PML error with source context. If the error carries a location
/// and the corresponding source line is available, the output includes the
/// line number, source text, and a caret pointing at the column.
[[nodiscard]] std::string format_error_with_source(
    const PMLException& err, SourceManager& src);

}  // namespace pml
