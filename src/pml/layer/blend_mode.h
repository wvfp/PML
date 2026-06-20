#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pml {

enum class BlendMode : uint8_t {
    Normal, Multiply, Screen, Overlay, SoftLight, HardLight,
    Darken, Lighten, ColorDodge, ColorBurn, Difference, Exclusion,
    Hue, Saturation, Color, Luminosity, Plus, DstIn, SrcAtop
};

[[nodiscard]] std::optional<BlendMode> blend_mode_from_keyword(std::string_view name);
[[nodiscard]] const char* blend_mode_to_keyword(BlendMode mode) noexcept;

} // namespace pml
