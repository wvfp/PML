#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML API — PMLRuntime facade for LLM agents.
//
// Ported from pml/api.py.  Wraps the full interpreter pipeline
// (lex → parse → expand → evaluate) and provides structured error
// feedback with repair hints.
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/core/error.h"
#include "pml/core/types.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// RenderResult — structured result of executing PML source code.
// ═══════════════════════════════════════════════════════════════════════════════

/// Result of executing PML source code, matching Python's RenderResult dataclass.
struct RenderResult {
    bool success{false};
    nlohmann::json value;            ///< Serialized return value (JSON).
    std::vector<std::string> files;  ///< Output files produced (e.g. rendered images).
    std::optional<nlohmann::json> error;  ///< Error dict if success is false.

    /// Serialise to a JSON object matching Python's RenderResult.to_dict().
    [[nodiscard]] nlohmann::json to_json() const;
};

// ═══════════════════════════════════════════════════════════════════════════════
// PMLRuntime — high-level API wrapping the full interpreter pipeline.
// ═══════════════════════════════════════════════════════════════════════════════

/// High-level API for LLM agents to interact with PML.
///
/// Wraps the full interpreter pipeline (lex → parse → expand → evaluate)
/// and provides structured execution with persistent environment across calls.
///
/// Usage::
///   PMLRuntime rt;
///   auto result = rt.execute("(+ 1 2)");
///   assert(result.success && result.value == 3);
class PMLRuntime {
public:
    /// Construct the runtime, creating a global environment and registering
    /// all builtins, graphics, sprites, animation, and skeleton procedures.
    PMLRuntime();

    /// Execute a PML source string.
    ///
    /// The environment persists across calls — definitions from a previous
    /// execute() are visible in the next.
    ///
    /// @param source   PML source code.
    /// @param filename Optional filename for error reporting (default "<stdin>").
    /// @return A RenderResult with the serialised last value (or error).
    [[nodiscard]] RenderResult execute(
        const std::string& source,
        const std::string& filename = "<stdin>");

    /// Execute a PML file.
    ///
    /// Convenience wrapper: reads the file and calls execute() with the
    /// filename set to the file path.
    ///
    /// @param path  Path to the .pml file.
    /// @return A RenderResult.
    [[nodiscard]] RenderResult execute_file(const std::string& path);

    /// Execute PML source and return a JSON structure.
    ///
    /// This is the LLM-friendly entry point — the result is a JSON object
    /// matching the Python API's RenderResult.to_dict() exactly.
    ///
    /// @param source  PML source code.
    /// @param options Optional map (reserved for future use).
    /// @return JSON object with "success", "value", "files", "error" keys.
    [[nodiscard]] nlohmann::json execute_pml(
        const std::string& source,
        const nlohmann::json& options = nlohmann::json::object());

    /// Access the underlying global environment (for advanced use/testing).
    [[nodiscard]] std::shared_ptr<Environment> env() const noexcept;

private:
    std::shared_ptr<Environment> m_env;

    /// Create and populate the global environment with all builtins.
    void init_global_env();
};

// ═══════════════════════════════════════════════════════════════════════════════
// Value serialisation helpers
// ═══════════════════════════════════════════════════════════════════════════════

/// Convert a PML runtime Value to a JSON-serialisable type.
/// Matches Python's _serialize_value() exactly.
[[nodiscard]] nlohmann::json serialize_value(const Value& val);

/// Convert a PMLException to a structured error dict with repair hints.
/// Matches Python's _error_to_dict() exactly.
[[nodiscard]] nlohmann::json error_to_dict(const PMLException& e);

/// Generate an LLM-friendly repair hint based on error code.
[[nodiscard]] std::string generate_hint(ErrorCode code);

}  // namespace pml
