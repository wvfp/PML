#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace pml {

/// Parse a CSS-style color string to normalized RGBA (0.0–1.0 range).
///
/// Supported formats:
///   - Named colors: "red", "blue", "transparent", "none", etc.
///   - Hex: "#RGB", "#RRGGBB", "#RRGGBBAA"
///   - Functional: "rgb(r,g,b)", "rgba(r,g,b,a)"
///
/// Returns nullopt on parse failure.
[[nodiscard]] std::optional<std::array<double, 4>> parse_color_rgba(const std::string& color);

} // namespace pml
