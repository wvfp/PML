#pragma once

// ==========================================================================================================================================================================================================================================═
// PML CLI — Execution mode entry points
//
// File execution, JSON output, and REPL modes.
// ==========================================================================================================================================================================================================================================═

struct CLIOptions;
namespace pml { class PMLRuntime; }

namespace pml {

/// Execute a PML file, optionally auto-rendering compositions to disk.
int run_file_mode(const CLIOptions& opts, PMLRuntime& runtime);

/// Execute a PML file and output results as JSON.
int run_json_mode(const CLIOptions& opts, PMLRuntime& runtime);

/// Run the interactive REPL.
int run_repl_mode(PMLRuntime& runtime);

}  // namespace pml
