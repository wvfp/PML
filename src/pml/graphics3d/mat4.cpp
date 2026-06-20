#include "mat4.h"

namespace pml {

Mat4 Mat4::translate(double x, double y, double z) noexcept {
    Mat4 r;
    r.set_identity();
    r.m[12] = x;
    r.m[13] = y;
    r.m[14] = z;
    return r;
}

Mat4 Mat4::scale(double sx, double sy, double sz) noexcept {
    Mat4 r;
    r.m = {sx, 0,  0,  0,
           0,  sy, 0,  0,
           0,  0,  sz, 0,
           0,  0,  0,  1};
    return r;
}

Mat4 Mat4::rotate_x(double angle_deg) noexcept {
    double rad = angle_deg * std::numbers::pi_v<double> / 180.0;
    double c = std::cos(rad);
    double s = std::sin(rad);
    Mat4 r;
    r.m = {1, 0, 0, 0,
           0, c,-s, 0,
           0, s, c, 0,
           0, 0, 0, 1};
    return r;
}

Mat4 Mat4::rotate_y(double angle_deg) noexcept {
    double rad = angle_deg * std::numbers::pi_v<double> / 180.0;
    double c = std::cos(rad);
    double s = std::sin(rad);
    Mat4 r;
    r.m = { c, 0, s, 0,
            0, 1, 0, 0,
           -s, 0, c, 0,
            0, 0, 0, 1};
    return r;
}

Mat4 Mat4::rotate_z(double angle_deg) noexcept {
    double rad = angle_deg * std::numbers::pi_v<double> / 180.0;
    double c = std::cos(rad);
    double s = std::sin(rad);
    Mat4 r;
    r.m = {c,-s, 0, 0,
           s, c, 0, 0,
           0, 0, 1, 0,
           0, 0, 0, 1};
    return r;
}

Mat4 Mat4::look_at(const Vec3& eye, const Vec3& target, const Vec3& up) noexcept {
    Vec3 f = (target - eye).normalized();
    Vec3 s = f.cross(up).normalized();
    Vec3 u = s.cross(f);

    Mat4 r;
    r.m = { s.x,  s.y,  s.z, 0,
            u.x,  u.y,  u.z, 0,
           -f.x, -f.y, -f.z, 0,
            0,    0,    0,   1};

    Mat4 t = Mat4::translate(-eye.x, -eye.y, -eye.z);
    return r * t;
}

Mat4 Mat4::perspective(double fov_deg, double aspect, double near_plane, double far_plane) noexcept {
    double fov_rad = fov_deg * std::numbers::pi_v<double> / 180.0;
    double f = 1.0 / std::tan(fov_rad / 2.0);
    double nf = 1.0 / (near_plane - far_plane);

    Mat4 r;
    r.m = {f / aspect, 0, 0,                    0,
           0,          f, 0,                    0,
           0,          0, (far_plane + near_plane) * nf, -1,
           0,          0, 2.0 * far_plane * near_plane * nf, 0};
    return r;
}

Mat4 Mat4::orthographic(double size, double aspect, double near_plane, double far_plane) noexcept {
    // Map view-space (x, y) directly to screen coordinates:
    // x in [0, size*aspect] -> ndc.x in [-1, 1]
    // y in [0, size]        -> ndc.y in [1, -1] (y down, matching PML screen coords).
    double width = size * aspect;
    double height = size;

    Mat4 r;
    r.m = {2.0 / width, 0,          0,                             0,
           0,          -2.0 / height, 0,                             0,
           0,           0,          2.0 / (near_plane - far_plane), 0,
           -1.0,        1.0,        -(far_plane + near_plane) / (far_plane - near_plane),
           1};
    return r;
}

Mat4 Mat4::operator*(const Mat4& o) const noexcept {
    Mat4 r;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            r.m[col * 4 + row] =
                m[0 * 4 + row] * o.m[col * 4 + 0] +
                m[1 * 4 + row] * o.m[col * 4 + 1] +
                m[2 * 4 + row] * o.m[col * 4 + 2] +
                m[3 * 4 + row] * o.m[col * 4 + 3];
        }
    }
    return r;
}

Vec3 Mat4::transform_point(const Vec3& p) const noexcept {
    double x = m[0]  * p.x + m[4]  * p.y + m[8]  * p.z + m[12];
    double y = m[1]  * p.x + m[5]  * p.y + m[9]  * p.z + m[13];
    double z = m[2]  * p.x + m[6]  * p.y + m[10] * p.z + m[14];
    double w = m[3]  * p.x + m[7]  * p.y + m[11] * p.z + m[15];
    if (std::abs(w) < 1e-12) w = 1.0;
    return Vec3(x / w, y / w, z / w);
}

Vec3 Mat4::transform_vector(const Vec3& v) const noexcept {
    double x = m[0]  * v.x + m[4]  * v.y + m[8]  * v.z;
    double y = m[1]  * v.x + m[5]  * v.y + m[9]  * v.z;
    double z = m[2]  * v.x + m[6]  * v.y + m[10] * v.z;
    return Vec3(x, y, z);
}

} // namespace pml
