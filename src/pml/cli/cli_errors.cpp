// ==========================================================================================================================================================================================================================================═
// PML CLI — Error formatting implementation
//
// Ported from pml/repl.py.  Provides structured error display with source
// snippets, JSON error serialization with call-stack and range info,
// and multi-error support.
// ==========================================================================================================================================================================================================================================═

#include "cli_errors.h"
#include "cli_shared.h"

#include "pml/core/error.h"
#include "pml/core/source_manager.h"

#include <format>
#include <iostream>
#include <nlohmann/json.hpp>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// JSON helpers
// ==========================================================================================================================================================================================================================================═

static nlohmann::json location_to_json(const SourceLocation& loc) {
    nlohmann::json j;
    if (loc.line > 0)   j["line"] = loc.line;
    if (loc.column > 0) j["column"] = loc.column;
    if (loc.end_line > 0)   j["end_line"] = loc.end_line;
    if (loc.end_column > 0) j["end_column"] = loc.end_column;
    if (!loc.filename.empty()) j["filename"] = loc.filename;
    return j;
}

static nlohmann::json call_stack_to_json(const std::vector<CallFrame>& frames) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& frame : frames) {
        nlohmann::json f;
        f["function"] = frame.function_name;
        if (frame.call_site.line > 0) {
            f["location"] = frame.call_site.to_string();
        }
        arr.push_back(std::move(f));
    }
    return arr;
}

// ==========================================================================================================================================================================================================================================═
// Single error to JSON
// ==========================================================================================================================================================================================================================================═

static nlohmann::json error_to_json_impl(const PMLException& e) {
    nlohmann::json j;
    j["type"] = std::string(to_string(e.code));
    j["message"] = e.message;
    if (e.location.has_value()) {
        j["location"] = location_to_json(*e.location);
    }
    j["hint"] = e.repair_hint.has_value() ? nlohmann::json(*e.repair_hint) : nlohmann::json(nullptr);

    if (!e.call_stack.empty()) {
        j["call_stack"] = call_stack_to_json(e.call_stack);
    }

    if (!e.details.empty()) {
        nlohmann::json details = nlohmann::json::array();
        for (const auto& sub : e.details) {
            details.push_back(error_to_json_impl(sub));
        }
        j["details"] = std::move(details);
    }

    return j;
}

// ==========================================================================================================================================================================================================================================═
// Public API
// ==========================================================================================================================================================================================================================================═

void print_error(const PMLException& err)
{
    // format_error_with_source now handles multi-error with context lines
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

    // Parse structured location with range info
    if (err.contains("location") && err["location"].is_object()) {
        SourceLocation loc;
        const auto& jloc = err["location"];
        loc.line = jloc.value("line", 0);
        loc.column = jloc.value("column", 0);
        loc.end_line = jloc.value("end_line", 0);
        loc.end_column = jloc.value("end_column", 0);
        loc.filename = jloc.value("filename", "");
        exc.location = std::move(loc);
    } else if (err.contains("line")) {
        // Fallback to flat fields
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

    // Parse nested details
    if (err.contains("details") && err["details"].is_array()) {
        for (const auto& detail : err["details"]) {
            PMLException sub;
            sub.code = ErrorCode::GeneralError;
            sub.message = detail.value("message", "");
            std::string dtype = detail.value("type", "GeneralError");
            if (dtype == "PMLSyntaxError") sub.code = ErrorCode::PMLSyntaxError;
            else if (dtype == "PMLTypeError") sub.code = ErrorCode::PMLTypeError;
            if (detail.contains("location") && detail["location"].is_object()) {
                SourceLocation sloc;
                sloc.line = detail["location"].value("line", 0);
                sloc.column = detail["location"].value("column", 0);
                sloc.end_line = detail["location"].value("end_line", 0);
                sloc.end_column = detail["location"].value("end_column", 0);
                sloc.filename = detail["location"].value("filename", "");
                sub.location = std::move(sloc);
            }
            if (detail.contains("hint") && !detail["hint"].is_null()) {
                sub.repair_hint = detail.value("hint", "");
            }
            exc.details.push_back(std::move(sub));
        }
    }

    std::cerr << format_error_with_source(exc, g_source_manager);
}

nlohmann::json error_to_json(const PMLException& e)
{
    return error_to_json_impl(e);
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

    if (err.details.empty()) {
        j["error"] = error_to_json_impl(err);
    } else {
        // Multi-error: produce an error list instead of a single error
        nlohmann::json err_list = nlohmann::json::array();
        for (const auto& sub : err.details) {
            err_list.push_back(error_to_json_impl(sub));
        }
        j["error"] = nlohmann::json{
            {"type", "MultiError"},
            {"message", std::format("Found {} error(s)", err.details.size())},
            {"details", std::move(err_list)}
        };
    }

    std::cout << j.dump(2, ' ', false, nlohmann::json::error_handler_t::replace) << std::endl;
}

}  // namespace pml
