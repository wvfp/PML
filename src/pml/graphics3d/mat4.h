#pragma once

// ==========================================================================================================================================================================================================================================═
// Mat4 — 4×4 column-major transformation / projection matrix
// ==========================================================================================================================================================================================================================================═

#include "vec3.h"

#include <array>
#include <cmath>
#include <numbers>

namespace pml {

struct Mat4 {
    std::array<double, 16> m{};

    Mat4() { set_identity(); }

    void set_identity() noexcept {
        m = {1, 0, 0, 0,
             0, 1, 0, 0,
             0, 0, 1, 0,
             0, 0, 0, 1};
    }

    [[nodiscard]] static Mat4 identity() noexcept {
        Mat4 r;
        r.set_identity();
        return r;
    }

    [[nodiscard]] static Mat4 translate(double x, double y, double z) noexcept;
    [[nodiscard]] static Mat4 scale(double sx, double sy, double sz) noexcept;
    [[nodiscard]] static Mat4 rotate_x(double angle_deg) noexcept;
    [[nodiscard]] static Mat4 rotate_y(double angle_deg) noexcept;
    [[nodiscard]] static Mat4 rotate_z(double angle_deg) noexcept;
    [[nodiscard]] static Mat4 look_at(const Vec3& eye, const Vec3& target, const Vec3& up) noexcept;
    [[nodiscard]] static Mat4 perspective(double fov_deg, double aspect, double near_plane, double far_plane) noexcept;
    [[nodiscard]] static Mat4 orthographic(double size, double aspect, double near_plane, double far_plane) noexcept;

    [[nodiscard]] Mat4 operator*(const Mat4& o) const noexcept;

    /// Transform a point (w = 1) including perspective divide.
    [[nodiscard]] Vec3 transform_point(const Vec3& p) const noexcept;

    /// Transform a vector (w = 0).
    [[nodiscard]] Vec3 transform_vector(const Vec3& v) const noexcept;
};

} // namespace pml
