#pragma once

#include <cstdint>

namespace pml {

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
