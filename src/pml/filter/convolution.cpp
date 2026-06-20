// ═══════════════════════════════════════════════════════════════════════════════
// Convolution filters — blur, sharpen, edge detection, emboss, custom kernel
// ═══════════════════════════════════════════════════════════════════════════════

#include "convolution.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace pml {

namespace {

int kernel_size_from_radius(float radius) {
    int r = static_cast<int>(std::ceil(std::max(radius, 0.0f)));
    return std::max(3, r * 2 + 1);
}

ConvolutionKernel make_box_kernel(float radius) {
    int size = kernel_size_from_radius(radius);
    ConvolutionKernel k;
    k.width = size;
    k.height = size;
    k.values.assign(static_cast<size_t>(size * size), 1.0f);
    k.divisor = static_cast<float>(size * size);
    k.anchor_x = size / 2;
    k.anchor_y = size / 2;
    return k;
}

ConvolutionKernel make_motion_kernel(float radius, float angle) {
    int size = kernel_size_from_radius(radius);
    ConvolutionKernel k;
    k.width = size;
    k.height = size;
    k.values.assign(static_cast<size_t>(size * size), 0.0f);

    int cx = size / 2;
    int cy = size / 2;
    int steps = std::max(2, size);
    float a = angle * 3.14159265358979323846f / 180.0f;
    float max_r = static_cast<float>(cx);

    int count = 0;
    for (int i = 0; i < steps; ++i) {
        float t = (steps > 1) ? (static_cast<float>(i) / (steps - 1) * 2.0f - 1.0f) : 0.0f;
        float x = cx + std::cos(a) * max_r * t;
        float y = cy + std::sin(a) * max_r * t;
        int ix = static_cast<int>(std::round(x));
        int iy = static_cast<int>(std::round(y));
        ix = std::clamp(ix, 0, size - 1);
        iy = std::clamp(iy, 0, size - 1);
        size_t idx = static_cast<size_t>(iy * size + ix);
        if (k.values[idx] == 0.0f) ++count;
        k.values[idx] = 1.0f;
    }
    if (count == 0) count = 1;
    k.divisor = static_cast<float>(count);
    k.anchor_x = cx;
    k.anchor_y = cy;
    return k;
}

ConvolutionKernel make_sharpen_kernel(float amount) {
    ConvolutionKernel k;
    k.width = 3;
    k.height = 3;
    float a = std::max(amount, 0.0f);
    k.values = {
        0.0f, -a,       0.0f,
        -a,   1.0f+4*a, -a,
        0.0f, -a,       0.0f
    };
    k.divisor = 1.0f;
    k.anchor_x = 1;
    k.anchor_y = 1;
    return k;
}

ConvolutionKernel make_sobel_kernel() {
    ConvolutionKernel k;
    k.width = 3;
    k.height = 3;
    k.values = {
        -1.0f, 0.0f, 1.0f,
        -2.0f, 0.0f, 2.0f,
        -1.0f, 0.0f, 1.0f
    };
    k.offset = 128.0f;
    k.divisor = 1.0f;
    k.anchor_x = 1;
    k.anchor_y = 1;
    return k;
}

ConvolutionKernel make_laplacian_kernel() {
    ConvolutionKernel k;
    k.width = 3;
    k.height = 3;
    k.values = {
        0.0f,  -1.0f, 0.0f,
        -1.0f, 4.0f,  -1.0f,
        0.0f,  -1.0f, 0.0f
    };
    k.offset = 128.0f;
    k.divisor = 1.0f;
    k.anchor_x = 1;
    k.anchor_y = 1;
    return k;
}

ConvolutionKernel make_emboss_kernel(float /*direction*/) {
    ConvolutionKernel k;
    k.width = 3;
    k.height = 3;
    k.values = {
        -2.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 1.0f,
         0.0f,  1.0f, 2.0f
    };
    k.offset = 128.0f;
    k.divisor = 1.0f;
    k.anchor_x = 1;
    k.anchor_y = 1;
    return k;
}

} // anonymous namespace

// ── BlurFilter ────────────────────────────────────────────────────────────────

BlurFilter::BlurFilter(float radius, BlurFilterType type, float angle)
    : m_radius(std::max(radius, 0.0f)), m_type(type), m_angle(angle) {}

Result<void> BlurFilter::apply(FilterBackend& backend, Surface& surface) const {
    switch (m_type) {
        case BlurFilterType::Gaussian:
            return backend.apply_blur(surface, m_radius, m_radius, BlurType::Gaussian);
        case BlurFilterType::Box:
            return backend.apply_convolution(surface, make_box_kernel(m_radius));
        case BlurFilterType::Motion:
            return backend.apply_convolution(surface, make_motion_kernel(m_radius, m_angle));
    }
    return std::unexpected(filter_error({}, "blur: unknown blur type"));
}

std::string BlurFilter::name() const {
    return "blur";
}

// ── SharpenFilter ─────────────────────────────────────────────────────────────

SharpenFilter::SharpenFilter(float amount, float /*radius*/)
    : m_amount(std::max(amount, 0.0f)) {}

Result<void> SharpenFilter::apply(FilterBackend& backend, Surface& surface) const {
    return backend.apply_convolution(surface, make_sharpen_kernel(m_amount));
}

std::string SharpenFilter::name() const {
    return "sharpen";
}

// ── EdgeDetectFilter ──────────────────────────────────────────────────────────

EdgeDetectFilter::EdgeDetectFilter(EdgeDetectType type) : m_type(type) {}

Result<void> EdgeDetectFilter::apply(FilterBackend& backend, Surface& surface) const {
    switch (m_type) {
        case EdgeDetectType::Sobel:
            return backend.apply_convolution(surface, make_sobel_kernel());
        case EdgeDetectType::Laplacian:
            return backend.apply_convolution(surface, make_laplacian_kernel());
    }
    return std::unexpected(filter_error({}, "edge-detect: unknown edge detect type"));
}

std::string EdgeDetectFilter::name() const {
    return "edge-detect";
}

// ── EmbossFilter ──────────────────────────────────────────────────────────────

EmbossFilter::EmbossFilter(float direction) : m_direction(direction) {}

Result<void> EmbossFilter::apply(FilterBackend& backend, Surface& surface) const {
    return backend.apply_convolution(surface, make_emboss_kernel(m_direction));
}

std::string EmbossFilter::name() const {
    return "emboss";
}

// ── ConvolutionFilter ─────────────────────────────────────────────────────────

ConvolutionFilter::ConvolutionFilter(int w, int h, std::vector<float> values,
                                     float offset, float divisor)
    : m_kernel{w, h, std::move(values), offset, divisor, -1, -1} {}

Result<void> ConvolutionFilter::apply(FilterBackend& backend, Surface& surface) const {
    return backend.apply_convolution(surface, m_kernel);
}

std::string ConvolutionFilter::name() const {
    return "convolution";
}

} // namespace pml
