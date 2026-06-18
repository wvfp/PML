#include "easing.h"

#include <cmath>
#include <numbers>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Easing function implementations
// (Ported from pml/animation/easing.py — IEEE 754 double precision, matching
//  Python's float behaviour exactly.)
// ═══════════════════════════════════════════════════════════════════════════════

double easing_linear(double t) {
    return t;
}

double easing_quad_in(double t) {
    return t * t;
}

double easing_quad_out(double t) {
    return t * (2.0 - t);
}

double easing_quad_in_out(double t) {
    if (t < 0.5) {
        return 2.0 * t * t;
    }
    return -1.0 + (4.0 - 2.0 * t) * t;
}

double easing_cubic_in(double t) {
    return std::pow(t, 3.0);
}

double easing_cubic_out(double t) {
    return std::pow(t - 1.0, 3.0) + 1.0;
}

double easing_cubic_in_out(double t) {
    if (t < 0.5) {
        return 4.0 * std::pow(t, 3.0);
    }
    return std::pow(2.0 * t - 2.0, 3.0) / 2.0 + 1.0;
}

double easing_sin_in(double t) {
    return 1.0 - std::cos(t * std::numbers::pi_v<double> / 2.0);
}

double easing_sin_out(double t) {
    return std::sin(t * std::numbers::pi_v<double> / 2.0);
}

double easing_sin_in_out(double t) {
    return -(std::cos(std::numbers::pi_v<double> * t) - 1.0) / 2.0;
}

double easing_bounce(double t) {
    if (t < 1.0 / 2.75) {
        return 7.5625 * t * t;
    } else if (t < 2.0 / 2.75) {
        t -= 1.5 / 2.75;
        return 7.5625 * t * t + 0.75;
    } else if (t < 2.5 / 2.75) {
        t -= 2.25 / 2.75;
        return 7.5625 * t * t + 0.9375;
    } else {
        t -= 2.625 / 2.75;
        return 7.5625 * t * t + 0.984375;
    }
}

double easing_elastic(double t) {
    if (t == 0.0 || t == 1.0) {
        return t;
    }
    const double p = 0.3;
    const double s = p / 4.0;
    return std::pow(2.0, -10.0 * t)
         * std::sin((t - s) * (2.0 * std::numbers::pi_v<double>) / p)
         + 1.0;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Lookup
// ═══════════════════════════════════════════════════════════════════════════════

EasingFn get_easing(const std::string& name) {
    static const auto easing_map = [] {
        std::unordered_map<std::string, EasingFn> m;
        m["linear"]       = easing_linear;
        m["quad-in"]      = easing_quad_in;
        m["quad-out"]     = easing_quad_out;
        m["quad-in-out"]  = easing_quad_in_out;
        m["cubic-in"]     = easing_cubic_in;
        m["cubic-out"]    = easing_cubic_out;
        m["cubic-in-out"] = easing_cubic_in_out;
        m["sin-in"]       = easing_sin_in;
        m["sin-out"]      = easing_sin_out;
        m["sin-in-out"]   = easing_sin_in_out;
        m["bounce"]       = easing_bounce;
        m["elastic"]      = easing_elastic;
        return m;
    }();

    auto it = easing_map.find(name);
    if (it != easing_map.end()) {
        return it->second;
    }
    // Fall back to linear (matching Python behaviour)
    return easing_linear;
}

std::vector<std::string> list_easings() {
    // Sorted alphabetically, matching Python's sorted(EASING_FUNCTIONS.keys())
    return {
        "bounce",
        "cubic-in",
        "cubic-in-out",
        "cubic-out",
        "elastic",
        "linear",
        "quad-in",
        "quad-in-out",
        "quad-out",
        "sin-in",
        "sin-in-out",
        "sin-out",
    };
}

} // namespace pml
