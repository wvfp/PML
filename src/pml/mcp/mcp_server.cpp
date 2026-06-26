// ==========================================================================================================================================================================================================================================═
// MCP Server — JSON-RPC 2.0 over stdio
//
// Ported from pml/mcp_server.py.  Implements 5 tools matching the Python MCP:
//   execute_pml  — execute PML source, return result
//   render_sprite — execute PML and render canvas to sprite file
//   validate     — validate PML source (tokenize + parse only)
//   list_components — list available sprite components (stub)
//   preview_params  — get parameter info for a component (stub)
// ==========================================================================================================================================================================================================================================═

#include "pml/mcp/mcp_server.h"

#include "pml/api/api.h"
#include "pml/core/error.h"
#include "pml/core/types.h"
#include "pml/sprites/registry.h"
#include "pml/sprites/validator.h"
// (frontend headers not needed directly — validate() delegates to PMLRuntime)

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// Construction / Destruction
// ==========================================================================================================================================================================================================================================═

MCPServer::MCPServer()
    : m_runtime(std::make_unique<PMLRuntime>())
{
}

MCPServer::~MCPServer() = default;

// ==========================================================================================================================================================================================================================================═
// Message Framing (Content-Length header + JSON body)
// ==========================================================================================================================================================================================================================================═

std::string MCPServer::read_message() {
    // Read headers until we find Content-Length or hit EOF
    int content_length = 0;
    std::string line;

    while (true) {
        if (!std::getline(std::cin, line)) {
            // EOF or error — signal end of input
            return {};
        }

        // Strip trailing \r (from \r\n line ending)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            // End of headers — body follows
            break;
        }

        // Parse Content-Length header (case-insensitive)
        // "Content-Length:" is 15 characters
        if (line.size() >= 15) {
            std::string prefix = line.substr(0, 15);
            std::transform(prefix.begin(), prefix.end(), prefix.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (prefix == "content-length:") {
                std::string value = line.substr(16);
                // Trim leading whitespace
                auto first = value.find_first_not_of(" \t");
                if (first != std::string::npos) {
                    value = value.substr(first);
                }
                content_length = std::stoi(value);
            }
        }
    }

    if (content_length <= 0) {
        return {};
    }

    // Read exactly content_length bytes of JSON body
    std::string body(content_length, '\0');
    std::cin.read(body.data(), content_length);

    if (std::cin.gcount() != content_length) {
        // Short read — unexpected EOF
        return {};
    }

    return body;
}

void MCPServer::write_message(const std::string& body) {
    std::ostringstream oss;
    oss << "Content-Length: " << body.size() << "\r\n\r\n";
    oss << body;
    std::cout << oss.str() << std::flush;
}

// ==========================================================================================================================================================================================================================================═
// JSON-RPC Error / Result Envelopes
// ==========================================================================================================================================================================================================================================═

nlohmann::json MCPServer::make_error(
    int code,
    const std::string& message,
    nlohmann::json data)
{
    nlohmann::json err;
    err["code"] = code;
    err["message"] = message;
    if (!data.is_null()) {
        err["data"] = std::move(data);
    }
    return err;
}

nlohmann::json MCPServer::make_result(
    const nlohmann::json& id,
    nlohmann::json result)
{
    nlohmann::json resp;
    resp["jsonrpc"] = "2.0";
    resp["id"] = id;
    resp["result"] = std::move(result);
    return resp;
}

nlohmann::json MCPServer::make_error_response(
    const nlohmann::json& id,
    int code,
    const std::string& message,
    nlohmann::json data)
{
    nlohmann::json resp;
    resp["jsonrpc"] = "2.0";
    resp["id"] = id;
    resp["error"] = make_error(code, message, std::move(data));
    return resp;
}

// ==========================================================================================================================================================================================================================================═
// Request Dispatch
// ==========================================================================================================================================================================================================================================═

nlohmann::json MCPServer::dispatch(const nlohmann::json& request) {
    // ---- Validate JSON-RPC 2.0 request ------------------------------------------------------------------------
    if (!request.is_object()) {
        return make_error_response(nullptr, -32600, "Invalid Request: expected object");
    }

    auto it_jsonrpc = request.find("jsonrpc");
    if (it_jsonrpc == request.end() || !it_jsonrpc->is_string()
        || it_jsonrpc->get<std::string>() != "2.0")
    {
        return make_error_response(
            request.value("id", nullptr), -32600, "Invalid Request: jsonrpc must be '2.0'");
    }

    // ---- Notifications (no id field) — no response ------------------------------------------------
    auto it_id = request.find("id");
    if (it_id == request.end()) {
        // This is a notification — silently ignore
        return {};
    }
    const nlohmann::json& id = *it_id;

    auto it_method = request.find("method");
    if (it_method == request.end() || !it_method->is_string()) {
        return make_error_response(id, -32600, "Invalid Request: method required");
    }
    std::string method = it_method->get<std::string>();

    nlohmann::json params = nlohmann::json::object();
    auto it_params = request.find("params");
    if (it_params != request.end()) {
        params = *it_params;
    }

    // ---- Dispatch known methods ------------------------------------------------------------------------------------─
    nlohmann::json result;

    if (method == "initialize") {
        result = handle_initialize(params);
    } else if (method == "tools/list") {
        if (!m_initialized) {
            return make_error_response(id, -32000, "Server not initialized");
        }
        result = handle_tools_list();
    } else if (method == "tools/call") {
        if (!m_initialized) {
            return make_error_response(id, -32000, "Server not initialized");
        }
        result = handle_tools_call(params);
    } else {
        return make_error_response(id, -32601, "Method not found: " + method);
    }

    return make_result(id, std::move(result));
}

// ==========================================================================================================================================================================================================================================═
// MCP Handlers: initialize
// ==========================================================================================================================================================================================================================================═

nlohmann::json MCPServer::handle_initialize(const nlohmann::json& /*params*/) {
    m_initialized = true;

    nlohmann::json capabilities;
    capabilities["tools"] = nlohmann::json::object();

    nlohmann::json server_info;
    server_info["name"] = "pml-mcp";
    server_info["version"] = "0.1.0";

    nlohmann::json result;
    result["protocolVersion"] = "2024-11-05";
    result["capabilities"] = std::move(capabilities);
    result["serverInfo"] = std::move(server_info);
    return result;
}

// ==========================================================================================================================================================================================================================================═
// MCP Handlers: tools/list
// ==========================================================================================================================================================================================================================================═

nlohmann::json MCPServer::handle_tools_list() {
    nlohmann::json tools = nlohmann::json::array();

    // ---- Tool 1: execute_pml --------------------------------------------------------------------------------------------
    {
        nlohmann::json tool;
        tool["name"] = "execute_pml";
        tool["description"] =
            "Execute PML source code and return the result. "
            "Use this to run any PML code: define variables, call functions, "
            "create graphics, set up animation timelines, etc. "
            "Environment persists across calls — previous definitions are visible.";

        nlohmann::json schema;
        schema["type"] = "object";

        nlohmann::json props;
        props["source"]["type"] = "string";
        props["source"]["description"] = "PML source code to execute";
        schema["properties"] = std::move(props);

        nlohmann::json required = nlohmann::json::array();
        required.push_back("source");
        schema["required"] = std::move(required);

        tool["inputSchema"] = std::move(schema);
        tools.push_back(std::move(tool));
    }

    // ---- Tool 2: render_sprite ----------------------------------------------------------------------------------------
    {
        nlohmann::json tool;
        tool["name"] = "render_sprite";
        tool["description"] =
            "Execute PML source and render the canvas to a PNG sprite file. "
            "The source should set up a canvas with (sprite-canvas ...), "
            "add graphics with (add ...), then this tool handles rendering. "
            "Returns the output file path and metadata.";

        nlohmann::json schema;
        schema["type"] = "object";

        nlohmann::json props;
        props["source"]["type"] = "string";
        props["source"]["description"] = "PML source code to execute and render";
        props["name"]["type"] = "string";
        props["name"]["description"] = "Output sprite name (without extension)";
        props["name"]["default"] = "sprite";
        schema["properties"] = std::move(props);

        nlohmann::json required = nlohmann::json::array();
        required.push_back("source");
        schema["required"] = std::move(required);

        tool["inputSchema"] = std::move(schema);
        tools.push_back(std::move(tool));
    }

    // ---- Tool 3: validate ------------------------------------------------------------------------------------------------─
    {
        nlohmann::json tool;
        tool["name"] = "validate";
        tool["description"] =
            "Validate PML source code without executing it. "
            "Checks lexing, parsing, and macro expansion for errors. "
            "Safe to use on untrusted or incomplete code.";

        nlohmann::json schema;
        schema["type"] = "object";

        nlohmann::json props;
        props["source"]["type"] = "string";
        props["source"]["description"] = "PML source code to validate";
        schema["properties"] = std::move(props);

        nlohmann::json required = nlohmann::json::array();
        required.push_back("source");
        schema["required"] = std::move(required);

        tool["inputSchema"] = std::move(schema);
        tools.push_back(std::move(tool));
    }

    // ---- Tool 4: list_components ------------------------------------------------------------------------------------
    {
        nlohmann::json tool;
        tool["name"] = "list_components";
        tool["description"] =
            "List available sprite components with parameter specs. "
            "Optionally filter by category: 'character', 'items', 'ui', or 'scene'. "
            "Returns component names, categories, and their parameters.";

        nlohmann::json schema;
        schema["type"] = "object";

        nlohmann::json props;
        props["category"]["type"] = "string";
        props["category"]["description"] =
            "Optional category filter: 'character', 'items', 'ui', or 'scene'";
        schema["properties"] = std::move(props);

        tool["inputSchema"] = std::move(schema);
        tools.push_back(std::move(tool));
    }

    // ---- Tool 5: preview_params ------------------------------------------------------------------------------------─
    {
        nlohmann::json tool;
        tool["name"] = "preview_params";
        tool["description"] =
            "Get the full parameter specification for a specific component. "
            "Includes types, default values, and allowed values for every parameter. "
            "Use this before generating PML code that uses a component.";

        nlohmann::json schema;
        schema["type"] = "object";

        nlohmann::json props;
        props["component"]["type"] = "string";
        props["component"]["description"] = "Component name to inspect";
        schema["properties"] = std::move(props);

        nlohmann::json required = nlohmann::json::array();
        required.push_back("component");
        schema["required"] = std::move(required);

        tool["inputSchema"] = std::move(schema);
        tools.push_back(std::move(tool));
    }

    nlohmann::json result;
    result["tools"] = std::move(tools);
    return result;
}

// ==========================================================================================================================================================================================================================================═
// MCP Handlers: tools/call
// ==========================================================================================================================================================================================================================================═

nlohmann::json MCPServer::handle_tools_call(const nlohmann::json& params) {
    // Extract tool name
    auto it_name = params.find("name");
    if (it_name == params.end() || !it_name->is_string()) {
        return make_error(-32602, "Invalid params: 'name' required");
    }
    std::string name = it_name->get<std::string>();

    // Extract arguments (optional, default to empty object)
    nlohmann::json args = nlohmann::json::object();
    auto it_args = params.find("arguments");
    if (it_args != params.end() && it_args->is_object()) {
        args = *it_args;
    }

    // Dispatch to tool implementation
    nlohmann::json tool_result;
    if (name == "execute_pml") {
        tool_result = tool_execute_pml(args);
    } else if (name == "render_sprite") {
        tool_result = tool_render_sprite(args);
    } else if (name == "validate") {
        tool_result = tool_validate(args);
    } else if (name == "list_components") {
        tool_result = tool_list_components(args);
    } else if (name == "preview_params") {
        tool_result = tool_preview_params(args);
    } else {
        return make_error(-32602, "Unknown tool: " + name);
    }

    // Wrap tool result in MCP content array
    nlohmann::json content_item;
    content_item["type"] = "text";
    content_item["text"] = tool_result.dump(2);

    nlohmann::json content = nlohmann::json::array();
    content.push_back(std::move(content_item));

    nlohmann::json result;
    result["content"] = std::move(content);
    return result;
}

// ==========================================================================================================================================================================================================================================═
// Tool: execute_pml
// ==========================================================================================================================================================================================================================================═

nlohmann::json MCPServer::tool_execute_pml(const nlohmann::json& args) {
    auto it_source = args.find("source");
    if (it_source == args.end() || !it_source->is_string()) {
        return {{"success", false}, {"value", nullptr},
                {"error", {{"type", "ArgumentError"},
                           {"message", "Missing required argument: 'source'"},
                           {"hint", "Provide a 'source' string containing PML code."}}}};
    }

    std::string source = it_source->get<std::string>();
    auto result = m_runtime->execute_pml(source);
    return result;
}

// ==========================================================================================================================================================================================================================================═
// Tool: render_sprite
// ==========================================================================================================================================================================================================================================═

nlohmann::json MCPServer::tool_render_sprite(const nlohmann::json& args) {
    auto it_source = args.find("source");
    if (it_source == args.end() || !it_source->is_string()) {
        return {{"success", false}, {"error", "Missing required argument: 'source'"}};
    }

    std::string source = it_source->get<std::string>();

    // Execute the PML source — the (render ...) builtin produces the file
    // as a side effect.
    nlohmann::json exec_result = m_runtime->execute_pml(source);

    if (!exec_result.value("success", false)) {
        return exec_result;
    }

    // Read output files tracked by the runtime context after execution.
    const auto& files = m_runtime->context().output_files;

    // Return JSON matching Python's SpriteAsset format
    nlohmann::json result;
    result["success"] = true;
    result["file"] = files.empty() ? "" : files.back();
    result["width"] = 0;
    result["height"] = 0;
    result["format"] = "PNG";
    result["meta"] = nullptr;
    return result;
}

// ==========================================================================================================================================================================================================================================═
// Tool: validate
// ==========================================================================================================================================================================================================================================═

nlohmann::json MCPServer::tool_validate(const nlohmann::json& args) {
    auto it_source = args.find("source");
    if (it_source == args.end() || !it_source->is_string()) {
        nlohmann::json err;
        err["type"] = "ArgumentError";
        err["message"] = "Missing required argument: 'source'";
        nlohmann::json errors = nlohmann::json::array();
        errors.push_back(std::move(err));
        return {{"valid", false}, {"errors", errors}, {"warnings", nlohmann::json::array()}};
    }

    std::string source = it_source->get<std::string>();

    // Delegate to PMLRuntime::validate() — consistent pipeline with other modes.
    auto vr = m_runtime->validate(source);

    nlohmann::json errors = nlohmann::json::array();
    for (const auto& e : vr.errors) {
        errors.push_back(e);
    }

    return {
        {"valid", vr.valid},
        {"errors", errors},
        {"warnings", nlohmann::json::array()},
    };
}

// ==========================================================================================================================================================================================================================================═
// Tool: list_components (stub)
// ==========================================================================================================================================================================================================================================═

// ---- Helper: convert a FieldSpec to a JSON object ------------------------------------------------

static nlohmann::json field_spec_to_json(const std::string& name, const FieldSpec& spec) {
    nlohmann::json j;
    j["name"] = name;
    switch (spec.type) {
        case FieldSpec::Enum:    j["type"] = "enum";    break;
        case FieldSpec::Number:  j["type"] = "number";  break;
        case FieldSpec::Boolean: j["type"] = "boolean"; break;
        case FieldSpec::Color:   j["type"] = "color";   break;
        case FieldSpec::Any:     j["type"] = "any";     break;
    }
    if (spec.enum_values.has_value()) {
        j["values"] = *spec.enum_values;
    }
    j["default"] = value_to_string(spec.default_value);
    if (spec.type == FieldSpec::Number) {
        j["min"] = spec.min_value;
        j["max"] = spec.max_value;
    }
    return j;
}

nlohmann::json MCPServer::tool_list_components(const nlohmann::json& args) {
    auto& registry = ComponentRegistry::instance();
    std::string category = args.value("category", "");

    std::vector<std::string> names;
    if (category.empty()) {
        names = registry.list_components();
    } else {
        names = registry.list_by_category(category);
    }

    nlohmann::json result = nlohmann::json::array();
    for (const auto& name : names) {
        nlohmann::json entry;
        entry["name"] = name;
        auto cat = registry.get_category(name);
        entry["category"] = cat.value_or("");

        // Include parameter schema
        auto schema = registry.get_schema(name);
        if (schema.has_value()) {
            nlohmann::json params = nlohmann::json::array();
            for (const auto& [field_name, spec] : schema->fields()) {
                params.push_back(field_spec_to_json(field_name, spec));
            }
            entry["params"] = std::move(params);
        }

        result.push_back(std::move(entry));
    }
    return result;
}

// ==========================================================================================================================================================================================================================================═
// Tool: preview_params (stub)
// ==========================================================================================================================================================================================================================================═

nlohmann::json MCPServer::tool_preview_params(const nlohmann::json& args) {
    std::string component = args.value("component", "");
    if (component.empty()) {
        return {{"error", "Missing required argument: 'component'"}};
    }

    auto& registry = ComponentRegistry::instance();
    if (!registry.has(component)) {
        return {
            {"error", "Unknown component: " + component},
            {"available", registry.list_components()},
        };
    }

    nlohmann::json result;
    result["name"] = component;
    result["category"] = registry.get_category(component).value_or("");

    auto schema = registry.get_schema(component);
    if (schema.has_value()) {
        nlohmann::json params = nlohmann::json::array();
        for (const auto& [field_name, spec] : schema->fields()) {
            params.push_back(field_spec_to_json(field_name, spec));
        }
        result["params"] = std::move(params);
    }

    return result;
}

// ==========================================================================================================================================================================================================================================═
// Main Loop
// ==========================================================================================================================================================================================================================================═

void MCPServer::run() {
    // Main request/response loop
    while (true) {
        std::string body = read_message();
        if (body.empty()) {
            // EOF on stdin — exit
            break;
        }

        nlohmann::json request;
        try {
            request = nlohmann::json::parse(body);
        } catch (const nlohmann::json::parse_error& e) {
            // JSON parse error — send JSON-RPC Parse error response
            nlohmann::json resp = make_error_response(
                nullptr, -32700, "Parse error: " + std::string(e.what()));
            write_message(resp.dump());
            continue;
        }

        // Dispatch
        nlohmann::json response = dispatch(request);

        // Notifications produce no response
        if (response.empty()) {
            continue;
        }

        write_message(response.dump());
    }
}

}  // namespace pml

// ==========================================================================================================================================================================================================================================═
// Entry point
// ==========================================================================================================================================================================================================================================═

int main() {
    pml::MCPServer server;
    server.run();
    return 0;
}
