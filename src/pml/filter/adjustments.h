#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// Adjustment filters — color matrix, levels, curves, threshold, posterize
// ═══════════════════════════════════════════════════════════════════════════════

#include "image_filter.h"
#include "filter_backend.h"

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace pml {

struct ColorAdjustParams {
    double brightness = 0.0;
    double contrast   = 1.0;
    double saturation = 1.0;
    double hue        = 0.0;
    double exposure   = 0.0;
    double gamma      = 1.0;
    double temperature= 0.0;
    double tint       = 0.0;
    bool grayscale    = false;
    bool sepia        = false;
    bool invert       = false;
};

class ColorMatrixFilter final : public ImageFilter {
public:
    explicit ColorMatrixFilter(ColorAdjustParams p);
    [[nodiscard]] Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;
private:
    ColorAdjustParams m_params;
};

class LevelsFilter final : public ImageFilter {
public:
    LevelsFilter(double in_low, double gamma, double in_high,
                 double out_low, double out_high);
    [[nodiscard]] Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;
private:
    double m_in_low, m_gamma, m_in_high, m_out_low, m_out_high;
};

class CurvesFilter final : public ImageFilter {
public:
    // channel: 0=R, 1=G, 2=B, 3=RGB
    CurvesFilter(int channel, std::vector<std::pair<uint8_t,uint8_t>> points);
    [[nodiscard]] Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;
private:
    int m_channel;
    std::vector<std::pair<uint8_t,uint8_t>> m_points;
};

class ThresholdFilter final : public ImageFilter {
public:
    explicit ThresholdFilter(uint8_t value);
    [[nodiscard]] Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;
private:
    uint8_t m_value;
};

class PosterizeFilter final : public ImageFilter {
public:
    explicit PosterizeFilter(int levels);
    [[nodiscard]] Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;
private:
    int m_levels;
};

} // namespace pml
