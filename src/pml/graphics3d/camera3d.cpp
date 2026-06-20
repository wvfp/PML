#include "camera3d.h"

namespace pml {

Camera3D& current_camera() noexcept {
    static Camera3D cam;
    return cam;
}

void reset_camera() noexcept {
    current_camera() = Camera3D{};
}

} // namespace pml
