#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Path Command Types
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Defines the enumeration and data structures for representing SVG-style path
// commands (moveTo, lineTo, cubic/quad bezier, arc, close).  Used both by the
// `path` builtin and the Skia render backend.
// ==========================================================================================================================================================================================================================================═

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pml {

/// The set of supported path command types, covering all SVG 2.0 path commands.
/// Each command has a fixed number of numeric arguments (see kArgCounts).
enum class PathCmdType : uint8_t {
    MoveTo,          // x y
    MoveToR,         // dx dy
    LineTo,          // x y
    LineToR,         // dx dy
    HLineTo,         // x
    HLineToR,        // dx
    VLineTo,         // y
    VLineToR,        // dy
    CubicTo,         // cx1 cy1 cx2 cy2 x y
    CubicToR,        // d cx1 d cy1 d cx2 d cy2 d x d y
    SmoothCubicTo,   // cx2 cy2 x y
    SmoothCubicToR,  // d cx2 d cy2 d x d y
    QuadTo,          // cx cy x y
    QuadToR,         // d cx d cy d x d y
    SmoothQuadTo,    // x y
    SmoothQuadToR,   // d x d y
    Arc,             // rx ry x-axis-rotation large-arc-flag sweep-flag x y
    ArcR,            // rx ry x-axis-rotation large-arc-flag sweep-flag dx dy
    Close            // no args
};

/// Number of numeric arguments for each command type.
constexpr uint8_t kArgCounts[] = {
    2,  // MoveTo
    2,  // MoveToR
    2,  // LineTo
    2,  // LineToR
    1,  // HLineTo
    1,  // HLineToR
    1,  // VLineTo
    1,  // VLineToR
    6,  // CubicTo
    6,  // CubicToR
    4,  // SmoothCubicTo
    4,  // SmoothCubicToR
    4,  // QuadTo
    4,  // QuadToR
    2,  // SmoothQuadTo
    2,  // SmoothQuadToR
    7,  // Arc
    7,  // ArcR
    0   // Close
};

/// A single path command with its parsed arguments.
struct PathCommand {
    PathCmdType type;
    std::vector<double> args;

    PathCommand() = default;
    PathCommand(PathCmdType t, std::vector<double> a)
        : type(t), args(std::move(a)) {}
};

/// Convert an SVG-style command letter to a PathCmdType.
/// Returns std::nullopt for unrecognised letters.
[[nodiscard]] std::optional<PathCmdType> svg_letter_to_cmd(char c) noexcept;

/// Convert a PML command symbol (string name) to a PathCmdType.
/// Returns std::nullopt for unrecognised names.
[[nodiscard]] std::optional<PathCmdType> string_to_cmd(std::string_view name) noexcept;

/// Get the canonical SVG command letter for a type (uppercase absolute).
[[nodiscard]] char cmd_to_svg_letter(PathCmdType type) noexcept;

/// Parse an SVG path `d` attribute string into a sequence of PathCommands.
/// Returns an error string on failure, or the parsed commands on success.
[[nodiscard]] std::expected<std::vector<PathCommand>, std::string>
parse_svg_path_string(std::string_view d) noexcept;

/// Check whether a command is relative (ends with 'R').
[[nodiscard]] inline bool is_relative(PathCmdType type) noexcept {
    auto u = static_cast<uint8_t>(type);
    return (u % 2) == 1;  // odd-numbered types are relative
}

} // namespace pml
