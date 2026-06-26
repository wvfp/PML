#pragma once

// ==========================================================================================================================================================================================================================================═
// MCP Server — JSON-RPC 2.0 over stdio for PML AI agent integration.
//
// Ported from pml/mcp_server.py.  Provides 5 tools matching the Python MCP:
//   execute_pml, render_sprite, validate, list_components, preview_params
//
// Protocol: JSON-RPC 2.0 with Content-Length framing over stdin/stdout.
// ==========================================================================================================================================================================================================================================═

#include <memory>
#include <string>

#include <nlohmann/json.hpp>

namespace pml {

class PMLRuntime;

/// MCP server exposing PML tools over stdio JSON-RPC 2.0 transport.
///
/// Singleton request/response loop.  Reads `Content-Length:` framed JSON
/// from stdin, dispatches to the appropriate handler, and writes framed
/// JSON responses to stdout.
class MCPServer {
public:
    MCPServer();
    ~MCPServer();

    /// Run the server loop (blocking).
    ///
    /// Reads requests from stdin until EOF.  Each request is dispatched
    /// and a response is written to stdout.
    void run();

private:
    // ---- Message framing (stdio) ------------------------------------------------------------------------------------

    /// Read one JSON-RPC message from stdin.
    /// Parses the Content-Length header and reads exactly N bytes of JSON.
    /// Returns empty string on EOF.
    [[nodiscard]] std::string read_message();

    /// Write a JSON-RPC response to stdout with Content-Length framing.
    void write_message(const std::string& body);

    // ---- JSON-RPC request dispatch --------------------------------------------------------------------------------

    /// Dispatch a parsed JSON-RPC request and return a response JSON object.
    /// Returns a full JSON-RPC response (with id, jsonrpc, result/error).
    [[nodiscard]] nlohmann::json dispatch(const nlohmann::json& request);

    // ---- MCP method handlers --------------------------------------------------------------------------------------------

    /// Handle initialize request.
    [[nodiscard]] nlohmann::json handle_initialize(const nlohmann::json& params);

    /// Handle tools/list request.
    [[nodiscard]] nlohmann::json handle_tools_list();

    /// Handle tools/call request.
    [[nodiscard]] nlohmann::json handle_tools_call(const nlohmann::json& params);

    // ---- Tool implementations ----------------------------------------------------------------------------------------─

    nlohmann::json tool_execute_pml(const nlohmann::json& args);
    nlohmann::json tool_render_sprite(const nlohmann::json& args);
    nlohmann::json tool_validate(const nlohmann::json& args);
    nlohmann::json tool_list_components(const nlohmann::json& args);
    nlohmann::json tool_preview_params(const nlohmann::json& args);

    // ---- Helpers --------------------------------------------------------------------------------------------------------------------

    /// Build a JSON-RPC error response object.
    [[nodiscard]] static nlohmann::json make_error(
        int code,
        const std::string& message,
        nlohmann::json data = nullptr);

    /// Build a JSON-RPC result response envelope.
    [[nodiscard]] static nlohmann::json make_result(
        const nlohmann::json& id,
        nlohmann::json result);

    /// Build a JSON-RPC error response envelope.
    [[nodiscard]] static nlohmann::json make_error_response(
        const nlohmann::json& id,
        int code,
        const std::string& message,
        nlohmann::json data = nullptr);

    // ---- State ------------------------------------------------------------------------------------------------------------------------

    bool m_initialized = false;
    std::unique_ptr<PMLRuntime> m_runtime;
};

}  // namespace pml
