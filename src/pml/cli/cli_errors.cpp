// ==========================================================================================================================================================================================================================================═
// PML CLI — Error formatting implementation
//
// Ported from pml/repl.py.  Provides structured error display with source
// snippets, JSON error serialization, and call-stack-aware error formatting.
// ==========================================================================================================================================================================================================================================═

#include "cli_errors.h"
#include "cli_shared.h"

#include "pml/core/error.h"

#include <iostream>
#include <nlohmann/json.hpp>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// Error formatting
// ==========================================================================================================================================================================================================================================═

void print_error(const PMLException& err)
{
    // Format with source snippet, line number and caret when available.
    std::cerr << format_error_with_source(err, g_source_manager);
}

/// Convert a structured RenderResult error JSON back to a printable message.
/// Loads the source file into `g_source_manager` so snippets can be shown.
void print_render_error(const nlohmann::json& err)
{
    PMLException exc;
    exc.code = ErrorCode::GeneralError;
    exc.message = err.value("message", "Unknown error");

    if (err.contains("type")) {
        const std::string type = err.value("type", "GeneralError");
        if (type == "PMLSyntaxError")    exc.code = ErrorCode::PMLSyntaxError;
        else if (type == "PMLTypeError") exc.code = ErrorCode::PMLTypeError;
        else if (type == "UnboundVariableError")
            exc.code = ErrorCode::UnboundVariableError;
        else if (type == "ArityError")   exc.code = ErrorCode::ArityError;
        else if (type == "ImportError")  exc.code = ErrorCode::ImportError;
        else if (type == "CircularImportError")
            exc.code = ErrorCode::CircularImportError;
        else if (type == "MacroExpansionDepthError")
            exc.code = ErrorCode::MacroExpansionDepthError;
        else if (type == "AccessError")  exc.code = ErrorCode::AccessError;
        else if (type == "ResourceError") exc.code = ErrorCode::ResourceError;
        else if (type == "IKNoSolutionError")
            exc.code = ErrorCode::IKNoSolutionError;
        else if (type == "PMLAssertionError")
            exc.code = ErrorCode::PMLAssertionError;
    }

    if (err.contains("line")) {
        SourceLocation loc;
        loc.line = err.value("line", 0);
        loc.column = err.value("column", 0);
        loc.filename = err.value("filename", "");
        exc.location = std::move(loc);
    }

    if (err.contains("hint") && !err["hint"].is_null()) {
        exc.repair_hint = err.value("hint", "");
    }

    if (err.contains("call_stack") && err["call_stack"].is_array()) {
        for (const auto& frame : err["call_stack"]) {
            CallFrame f;
            f.function_name = frame.value("function", "");
            // Parse location string "filename:line:col" or "line line:col"
            std::string loc_str = frame.value("location", "");
            if (!loc_str.empty()) {
                auto last_colon = loc_str.find_last_of(':');
                if (last_colon != std::string::npos) {
                    auto prev_colon = loc_str.find_last_of(':', last_colon - 1);
                    try {
                        if (prev_colon != std::string::npos) {
                            f.call_site.filename = loc_str.substr(0, prev_colon);
                            f.call_site.line = std::stoi(loc_str.substr(prev_colon + 1, last_colon - prev_colon - 1));
                            f.call_site.column = std::stoi(loc_str.substr(last_colon + 1));
                        } else {
                            f.call_site.line = std::stoi(loc_str.substr(0, last_colon));
                            f.call_site.column = std::stoi(loc_str.substr(last_colon + 1));
                        }
                    } catch (...) {
                        // Ignore parse errors; leave location empty.
                    }
                }
            }
            exc.call_stack.push_back(std::move(f));
        }
    }

    if (err.contains("details") && err["details"].is_array()) {
        std::cerr << format_error_with_source(exc, g_source_manager);
        int index = 1;
        for (const auto& detail : err["details"]) {
            std::cerr << "\n  --- Error " << index++ << " ---\n";
            print_render_error(detail);
        }
    } else {
        std::cerr << format_error_with_source(exc, g_source_manager);
    }
}

nlohmann::json error_to_json(const PMLException& e)
{
    nlohmann::json j;
    j["type"] = std::string(to_string(e.code));
    j["message"] = e.message;
    if (e.location.has_value()) {
        const auto& loc = *e.location;
        if (loc.line > 0)   j["line"] = loc.line;
        if (loc.column > 0) j["column"] = loc.column;
        if (!loc.filename.empty()) j["filename"] = loc.filename;
    }
    j["hint"] = e.repair_hint.has_value() ? nlohmann::json(*e.repair_hint) : nlohmann::json(nullptr);
    return j;
}

nlohmann::json error_to_json(int /*code*/, const std::string& type,
                             const std::string& message)
{
    nlohmann::json j;
    j["type"] = type;
    j["message"] = message;
    return j;
}

void print_json_error(const PMLException& err)
{
    nlohmann::json j;
    j["success"] = false;
    j["result"] = nullptr;
    j["files"] = nlohmann::json::array();
    j["error"] = error_to_json(err);
    std::cout << j.dump(2) << std::endl;
}

}  // namespace pml
