#include "color.h"

#include "color_helpers.h" // named_color_map

#include <array>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string_view>

namespace pml {

namespace {

[[nodiscard]] int hex_value(char c) noexcept {
    if (c >= '0' && c <= '9') return static_cast<int>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<int>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<int>(c - 'A' + 10);
    return -1;
}

[[nodiscard]] std::optional<uint32_t> parse_hex_color(std::string_view s) {
    if (s.empty() || s[0] != '#') {
        return std::nullopt;
    }
    const std::string_view hex = s.substr(1);

    auto to_rgba = [](int r, int g, int b, int a) -> uint32_t {
        return (static_cast<uint32_t>(a) << 24)
             | (static_cast<uint32_t>(r) << 16)
             | (static_cast<uint32_t>(g) << 8)
             | static_cast<uint32_t>(b);
    };

    if (hex.size() == 3) {
        int r = hex_value(hex[0]);
        int g = hex_value(hex[1]);
        int b = hex_value(hex[2]);
        if (r < 0 || g < 0 || b < 0) return std::nullopt;
        r = (r << 4) | r;
        g = (g << 4) | g;
        b = (b << 4) | b;
        return to_rgba(r, g, b, 0xFF);
    }
    if (hex.size() == 4) {
        int r = hex_value(hex[0]);
        int g = hex_value(hex[1]);
        int b = hex_value(hex[2]);
        int a = hex_value(hex[3]);
        if (r < 0 || g < 0 || b < 0 || a < 0) return std::nullopt;
        r = (r << 4) | r;
        g = (g << 4) | g;
        b = (b << 4) | b;
        a = (a << 4) | a;
        return to_rgba(r, g, b, a);
    }
    if (hex.size() == 6) {
        int r = (hex_value(hex[0]) << 4) | hex_value(hex[1]);
        int g = (hex_value(hex[2]) << 4) | hex_value(hex[3]);
        int b = (hex_value(hex[4]) << 4) | hex_value(hex[5]);
        if (r < 0 || g < 0 || b < 0) return std::nullopt;
        return to_rgba(r, g, b, 0xFF);
    }
    if (hex.size() == 8) {
        int r = (hex_value(hex[0]) << 4) | hex_value(hex[1]);
        int g = (hex_value(hex[2]) << 4) | hex_value(hex[3]);
        int b = (hex_value(hex[4]) << 4) | hex_value(hex[5]);
        int a = (hex_value(hex[6]) << 4) | hex_value(hex[7]);
        if (r < 0 || g < 0 || b < 0 || a < 0) return std::nullopt;
        return to_rgba(r, g, b, a);
    }
    return std::nullopt;
}

} // anonymous namespace

std::optional<std::array<double, 4>> parse_color_rgba(const std::string& color) {
    // Trim and lowercase for named-color lookup.
    std::string trimmed;
    trimmed.reserve(color.size());
    bool started = false;
    size_t end = color.size();
    while (end > 0 && std::isspace(static_cast<unsigned char>(color[end - 1]))) {
        --end;
    }
    for (size_t i = 0; i < end; ++i) {
        char c = color[i];
        if (!started && std::isspace(static_cast<unsigned char>(c))) {
            continue;
        }
        started = true;
        trimmed.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    if (trimmed.empty()) {
        return std::nullopt;
    }

    // Named color lookup.
    const auto& named = named_color_map();
    if (auto it = named.find(trimmed); it != named.end()) {
        const uint32_t argb = it->second;
        return std::array<double, 4>{
            static_cast<double>((argb >> 16) & 0xFF) / 255.0,
            static_cast<double>((argb >> 8)  & 0xFF) / 255.0,
            static_cast<double>((argb >> 0)  & 0xFF) / 255.0,
            static_cast<double>((argb >> 24) & 0xFF) / 255.0,
        };
    }

    // Hex parsing.
    if (auto argb = parse_hex_color(trimmed)) {
        return std::array<double, 4>{
            static_cast<double>((*argb >> 16) & 0xFF) / 255.0,
            static_cast<double>((*argb >> 8)  & 0xFF) / 255.0,
            static_cast<double>((*argb >> 0)  & 0xFF) / 255.0,
            static_cast<double>((*argb >> 24) & 0xFF) / 255.0,
        };
    }

    return std::nullopt;
}

} // namespace pml
