// ═══════════════════════════════════════════════════════════════════════════════
// Layer-style filters — drop shadow / inner shadow / glow / bevel emboss
// ═══════════════════════════════════════════════════════════════════════════════

#include "layer_style.h"

namespace pml {

// ── DropShadowFilter ──────────────────────────────────────────────────────────

DropShadowFilter::DropShadowFilter(float dx, float dy, float blur_x, float blur_y, uint32_t color)
    : m_dx(dx), m_dy(dy), m_blur_x(blur_x), m_blur_y(blur_y), m_color(color) {}

Result<void> DropShadowFilter::apply(FilterBackend& backend, Surface& surface) const {
    return backend.apply_drop_shadow(surface, m_dx, m_dy, m_blur_x, m_blur_y, m_color);
}

std::string DropShadowFilter::name() const {
    return "drop-shadow";
}

// ── InnerShadowFilter ─────────────────────────────────────────────────────────

InnerShadowFilter::InnerShadowFilter(float dx, float dy, float blur, uint32_t color)
    : m_dx(dx), m_dy(dy), m_blur(blur), m_color(color) {}

Result<void> InnerShadowFilter::apply(FilterBackend& backend, Surface& surface) const {
    return backend.apply_inner_shadow(surface, m_dx, m_dy, m_blur, m_color);
}

std::string InnerShadowFilter::name() const {
    return "inner-shadow";
}

// ── OuterGlowFilter ───────────────────────────────────────────────────────────

OuterGlowFilter::OuterGlowFilter(float blur, uint32_t color)
    : m_blur(blur), m_color(color) {}

Result<void> OuterGlowFilter::apply(FilterBackend& backend, Surface& surface) const {
    return backend.apply_outer_glow(surface, m_blur, m_color);
}

std::string OuterGlowFilter::name() const {
    return "outer-glow";
}

// ── InnerGlowFilter ───────────────────────────────────────────────────────────

InnerGlowFilter::InnerGlowFilter(float blur, uint32_t color)
    : m_blur(blur), m_color(color) {}

Result<void> InnerGlowFilter::apply(FilterBackend& backend, Surface& surface) const {
    return backend.apply_inner_glow(surface, m_blur, m_color);
}

std::string InnerGlowFilter::name() const {
    return "inner-glow";
}

// ── BevelEmbossFilter ─────────────────────────────────────────────────────────

BevelEmbossFilter::BevelEmbossFilter(float angle, float altitude, float blur,
                                     uint32_t highlight, uint32_t shadow)
    : m_angle(angle), m_altitude(altitude), m_blur(blur),
      m_highlight(highlight), m_shadow(shadow) {}

Result<void> BevelEmbossFilter::apply(FilterBackend& backend, Surface& surface) const {
    return backend.apply_bevel_emboss(surface, m_angle, m_altitude, m_blur,
                                      m_highlight, m_shadow);
}

std::string BevelEmbossFilter::name() const {
    return "bevel-emboss";
}

} // namespace pml
