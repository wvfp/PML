// ═══════════════════════════════════════════════════════════════════════════════
// PML Skia render backend — FilterBackend member functions
// ═══════════════════════════════════════════════════════════════════════════════
//
// Member function definitions for SkiaBackend's FilterBackend interface.
// ═══════════════════════════════════════════════════════════════════════════════

#include "skia_backend_internal.h"

namespace pml {

// ── apply_sk_image_filter (private helper) ──────────────────────────────────

Result<void> SkiaBackend::apply_sk_image_filter(
    SkiaSurface& surf, sk_sp<SkImageFilter> filter)
{
    if (!filter) return {};

    auto tmp = SkSurfaces::Raster(surf.bitmap.info());
    if (!tmp) {
        return std::unexpected(filter_error(
            {}, "skia: failed to create filter scratch surface"));
    }

    SkPaint paint;
    paint.setImageFilter(std::move(filter));
    tmp->getCanvas()->drawImage(
        surf.bitmap.asImage().get(), 0, 0, SkSamplingOptions(), &paint);

    SkPixmap pixmap;
    if (!tmp->peekPixels(&pixmap)) {
        return std::unexpected(filter_error(
            {}, "skia: failed to read filtered pixels"));
    }
    if (!surf.bitmap.writePixels(pixmap)) {
        return std::unexpected(filter_error(
            {}, "skia: failed to copy filtered pixels back"));
    }
    return {};
}

// ── Color Matrix ────────────────────────────────────────────────────────────

Result<void> SkiaBackend::apply_color_matrix(
    Surface& s, const std::array<float,20>& m)
{
    auto* surf = dynamic_cast<SkiaSurface*>(&s);
    if (!surf) {
        return std::unexpected(filter_error(
            {}, "skia: invalid surface for color matrix"));
    }
    sk_sp<SkColorFilter> cf = SkColorFilters::Matrix(m.data());
    sk_sp<SkImageFilter> imf = SkImageFilters::ColorFilter(
        std::move(cf), nullptr);
    return apply_sk_image_filter(*surf, std::move(imf));
}

// ── Blur ────────────────────────────────────────────────────────────────────

Result<void> SkiaBackend::apply_blur(
    Surface& s, float rx, float ry, BlurType type)
{
    auto* surf = dynamic_cast<SkiaSurface*>(&s);
    if (!surf) {
        return std::unexpected(filter_error(
            {}, "skia: invalid surface for blur"));
    }
    if (type == BlurType::Gaussian) {
        auto imf = SkImageFilters::Blur(
            rx, ry, SkTileMode::kDecal, nullptr);
        return apply_sk_image_filter(*surf, std::move(imf));
    }
    return std::unexpected(filter_not_supported(
        {}, "skia: only gaussian blur is supported natively"));
}

// ── Convolution ─────────────────────────────────────────────────────────────

Result<void> SkiaBackend::apply_convolution(
    Surface& s, const ConvolutionKernel& k)
{
    auto* surf = dynamic_cast<SkiaSurface*>(&s);
    if (!surf) {
        return std::unexpected(filter_error(
            {}, "skia: invalid surface for convolution"));
    }

    SkISize size{k.width, k.height};
    std::vector<SkScalar> kernel(k.values.begin(), k.values.end());
    SkScalar gain = k.divisor != 0.0f ? 1.0f / k.divisor : 1.0f;
    SkScalar bias = static_cast<SkScalar>(k.offset);
    SkIPoint target{
        k.anchor_x >= 0 ? k.anchor_x : k.width / 2,
        k.anchor_y >= 0 ? k.anchor_y : k.height / 2};

    auto imf = SkImageFilters::MatrixConvolution(
        size, kernel.data(), gain, bias, target,
        SkTileMode::kDecal, false, nullptr);
    return apply_sk_image_filter(*surf, std::move(imf));
}

// ── Color Table ─────────────────────────────────────────────────────────────

Result<void> SkiaBackend::apply_color_table(
    Surface& s,
    const std::array<uint8_t,256>& r,
    const std::array<uint8_t,256>& g,
    const std::array<uint8_t,256>& b,
    const std::array<uint8_t,256>& a)
{
    auto* surf = dynamic_cast<SkiaSurface*>(&s);
    if (!surf) {
        return std::unexpected(filter_error(
            {}, "skia: invalid surface for color table"));
    }
    sk_sp<SkColorFilter> cf = SkColorFilters::TableARGB(
        const_cast<uint8_t*>(a.data()),
        const_cast<uint8_t*>(r.data()),
        const_cast<uint8_t*>(g.data()),
        const_cast<uint8_t*>(b.data()));
    sk_sp<SkImageFilter> imf = SkImageFilters::ColorFilter(
        std::move(cf), nullptr);
    return apply_sk_image_filter(*surf, std::move(imf));
}

// ── Offset ──────────────────────────────────────────────────────────────────

Result<void> SkiaBackend::apply_offset(Surface& s, float dx, float dy)
{
    auto* surf = dynamic_cast<SkiaSurface*>(&s);
    if (!surf) {
        return std::unexpected(filter_error(
            {}, "skia: invalid surface for offset"));
    }
    auto imf = SkImageFilters::Offset(dx, dy, nullptr);
    return apply_sk_image_filter(*surf, std::move(imf));
}

// ── Drop Shadow ─────────────────────────────────────────────────────────────

Result<void> SkiaBackend::apply_drop_shadow(
    Surface& s, float dx, float dy,
    float blur_x, float blur_y, uint32_t color)
{
    auto* surf = dynamic_cast<SkiaSurface*>(&s);
    if (!surf) {
        return std::unexpected(filter_error(
            {}, "skia: invalid surface for drop shadow"));
    }
    auto imf = SkImageFilters::DropShadow(
        dx, dy, blur_x, blur_y, to_sk_color(color), nullptr);
    return apply_sk_image_filter(*surf, std::move(imf));
}

// ── Inner Shadow ────────────────────────────────────────────────────────────

Result<void> SkiaBackend::apply_inner_shadow(
    Surface& s, float dx, float dy, float blur, uint32_t color)
{
    auto* surf = dynamic_cast<SkiaSurface*>(&s);
    if (!surf) {
        return std::unexpected(filter_error(
            {}, "skia: invalid surface for inner shadow"));
    }

    // 1. Render the shadow outside the shape (DropShadowOnly).
    // 2. Mask it by the source alpha so it only appears inside the shape.
    // 3. Composite the inner shadow on top of the original source.
    auto shadow = SkImageFilters::DropShadowOnly(
        dx, dy, blur, blur, to_sk_color(color), nullptr);
    auto masked = SkImageFilters::Blend(
        SkBlendMode::kSrcIn, nullptr, std::move(shadow));
    auto merge = SkImageFilters::Merge(nullptr, std::move(masked));
    return apply_sk_image_filter(*surf, std::move(merge));
}

// ── Outer Glow ──────────────────────────────────────────────────────────────

Result<void> SkiaBackend::apply_outer_glow(
    Surface& s, float blur, uint32_t color)
{
    auto* surf = dynamic_cast<SkiaSurface*>(&s);
    if (!surf) {
        return std::unexpected(filter_error(
            {}, "skia: invalid surface for outer glow"));
    }

    // Glow is a colored blur behind the source.
    auto glow = SkImageFilters::DropShadowOnly(
        0, 0, blur, blur, to_sk_color(color), nullptr);
    auto merge = SkImageFilters::Merge(std::move(glow), nullptr);
    return apply_sk_image_filter(*surf, std::move(merge));
}

// ── Inner Glow ──────────────────────────────────────────────────────────────

Result<void> SkiaBackend::apply_inner_glow(
    Surface& s, float blur, uint32_t color)
{
    auto* surf = dynamic_cast<SkiaSurface*>(&s);
    if (!surf) {
        return std::unexpected(filter_error(
            {}, "skia: invalid surface for inner glow"));
    }

    // Same as inner shadow but centered (no offset).
    auto glow = SkImageFilters::DropShadowOnly(
        0, 0, blur, blur, to_sk_color(color), nullptr);
    auto masked = SkImageFilters::Blend(
        SkBlendMode::kSrcIn, nullptr, std::move(glow));
    auto merge = SkImageFilters::Merge(nullptr, std::move(masked));
    return apply_sk_image_filter(*surf, std::move(merge));
}

// ── Bevel / Emboss ──────────────────────────────────────────────────────────

Result<void> SkiaBackend::apply_bevel_emboss(
    Surface& s, float angle, float altitude, float blur,
    uint32_t highlight, uint32_t shadow)
{
    auto* surf = dynamic_cast<SkiaSurface*>(&s);
    if (!surf) {
        return std::unexpected(filter_error(
            {}, "skia: invalid surface for bevel/emboss"));
    }

    // Convert spherical angle/altitude to a normalized light direction.
    const double deg2rad = 3.14159265358979323846 / 180.0;
    const double a = angle * deg2rad;
    const double alt = altitude * deg2rad;
    const float z = static_cast<float>(std::sin(alt));
    const float xy = static_cast<float>(std::cos(alt));
    SkPoint3 dir = SkPoint3::Make(
        xy * static_cast<float>(std::cos(a)),
        xy * static_cast<float>(std::sin(a)),
        z);
    dir.normalize();
    SkPoint3 opp = -dir;

    // Use Skia's diffuse lighting filters with the source alpha as bump map.
    // Two opposite lights give highlight and shadow edges.
    const SkScalar surface_scale = std::max<SkScalar>(1.0f, blur / 2.0f);
    auto highlight_lit = SkImageFilters::DistantLitDiffuse(
        dir, to_sk_color(highlight), surface_scale, 1.0f, nullptr);
    auto shadow_lit = SkImageFilters::DistantLitDiffuse(
        opp, to_sk_color(shadow), surface_scale, 1.0f, nullptr);

    sk_sp<SkImageFilter> filters[] = {
        nullptr,
        std::move(shadow_lit),
        std::move(highlight_lit),
    };
    auto merge = SkImageFilters::Merge(filters, 3);
    return apply_sk_image_filter(*surf, std::move(merge));
}

}  // namespace pml
