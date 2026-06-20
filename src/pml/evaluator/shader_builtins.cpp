// ═══════════════════════════════════════════════════════════════════════════════
// PML Shader Builtins — runtime shader handle management
// ─────────────────═════════════════════════════════════════════════════════════
// Shader compilation is delegated to the active render backend. The returned
// opaque handle is stored as an int64_t Value and can be attached to a
// GraphicObject via apply-shader!. Backends that support Shaders (e.g. skia)
// will use the handle during draw.
//
// Noise builtins create Perlin noise shaders (fractal or turbulence) that
// can be used for procedural texture generation, including seamless tiling.
// ═══════════════════════════════════════════════════════════════════════════════

#include "shader_builtins.h"

#include "pml/backend/backend.h"
#include "pml/backend/registry.h"
#include "environment.h"
#include "error.h"
#include "kwargs.h"
#include "pml/graphics/graphic_object.h"
#include "types.h"

#include <algorithm>
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

    // ── Helper: parse noise kwargs and create noise shader ─────────────
    auto make_noise_builtin = [&](RenderBackend::NoiseType type) {
        return [type](const std::vector<Value>& args,
                       Environment& /*env*/) -> Result<Value> {
            // Default values
            double freq_x = 0.02;
            double freq_y = 0.02;
            int octaves = 4;
            double seed = 0.0;
            int tile_w = 0;
            int tile_h = 0;

            // Parse keyword arguments (all args are treated as kwargs)
            auto kwargs = pml::kwargs::parse_kwargs(args, 0);
            freq_x = pml::kwargs::kw_double(kwargs, "freq-x", freq_x);
            freq_x = pml::kwargs::kw_double(kwargs, "frequency-x", freq_x);
            freq_y = pml::kwargs::kw_double(kwargs, "freq-y", freq_y);
            freq_y = pml::kwargs::kw_double(kwargs, "frequency-y", freq_y);
            octaves = pml::kwargs::kw_int(kwargs, "octaves", octaves);
            seed = pml::kwargs::kw_double(kwargs, "seed", seed);
            tile_w = pml::kwargs::kw_int(kwargs, "tile-width", tile_w);
            tile_w = pml::kwargs::kw_int(kwargs, "tile-w", tile_w);
            tile_h = pml::kwargs::kw_int(kwargs, "tile-height", tile_h);
            tile_h = pml::kwargs::kw_int(kwargs, "tile-h", tile_h);

            // Clamp octaves to valid range [1, 255]
            octaves = std::clamp(octaves, 1, 255);

            RenderBackend& backend = BackendRegistry::instance().active();
            auto handle = backend.create_noise_shader(
                type,
                static_cast<float>(freq_x),
                static_cast<float>(freq_y),
                octaves,
                static_cast<float>(seed),
                tile_w,
                tile_h);
            if (!handle) return std::unexpected(std::move(handle.error()));

            return Value(static_cast<int64_t>(*handle));
        };
    };

    // ── (noise-fractal [:freq-x 0.02] [:freq-y 0.02] [:octaves 4] [:seed 0] [:tile-width 0] [:tile-height 0]) ──
    // Create a Perlin fractal noise shader.
    // Returns a shader handle that can be used with apply-shader!.
    // Set :tile-width and :tile-height to non-zero values for seamless tiling.
    def("noise-fractal", make_noise_builtin(RenderBackend::NoiseType::Fractal));

    // ── (noise-turbulence [:freq-x 0.02] [:freq-y 0.02] [:octaves 4] [:seed 0] [:tile-width 0] [:tile-height 0]) ──
    // Create a Perlin turbulence noise shader.
    // Returns a shader handle that can be used with apply-shader!.
    // Set :tile-width and :tile-height to non-zero values for seamless tiling.
    def("noise-turbulence", make_noise_builtin(RenderBackend::NoiseType::Turbulence));

    // ── (make-uniforms float1 float2 ...) ────────────────────────
    // Create a raw byte vector from floating-point values for SkSL uniforms.
    // SkSL uniforms are laid out as 4-byte aligned floats.
    // Example: (make-uniforms 0.5 1.0) creates 8 bytes of uniform data.
    def("make-uniforms", [](const std::vector<Value>& args,
                            Environment& /*env*/) -> Result<Value> {
        if (args.empty()) {
            return std::unexpected(arity_error(
                SourceLocation{}, 1, 0));
        }

        std::vector<uint8_t> data;
        for (const auto& arg : args) {
            float val = 0.0f;
            if (arg.is_double()) {
                val = static_cast<float>(arg.double_val());
            } else if (arg.is_int()) {
                val = static_cast<float>(arg.int_val());
            } else {
                return std::unexpected(type_error(
                    "make-uniforms: expected number, got " +
                    value_to_string(arg)));
            }

            // Append 4 bytes (little-endian float)
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
            data.insert(data.end(), bytes, bytes + sizeof(float));
        }

        // Return as a string (byte vector encoded as string for PML)
        std::string data_str(data.begin(), data.end());
        return Value(data_str);
    });

    // ── (apply-uniforms shader-handle uniform-data-string) ───────────
    // Create a new shader handle with uniform data bound.
    // uniform-data-string is a raw byte string from make-uniforms.
    def("apply-uniforms", [](const std::vector<Value>& args,
                             Environment& /*env*/) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(arity_error(
                SourceLocation{}, 2, static_cast<int>(args.size())));
        }

        int64_t handle = 0;
        if (args[0].is_int()) {
            handle = args[0].int_val();
        } else if (args[0].is_double()) {
            handle = static_cast<int64_t>(args[0].double_val());
        } else {
            return std::unexpected(type_error(
                "apply-uniforms: first argument must be a shader handle"));
        }

        auto uniform_str = value_to_opt_string(args[1]);
        if (!uniform_str) {
            return std::unexpected(type_error(
                "apply-uniforms: second argument must be a uniform data string"));
        }

        std::vector<uint8_t> uniform_data(uniform_str->begin(), uniform_str->end());

        RenderBackend& backend = BackendRegistry::instance().active();
        auto new_handle = backend.create_shader_with_uniforms(
            static_cast<uint64_t>(handle), uniform_data);
        if (!new_handle) return std::unexpected(std::move(new_handle.error()));

        return Value(static_cast<int64_t>(*new_handle));
    });

    // ── (uniform-float shader-handle name value) ───────────────────
    // Convenience: set a single float uniform on a shader.
    // Returns a new shader handle with the uniform bound.
    // This is a simplified interface that recreates the shader with new uniforms.
    // For complex uniforms, use make-uniforms + apply-uniforms.
    def("uniform-float", [](const std::vector<Value>& args,
                              Environment& /*env*/) -> Result<Value> {
        if (args.size() != 3) {
            return std::unexpected(arity_error(
                SourceLocation{}, 3, static_cast<int>(args.size())));
        }

        int64_t handle = 0;
        if (args[0].is_int()) {
            handle = args[0].int_val();
        } else if (args[0].is_double()) {
            handle = static_cast<int64_t>(args[0].double_val());
        } else {
            return std::unexpected(type_error(
                "uniform-float: first argument must be a shader handle"));
        }

        // Ignore name for now (would need shader reflection)
        float val = 0.0f;
        if (args[2].is_double()) {
            val = static_cast<float>(args[2].double_val());
        } else if (args[2].is_int()) {
            val = static_cast<float>(args[2].int_val());
        } else {
            return std::unexpected(type_error(
                "uniform-float: third argument must be a number"));
        }

        std::vector<uint8_t> data(sizeof(float));
        std::memcpy(data.data(), &val, sizeof(float));

        RenderBackend& backend = BackendRegistry::instance().active();
        auto new_handle = backend.create_shader_with_uniforms(
            static_cast<uint64_t>(handle), data);
        if (!new_handle) return std::unexpected(std::move(new_handle.error()));

        return Value(static_cast<int64_t>(*new_handle));
    });
}

}  // namespace pml
