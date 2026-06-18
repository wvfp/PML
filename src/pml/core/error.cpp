#include "error.h"

#include <format>
#include <utility>

namespace pml {

// ── SourceLocation ─────────────────────────────────────────────────────────

auto SourceLocation::to_string() const -> std::string {
    if (filename.empty()) {
        return std::format("line {}:{}", line, column);
    }
    return std::format("{}:{}:{}", filename, line, column);
}

// ── ErrorCode string conversion ────────────────────────────────────────────

auto to_string(ErrorCode code) -> std::string_view {
    using enum ErrorCode;
    switch (code) {
        case PMLSyntaxError:          return "PMLSyntaxError";
        case PMLTypeError:            return "PMLTypeError";
        case UnboundVariableError:    return "UnboundVariableError";
        case ArityError:              return "ArityError";
        case ImportError:             return "ImportError";
        case CircularImportError:     return "CircularImportError";
        case MacroExpansionDepthError: return "MacroExpansionDepthError";
        case AccessError:             return "AccessError";
        case ResourceError:           return "ResourceError";
        case IKNoSolutionError:       return "IKNoSolutionError";
        case PMLAssertionError:       return "PMLAssertionError";
        case GeneralError:            return "GeneralError";
    }
    return "UnknownError";
}

// ── PMLException::what() ───────────────────────────────────────────────────

auto PMLException::what() const -> std::string {
    std::string result;
    if (location.has_value()) {
        result = location->to_string();
        result += ": ";
    }
    result += ::pml::to_string(code);
    result += ": ";
    result += message;
    return result;
}

// ── Factory functions (with SourceLocation) ────────────────────────────────

auto syntax_error(SourceLocation loc, std::string msg,
                  std::optional<std::string> hint) -> PMLException {
    return PMLException{ErrorCode::PMLSyntaxError, std::move(loc),
                        std::move(msg), std::move(hint)};
}

auto type_error(SourceLocation loc, std::string msg,
                std::optional<std::string> hint) -> PMLException {
    return PMLException{ErrorCode::PMLTypeError, std::move(loc),
                        std::move(msg), std::move(hint)};
}

auto unbound_error(SourceLocation loc, std::string_view name) -> PMLException {
    return PMLException{ErrorCode::UnboundVariableError, std::move(loc),
                        std::format("unbound variable: {}", name), std::nullopt};
}

auto arity_error(SourceLocation loc, int expected, int got) -> PMLException {
    return PMLException{ErrorCode::ArityError, std::move(loc),
                        std::format("expected {} arguments, got {}", expected, got),
                        std::nullopt};
}

auto import_error(SourceLocation loc, std::string msg,
                  std::optional<std::string> hint) -> PMLException {
    return PMLException{ErrorCode::ImportError, std::move(loc),
                        std::move(msg), std::move(hint)};
}

auto circular_import_error(std::string_view name) -> PMLException {
    return PMLException{ErrorCode::CircularImportError, std::nullopt,
                        std::format("circular import detected: {}", name),
                        std::nullopt};
}

auto macro_expansion_depth_error(SourceLocation loc, int depth) -> PMLException {
    return PMLException{ErrorCode::MacroExpansionDepthError, std::move(loc),
                        std::format("macro expansion exceeded maximum depth of {}",
                                    depth),
                        std::nullopt};
}

auto access_error(std::string_view name) -> PMLException {
    return PMLException{ErrorCode::AccessError, std::nullopt,
                        std::format("cannot access non-exported symbol: {}", name),
                        std::nullopt};
}

auto resource_error(std::string msg,
                    std::optional<std::string> hint) -> PMLException {
    return PMLException{ErrorCode::ResourceError, std::nullopt,
                        std::move(msg), std::move(hint)};
}

auto ik_no_solution_error(std::string_view name) -> PMLException {
    return PMLException{ErrorCode::IKNoSolutionError, std::nullopt,
                        std::format("IK solver failed to converge for {}", name),
                        std::nullopt};
}

auto assertion_error(SourceLocation loc, std::string msg) -> PMLException {
    return PMLException{ErrorCode::PMLAssertionError, std::move(loc),
                        std::move(msg), std::nullopt};
}

auto general_error(std::string msg,
                   std::optional<std::string> hint) -> PMLException {
    return PMLException{ErrorCode::GeneralError, std::nullopt,
                        std::move(msg), std::move(hint)};
}

// ── Factory functions (without SourceLocation) ─────────────────────────────

auto syntax_error(std::string msg,
                  std::optional<std::string> hint) -> PMLException {
    return PMLException{ErrorCode::PMLSyntaxError, std::nullopt,
                        std::move(msg), std::move(hint)};
}

auto type_error(std::string msg,
                std::optional<std::string> hint) -> PMLException {
    return PMLException{ErrorCode::PMLTypeError, std::nullopt,
                        std::move(msg), std::move(hint)};
}

auto import_error(std::string msg,
                  std::optional<std::string> hint) -> PMLException {
    return PMLException{ErrorCode::ImportError, std::nullopt,
                        std::move(msg), std::move(hint)};
}

auto assertion_error(std::string msg) -> PMLException {
    return PMLException{ErrorCode::PMLAssertionError, std::nullopt,
                        std::move(msg), std::nullopt};
}

}  // namespace pml
