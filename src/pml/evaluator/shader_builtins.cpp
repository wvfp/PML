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
#include "pml/graphics/graphic_object.h"
#include "types.h"

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace pml {

namespace {

[[nodiscard]] std::optional<std::string> value_to_opt_string(const Value& v)
{
    return std::visit([](const auto& arg) -> std::optional<std::string> {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return arg;
        } else if constexpr (std::is_same_v<T, Symbol>) {
            return arg.name;
        } else if constexpr (std::is_same_v<T, Keyword>) {
            return arg.name;
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::to_string(arg);
        }
        return std::nullopt;
    }, v);
}

}  // anonymous namespace

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

        const auto* obj = std::get_if<std::shared_ptr<GraphicObject>>(&args[0]);
        if (!obj || !*obj) {
            return std::unexpected(type_error(
                "apply-shader!: first argument must be a GraphicObject"));
        }

        int64_t handle = 0;
        if (std::holds_alternative<int64_t>(args[1])) {
            handle = std::get<int64_t>(args[1]);
        } else if (std::holds_alternative<double>(args[1])) {
            handle = static_cast<int64_t>(std::get<double>(args[1]));
        } else {
            return std::unexpected(type_error(
                "apply-shader!: second argument must be a shader handle"));
        }

        auto modified = (*obj)->with_param("shader", Value(handle));
        return Value(std::make_shared<GraphicObject>(std::move(modified)));
    });
}

}  // namespace pml
