#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Color Helpers
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Pure utility functions for color parsing and application.
// No Skia/Cairo dependency — works with raw uint32 (ARGB) values.
// ==========================================================================================================================================================================================================================================═

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pml {

// ---- Parse --------------------------------------------------------------------------------------------------------------------------------------------

/// Parse a color string to ARGB uint32.
///
/// Supported formats:
///   - Named colors: "red", "blue", "transparent", etc.
///   - Hex: "#RGB" (duplicates each digit), "#RRGGBB", "#RRGGBBAA"
///
/// Returns the color as AARRGGBB (alpha in high 8 bits), or std::nullopt
/// if the string cannot be parsed.
[[nodiscard]] std::optional<uint32_t> parse_color(const std::string& color);

/// Parse a color from a string_view (convenience overload).
[[nodiscard]] inline std::optional<uint32_t> parse_color(std::string_view color)
{
    return parse_color(std::string(color));
}

// ---- Apply (raw ARGB) --------------------------------------------------------------------------------------------------------------------─

/// Write a parsed ARGB color into a memory region at the given byte offsets.
///
/// @param user_data  Pointer to the start of the target struct/pixel.
/// @param argb       Color as 0xAARRGGBB uint32.
/// @param r_offset   Byte offset from user_data for the red channel.
/// @param g_offset   Byte offset from user_data for the green channel.
/// @param b_offset   Byte offset from user_data for the blue channel.
/// @param a_offset   Byte offset from user_data for the alpha channel.
void apply_color(void* user_data, uint32_t argb,
                 int r_offset, int g_offset, int b_offset, int a_offset);

// ---- Apply (string → parse → apply) ----------------------------------------------------------------------------------------

/// Parse a color string and apply it into a memory region.
///
/// Convenience wrapper: calls parse_color() then apply_color().
///
/// @return true if parsing succeeded and the color was applied, false otherwise.
[[nodiscard]] bool apply_color(void* user_data, const std::string& color,
                               int r_offset, int g_offset,
                               int b_offset, int a_offset);

// ---- Named colors table (for inspection / iteration) ----------------------------------------------------─

/// Returns a reference to the internal named-colors map.
/// Keys are lowercase color names; values are ARGB uint32.
const std::unordered_map<std::string, uint32_t>& named_color_map();

}  // namespace pml
