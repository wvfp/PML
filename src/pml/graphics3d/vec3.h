#pragma once

// ==========================================================================================================================================================================================================================================═
// Vec3 — 3D vector / point
// ==========================================================================================================================================================================================================================================═

#include <cmath>

namespace pml {

struct Vec3 {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    Vec3() = default;
    constexpr Vec3(double x_, double y_, double z_) noexcept
        : x(x_), y(y_), z(z_) {}

    [[nodiscard]] constexpr Vec3 operator+(const Vec3& o) const noexcept {
        return Vec3(x + o.x, y + o.y, z + o.z);
    }
    [[nodiscard]] constexpr Vec3 operator-(const Vec3& o) const noexcept {
        return Vec3(x - o.x, y - o.y, z - o.z);
    }
    [[nodiscard]] constexpr Vec3 operator*(double s) const noexcept {
        return Vec3(x * s, y * s, z * s);
    }
    [[nodiscard]] constexpr Vec3 operator/(double s) const noexcept {
        return Vec3(x / s, y / s, z / s);
    }

    [[nodiscard]] constexpr double dot(const Vec3& o) const noexcept {
        return x * o.x + y * o.y + z * o.z;
    }
    [[nodiscard]] constexpr Vec3 cross(const Vec3& o) const noexcept {
        return Vec3(
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x);
    }
    [[nodiscard]] double length() const noexcept {
        return std::sqrt(x * x + y * y + z * z);
    }
    [[nodiscard]] Vec3 normalized() const noexcept {
        double len = length();
        if (len < 1e-12) return Vec3(0, 0, 0);
        return *this / len;
    }
};

} // namespace pml
