// ==========================================================================================================================================================================================================================================═
// PML Shader Builtins — runtime shader handle management
// --------------------------------─====================================================================================================================================================================================═
// Shader compilation is delegated to the active render backend. The returned
// opaque handle is stored as an int64_t Value and can be attached to a
// GraphicObject via apply-shader!. Backends that support Shaders (e.g. skia)
// will use the handle during draw.
//
// Noise builtins create Perlin noise shaders (fractal or turbulence) that
// can be used for procedural texture generation, including seamless tiling.
// ==========================================================================================================================================================================================================================================═

#include "shader_builtins.h"

#include "pml/backend/backend.h"
#include "pml/backend/color_helpers.h"
#include "pml/backend/registry.h"
#include "environment.h"
#include "error.h"
#include "kwargs.h"

#include <cstring>
#include <format>
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

    // Builtins that accept keyword arguments must use accepts_kwargs=true
    // so the evaluator merges keyword values back into the flat args list.
    auto def_kw = [&](const std::string& name, BuiltinProcedure::BuiltinFn fn) {
        auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn), true);
        env->define(name, Value(proc));
    };

    // ---- (shader "...sksl...") ------------------------------------------------------------------------------------------------
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

    // ---- (apply-shader! graphic-object shader-handle [:coordinate-space :world]) ----
    // Return a new GraphicObject with the shader handle attached.
    // Optional :coordinate-space kwarg accepts :world (default :local).
    def_kw("apply-shader!", [](const std::vector<Value>& args,
                            Environment& /*env*/) -> Result<Value> {
        if (args.size() < 2) {
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

        // Parse optional keyword arguments
        auto kwargs = pml::kwargs::parse_kwargs(args, 2);
        auto coord_it = kwargs.find("coordinate-space");
        if (coord_it != kwargs.end()) {
            if (auto s = value_to_opt_string(coord_it->second)) {
                if (*s == "world") {
                    modified.metadata["shader_coord_space"] = Value(std::string("world"));
                }
            }
        }

        return Value(std::make_shared<GraphicObject>(std::move(modified)));
    });

    // ---- Helper: parse noise kwargs and create noise shader ------------------------─
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
            double lacunarity = 2.0;
            double persistence = 0.5;

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
            lacunarity = pml::kwargs::kw_double(kwargs, "lacunarity", lacunarity);
            persistence = pml::kwargs::kw_double(kwargs, "persistence", persistence);

            // Clamp octaves to valid range [1, 32]
            octaves = std::clamp(octaves, 1, 32);

            // Clamp lacunarity and persistence to valid ranges
            lacunarity = std::clamp(lacunarity, 1.0, 10.0);
            persistence = std::clamp(persistence, 0.0, 1.0);

            RenderBackend& backend = BackendRegistry::instance().active();
            auto handle = backend.create_noise_shader(
                type,
                static_cast<float>(freq_x),
                static_cast<float>(freq_y),
                octaves,
                static_cast<float>(seed),
                tile_w,
                tile_h,
                static_cast<float>(lacunarity),
                static_cast<float>(persistence));
            if (!handle) return std::unexpected(std::move(handle.error()));

            return Value(static_cast<int64_t>(*handle));
        };
    };

    // ---- (noise-fractal [:freq-x 0.02] [:freq-y 0.02] [:octaves 4] [:seed 0] [:tile-width 0] [:tile-height 0] [:lacunarity 2.0] [:persistence 0.5]) ----
    // Create a Perlin fractal noise shader.
    // Returns a shader handle that can be used with apply-shader!.
    // Set :tile-width and :tile-height to non-zero values for seamless tiling.
    // :lacunarity controls frequency scaling per octave (default 2.0).
    // :persistence controls amplitude scaling per octave (default 0.5).
    def_kw("noise-fractal", make_noise_builtin(RenderBackend::NoiseType::Fractal));

    // ---- (noise-turbulence [:freq-x 0.02] [:freq-y 0.02] [:octaves 4] [:seed 0] [:tile-width 0] [:tile-height 0] [:lacunarity 2.0] [:persistence 0.5]) ----
    // Create a Perlin turbulence noise shader.
    // Returns a shader handle that can be used with apply-shader!.
    // Set :tile-width and :tile-height to non-zero values for seamless tiling.
    // :lacunarity controls frequency scaling per octave (default 2.0).
    // :persistence controls amplitude scaling per octave (default 0.5).
    def_kw("noise-turbulence", make_noise_builtin(RenderBackend::NoiseType::Turbulence));

    // ---- (noise-voronoi [:cell-size 32] [:seed 0] [:jitter 0.5]) ------------------------
    // Create a Voronoi / cellular noise shader.
    // :cell-size controls feature point spacing (in pixels).
    // :jitter (0.0–1.0) controls randomness of feature point positions.
    // Returns a shader handle that can be used with apply-shader!.
    def_kw("noise-voronoi", [](const std::vector<Value>& args,
                                Environment& /*env*/) -> Result<Value> {
        double cell_size = 32.0;
        double seed = 0.0;
        double jitter = 0.5;

        auto kwargs = pml::kwargs::parse_kwargs(args, 0);
        cell_size = pml::kwargs::kw_double(kwargs, "cell-size", cell_size);
        seed = pml::kwargs::kw_double(kwargs, "seed", seed);
        jitter = pml::kwargs::kw_double(kwargs, "jitter", jitter);

        cell_size = std::clamp(cell_size, 1.0, 1024.0);
        jitter = std::clamp(jitter, 0.0, 1.0);

        // Generate Voronoi SkSL
        std::string sksl = R"(
            uniform float u_cell_size;
            uniform float u_seed;
            uniform float u_jitter;

            float2 voronoi_hash(float2 p) {
                p += float2(u_seed * 0.771, u_seed * 0.453);
                float3 p3 = fract(float3(p.xyx) * 0.1031);
                p3 += dot(p3, p3.yzx + 33.33);
                return fract(float2(p3.x + p3.y, p3.y + p3.z));
            }

            half4 main(float2 xy) {
                float2 p = xy / u_cell_size;
                float2 i = floor(p);
                float2 f = fract(p);

                float min_dist = 1.0;
                float2 min_cell = float2(0.0, 0.0);

                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        float2 neighbor = float2(float(dx), float(dy));
                        float2 cell = i + neighbor;
                        float2 ch = voronoi_hash(cell);
                        float2 jit = (ch - 0.5) * 2.0 * u_jitter;
                        float2 point = neighbor + jit;
                        float2 diff = point - f;
                        float d = dot(diff, diff);
                        if (d < min_dist) {
                            min_dist = d;
                            min_cell = cell;
                        }
                    }
                }

                float v = min_dist;
                return half4(v, v, v, 1.0);
            }
        )";

        RenderBackend& backend = BackendRegistry::instance().active();

        // Compile
        auto effect_handle = backend.compile_shader(sksl);
        if (!effect_handle) return std::unexpected(std::move(effect_handle.error()));

        // Build uniform data: cell_size (float), seed (float), jitter (float) = 12 bytes
        std::vector<uint8_t> uniform_data(12);
        float f_cell = static_cast<float>(cell_size);
        float f_seed = static_cast<float>(seed);
        float f_jit  = static_cast<float>(jitter);
        std::memcpy(uniform_data.data(),       &f_cell, 4);
        std::memcpy(uniform_data.data() + 4,   &f_seed, 4);
        std::memcpy(uniform_data.data() + 8,   &f_jit,  4);

        auto preshader = backend.create_shader_with_uniforms(
            *effect_handle, uniform_data);
        if (!preshader) return std::unexpected(std::move(preshader.error()));

        return Value(static_cast<int64_t>(*preshader));
    });

    // ---- (noise-warp base-handle warp-handle [:amount 30.0] [:freq 0.01]) ----
    // Domain warp: use warp-handle noise to distort the sampling coordinates
    // of base-handle noise, creating flowing/fractured organic textures.
    // :amount controls distortion strength in pixels.
    // :freq controls the frequency at which the warp field is sampled.
    def_kw("noise-warp", [](const std::vector<Value>& args,
                             Environment& /*env*/) -> Result<Value> {
        if (args.empty()) {
            return std::unexpected(arity_error(
                SourceLocation{}, 2, static_cast<int>(args.size())));
        }

        // First two args are positional: base-handle, warp-handle
        auto get_handle = [](const Value& v) -> std::optional<uint64_t> {
            if (v.is_int()) return static_cast<uint64_t>(v.int_val());
            if (v.is_double()) return static_cast<uint64_t>(v.double_val());
            return std::nullopt;
        };

        auto base_h = get_handle(args[0]);
        if (!base_h || *base_h == 0) {
            return std::unexpected(type_error(
                "noise-warp: first argument must be a shader handle"));
        }
        auto warp_h = get_handle(args[1]);
        if (!warp_h || *warp_h == 0) {
            return std::unexpected(type_error(
                "noise-warp: second argument must be a shader handle"));
        }

        auto kwargs = pml::kwargs::parse_kwargs(args, 2);
        double amount = pml::kwargs::kw_double(kwargs, "amount", 30.0);
        double freq   = pml::kwargs::kw_double(kwargs, "freq", 0.01);

        // Domain warp SkSL: uses two child shaders (base, warp_field)
        std::string sksl = R"(
            uniform shader child_0;
            uniform shader child_1;
            uniform float u_amount;
            uniform float u_freq;
            half4 main(float2 xy) {
                half4 w = child_1.eval(xy * u_freq);
                float2 offset = (w.xy * 2.0 - 1.0) * u_amount;
                return child_0.eval(xy + offset);
            }
        )";

        RenderBackend& backend = BackendRegistry::instance().active();

        // Uniform data: amount (float), freq (float) = 8 bytes
        std::vector<uint8_t> uniform_data(8);
        float f_amt = static_cast<float>(amount);
        float f_frq = static_cast<float>(freq);
        std::memcpy(uniform_data.data(),     &f_amt, 4);
        std::memcpy(uniform_data.data() + 4, &f_frq, 4);

        auto result = backend.compose_with_child_shaders(
            {*base_h, *warp_h}, sksl, uniform_data);
        if (!result) return std::unexpected(std::move(result.error()));

        return Value(static_cast<int64_t>(*result));
    });

    // ---- (noise-blend noise-a noise-b [:mode 'gradient] [:direction 'vertical] [:weight 0.5]) ----
    // Blend two noise shaders with a transition.
    // :mode controls transition type:
    //   'gradient  — linear gradient from noise A to noise B
    //   'horizontal — horizontal gradient (left-to-right: A→B)
    //   'vertical   — vertical gradient (top-to-bottom: A→B)
    // :weight controls the midpoint of the transition (0.0–1.0).
    // Returns a composed shader handle.
    def_kw("noise-blend", [](const std::vector<Value>& args,
                              Environment& /*env*/) -> Result<Value> {
        if (args.empty()) {
            return std::unexpected(arity_error(
                SourceLocation{}, 2, static_cast<int>(args.size())));
        }

        auto get_handle = [](const Value& v) -> std::optional<uint64_t> {
            if (v.is_int()) return static_cast<uint64_t>(v.int_val());
            if (v.is_double()) return static_cast<uint64_t>(v.double_val());
            return std::nullopt;
        };

        auto handle_a = get_handle(args[0]);
        if (!handle_a || *handle_a == 0) {
            return std::unexpected(type_error(
                "noise-blend: first argument must be a shader handle"));
        }
        auto handle_b = get_handle(args[1]);
        if (!handle_b || *handle_b == 0) {
            return std::unexpected(type_error(
                "noise-blend: second argument must be a shader handle"));
        }

        auto kwargs = pml::kwargs::parse_kwargs(args, 2);
        std::string mode = pml::kwargs::kw_string(kwargs, "mode", "gradient");
        std::string direction = pml::kwargs::kw_string(kwargs, "direction", "vertical");
        double weight = pml::kwargs::kw_double(kwargs, "weight", 0.5);
        weight = std::clamp(weight, 0.0, 1.0);

        // Generate blend SkSL with the selected mode
        std::string blend_expr;
        if (mode == "horizontal") {
            blend_expr = "float t = xy.x / 512.0;";
        } else if (mode == "gradient") {
            blend_expr = "float t = xy.y / 512.0;";
        } else {  // default vertical
            blend_expr = "float t = xy.y / 512.0;";
        }
        blend_expr += " t = clamp((t - " +
                      std::to_string(weight - 0.1) + ") / 0.2, 0.0, 1.0);";

        std::string sksl = R"(
            uniform shader child_0;
            uniform shader child_1;
            uniform float u_weight;
            half4 main(float2 xy) {
                half4 a = child_0.eval(xy);
                half4 b = child_1.eval(xy);
        )";
        sksl += "            " + blend_expr + "\n";
        sksl += R"(
                return mix(a, b, t);
            }
        )";

        RenderBackend& backend = BackendRegistry::instance().active();

        std::vector<uint8_t> uniform_data(4);
        float f_w = static_cast<float>(weight);
        std::memcpy(uniform_data.data(), &f_w, 4);

        auto result = backend.compose_with_child_shaders(
            {*handle_a, *handle_b}, sksl, uniform_data);
        if (!result) return std::unexpected(std::move(result.error()));

        return Value(static_cast<int64_t>(*result));
    });

    // ---- (make-uniforms float1 float2 ...) ------------------------------------------------
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

    // ---- (apply-uniforms shader-handle uniform-data-string) --------------------─
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

    // ---- (uniform-float shader-handle name value) ------------------------------------─
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

    // ---- (quantize-noise noise-handle :levels '((threshold "color") ...)) --------
    // Quantize a continuous noise shader into discrete color bands.
    //
    // Arguments:
    //   noise-handle  — shader handle from noise-fractal or noise-turbulence
    //   :levels       — list of (threshold color) pairs, sorted by threshold
    //                   Each threshold is a float 0–1; the last should be 1.0.
    //                   Each color is a CSS color string.
    //
    // Returns a new shader handle that outputs the quantized colors.
    //
    // Example:
    //   (define n (noise-fractal :seed 42 :octaves 6))
    //   (define q (quantize-noise n
    //                :levels '((0.33 "#2d5a2d") (0.66 "#6abf4a") (1.0 "#4a9bd6"))))
    def_kw("quantize-noise", [](const std::vector<Value>& args,
                              Environment& /*env*/) -> Result<Value> {
        if (args.empty()) {
            return std::unexpected(arity_error(
                SourceLocation{}, 1, static_cast<int>(args.size())));
        }

        // 1. Extract noise shader handle from first positional arg
        uint64_t noise_handle = 0;
        if (args[0].is_int()) {
            noise_handle = static_cast<uint64_t>(args[0].int_val());
        } else if (args[0].is_double()) {
            noise_handle = static_cast<uint64_t>(args[0].double_val());
        } else {
            return std::unexpected(type_error(
                "quantize-noise: first argument must be a shader handle (integer), got " +
                value_to_string(args[0])));
        }
        if (noise_handle == 0) {
            return std::unexpected(type_error(
                "quantize-noise: invalid shader handle (0)"));
        }

        // 2. Parse kwargs
        auto kwargs = pml::kwargs::parse_kwargs(args, 1);
        auto levels_it = kwargs.find("levels");
        if (levels_it == kwargs.end()) {
            return std::unexpected(type_error(
                "quantize-noise: :levels keyword argument is required"));
        }

        const auto* levels_list = levels_it->second.as_list();
        if (!levels_list || !*levels_list) {
            return std::unexpected(type_error(
                "quantize-noise: :levels must be a list of (threshold color) pairs"));
        }
        const auto& entries = (*levels_list)->elements;
        if (entries.empty()) {
            return std::unexpected(type_error(
                "quantize-noise: :levels must have at least one entry"));
        }

        // 3. Parse each (threshold color) entry
        struct Level {
            double threshold;
            uint32_t argb;  // 0xAARRGGBB
        };
        std::vector<Level> levels;
        levels.reserve(entries.size());

        for (size_t i = 0; i < entries.size(); ++i) {
            const auto* entry_list = entries[i].as_list();
            if (!entry_list || !*entry_list) {
                return std::unexpected(type_error(
                    std::format("quantize-noise: entry {} in :levels is not a list", i)));
            }
            const auto& elems = (*entry_list)->elements;
            if (elems.size() < 2) {
                return std::unexpected(type_error(
                    std::format("quantize-noise: entry {} in :levels needs at least "
                                "2 elements (threshold color)", i)));
            }

            // Parse threshold (must be a number)
            double threshold = 0.0;
            if (elems[0].is_double()) {
                threshold = elems[0].double_val();
            } else if (elems[0].is_int()) {
                threshold = static_cast<double>(elems[0].int_val());
            } else {
                return std::unexpected(type_error(
                    std::format("quantize-noise: entry {} threshold must be a number", i)));
            }
            if (threshold < 0.0 || threshold > 1.0) {
                return std::unexpected(type_error(
                    std::format("quantize-noise: entry {} threshold {} out of range [0,1]",
                                i, threshold)));
            }

            // Parse color string
            auto color_str = pml::kwargs::value_to_opt_string(elems[1]);
            if (!color_str) {
                return std::unexpected(type_error(
                    std::format("quantize-noise: entry {} color must be a string", i)));
            }
            auto parsed = parse_color(*color_str);
            if (!parsed) {
                return std::unexpected(type_error(
                    std::format("quantize-noise: entry {} invalid color '{}'",
                                i, *color_str)));
            }

            levels.push_back({threshold, *parsed});
        }

        // 4. Sort levels by threshold ascending
        std::sort(levels.begin(), levels.end(),
                  [](const Level& a, const Level& b) {
                      return a.threshold < b.threshold;
                  });

        // 5. Generate SkSL wrapper source with if-else quantization chain
        //
        //    SkSL half4 constructor takes float args in [0,1].
        //    parse_color returns 0xAARRGGBB → extract R,G,B,A /255.0
        auto color_to_half4 = [](uint32_t argb) -> std::string {
            int a = static_cast<int>((argb >> 24) & 0xFF);
            int r = static_cast<int>((argb >> 16) & 0xFF);
            int g = static_cast<int>((argb >> 8) & 0xFF);
            int b = static_cast<int>(argb & 0xFF);
            return std::format("half4({:.6f}, {:.6f}, {:.6f}, {:.6f})",
                               r / 255.0, g / 255.0, b / 255.0, a / 255.0);
        };

        std::string sksl;
        sksl += "uniform shader src;\n";
        sksl += "half4 main(float2 xy) {\n";
        sksl += "  half4 n = src.eval(xy);\n";
        sksl += "  float v = (n.r + n.g + n.b) * 0.33333333;\n";

        for (size_t i = 0; i < levels.size(); ++i) {
            std::string color_expr = color_to_half4(levels[i].argb);
            if (i < levels.size() - 1) {
                sksl += std::format("  if (v < {:.6f}) return {};\n",
                                    levels[i].threshold, color_expr);
            } else {
                // Last level: else (no condition)
                sksl += std::format("  return {};\n", color_expr);
            }
        }
        sksl += "}\n";

        // 6. Compose with backend
        RenderBackend& backend = BackendRegistry::instance().active();
        auto new_handle = backend.compose_with_child_shader(noise_handle, sksl);
        if (!new_handle) return std::unexpected(std::move(new_handle.error()));

        return Value(static_cast<int64_t>(*new_handle));
    });
}

}  // namespace pml
