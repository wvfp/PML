// ==========================================================================================================================================================================================================================================═
// Adjustment filters — color matrix, levels, curves, threshold, posterize
// ==========================================================================================================================================================================================================================================═

#include "adjustments.h"
#include "filter_backend.h"

#include <algorithm>
#include <cmath>

namespace pml {

namespace {

std::array<float,20> identity_color_matrix() {
    return {
        1,0,0,0,0,
        0,1,0,0,0,
        0,0,1,0,0,
        0,0,0,1,0
    };
}

void multiply_matrix(std::array<float,20>& a, const std::array<float,20>& b) {
    std::array<float,20> tmp{};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 5; ++col) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a[row * 5 + k] * b[k * 5 + col];
            }
            // Last column of b is translation; last row of a is (0,0,0,1,0)
            if (col == 4) {
                sum += a[row * 5 + 4]; // translation from a
            }
            tmp[row * 5 + col] = sum;
        }
    }
    a = tmp;
}

std::array<float,20> brightness_contrast_matrix(double brightness, double contrast) {
    // brightness: [-1,1] offset
    // contrast: 0..2 scale around 0.5
    double c = std::max(contrast, 0.0);
    double b = std::clamp(brightness, -1.0, 1.0) * 255.0;
    double intercept = 0.5 * 255.0 * (1.0 - c) + b;
    std::array<float,20> m = identity_color_matrix();
    m[0] = static_cast<float>(c);
    m[6] = static_cast<float>(c);
    m[12] = static_cast<float>(c);
    m[4] = static_cast<float>(intercept);
    m[9] = static_cast<float>(intercept);
    m[14] = static_cast<float>(intercept);
    return m;
}

std::array<float,20> saturation_matrix(double saturation) {
    double s = std::max(saturation, 0.0);
    double is = 1.0 - s;
    double lr = 0.2126;
    double lg = 0.7152;
    double lb = 0.0722;
    return {
        static_cast<float>(lr*is + s), static_cast<float>(lg*is), static_cast<float>(lb*is), 0, 0,
        static_cast<float>(lr*is), static_cast<float>(lg*is + s), static_cast<float>(lb*is), 0, 0,
        static_cast<float>(lr*is), static_cast<float>(lg*is), static_cast<float>(lb*is + s), 0, 0,
        0, 0, 0, 1, 0
    };
}

std::array<float,20> hue_matrix(double degrees) {
    double rad = degrees * 3.14159265358979323846 / 180.0;
    float cos_h = static_cast<float>(std::cos(rad));
    float sin_h = static_cast<float>(std::sin(rad));
    float lum_r = 0.2126f;
    float lum_g = 0.7152f;
    float lum_b = 0.0722f;
    return {
        lum_r + cos_h*(1-lum_r) + sin_h*(-lum_r),
        lum_g + cos_h*(-lum_g) + sin_h*(-lum_g),
        lum_b + cos_h*(-lum_b) + sin_h*(1-lum_b),
        0, 0,
        lum_r + cos_h*(-lum_r) + sin_h*(0.143f),
        lum_g + cos_h*(1-lum_g) + sin_h*(0.140f),
        lum_b + cos_h*(-lum_b) + sin_h*(-0.283f),
        0, 0,
        lum_r + cos_h*(-lum_r) + sin_h*(-(1-lum_r)),
        lum_g + cos_h*(-lum_g) + sin_h*(lum_g),
        lum_b + cos_h*(1-lum_b) + sin_h*(lum_b),
        0, 0,
        0, 0, 0, 1, 0
    };
}

std::array<float,20> invert_matrix() {
    std::array<float,20> m = identity_color_matrix();
    m[0] = -1; m[4] = 255;
    m[6] = -1; m[9] = 255;
    m[12] = -1; m[14] = 255;
    return m;
}

std::array<float,20> grayscale_matrix() {
    float r = 0.2126f;
    float g = 0.7152f;
    float b = 0.0722f;
    return {
        r, g, b, 0, 0,
        r, g, b, 0, 0,
        r, g, b, 0, 0,
        0, 0, 0, 1, 0
    };
}

std::array<float,20> sepia_matrix() {
    return {
        0.393f, 0.769f, 0.189f, 0, 0,
        0.349f, 0.686f, 0.168f, 0, 0,
        0.272f, 0.534f, 0.131f, 0, 0,
        0, 0, 0, 1, 0
    };
}

std::array<float,20> temperature_matrix(double temperature) {
    // temperature: -1 (cool) to 1 (warm)
    double t = std::clamp(temperature, -1.0, 1.0);
    float r_scale = static_cast<float>(1.0 + t * 0.2);
    float b_scale = static_cast<float>(1.0 - t * 0.2);
    std::array<float,20> m = identity_color_matrix();
    m[0] = r_scale;
    m[12] = b_scale;
    return m;
}

std::array<float,20> tint_matrix(double tint) {
    double t = std::clamp(tint, -1.0, 1.0);
    float g_shift = static_cast<float>(t * 30.0f);
    std::array<float,20> m = identity_color_matrix();
    m[9] = g_shift;
    return m;
}

std::array<float,20> exposure_matrix(double exposure) {
    // exposure in stops
    double scale = std::pow(2.0, exposure);
    std::array<float,20> m = identity_color_matrix();
    m[0] = static_cast<float>(scale);
    m[6] = static_cast<float>(scale);
    m[12] = static_cast<float>(scale);
    return m;
}

std::array<float,20> gamma_matrix(double gamma) {
    // Gamma is non-linear; color matrix cannot represent it exactly.
    // Return identity and let the caller use a color table for true gamma.
    (void)gamma;
    return identity_color_matrix();
}

std::array<uint8_t,256> make_gamma_lut(double gamma) {
    std::array<uint8_t,256> lut{};
    double g = gamma != 0.0 ? 1.0 / gamma : 1.0;
    for (int i = 0; i < 256; ++i) {
        double v = std::pow(i / 255.0, g) * 255.0 + 0.5;
        lut[i] = static_cast<uint8_t>(std::clamp(v, 0.0, 255.0));
    }
    return lut;
}

} // anonymous namespace

// ---- ColorMatrixFilter ----------------------------------------------------------------------------------------------------------------─

ColorMatrixFilter::ColorMatrixFilter(ColorAdjustParams p) : m_params(p) {}

Result<void> ColorMatrixFilter::apply(FilterBackend& backend, Surface& surface) const {
    std::array<float,20> matrix = identity_color_matrix();

    // Apply in order: exposure, brightness/contrast, temperature, tint, saturation, hue, gamma approx
    multiply_matrix(matrix, exposure_matrix(m_params.exposure));
    multiply_matrix(matrix, brightness_contrast_matrix(m_params.brightness, m_params.contrast));
    multiply_matrix(matrix, temperature_matrix(m_params.temperature));
    multiply_matrix(matrix, tint_matrix(m_params.tint));
    multiply_matrix(matrix, saturation_matrix(m_params.saturation));
    multiply_matrix(matrix, hue_matrix(m_params.hue));

    if (m_params.grayscale) {
        multiply_matrix(matrix, grayscale_matrix());
    } else if (m_params.sepia) {
        multiply_matrix(matrix, sepia_matrix());
    }

    if (m_params.invert) {
        multiply_matrix(matrix, invert_matrix());
    }

    multiply_matrix(matrix, gamma_matrix(m_params.gamma));

    auto r = backend.apply_color_matrix(surface, matrix);
    if (!r) return r;

    // True gamma correction via color table if needed.
    if (std::abs(m_params.gamma - 1.0) > 1e-6) {
        auto lut = make_gamma_lut(m_params.gamma);
        return backend.apply_color_table(surface, lut, lut, lut, lut);
    }

    return {};
}

std::string ColorMatrixFilter::name() const {
    return "color-adjust";
}

// ---- LevelsFilter ----------------------------------------------------------------------------------------------------------------------------

LevelsFilter::LevelsFilter(double in_low, double gamma, double in_high,
                           double out_low, double out_high)
    : m_in_low(in_low), m_gamma(gamma), m_in_high(in_high),
      m_out_low(out_low), m_out_high(out_high) {}

Result<void> LevelsFilter::apply(FilterBackend& backend, Surface& surface) const {
    std::array<uint8_t,256> lut{};
    double in_low = std::clamp(m_in_low, 0.0, 255.0);
    double in_high = std::clamp(m_in_high, in_low, 255.0);
    double out_low = std::clamp(m_out_low, 0.0, 255.0);
    double out_high = std::clamp(m_out_high, out_low, 255.0);
    double gamma = m_gamma != 0.0 ? m_gamma : 1.0;
    double in_range = in_high - in_low;
    double out_range = out_high - out_low;

    for (int i = 0; i < 256; ++i) {
        double v = (i - in_low) / in_range;
        v = std::clamp(v, 0.0, 1.0);
        v = std::pow(v, 1.0 / gamma);
        v = out_low + v * out_range;
        lut[i] = static_cast<uint8_t>(std::clamp(v, 0.0, 255.0));
    }
    return backend.apply_color_table(surface, lut, lut, lut, lut);
}

std::string LevelsFilter::name() const {
    return "levels";
}

// ---- CurvesFilter ----------------------------------------------------------------------------------------------------------------------------

CurvesFilter::CurvesFilter(int channel, std::vector<std::pair<uint8_t,uint8_t>> points)
    : m_channel(channel), m_points(std::move(points)) {}

namespace {

std::array<uint8_t,256> interpolate_curve(const std::vector<std::pair<uint8_t,uint8_t>>& points) {
    std::array<uint8_t,256> lut{};
    if (points.empty()) {
        for (int i = 0; i < 256; ++i) lut[i] = static_cast<uint8_t>(i);
        return lut;
    }
    std::vector<std::pair<uint8_t,uint8_t>> sorted = points;
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    for (int i = 0; i < 256; ++i) {
        uint8_t x = static_cast<uint8_t>(i);
        auto it = std::lower_bound(sorted.begin(), sorted.end(), x,
            [](const auto& p, uint8_t val) { return p.first < val; });
        if (it == sorted.begin()) {
            lut[i] = sorted.front().second;
        } else if (it == sorted.end()) {
            lut[i] = sorted.back().second;
        } else {
            auto prev = it - 1;
            double t = (x - prev->first) / static_cast<double>(it->first - prev->first);
            double y = prev->second + t * (it->second - prev->second);
            lut[i] = static_cast<uint8_t>(std::clamp(y, 0.0, 255.0));
        }
    }
    return lut;
}

} // anonymous namespace

Result<void> CurvesFilter::apply(FilterBackend& backend, Surface& surface) const {
    auto lut = interpolate_curve(m_points);
    std::array<uint8_t,256> identity{};
    for (int i = 0; i < 256; ++i) identity[i] = static_cast<uint8_t>(i);

    std::array<uint8_t,256> r = identity;
    std::array<uint8_t,256> g = identity;
    std::array<uint8_t,256> b = identity;

    switch (m_channel) {
        case 0: r = lut; break;
        case 1: g = lut; break;
        case 2: b = lut; break;
        default: r = g = b = lut; break;
    }
    return backend.apply_color_table(surface, r, g, b, identity);
}

std::string CurvesFilter::name() const {
    return "curves";
}

// ---- ThresholdFilter --------------------------------------------------------------------------------------------------------------------─

ThresholdFilter::ThresholdFilter(uint8_t value) : m_value(value) {}

Result<void> ThresholdFilter::apply(FilterBackend& backend, Surface& surface) const {
    std::array<uint8_t,256> lut{};
    for (int i = 0; i < 256; ++i) {
        lut[i] = (i < m_value) ? 0 : 255;
    }
    return backend.apply_color_table(surface, lut, lut, lut, lut);
}

std::string ThresholdFilter::name() const {
    return "threshold";
}

// ---- PosterizeFilter --------------------------------------------------------------------------------------------------------------------─

PosterizeFilter::PosterizeFilter(int levels) : m_levels(std::max(levels, 2)) {}

Result<void> PosterizeFilter::apply(FilterBackend& backend, Surface& surface) const {
    std::array<uint8_t,256> lut{};
    for (int i = 0; i < 256; ++i) {
        double v = std::floor(i / 255.0 * m_levels) / (m_levels - 1) * 255.0;
        lut[i] = static_cast<uint8_t>(std::clamp(v, 0.0, 255.0));
    }
    return backend.apply_color_table(surface, lut, lut, lut, lut);
}

std::string PosterizeFilter::name() const {
    return "posterize";
}

} // namespace pml
