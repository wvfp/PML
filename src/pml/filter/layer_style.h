#pragma once

// ==========================================================================================================================================================================================================================================═
// Layer-style filters — drop shadow / inner shadow / glow / bevel emboss
// ==========================================================================================================================================================================================================================================═

#include "image_filter.h"
#include "filter_backend.h"

#include <cstdint>

namespace pml {

class DropShadowFilter final : public ImageFilter {
public:
    DropShadowFilter(float dx, float dy, float blur_x, float blur_y, uint32_t color);
    [[nodiscard]] Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;
private:
    float m_dx, m_dy, m_blur_x, m_blur_y;
    uint32_t m_color;
};

class InnerShadowFilter final : public ImageFilter {
public:
    InnerShadowFilter(float dx, float dy, float blur, uint32_t color);
    [[nodiscard]] Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;
private:
    float m_dx, m_dy, m_blur;
    uint32_t m_color;
};

class OuterGlowFilter final : public ImageFilter {
public:
    OuterGlowFilter(float blur, uint32_t color);
    [[nodiscard]] Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;
private:
    float m_blur;
    uint32_t m_color;
};

class InnerGlowFilter final : public ImageFilter {
public:
    InnerGlowFilter(float blur, uint32_t color);
    [[nodiscard]] Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;
private:
    float m_blur;
    uint32_t m_color;
};

class BevelEmbossFilter final : public ImageFilter {
public:
    BevelEmbossFilter(float angle, float altitude, float blur,
                      uint32_t highlight, uint32_t shadow);
    [[nodiscard]] Result<void> apply(FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;
private:
    float m_angle, m_altitude, m_blur;
    uint32_t m_highlight, m_shadow;
};

} // namespace pml
