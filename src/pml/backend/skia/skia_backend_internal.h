#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Skia render backend — shared internal declarations
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Shared types, helpers, and the SkiaBackend class declaration used by all
// skia_backend_*.cpp compilation units.
// ==========================================================================================================================================================================================================================================═

#include "pml/backend/backend.h"
#include "pml/backend/registry.h"
#include "pml/backend/color_helpers.h"
#include "pml/backend/gif/gif_exporter.h"
#include "pml/graphics/objects.h"
#include "pml/graphics/rough.h"
#include "pml/graphics/rough_filler.h"
#include "pml/graphics/transform.h"
#include "pml/graphics3d/camera3d.h"
#include "pml/graphics3d/mesh3d.h"
#include "pml/graphics3d/transform3d.h"
#include "pml/layer/blend_mode_skia.h"

#include <skia.h>
#include <codec/SkCodec.h>
#include <effects/SkPerlinNoiseShader.h>
#include <effects/SkGradient.h>

#include <png.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// SkiaSurface
// ==========================================================================================================================================================================================================================================═

struct SkiaSurface final : Surface {
    // Bitmap must outlive `surface`, therefore declare it first.
    SkBitmap bitmap;
    sk_sp<SkSurface> surface;

    SkiaSurface(int w, int h, uint32_t bg_color)
    {
        width = w;
        height = h;
        bitmap.allocN32Pixels(w, h);
        bitmap.eraseColor(static_cast<SkColor>(bg_color));
        surface = SkSurfaces::WrapPixels(
            bitmap.info(), bitmap.getPixels(), bitmap.rowBytes());
    }

    // Construct from an already-decoded SkBitmap.
    explicit SkiaSurface(SkBitmap decoded)
    {
        width = decoded.width();
        height = decoded.height();
        bitmap = std::move(decoded);
        surface = SkSurfaces::WrapPixels(
            bitmap.info(), bitmap.getPixels(), bitmap.rowBytes());
    }
};

// ==========================================================================================================================================================================================================================================═
// Color helpers
// ==========================================================================================================================================================================================================================================═

inline SkColor to_sk_color(uint32_t argb)
{
    // parse_color() returns 0xAARRGGBB, which matches SkColor layout.
    return static_cast<SkColor>(argb);
}

inline std::optional<SkColor> parse_sk_color(const std::string& s)
{
    auto c = parse_color(s);
    if (!c) return std::nullopt;
    return to_sk_color(*c);
}

// ==========================================================================================================================================================================================================================================═
// Paint helpers
// ==========================================================================================================================================================================================================================================═

inline void configure_fill_paint(
    SkPaint& paint,
    const std::optional<std::string>& fill,
    double /*stroke_width*/,
    sk_sp<SkShader> shader = nullptr,
    std::optional<BlendMode> blend_mode = std::nullopt,
    float opacity = 1.0f,
    const std::optional<Gradient>* fill_gradient = nullptr)
{
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);

    // Gradient fill takes highest precedence
    bool has_gradient = false;
    if (fill_gradient && *fill_gradient) {
        const Gradient& g = **fill_gradient;
        const size_t n = g.stops.size();
        std::vector<SkColor4f> colors;
        std::vector<float> positions;
        colors.reserve(n);
        positions.reserve(n);
        for (const auto& stop : g.stops) {
            auto skc = parse_sk_color(stop.color);
            colors.push_back(skc.has_value()
                ? SkColor4f::FromColor(*skc)
                : SkColors::kTransparent);
            positions.push_back(static_cast<float>(
                std::clamp(stop.position, 0.0, 1.0)));
        }

        SkGradient::Colors grad_colors(
            SkSpan(colors.data(), n),
            SkSpan(positions.data(), n),
            SkTileMode::kClamp);

        sk_sp<SkShader> grad_shader;
        if (g.type == GradientType::Radial) {
            grad_shader = SkShaders::RadialGradient(
                SkPoint::Make(static_cast<float>(g.cx),
                              static_cast<float>(g.cy)),
                static_cast<float>(g.r),
                SkGradient(grad_colors, SkGradient::Interpolation{}));
        } else {
            SkPoint pts[2] = {
                SkPoint::Make(static_cast<float>(g.x1), static_cast<float>(g.y1)),
                SkPoint::Make(static_cast<float>(g.x2), static_cast<float>(g.y2)),
            };
            grad_shader = SkShaders::LinearGradient(pts, SkGradient(grad_colors, SkGradient::Interpolation{}));
        }
        if (grad_shader) {
            paint.setShader(std::move(grad_shader));
            paint.setColor(SK_ColorWHITE);
            has_gradient = true;
        }
    }

    if (!has_gradient) {
        if (shader) {
            paint.setShader(std::move(shader));
            paint.setColor(SK_ColorWHITE);
        } else if (fill) {
            if (auto c = parse_sk_color(*fill)) {
                paint.setColor(*c);
            } else {
                paint.setColor(SK_ColorTRANSPARENT);
            }
        } else {
            paint.setColor(SK_ColorTRANSPARENT);
        }
    }

    if (blend_mode.has_value()) {
        paint.setBlendMode(to_skia_blend_mode(*blend_mode));
    }
    if (opacity < 1.0f) {
        paint.setAlphaf(paint.getColor4f().fA * opacity);
    }
}

inline void configure_stroke_paint(
    SkPaint& paint,
    const std::optional<std::string>& stroke,
    double stroke_width,
    std::optional<BlendMode> blend_mode = std::nullopt,
    float opacity = 1.0f)
{
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(static_cast<SkScalar>(stroke_width));
    if (stroke) {
        if (auto c = parse_sk_color(*stroke)) {
            paint.setColor(*c);
        } else {
            paint.setColor(SK_ColorTRANSPARENT);
        }
    } else {
        paint.setColor(SK_ColorTRANSPARENT);
    }
    if (blend_mode.has_value()) {
        paint.setBlendMode(to_skia_blend_mode(*blend_mode));
    }
    if (opacity < 1.0f) {
        paint.setAlphaf(paint.getColor4f().fA * opacity);
    }
}

// ==========================================================================================================================================================================================================================================═
// Param extraction
// ==========================================================================================================================================================================================================================================═

[[nodiscard]] inline double get_double(
    const Params& params,
    ParamKey key,
    double default_val)
{
    const Value* v = params.find(key);
    if (!v) return default_val;
    if (v->is_int()) {
        return static_cast<double>(v->int_val());
    }
    if (v->is_double()) {
        return v->double_val();
    }
    return default_val;
}

[[nodiscard]] inline std::string get_string(
    const Params& params,
    ParamKey key,
    const std::string& default_val)
{
    const Value* v = params.find(key);
    if (!v) return default_val;
    if (const auto* s = v->as_string()) {
        return *s;
    }
    return default_val;
}

[[nodiscard]] inline std::optional<SkRect> parse_rect_value(const Value& v)
{
    const auto* lst = v.as_list();
    if (!lst || !*lst) return std::nullopt;
    const auto& elems = (*lst)->elements;
    if (elems.size() < 4) return std::nullopt;

    double vals[4] = {0.0, 0.0, 0.0, 0.0};
    for (int i = 0; i < 4; ++i) {
        if (elems[i].is_int()) {
            vals[i] = static_cast<double>(elems[i].int_val());
        } else if (elems[i].is_double()) {
            vals[i] = elems[i].double_val();
        } else {
            return std::nullopt;
        }
    }
    return SkRect::MakeXYWH(
        static_cast<SkScalar>(vals[0]),
        static_cast<SkScalar>(vals[1]),
        static_cast<SkScalar>(vals[2]),
        static_cast<SkScalar>(vals[3]));
}

// ==========================================================================================================================================================================================================================================═
// Forward declarations for free functions in skia_backend_draw.cpp
// ==========================================================================================================================================================================================================================================═

using ShaderLookup = std::function<sk_sp<SkShader>(int64_t)>;

/// Draw function signature for the shape registry.
using DrawFn = std::function<Result<void>(SkCanvas*, const GraphicObject&,
                                          sk_sp<SkShader>, const ShaderLookup&)>;

/// Register a draw function for a shape type.
void register_draw_fn(const std::string& shape_type, DrawFn fn);

Result<void> draw_object(SkCanvas* canvas, const GraphicObject& obj,
                         sk_sp<SkShader> parent_shader,
                         const ShaderLookup& lookup);

Result<void> draw_mesh3d(SkCanvas* canvas, const GraphicObject& obj,
                         const ShaderLookup& lookup);

// ---- Rough-style draw functions --------------------------------------------------------------------------------─
// Declared in skia_backend_internal.h, implemented in skia_backend_draw.cpp.

/// Extract RoughStyleParams + RoughRandom from a GraphicObject's metadata.
/// Returns false if no rough params are present (shape should render normally).
bool extract_rough_params(const GraphicObject& obj,
                          RoughStyleParams& params,
                          RoughRandom& rng);

/// Draw a rough circle (perturbed stroke + optional fill pattern).
Result<void> draw_rough_circle(SkCanvas* canvas, const GraphicObject& obj,
                               sk_sp<SkShader> shader,
                               const RoughStyleParams& params,
                               RoughRandom& rng);

/// Draw a rough rect (perturbed stroke + optional fill pattern).
Result<void> draw_rough_rect(SkCanvas* canvas, const GraphicObject& obj,
                             sk_sp<SkShader> shader,
                             const RoughStyleParams& params,
                             RoughRandom& rng);

/// Draw a rough ellipse.
Result<void> draw_rough_ellipse(SkCanvas* canvas, const GraphicObject& obj,
                                sk_sp<SkShader> shader,
                                const RoughStyleParams& params,
                                RoughRandom& rng);

/// Draw a rough line.
Result<void> draw_rough_line(SkCanvas* canvas, const GraphicObject& obj,
                             const RoughStyleParams& params,
                             RoughRandom& rng);

/// Draw a rough polygon.
Result<void> draw_rough_polygon(SkCanvas* canvas, const GraphicObject& obj,
                                sk_sp<SkShader> shader,
                                const RoughStyleParams& params,
                                RoughRandom& rng);

/// Render rough fill pattern (hachure/zigzag/etc.) for a closed polygon.
Result<void> draw_rough_fill(SkCanvas* canvas,
                             const std::vector<RoughPoint>& polygon,
                             const std::optional<std::string>& fill_color,
                             const RoughStyleParams& params,
                             RoughRandom& rng);

/// Decode a PNG file via libpng into a SkiaSurface.
/// Defined in skia_backend_png.cpp.
Result<std::unique_ptr<Surface>> load_png_with_libpng(
    const std::string& path);

// ==========================================================================================================================================================================================================================================═
// SkiaBackend class declaration
// ==========================================================================================================================================================================================================================================═
//
// The class is NOT in an anonymous namespace so that its member functions can be
// defined in multiple translation units (skia_backend.cpp, skia_backend_filter.cpp).

class SkiaBackend final : public RenderBackend {
public:
    [[nodiscard]] auto info() const -> BackendInfo override
    {
        return BackendInfo{
            .name = "skia",
            .description = "Skia raster/GPU backend",
            .capabilities =
                BackendCap::RasterCPU
              | BackendCap::GPUAccel
              | BackendCap::Shaders
              | BackendCap::VectorOutput
              | BackendCap::AnimationGIF
              | BackendCap::FontRendering
              | BackendCap::LoadPNG
              | BackendCap::LoadImage,
        };
    }

    [[nodiscard]] auto create_surface(int width, int height, uint32_t bg_color)
        -> std::unique_ptr<Surface> override
    {
        return std::make_unique<SkiaSurface>(width, height, bg_color);
    }

    [[nodiscard]] sk_sp<SkShader> lookup_shader(int64_t handle) const
    {
        if (handle <= 0) return nullptr;

        auto it = shader_cache_.find(static_cast<uint64_t>(handle));
        if (it != shader_cache_.end() && it->second) {
            return it->second->makeShader(SkData::MakeEmpty(), {}, nullptr);
        }

        auto pit = preshader_cache_.find(static_cast<uint64_t>(handle));
        if (pit != preshader_cache_.end() && pit->second) {
            return pit->second;
        }

        return nullptr;
    }

private:
    uint64_t next_shader_handle_{1};
    uint64_t next_preshader_handle_{1000000};
    std::unordered_map<uint64_t, sk_sp<SkRuntimeEffect>> shader_cache_;
    std::unordered_map<uint64_t, sk_sp<SkShader>> preshader_cache_;

    auto draw(Surface& surface, const GraphicObject& obj)
        -> Result<void> override;

    auto save_image(Surface& surface, const std::string& path,
                    const std::string& format) -> Result<void> override;

    auto save_animation(const std::vector<Surface*>& frames,
                        const std::string& path,
                        const std::string& /*format*/,
                        int fps) -> Result<void> override;

    [[nodiscard]] auto load_image(const std::string& path)
        -> Result<std::unique_ptr<Surface>> override;

    auto composite(Surface& dst, Surface& src, int x, int y)
        -> Result<void> override;

    [[nodiscard]] auto draw_to_new_surface(
        const GraphicObject& obj, int w, int h, uint32_t bg)
        -> Result<std::unique_ptr<Surface>> override;

    auto composite_with_blend(Surface& dst, Surface& src, int x, int y,
                              BlendMode blend, float opacity)
        -> Result<void> override;

    auto apply_mask(Surface& dst, Surface& mask) -> Result<void> override;

    [[nodiscard]] auto supports_blend_mode() const noexcept -> bool override { return true; }
    [[nodiscard]] auto supports_layer_compositing() const noexcept -> bool override { return true; }

    auto compile_shader(const std::string& sksl) -> Result<uint64_t> override;

    auto create_noise_shader(NoiseType type,
                            float base_freq_x, float base_freq_y,
                            int octaves, float seed,
                            int tile_w, int tile_h,
                            float lacunarity = 2.0f,
                            float persistence = 0.5f) -> Result<uint64_t> override;

    auto create_shader_with_uniforms(uint64_t shader_handle,
                                     const std::vector<uint8_t>& uniform_data)
        -> Result<uint64_t> override;

    auto create_shader_with_children(
        const std::string& src,
        const std::vector<ShaderChildInfo>& childDescs,
        const std::vector<Value>& uniforms)
        -> Result<Value> override;

    auto bind_textures_to_shader(
        uint64_t shader_handle,
        const std::vector<std::pair<std::string, Value>>& textures)
        -> Result<uint64_t> override;

    auto compose_with_child_shader(
        uint64_t preshader_handle,
        const std::string& sksl_wrapper_src)
        -> Result<uint64_t> override;

    auto compose_with_child_shaders(
        const std::vector<uint64_t>& preshader_handles,
        const std::string& sksl_wrapper_src,
        const std::vector<uint8_t>& uniform_data = {})
        -> Result<uint64_t> override;

    // ---- FilterBackend ------------------------------------------------------------------------------------------------

    [[nodiscard]] Result<void> apply_sk_image_filter(
        SkiaSurface& surf, sk_sp<SkImageFilter> filter);

    Result<void> apply_color_matrix(
        Surface& s, const std::array<float,20>& m) override;

    Result<void> apply_blur(
        Surface& s, float rx, float ry, BlurType type) override;

    Result<void> apply_convolution(
        Surface& s, const ConvolutionKernel& k) override;

    Result<void> apply_color_table(
        Surface& s,
        const std::array<uint8_t,256>& r,
        const std::array<uint8_t,256>& g,
        const std::array<uint8_t,256>& b,
        const std::array<uint8_t,256>& a) override;

    Result<void> apply_offset(Surface& s, float dx, float dy) override;

    Result<void> apply_drop_shadow(
        Surface& s, float dx, float dy,
        float blur_x, float blur_y, uint32_t color) override;

    Result<void> apply_inner_shadow(
        Surface& s, float dx, float dy,
        float blur, uint32_t color) override;

    Result<void> apply_outer_glow(
        Surface& s, float blur, uint32_t color) override;

    Result<void> apply_inner_glow(
        Surface& s, float blur, uint32_t color) override;

    Result<void> apply_bevel_emboss(
        Surface& s, float angle, float altitude,
        float blur, uint32_t highlight, uint32_t shadow) override;
};

}  // namespace pml
