#pragma once

#include <cmath>
#include <cairo.h>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

#ifdef PML_USE_SKIA
class SkMatrix;
#endif

namespace pml {

/// Immutable 2D affine transformation matrix.
///
/// Represented as 6 double-precision fields (a, b, c, d, e, f):
///
///     | a  c  e |     x' = a*x + c*y + e
///     | b  d  f |     y' = b*x + d*y + f
///     | 0  0  1 |
///
/// All mutating methods return new instances. The struct is a trivial
/// value type — copy and move are compiler-generated.
struct AffineTransform {
    double a{1.0};
    double b{0.0};
    double c{0.0};
    double d{1.0};
    double e{0.0};
    double f{0.0};

    // ── Static factories ──────────────────────────────────────────

    /// Identity matrix.
    static constexpr AffineTransform identity() noexcept
    {
        return AffineTransform{};
    }

    /// Translation by (tx, ty).
    static constexpr AffineTransform translate(double tx, double ty) noexcept
    {
        return AffineTransform{1.0, 0.0, 0.0, 1.0, tx, ty};
    }

    /// Rotation by @p angle_deg degrees (counter-clockwise).
    static constexpr AffineTransform rotate(double angle_deg) noexcept
    {
        const double rad = angle_deg * std::numbers::pi_v<double> / 180.0;
        const double ct = std::cos(rad);
        const double st = std::sin(rad);
        return AffineTransform{ct, st, -st, ct, 0.0, 0.0};
    }

    /// Uniform or non-uniform scale.
    static constexpr AffineTransform scale(double sx, double sy) noexcept
    {
        return AffineTransform{sx, 0.0, 0.0, sy, 0.0, 0.0};
    }

    /// Shear by (shx, shy).
    static constexpr AffineTransform shear(double shx, double shy) noexcept
    {
        return AffineTransform{1.0, shy, shx, 1.0, 0.0, 0.0};
    }

    // ── Operations ────────────────────────────────────────────────

    /// Return self · other  (apply @p other first, then self).
    [[nodiscard]] constexpr AffineTransform compose(const AffineTransform& other) const noexcept
    {
        return AffineTransform{
            a * other.a + c * other.b,
            b * other.a + d * other.b,
            a * other.c + c * other.d,
            b * other.c + d * other.d,
            a * other.e + c * other.f + e,
            b * other.e + d * other.f + f,
        };
    }

    /// Return the inverse matrix, or std::nullopt if singular.
    [[nodiscard]] constexpr std::optional<AffineTransform> inverse() const noexcept
    {
        const double det = a * d - b * c;
        if (std::abs(det) < 1e-12) {
            return std::nullopt;
        }
        const double inv_det = 1.0 / det;
        return AffineTransform{
            d * inv_det,
            -b * inv_det,
            -c * inv_det,
            a * inv_det,
            (c * f - d * e) * inv_det,
            (b * e - a * f) * inv_det,
        };
    }

    /// Apply transform to point (x, y) → (x', y').
    [[nodiscard]] constexpr std::pair<double, double> apply(double x, double y) const noexcept
    {
        return {
            a * x + c * y + e,
            b * x + d * y + f,
        };
    }

    /// Apply transform to every point in @p points.
    [[nodiscard]] std::vector<std::pair<double, double>> apply_points(
        const std::vector<std::pair<double, double>>& points) const
    {
        std::vector<std::pair<double, double>> result;
        result.reserve(points.size());
        for (const auto& pt : points) {
            result.push_back(apply(pt.first, pt.second));
        }
        return result;
    }

    /// True if this is approximately the identity matrix (epsilon 1e-9).
    [[nodiscard]] constexpr bool is_identity() const noexcept
    {
        using std::abs;
        return abs(a - 1.0) < 1e-9
            && abs(b) < 1e-9
            && abs(c) < 1e-9
            && abs(d - 1.0) < 1e-9
            && abs(e) < 1e-9
            && abs(f) < 1e-9;
    }

    // ── Skia integration (defined in transform.cpp) ───────────────

#ifdef PML_USE_SKIA
    /// Convert to SkMatrix via setAffine().
    [[nodiscard]] SkMatrix to_skmatrix() const;
#endif

    /// Convert to Cairo matrix for use with cairo_transform().
    /// Cairo is always available (required dependency).
    [[nodiscard]] cairo_matrix_t to_cairo_matrix() const;
};

// ── Operator overloads ───────────────────────────────────────────

/// Convenience: t1 * t2  ⇔  t1.compose(t2)
[[nodiscard]] inline constexpr AffineTransform operator*(
    const AffineTransform& lhs,
    const AffineTransform& rhs) noexcept
{
    return lhs.compose(rhs);
}

/// Convenience: t * (x, y)  ⇔  t.apply(x, y)
[[nodiscard]] inline constexpr std::pair<double, double> operator*(
    const AffineTransform& t,
    const std::pair<double, double>& pt) noexcept
{
    return t.apply(pt.first, pt.second);
}

}  // namespace pml
