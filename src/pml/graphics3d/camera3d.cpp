#include "camera3d.h"

#include "pml/api/context.h"

namespace pml {

Camera3D& current_camera() noexcept {
    auto* ctx = PMLContext::current_ptr();
    if (ctx && ctx->camera) {
        return *ctx->camera;
    }
    thread_local Camera3D fallback;
    return fallback;
}

void reset_camera() noexcept {
    current_camera() = Camera3D{};
}

} // namespace pml
