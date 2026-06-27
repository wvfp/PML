// ==========================================================================================================================================================================================================================================═
// PML CLI — Execution mode implementation
//
// Ported from pml/repl.py.  Provides file execution, JSON output, and REPL
// mode dispatch.
// ==========================================================================================================================================================================================================================================═

#include "cli_execute.h"
#include "cli_shared.h"
#include "cli_errors.h"

#include "pml/api/api.h"
#include "pml/core/error.h"
#include "pml/layer/layer_builtins.h"
#include "pml/core/source_manager.h"
#include "pml/module/embedded_stdlib.h"
#include "repl.h"

#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// File execution mode
// ==========================================================================================================================================================================================================================================═

int run_file_mode(const CLIOptions& opts, PMLRuntime& runtime)
{
    // Load embedded standard library (defn, defconst, iteration macros, etc.)
    load_embedded_stdlib(runtime.env());

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

// ==========================================================================================================================================================================================================================================═
// JSON output mode
// ==========================================================================================================================================================================================================================================═

int run_json_mode(const CLIOptions& opts, PMLRuntime& runtime)
{
    // Load embedded standard library (defn, defconst, iteration macros, etc.)
    load_embedded_stdlib(runtime.env());

    if (!opts.output_dir.empty()) {
        runtime.context().output_dir = opts.output_dir;
    }

    // Redirect stdout during execution so (print ...) output doesn't
    // pollute the JSON response. Captured output is optionally included
    // as a "messages" field.
    auto old_buf = std::cout.rdbuf();
    std::ostringstream capture_buf;
    std::cout.rdbuf(capture_buf.rdbuf());

    auto r = runtime.execute_file(opts.file);

    // Restore stdout before writing JSON
    std::cout.rdbuf(old_buf);

    auto j = r.to_json();
    std::string captured = capture_buf.str();
    if (!captured.empty()) {
        j["messages"] = std::move(captured);
    }
    std::cout << j.dump(2, ' ', false, nlohmann::json::error_handler_t::replace) << std::endl;

    return r.success ? 0 : 1;
}

// ==========================================================================================================================================================================================================================================═
// Check mode (validate only)
// ==========================================================================================================================================================================================================================================═

int run_check_mode(const CLIOptions& opts, PMLRuntime& runtime)
{
    // Load embedded standard library
    load_embedded_stdlib(runtime.env());

    // Read the file
    std::ifstream file(opts.file);
    if (!file.is_open()) {
        std::cerr << "Error: file not found: " << opts.file << std::endl;
        return 1;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string source = std::move(ss).str();

    // Cache source for error snippet display
    g_source_manager.load_source(opts.file, source);

    // Validate without executing — collects ALL errors
    auto result = runtime.validate(source, opts.file);

    if (result.valid) {
        std::cout << "No errors found." << std::endl;
        return 0;
    }

    if (opts.json) {
        // JSON output for check mode
        nlohmann::json j;
        j["success"] = false;
        j["file"] = opts.file;
        nlohmann::json err_list = nlohmann::json::array();
        for (const auto& e : result.errors) {
            err_list.push_back(e);
        }
        j["errors"] = std::move(err_list);
        j["count"] = result.errors.size();
        std::cout << j.dump(2, ' ', false, nlohmann::json::error_handler_t::replace) << std::endl;
    } else {
        // Human-readable output using the rich error formatter
        std::cerr << std::format("Found {} error(s) in {}:\n",
                                 result.errors.size(), opts.file);
        for (size_t i = 0; i < result.errors.size(); ++i) {
            if (result.errors.size() > 1) {
                std::cerr << std::format("\n--- Error {} of {} ---\n",
                                         i + 1, result.errors.size());
            }
            // Convert JSON error back to PMLException and use rich formatting
            PMLException exc;
            exc.code = ErrorCode::GeneralError;
            exc.message = result.errors[i].value("message", "");
            std::string type = result.errors[i].value("type", "GeneralError");
            if (type == "PMLSyntaxError")    exc.code = ErrorCode::PMLSyntaxError;
            else if (type == "PMLTypeError") exc.code = ErrorCode::PMLTypeError;
            // Parse structured location (nested or flat)
            bool has_loc = false;
            SourceLocation loc;
            if (result.errors[i].contains("location") && result.errors[i]["location"].is_object()) {
                const auto& jloc = result.errors[i]["location"];
                loc.line = jloc.value("line", 0);
                loc.column = jloc.value("column", 0);
                loc.end_line = jloc.value("end_line", 0);
                loc.end_column = jloc.value("end_column", 0);
                loc.filename = jloc.value("filename", opts.file);
                has_loc = true;
            } else if (result.errors[i].contains("line")) {
                loc.line = result.errors[i].value("line", 0);
                loc.column = result.errors[i].value("column", 0);
                loc.end_line = result.errors[i].value("end_line", 0);
                loc.end_column = result.errors[i].value("end_column", 0);
                loc.filename = result.errors[i].value("filename", opts.file);
                has_loc = true;
            }
            if (has_loc) {
                exc.location = std::move(loc);
            }
            if (result.errors[i].contains("hint") && !result.errors[i]["hint"].is_null()) {
                exc.repair_hint = result.errors[i].value("hint", "");
            }
            std::cerr << format_error_with_source(exc, g_source_manager);
        }
    }

    return 1;
}

// ==========================================================================================================================================================================================================================================═
// REPL mode
// ==========================================================================================================================================================================================================================================═

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
