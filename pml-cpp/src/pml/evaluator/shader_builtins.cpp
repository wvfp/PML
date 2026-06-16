// ═══════════════════════════════════════════════════════════════════════════════
// PML Shader Builtins — stub implementation (Skia not available)
// ───────────────────────────────────────────────────────────────────────────────
// Shader operations require a Skia rendering backend which is not available
// in this build.  Each builtin returns a clear ResourceError so callers
// get a descriptive message instead of a missing-symbol crash.
// ═══════════════════════════════════════════════════════════════════════════════

#include "shader_builtins.h"

#include "error.h"
#include "types.h"

#include <memory>
#include <string>
#include <vector>

namespace pml {

namespace {

/// Error message returned by all shader stubs.
constexpr const char* kSkiaUnavailable =
    "Shader support requires Skia backend (not available in this build)";

}  // anonymous namespace

void register_shader_builtins(std::shared_ptr<Environment> env) {
    auto def = [&](const std::string& name, BuiltinProcedure::BuiltinFn fn) {
        auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn));
        env->define(name, Value(proc));
    };

    // ── (shader "...glsl...") ────────────────────────────────────────────────
    // Compile a GLSL shader source string and return a shader handle.
    // Stub: always returns a ResourceError.
    def("shader", [](const std::vector<Value>&, Environment&) -> Result<Value> {
        return std::unexpected(resource_error(std::string(kSkiaUnavailable)));
    });

    // ── (apply-shader! graphic-object shader-handle) ─────────────────────────
    // Apply a compiled shader to a graphic object (side-effecting).
    // Stub: always returns a ResourceError.
    def("apply-shader!", [](const std::vector<Value>&, Environment&) -> Result<Value> {
        return std::unexpected(resource_error(std::string(kSkiaUnavailable)));
    });
}

}  // namespace pml
