#pragma once

#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pml {

/// 1-based source location for error reporting.
struct SourceLocation {
    int line{0};             ///< 1-based line number
    int column{0};           ///< 1-based column number
    std::string filename;    ///< Empty for REPL input

    /// Format as "filename:line:col" or "line N:M" when filename is empty.
    [[nodiscard]] auto to_string() const -> std::string;
};

/// A single frame in the runtime call stack.
struct CallFrame {
    std::string function_name;   ///< Name of the called function (may be empty).
    SourceLocation call_site;    ///< Source location of the call expression.
};

/// Error codes matching Python PML error hierarchy.
enum class ErrorCode {
    PMLSyntaxError,
    PMLTypeError,
    UnboundVariableError,
    ArityError,
    ImportError,
    CircularImportError,
    MacroExpansionDepthError,
    AccessError,
    ResourceError,
    IKNoSolutionError,
    PMLAssertionError,
    LayerError,
    CompositionError,
    BlendModeNotSupported,
    FilterError,
    FilterNotSupported,
    GeneralError,
};

/// Convert ErrorCode to human-readable name (e.g. "PMLSyntaxError").
[[nodiscard]] auto to_string(ErrorCode code) -> std::string_view;

/// A PML error with code, optional location, message, optional repair hint,
/// and optional runtime call stack.
struct PMLException {
    ErrorCode code;
    std::optional<SourceLocation> location;
    std::string message;
    std::optional<std::string> repair_hint;
    std::vector<CallFrame> call_stack;

    /// Format: "filename:line:col: PMLErrorName: message" or "PMLErrorName: message".
    [[nodiscard]] auto what() const -> std::string;
};

/// Result alias using std::expected (C++23) — primary control flow, no exceptions.
template<typename T>
using Result = std::expected<T, PMLException>;

// ---- Factory functions (with SourceLocation) ----------------------------------------------------------------

[[nodiscard]] auto syntax_error(SourceLocation loc, std::string msg,
                                std::optional<std::string> hint = std::nullopt) -> PMLException;

[[nodiscard]] auto type_error(SourceLocation loc, std::string msg,
                              std::optional<std::string> hint = std::nullopt) -> PMLException;

[[nodiscard]] auto unbound_error(SourceLocation loc, std::string_view name) -> PMLException;

[[nodiscard]] auto arity_error(SourceLocation loc, int expected, int got) -> PMLException;

[[nodiscard]] auto import_error(SourceLocation loc, std::string msg,
                                std::optional<std::string> hint = std::nullopt) -> PMLException;

[[nodiscard]] auto circular_import_error(std::string_view name) -> PMLException;

[[nodiscard]] auto macro_expansion_depth_error(SourceLocation loc, int depth) -> PMLException;

[[nodiscard]] auto access_error(std::string_view name) -> PMLException;

[[nodiscard]] auto resource_error(std::string msg,
                                  std::optional<std::string> hint = std::nullopt) -> PMLException;

[[nodiscard]] auto ik_no_solution_error(std::string_view name) -> PMLException;

[[nodiscard]] auto assertion_error(SourceLocation loc, std::string msg) -> PMLException;

[[nodiscard]] auto layer_error(SourceLocation loc, std::string msg,
                               std::optional<std::string> hint = std::nullopt) -> PMLException;

[[nodiscard]] auto composition_error(SourceLocation loc, std::string msg,
                                     std::optional<std::string> hint = std::nullopt) -> PMLException;

[[nodiscard]] auto blend_mode_error(SourceLocation loc, std::string msg,
                                    std::optional<std::string> hint = std::nullopt) -> PMLException;

[[nodiscard]] auto filter_error(SourceLocation loc, std::string msg,
                                std::optional<std::string> hint = std::nullopt) -> PMLException;

[[nodiscard]] auto filter_not_supported(SourceLocation loc, std::string msg,
                                        std::optional<std::string> hint = std::nullopt) -> PMLException;

[[nodiscard]] auto general_error(std::string msg,
                                 std::optional<std::string> hint = std::nullopt) -> PMLException;

// ---- Factory functions (without SourceLocation) --------------------------------------------------------─

[[nodiscard]] auto syntax_error(std::string msg,
                                std::optional<std::string> hint = std::nullopt) -> PMLException;

[[nodiscard]] auto type_error(std::string msg,
                              std::optional<std::string> hint = std::nullopt) -> PMLException;

[[nodiscard]] auto import_error(std::string msg,
                                std::optional<std::string> hint = std::nullopt) -> PMLException;

[[nodiscard]] auto assertion_error(std::string msg) -> PMLException;

}  // namespace pml
