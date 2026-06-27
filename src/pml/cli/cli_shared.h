#pragma once

// ==========================================================================================================================================================================================================================================═
// PML CLI — Shared declarations
//
// Types, globals, and forward declarations shared across the CLI module.
// ==========================================================================================================================================================================================================================================═

#include "pml/core/source_manager.h"

#include <string>

namespace pml {
struct PMLException;
}  // namespace pml

// Global source cache for file-mode error snippets (backed by the
// process-wide SourceManager singleton so module loaders can also cache).
extern pml::SourceManager& g_source_manager;

// ==========================================================================================================================================================================================================================================═
// CLI options struct
// ==========================================================================================================================================================================================================================================═

struct CLIOptions {
    std::string file;             ///< File to execute (empty = REPL mode)
    std::string output_dir;       ///< -o / --output-dir
    bool watch{false};            ///< -w / --watch
    bool json{false};             ///< --json
    bool gif{false};              ///< --gif
    bool check{false};            ///< --check (validate only, no execution)
    bool help{false};             ///< -h / --help
};
