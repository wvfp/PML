#pragma once

// ==========================================================================================================================================================================================================================================═
// Convolution filters — blur, sharpen, edge detection, emboss, custom kernel
// ==========================================================================================================================================================================================================================================═

#include "image_filter.h"
#include "filter_backend.h"

#include <cstdint>
#include <vector>

namespace pml {

enum class BlurFilterType : uint8_t { Gaussian, Box, Motion };
enum class EdgeDetectType : uint8_t { Sobel, Laplacian };

class BlurFilter final : public ImageFilter {
public:
    BlurFilter(float radius, BlurFilterType type, float angle);
    [[nodiscard]] Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;
private:
    float m_radius;
    float m_angle;
    BlurFilterType m_type;
};

class SharpenFilter final : public ImageFilter {
public:
    SharpenFilter(float amount, float radius);
    [[nodiscard]] Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;
private:
    float m_amount;
    float m_radius;
};

class EdgeDetectFilter final : public ImageFilter {
public:
    explicit EdgeDetectFilter(EdgeDetectType type);
    [[nodiscard]] Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;
private:
    EdgeDetectType m_type;
};

class EmbossFilter final : public ImageFilter {
public:
    explicit EmbossFilter(float direction);
    [[nodiscard]] Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;
private:
    float m_direction;
};

class ConvolutionFilter final : public ImageFilter {
public:
    ConvolutionFilter(int w, int h, std::vector<float> values,
                      float offset, float divisor);
    [[nodiscard]] Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;
private:
    ConvolutionKernel m_kernel;
};

} // namespace pml
