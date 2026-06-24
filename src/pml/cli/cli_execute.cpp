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

    nlohmann::json result;

    try {
        auto r = runtime.execute_file(opts.file);
        if (r.success) {
            result["success"] = true;
            result["value"] = r.value;
            result["files"] = r.files;
            result["error"] = nullptr;
        } else {
            result["success"] = false;
            result["value"] = nullptr;
            result["files"] = nlohmann::json::array();
            if (r.error.has_value()) {
                result["error"] = *r.error;
            } else {
                result["error"] = error_to_json(0, "UnknownError", "Execution failed");
            }
        }
    } catch (const std::exception& e) {
        result["success"] = false;
        result["value"] = nullptr;
        result["files"] = nlohmann::json::array();
        result["error"] = error_to_json(0, "InternalError", e.what());
    }

    std::cout << result.dump(2) << std::endl;

    if (!result["success"].get<bool>()) {
        return 1;
    }
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════════
// REPL mode
// ═══════════════════════════════════════════════════════════════════════════════

int run_repl_mode(PMLRuntime& runtime)
{
    // Load embedded standard library modules into the global environment
    auto env = runtime.env();
    load_embedded_stdlib(env);

    // Enter interactive REPL
    run_repl(env);
    return 0;
}

}  // namespace pml
