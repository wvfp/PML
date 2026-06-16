#include "color_helpers.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <string_view>
#include <utility>

namespace pml {

// ======================================================================
// Internal helpers
// ======================================================================

/// Convert a single hex character to its numeric value (0–15).
/// Returns -1 for invalid characters.
[[nodiscard]] static int hex_value(char c) noexcept
{
    if (c >= '0' && c <= '9') return static_cast<int>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<int>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<int>(c - 'A' + 10);
    return -1;
}

/// Parse exactly @p count hex characters from @p s into an integer.
/// Returns -1 on any invalid character.
[[nodiscard]] static int parse_hex_chars(const char* s, int count) noexcept
{
    int result = 0;
    for (int i = 0; i < count; ++i) {
        const int v = hex_value(s[i]);
        if (v < 0) return -1;
        result = (result << 4) | v;
    }
    return result;
}

// ======================================================================
// Named colors table
// ======================================================================

/// Build the static named-colors map (ARGB uint32: A in high 8 bits).
static const auto& build_named_colors() noexcept
{
    static const std::unordered_map<std::string, uint32_t> map{
        // Fully transparent
        {"transparent", 0x00000000u},
        {"none",        0x00000000u},

        // Grayscale
        {"black",  0xFF000000u},
        {"white",  0xFFFFFFFFu},
        {"gray",   0xFF808080u},
        {"grey",   0xFF808080u},

        // Primary
        {"red",    0xFFFF0000u},
        {"green",  0xFF008000u},
        {"blue",   0xFF0000FFu},

        // Secondary
        {"yellow", 0xFFFFFF00u},
        {"cyan",   0xFF00FFFFu},
        {"magenta", 0xFFFF00FFu},

        // Extended
        {"orange", 0xFFFFA500u},
        {"purple", 0xFF800080u},
        {"pink",   0xFFFFC0CBu},
        {"brown",  0xFFA52A2Au},
        {"navy",   0xFF000080u},
        {"coral",  0xFFFF7F50u},
    };
    return map;
}

const std::unordered_map<std::string, uint32_t>& named_color_map()
{
    return build_named_colors();
}

// ======================================================================
// parse_color
// ======================================================================

std::optional<uint32_t> parse_color(const std::string& color)
{
    if (color.empty()) {
        return std::nullopt;
    }

    // Trim and lowercase
    auto begin = color.data();
    auto end = color.data() + color.size();

    // Trim leading whitespace
    while (begin < end && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }
    // Trim trailing whitespace
    while (end > begin && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }

    const std::ptrdiff_t len = end - begin;
    if (len <= 0) {
        return std::nullopt;
    }

    // Build a lowercase copy of the trimmed string for lookup
    std::string trimmed_lower(static_cast<size_t>(len), '\0');
    for (std::ptrdiff_t i = 0; i < len; ++i) {
        trimmed_lower[static_cast<size_t>(i)] =
            static_cast<char>(std::tolower(static_cast<unsigned char>(begin[i])));
    }

    // 1. Named color lookup
    {
        const auto& map = build_named_colors();
        auto it = map.find(trimmed_lower);
        if (it != map.end()) {
            return it->second;
        }
    }

    // 2. Hex parsing
    if (trimmed_lower.size() < 2 || trimmed_lower[0] != '#') {
        return std::nullopt;
    }

    const char* hex_start = trimmed_lower.data() + 1;
    const std::ptrdiff_t hex_len = len - 1;

    if (hex_len == 3) {
        // #RGB → duplicate each digit → RRGGBB (alpha = FF)
        int dr = hex_value(hex_start[0]);
        int dg = hex_value(hex_start[1]);
        int db = hex_value(hex_start[2]);
        if (dr < 0 || dg < 0 || db < 0) {
            return std::nullopt;
        }
        uint32_t r = static_cast<uint32_t>((dr << 4) | dr);
        uint32_t g = static_cast<uint32_t>((dg << 4) | dg);
        uint32_t b = static_cast<uint32_t>((db << 4) | db);
        return 0xFF000000u | (r << 16) | (g << 8) | b;
    }

    if (hex_len == 6) {
        // #RRGGBB (alpha = FF)
        int r = parse_hex_chars(hex_start, 2);
        int g = parse_hex_chars(hex_start + 2, 2);
        int b = parse_hex_chars(hex_start + 4, 2);
        if (r < 0 || g < 0 || b < 0) {
            return std::nullopt;
        }
        return 0xFF000000u
             | (static_cast<uint32_t>(r) << 16)
             | (static_cast<uint32_t>(g) << 8)
             | static_cast<uint32_t>(b);
    }

    if (hex_len == 8) {
        // #RRGGBBAA
        int r = parse_hex_chars(hex_start, 2);
        int g = parse_hex_chars(hex_start + 2, 2);
        int b = parse_hex_chars(hex_start + 4, 2);
        int a = parse_hex_chars(hex_start + 6, 2);
        if (r < 0 || g < 0 || b < 0 || a < 0) {
            return std::nullopt;
        }
        return (static_cast<uint32_t>(a) << 24)
             | (static_cast<uint32_t>(r) << 16)
             | (static_cast<uint32_t>(g) << 8)
             | static_cast<uint32_t>(b);
    }

    return std::nullopt;
}

// ======================================================================
// apply_color (raw ARGB)
// ======================================================================

void apply_color(void* user_data, uint32_t argb,
                 int r_offset, int g_offset,
                 int b_offset, int a_offset)
{
    auto* bytes = static_cast<uint8_t*>(user_data);
    // ARGB: A in bits 24-31, R in bits 16-23, G in bits 8-15, B in bits 0-7
    bytes[r_offset] = static_cast<uint8_t>((argb >> 16) & 0xFF);  // R
    bytes[g_offset] = static_cast<uint8_t>((argb >> 8)  & 0xFF);  // G
    bytes[b_offset] = static_cast<uint8_t>((argb >> 0)  & 0xFF);  // B
    bytes[a_offset] = static_cast<uint8_t>((argb >> 24) & 0xFF);  // A
}

// ======================================================================
// apply_color (string → parse → apply)
// ======================================================================

bool apply_color(void* user_data, const std::string& color,
                 int r_offset, int g_offset,
                 int b_offset, int a_offset)
{
    auto parsed = parse_color(color);
    if (!parsed.has_value()) {
        return false;
    }
    apply_color(user_data, *parsed, r_offset, g_offset, b_offset, a_offset);
    return true;
}

}  // namespace pml
