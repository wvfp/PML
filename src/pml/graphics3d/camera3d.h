#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// Camera3D — 3D camera with perspective / orthographic projection
// ═══════════════════════════════════════════════════════════════════════════════

#include "mat4.h"
#include "vec3.h"

namespace pml {

struct Camera3D {
    enum class Projection : uint8_t { Perspective, Orthographic };

    Vec3 position{0.0, 0.0, 300.0};
    Vec3 target{0.0, 0.0, 0.0};
    Vec3 up{0.0, 1.0, 0.0};
    Projection projection{Projection::Orthographic};
    double fov{45.0};
    double ortho_size{200.0};
    double near_plane{1.0};
    double far_plane{1000.0};
    double aspect{1.0};
    bool backface_culling{true};
    bool user_ortho_size{false};  ///< True when :size was explicitly supplied.

    [[nodiscard]] Mat4 view_matrix() const noexcept {
        return Mat4::look_at(position, target, up);
    }

    [[nodiscard]] Mat4 projection_matrix() const noexcept {
        if (projection == Projection::Orthographic) {
            return Mat4::orthographic(ortho_size, aspect, near_plane, far_plane);
        }
        return Mat4::perspective(fov, aspect, near_plane, far_plane);
    }
};

/// Global camera singleton (similar to g_current_canvas).
[[nodiscard]] Camera3D& current_camera() noexcept;

/// Reset the global camera to defaults.
void reset_camera() noexcept;

} // namespace pml
