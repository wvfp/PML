// ═══════════════════════════════════════════════════════════════════════════════
// PML Shader Builtins — runtime shader handle management
// ─────────────────═════════════════════════════════════════════════════════════
// Shader compilation is delegated to the active render backend. The returned
// opaque handle is stored as an int64_t Value and can be attached to a
// GraphicObject via apply-shader!. Backends that support Shaders (e.g. skia)
// will use the handle during draw.
// ═══════════════════════════════════════════════════════════════════════════════

#include "shader_builtins.h"

#include "pml/backend/backend.h"
#include "pml/backend/registry.h"
#include "environment.h"
#include "error.h"
#include "kwargs.h"
#include "pml/graphics/graphic_object.h"
#include "types.h"

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace pml {

using pml::kwargs::value_to_opt_string;

void register_shader_builtins(std::shared_ptr<Environment> env) {
    if (!env) return;

    auto def = [&](const std::string& name, BuiltinProcedure::BuiltinFn fn) {
        auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn));
        env->define(name, Value(proc));
    };

    // ── (shader "...sksl...") ────────────────────────────────────────────────
    // Compile a SkSL/GLSL-like shader source string and return a shader handle.
    def("shader", [](const std::vector<Value>& args,
                     Environment& /*env*/) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(arity_error(
                SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        auto source = value_to_opt_string(args[0]);
        if (!source) {
            return std::unexpected(type_error(
                "shader: expected a string source, got " +
                value_to_string(args[0])));
        }

        RenderBackend& backend = BackendRegistry::instance().active();
        auto handle = backend.compile_shader(*source);
        if (!handle) return std::unexpected(std::move(handle.error()));

        return Value(static_cast<int64_t>(*handle));
    });

    // ── (apply-shader! graphic-object shader-handle) ─────────────────────────
    // Return a new GraphicObject with the shader handle attached.
    def("apply-shader!", [](const std::vector<Value>& args,
                            Environment& /*env*/) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(arity_error(
                SourceLocation{}, 2, static_cast<int>(args.size())));
        }

        const auto* obj = args[0].as_graphic_object();
        if (!obj || !*obj) {
            return std::unexpected(type_error(
                "apply-shader!: first argument must be a GraphicObject"));
        }

        int64_t handle = 0;
        if (args[1].is_int()) {
            handle = args[1].int_val();
        } else if (args[1].is_double()) {
            handle = static_cast<int64_t>(args[1].double_val());
        } else {
            return std::unexpected(type_error(
                "apply-shader!: second argument must be a shader handle"));
        }

        auto modified = (*obj)->with_param("shader", Value(handle));
        return Value(std::make_shared<GraphicObject>(std::move(modified)));
    });
}

}  // namespace pml
