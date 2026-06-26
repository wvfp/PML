#pragma once

// ==========================================================================================================================================================================================================================================═
// Transform3D — immutable model-space TRS transform
// ==========================================================================================================================================================================================================================================═

#include "mat4.h"

namespace pml {

struct Transform3D {
    Mat4 matrix{Mat4::identity()};

    [[nodiscard]] static Transform3D identity() noexcept { return Transform3D{}; }

    [[nodiscard]] Transform3D translated(double x, double y, double z) const noexcept {
        Transform3D r;
        r.matrix = Mat4::translate(x, y, z) * matrix;
        return r;
    }

    [[nodiscard]] Transform3D scaled(double sx, double sy, double sz) const noexcept {
        Transform3D r;
        r.matrix = Mat4::scale(sx, sy, sz) * matrix;
        return r;
    }

    [[nodiscard]] Transform3D rotated_x(double angle_deg) const noexcept {
        Transform3D r;
        r.matrix = Mat4::rotate_x(angle_deg) * matrix;
        return r;
    }

    [[nodiscard]] Transform3D rotated_y(double angle_deg) const noexcept {
        Transform3D r;
        r.matrix = Mat4::rotate_y(angle_deg) * matrix;
        return r;
    }

    [[nodiscard]] Transform3D rotated_z(double angle_deg) const noexcept {
        Transform3D r;
        r.matrix = Mat4::rotate_z(angle_deg) * matrix;
        return r;
    }

    [[nodiscard]] Vec3 apply(const Vec3& v) const noexcept {
        return matrix.transform_point(v);
    }

    [[nodiscard]] Vec3 apply_vector(const Vec3& v) const noexcept {
        return matrix.transform_vector(v);
    }
};

} // namespace pml
