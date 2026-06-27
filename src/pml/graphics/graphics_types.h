#pragma once

#include <cstdint>
#include <string>

namespace pml {

struct Vec2 {
    double x = 0.0;
    double y = 0.0;
};

// Forward declaration — full definition in objects.h.
enum class ShapeType : uint8_t;

/// Convert ShapeType to a string for serialization/debug output.
[[nodiscard]] std::string to_string(ShapeType type);

struct Size2D {
    int width = 0;
    int height = 0;
};

struct Color {
    uint32_t rgba = 0x00000000;

    static constexpr Color Transparent() noexcept { return Color{0x00000000}; }
    static constexpr Color Black() noexcept { return Color{0xFF000000}; }
    static constexpr Color White() noexcept { return Color{0xFFFFFFFF}; }
};

} // namespace pml
