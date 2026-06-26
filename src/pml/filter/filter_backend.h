#pragma once

// ==========================================================================================================================================================================================================================================═
// FilterBackend — accelerated primitives for image filters
// ==========================================================================================================================================================================================================================================═
//
// RenderBackend inherits from this interface.  Each concrete backend implements
// the methods using native APIs (SkImageFilters / SkColorFilters for Skia).
//
// ==========================================================================================================================================================================================================================================═

#include "pml/core/error.h"

#include <array>
#include <cstdint>
#include <vector>

namespace pml {

// Forward declarations
struct Surface; // pml/backend/backend.h

// ---- Types ----------------------------------------------------------------------------------------------------------------------------------------─

enum class BlurType : uint8_t { Gaussian, Box, Motion };

struct ConvolutionKernel {
    int width{};
    int height{};
    std::vector<float> values; // row-major, size = width * height
    float offset{0.0f};
    float divisor{1.0f};
    int anchor_x{-1}; // -1 == center
    int anchor_y{-1};
};

// ---- FilterBackend ------------------------------------------------------------------------------------------------------------------------─

class FilterBackend {
public:
    virtual ~FilterBackend() = default;

    // 5x4 color matrix (row-major, RGBA offsets + alpha scale/offset).
    [[nodiscard]] virtual Result<void> apply_color_matrix(
        Surface&, const std::array<float,20>& matrix) = 0;

    // Separable Gaussian / box / motion blur.
    [[nodiscard]] virtual Result<void> apply_blur(
        Surface&, float radius_x, float radius_y, BlurType type) = 0;

    // Generic matrix convolution.
    [[nodiscard]] virtual Result<void> apply_convolution(
        Surface&, const ConvolutionKernel& kernel) = 0;

    // Per-channel 1D lookup tables (RGBA).
    [[nodiscard]] virtual Result<void> apply_color_table(
        Surface&,
        const std::array<uint8_t,256>& r,
        const std::array<uint8_t,256>& g,
        const std::array<uint8_t,256>& b,
        const std::array<uint8_t,256>& a) = 0;

    // Offset / displacement.
    [[nodiscard]] virtual Result<void> apply_offset(
        Surface&, float dx, float dy) = 0;

    // Layer-style primitives.
    [[nodiscard]] virtual Result<void> apply_drop_shadow(
        Surface&, float dx, float dy,
        float blur_x, float blur_y, uint32_t color) = 0;

    [[nodiscard]] virtual Result<void> apply_inner_shadow(
        Surface&, float dx, float dy,
        float blur, uint32_t color) = 0;

    [[nodiscard]] virtual Result<void> apply_outer_glow(
        Surface&, float blur, uint32_t color) = 0;

    [[nodiscard]] virtual Result<void> apply_inner_glow(
        Surface&, float blur, uint32_t color) = 0;

    [[nodiscard]] virtual Result<void> apply_bevel_emboss(
        Surface&, float angle, float altitude,
        float blur, uint32_t highlight, uint32_t shadow) = 0;
};

} // namespace pml
