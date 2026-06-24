// ═══════════════════════════════════════════════════════════════════════════════
// PML CLI — Execution mode implementation
//
// Ported from pml/repl.py.  Provides file execution, JSON output, and REPL
// mode dispatch.
// ═══════════════════════════════════════════════════════════════════════════════

#include "cli_execute.h"
#include "cli_shared.h"
#include "cli_errors.h"

#include "pml/api/api.h"
#include "pml/core/error.h"
#include "pml/layer/layer_builtins.h"
#include "pml/core/source_manager.h"
#include "pml/module/embedded_stdlib.h"
#include "repl.h"

#include <iostream>
#include <nlohmann/json.hpp>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// File execution mode
// ═══════════════════════════════════════════════════════════════════════════════

int run_file_mode(const CLIOptions& opts, PMLRuntime& runtime)
{
    // Propagate CLI output directory to the runtime context so that
    // (render ...) respects the -o flag.
    if (!opts.output_dir.empty()) {
        runtime.context().output_dir = opts.output_dir;
    }

    // Pre-load source so error messages can show source snippets.
    g_source_manager.load_file(opts.file);

    auto result = runtime.execute_file(opts.file);

    if (!result.success) {
        if (result.error.has_value()) {
            const auto& err = *result.error;
            std::string type = err.value("type", "Error");
            std::string msg = err.value("message", "Unknown error");

            // Check if it's a file-not-found type of error
            if (type == "ResourceError" &&
                msg.find("Cannot open file") != std::string::npos) {
                std::cerr << "Error: file not found: " << opts.file << std::endl;
            } else {
                print_render_error(err);
            }
        } else {
            std::cerr << "Error: execution failed" << std::endl;
        }
        return 1;
    }

    // Auto-render any registered compositions when an output directory is set.
    if (!opts.output_dir.empty()) {
        for (const auto& comp : runtime.compositions()) {
            if (!comp) continue;
            auto render_result = render_composition_to_disk(*comp, opts.output_dir);
            if (!render_result) {
                print_render_error(error_to_dict(render_result.error()));
                return 1;
            }
        }
    }

    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════════
// JSON output mode
// ═══════════════════════════════════════════════════════════════════════════════

int run_json_mode(const CLIOptions& opts, PMLRuntime& runtime)
{
    if (!opts.output_dir.empty()) {
        runtime.context().output_dir = opts.output_dir;
    }

    // PMLRuntime::execute_file returns errors via RenderResult, not exceptions.
    auto r = runtime.execute_file(opts.file);
    std::cout << r.to_json().dump(2) << std::endl;

    return r.success ? 0 : 1;
}

// ═══════════════════════════════════════════════════════════════════════════════
// REPL mode
// ═══════════════════════════════════════════════════════════════════════════════

int run_repl_mode(PMLRuntime& runtime)
{
    // Load embedded standard library modules into the global environment
    auto env = runtime.env();
    load_embedded_stdlib(env);

    // Enter interactive REPL (uses PMLRuntime for consistent pipeline)
    run_repl(runtime);
    return 0;
}

}  // namespace pml
