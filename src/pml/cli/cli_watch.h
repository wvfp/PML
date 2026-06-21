#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML CLI — Watch mode
//
// Platform-specific file-watch implementations (Linux inotify / Windows /
// fallback poll).  Ported from pml/repl.py watch logic.
// ═══════════════════════════════════════════════════════════════════════════════

#include <string>

struct CLIOptions;
namespace pml { class PMLRuntime; }

namespace pml {

/// Run watch mode: re-execute the file on every change.
int run_watch_mode(const CLIOptions& opts, PMLRuntime& runtime);

}  // namespace pml
