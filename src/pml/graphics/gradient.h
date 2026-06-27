#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Gradient — gradient fill descriptor
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Data-only struct describing either a linear, radial, or sweep gradient, used as the
// fill style for GraphicObject shapes.  Constructed by the `linear`, `radial`, and `sweep`
// builtins and consumed by the Skia render backend.
// ==========================================================================================================================================================================================================================================═

#include <cstdint>
#include <string>
#include <vector>

namespace pml {

/// Kind of gradient.
enum class GradientType : uint8_t {
    Linear,
    Radial,
    Sweep,
};

/// A single colour stop along the gradient.
struct GradientStop {
    double position{0.0};        ///< 0.0–1.0 position along the gradient
    std::string color;           ///< "#rrggbb" hex colour string
};

/// Describes a linear or radial gradient fill.
struct Gradient {
    GradientType type{GradientType::Linear};

    // Linear endpoints (in shape-local coordinates, normalised 0–1)
    double x1{0.0}, y1{0.0};
    double x2{0.0}, y2{1.0};

    // Radial centre and radius (shape-local, normalised 0–1)
    double cx{0.5}, cy{0.5}, r{0.5};

    // Sweep start/end angles in degrees (shape-local, normalised 0–1 maps to 0°–360°)
    double start_angle{0.0};
    double end_angle{360.0};

    /// Colour stops along the gradient.
    std::vector<GradientStop> stops;
};

} // namespace pml
