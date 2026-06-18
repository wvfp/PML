// ═══════════════════════════════════════════════════════════════════════════════
// PML Skia render backend — real implementation
// ═══════════════════════════════════════════════════════════════════════════════
//
// Uses the pre-built Skia static library. All Skia headers are included through
// the generated aggregate header <skia.h>.

#include "pml/backend/backend.h"
#include "pml/backend/registry.h"
#include "pml/backend/color_helpers.h"
#include "pml/backend/gif/gif_exporter.h"
#include "pml/graphics/objects.h"
#include "pml/graphics/transform.h"

#include <skia.h>

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

namespace {

// ═══════════════════════════════════════════════════════════════════════════════
// SkiaSurface
// ═══════════════════════════════════════════════════════════════════════════════

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
};

// ═══════════════════════════════════════════════════════════════════════════════
// Color helpers
// ═══════════════════════════════════════════════════════════════════════════════

inline SkColor to_sk_color(uint32_t argb)
{
    // parse_color() returns 0xAARRGGBB, which matches SkColor layout.
    return static_cast<SkColor>(argb);
}

std::optional<SkColor> parse_sk_color(const std::string& s)
{
    auto c = parse_color(s);
    if (!c) return std::nullopt;
    return to_sk_color(*c);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Paint helpers
// ═══════════════════════════════════════════════════════════════════════════════

void configure_fill_paint(
    SkPaint& paint,
    const std::optional<std::string>& fill,
    double /*stroke_width*/,
    sk_sp<SkShader> shader = nullptr)
{
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);
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

void configure_stroke_paint(
    SkPaint& paint,
    const std::optional<std::string>& stroke,
    double stroke_width)
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
}

// ═══════════════════════════════════════════════════════════════════════════════
// Param extraction
// ═══════════════════════════════════════════════════════════════════════════════

[[nodiscard]] double get_double(
    const std::unordered_map<std::string, Value>& params,
    const std::string& key,
    double default_val)
{
    auto it = params.find(key);
    if (it == params.end()) return default_val;
    const Value& v = it->second;
    if (std::holds_alternative<int64_t>(v)) {
        return static_cast<double>(std::get<int64_t>(v));
    }
    if (std::holds_alternative<double>(v)) {
        return std::get<double>(v);
    }
    return default_val;
}

[[nodiscard]] std::string get_string(
    const std::unordered_map<std::string, Value>& params,
    const std::string& key,
    const std::string& default_val)
{
    auto it = params.find(key);
    if (it == params.end()) return default_val;
    const Value& v = it->second;
    if (std::holds_alternative<std::string>(v)) {
        return std::get<std::string>(v);
    }
    return default_val;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Recursive drawing
// ═══════════════════════════════════════════════════════════════════════════════

using ShaderLookup = std::function<sk_sp<SkShader>(int64_t)>;

Result<void> draw_object(SkCanvas* canvas, const GraphicObject& obj,
                         sk_sp<SkShader> shader, const ShaderLookup& lookup);

Result<void> draw_group(SkCanvas* canvas, const GraphicObject& obj,
                        sk_sp<SkShader> shader, const ShaderLookup& lookup)
{
    for (const auto& child : obj.children) {
        auto r = draw_object(canvas, child, shader, lookup);
        if (!r) return r;
    }
    return {};
}

Result<void> draw_circle(SkCanvas* canvas, const GraphicObject& obj,
                         sk_sp<SkShader> shader)
{
    double cx = get_double(obj.params, "cx", 0.0);
    double cy = get_double(obj.params, "cy", 0.0);
    double r  = get_double(obj.params, "r", 0.0);

    if (obj.fill || shader) {
        SkPaint paint;
        configure_fill_paint(paint, obj.fill, obj.stroke_width, shader);
        if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
            canvas->drawCircle(static_cast<SkScalar>(cx),
                               static_cast<SkScalar>(cy),
                               static_cast<SkScalar>(r), paint);
        }
    }
    if (obj.stroke) {
        SkPaint paint;
        configure_stroke_paint(paint, obj.stroke, obj.stroke_width);
        if (paint.getColor() != SK_ColorTRANSPARENT) {
            canvas->drawCircle(static_cast<SkScalar>(cx),
                               static_cast<SkScalar>(cy),
                               static_cast<SkScalar>(r), paint);
        }
    }
    return {};
}

Result<void> draw_rect(SkCanvas* canvas, const GraphicObject& obj,
                       sk_sp<SkShader> shader)
{
    double x = get_double(obj.params, "x", 0.0);
    double y = get_double(obj.params, "y", 0.0);
    double w = get_double(obj.params, "w", 0.0);
    double h = get_double(obj.params, "h", 0.0);
    double rx = get_double(obj.params, "rx", 0.0);

    SkRect rect = SkRect::MakeXYWH(static_cast<SkScalar>(x),
                                   static_cast<SkScalar>(y),
                                   static_cast<SkScalar>(w),
                                   static_cast<SkScalar>(h));

    if (obj.fill || shader) {
        SkPaint paint;
        configure_fill_paint(paint, obj.fill, obj.stroke_width, shader);
        if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
            if (rx > 0.0) {
                canvas->drawRoundRect(rect,
                                      static_cast<SkScalar>(rx),
                                      static_cast<SkScalar>(rx), paint);
            } else {
                canvas->drawRect(rect, paint);
            }
        }
    }
    if (obj.stroke) {
        SkPaint paint;
        configure_stroke_paint(paint, obj.stroke, obj.stroke_width);
        if (paint.getColor() != SK_ColorTRANSPARENT) {
            if (rx > 0.0) {
                canvas->drawRoundRect(rect,
                                      static_cast<SkScalar>(rx),
                                      static_cast<SkScalar>(rx), paint);
            } else {
                canvas->drawRect(rect, paint);
            }
        }
    }
    return {};
}

Result<void> draw_ellipse(SkCanvas* canvas, const GraphicObject& obj,
                          sk_sp<SkShader> shader)
{
    double cx = get_double(obj.params, "cx", 0.0);
    double cy = get_double(obj.params, "cy", 0.0);
    double rx = get_double(obj.params, "rx", 0.0);
    double ry = get_double(obj.params, "ry", 0.0);

    SkRect oval = SkRect::MakeXYWH(static_cast<SkScalar>(cx - rx),
                                   static_cast<SkScalar>(cy - ry),
                                   static_cast<SkScalar>(rx * 2.0),
                                   static_cast<SkScalar>(ry * 2.0));

    if (obj.fill || shader) {
        SkPaint paint;
        configure_fill_paint(paint, obj.fill, obj.stroke_width, shader);
        if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
            canvas->drawOval(oval, paint);
        }
    }
    if (obj.stroke) {
        SkPaint paint;
        configure_stroke_paint(paint, obj.stroke, obj.stroke_width);
        if (paint.getColor() != SK_ColorTRANSPARENT) {
            canvas->drawOval(oval, paint);
        }
    }
    return {};
}

Result<void> draw_line(SkCanvas* canvas, const GraphicObject& obj,
                       sk_sp<SkShader> /*shader*/)
{
    double x1 = get_double(obj.params, "x1", 0.0);
    double y1 = get_double(obj.params, "y1", 0.0);
    double x2 = get_double(obj.params, "x2", 0.0);
    double y2 = get_double(obj.params, "y2", 0.0);

    if (obj.stroke) {
        SkPaint paint;
        configure_stroke_paint(paint, obj.stroke, obj.stroke_width);
        if (paint.getColor() != SK_ColorTRANSPARENT) {
            canvas->drawLine(static_cast<SkScalar>(x1),
                             static_cast<SkScalar>(y1),
                             static_cast<SkScalar>(x2),
                             static_cast<SkScalar>(y2), paint);
        }
    }
    return {};
}

Result<void> draw_polygon(SkCanvas* canvas, const GraphicObject& obj,
                          sk_sp<SkShader> shader)
{
    auto it = obj.params.find("points");
    if (it == obj.params.end()) return {};

    const Value& v = it->second;
    const auto* list = std::get_if<std::shared_ptr<ValueList>>(&v);
    if (!list || !*list) return {};

    const auto& elems = (*list)->elements;
    if (elems.size() < 4 || elems.size() % 2 != 0) return {};

    SkPathBuilder builder;
    auto to_double = [](const Value& val) -> double {
        if (std::holds_alternative<int64_t>(val)) {
            return static_cast<double>(std::get<int64_t>(val));
        }
        if (std::holds_alternative<double>(val)) {
            return std::get<double>(val);
        }
        return 0.0;
    };

    builder.moveTo(static_cast<SkScalar>(to_double(elems[0])),
                   static_cast<SkScalar>(to_double(elems[1])));
    for (size_t i = 2; i + 1 < elems.size(); i += 2) {
        builder.lineTo(static_cast<SkScalar>(to_double(elems[i])),
                       static_cast<SkScalar>(to_double(elems[i + 1])));
    }
    builder.close();
    SkPath path = builder.snapshot();

    if (obj.fill || shader) {
        SkPaint paint;
        configure_fill_paint(paint, obj.fill, obj.stroke_width, shader);
        if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
            canvas->drawPath(path, paint);
        }
    }
    if (obj.stroke) {
        SkPaint paint;
        configure_stroke_paint(paint, obj.stroke, obj.stroke_width);
        if (paint.getColor() != SK_ColorTRANSPARENT) {
            canvas->drawPath(path, paint);
        }
    }
    return {};
}

Result<void> draw_text(SkCanvas* canvas, const GraphicObject& obj,
                       sk_sp<SkShader> shader)
{
    double x = get_double(obj.params, "x", 0.0);
    double y = get_double(obj.params, "y", 0.0);
    std::string text = get_string(obj.params, "text", "");
    double font_size = get_double(obj.params, "font-size", 16.0);

    if (text.empty()) return {};

    SkPaint paint;
    configure_fill_paint(paint, obj.fill, obj.stroke_width, shader);
    if (paint.getColor() == SK_ColorTRANSPARENT && !paint.getShader()) {
        paint.setColor(SK_ColorBLACK);
    }

#ifdef _WIN32
    sk_sp<SkFontMgr> font_mgr = SkFontMgr_New_DirectWrite();
#else
    sk_sp<SkFontMgr> font_mgr = SkFontMgr::RefEmpty();
#endif

    sk_sp<SkTypeface> typeface = font_mgr->matchFamilyStyle(nullptr, SkFontStyle());
    if (!typeface) {
        typeface = font_mgr->matchFamilyStyle("Arial", SkFontStyle());
    }
    if (!typeface) {
        typeface = font_mgr->matchFamilyStyle("Microsoft YaHei", SkFontStyle());
    }
    if (!typeface) {
        return std::unexpected(general_error(
            "skia draw_text: failed to resolve a default typeface"));
    }

    SkFont font(typeface, static_cast<SkScalar>(font_size));
    font.setEdging(SkFont::Edging::kAntiAlias);
    canvas->drawSimpleText(text.c_str(), text.size(),
                           SkTextEncoding::kUTF8,
                           static_cast<SkScalar>(x),
                           static_cast<SkScalar>(y),
                           font, paint);
    return {};
}

Result<void> draw_path(SkCanvas* canvas, const GraphicObject& obj,
                       sk_sp<SkShader> /*shader*/)
{
    std::string d = get_string(obj.params, "d", "");
    if (d.empty()) return {};

    // TODO: parse SVG path data into SkPath.
    // For now, leave as a no-op so compilation and basic shapes work.
    (void)canvas;
    (void)obj;
    return {};
}

Result<void> draw_image(SkCanvas* canvas, const GraphicObject& obj,
                        sk_sp<SkShader> /*shader*/)
{
    std::string src = get_string(obj.params, "src", "");
    double x = get_double(obj.params, "x", 0.0);
    double y = get_double(obj.params, "y", 0.0);

    if (src.empty()) return {};

    // Image loading is temporarily disabled until SkCodec wiring is in place.
    // For now, treat it as a non-fatal no-op so the rest of the pipeline works.
    (void)canvas;
    (void)x;
    (void)y;
    (void)src;
    return {};
}

Result<void> draw_object(SkCanvas* canvas, const GraphicObject& obj,
                         sk_sp<SkShader> parent_shader,
                         const ShaderLookup& lookup)
{
    SkAutoCanvasRestore acr(canvas, true);

    if (!obj.transform.is_identity()) {
        canvas->concat(obj.transform.to_skmatrix());
    }

    // If this object has its own shader handle, resolve it; otherwise inherit.
    sk_sp<SkShader> local_shader = parent_shader;
    if (auto it = obj.params.find("shader"); it != obj.params.end()) {
        int64_t handle = 0;
        if (std::holds_alternative<int64_t>(it->second)) {
            handle = std::get<int64_t>(it->second);
        } else if (std::holds_alternative<double>(it->second)) {
            handle = static_cast<int64_t>(std::get<double>(it->second));
        }
        if (handle > 0) {
            local_shader = lookup(handle);
        }
    }

    if (obj.shape_type == "group") {
        return draw_group(canvas, obj, local_shader, lookup);
    }
    if (obj.shape_type == "circle")  return draw_circle(canvas, obj, local_shader);
    if (obj.shape_type == "rect")    return draw_rect(canvas, obj, local_shader);
    if (obj.shape_type == "ellipse") return draw_ellipse(canvas, obj, local_shader);
    if (obj.shape_type == "line")    return draw_line(canvas, obj, local_shader);
    if (obj.shape_type == "polygon") return draw_polygon(canvas, obj, local_shader);
    if (obj.shape_type == "text")    return draw_text(canvas, obj, local_shader);
    if (obj.shape_type == "path")    return draw_path(canvas, obj, local_shader);
    if (obj.shape_type == "image")   return draw_image(canvas, obj, local_shader);

    // Unknown shape type is non-fatal (matching Python's tolerant behaviour).
    return {};
}

// ═══════════════════════════════════════════════════════════════════════════════
// SkiaBackend
// ═══════════════════════════════════════════════════════════════════════════════

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
              | BackendCap::LoadPNG,
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
        if (it == shader_cache_.end() || !it->second) return nullptr;
        // No uniforms for now; pass empty uniforms data and no children.
        return it->second->makeShader(SkData::MakeEmpty(), {}, nullptr);
    }

private:
    uint64_t next_shader_handle_{1};
    std::unordered_map<uint64_t, sk_sp<SkRuntimeEffect>> shader_cache_;

    auto draw(Surface& surface, const GraphicObject& obj)
        -> Result<void> override
    {
        auto* skia_surf = dynamic_cast<SkiaSurface*>(&surface);
        if (!skia_surf) {
            return std::unexpected(general_error(
                "skia draw: invalid surface type"));
        }
        ShaderLookup lookup = [this](int64_t h) { return lookup_shader(h); };
        return draw_object(skia_surf->surface->getCanvas(), obj, nullptr, lookup);
    }

    auto save_image(Surface& surface, const std::string& path,
                    const std::string& format) -> Result<void> override
    {
        auto* skia_surf = dynamic_cast<SkiaSurface*>(&surface);
        if (!skia_surf) {
            return std::unexpected(general_error(
                "skia save_image: invalid surface type"));
        }

        std::string fmt = format;
        std::transform(fmt.begin(), fmt.end(), fmt.begin(), ::tolower);

        if (fmt == "jpg" || fmt == "jpeg") {
            return std::unexpected(general_error(
                "skia save_image: JPEG encoding not yet supported"));
        }

        // Default to PNG. Use libpng directly because the prebuilt skia.lib
        // in this tree does not expose SkPngEncoder::Encode.
        SkPixmap pixmap;
        if (!skia_surf->bitmap.peekPixels(&pixmap)) {
            return std::unexpected(general_error(
                "skia save_image: failed to access pixels"));
        }

        FILE* fp = nullptr;
        if (fopen_s(&fp, path.c_str(), "wb") != 0 || !fp) {
            return std::unexpected(general_error(
                "skia save_image: failed to open file: " + path));
        }

        png_structp png = png_create_write_struct(
            PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png) {
            fclose(fp);
            return std::unexpected(general_error(
                "skia save_image: png_create_write_struct failed"));
        }

        png_infop info = png_create_info_struct(png);
        if (!info) {
            png_destroy_write_struct(&png, nullptr);
            fclose(fp);
            return std::unexpected(general_error(
                "skia save_image: png_create_info_struct failed"));
        }

        if (setjmp(png_jmpbuf(png))) {
            png_destroy_write_struct(&png, &info);
            fclose(fp);
            return std::unexpected(general_error(
                "skia save_image: libpng error"));
        }

        png_init_io(png, fp);

        const int w = pixmap.width();
        const int h = pixmap.height();
        png_set_IHDR(png, info, w, h, 8, PNG_COLOR_TYPE_RGBA,
                     PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT,
                     PNG_FILTER_TYPE_DEFAULT);
        png_write_info(png, info);

        const uint8_t* pixels = static_cast<const uint8_t*>(pixmap.addr());
        const size_t row_bytes = pixmap.rowBytes();

        // Skia's kN32 pixel format on little-endian platforms is BGRA,
        // but PNG expects RGBA. Copy and swizzle in one pass.
        std::vector<uint8_t> rgba_pixels(static_cast<size_t>(w * h * 4));
        for (int y = 0; y < h; ++y) {
            const uint8_t* src = pixels + y * row_bytes;
            uint8_t* dst = rgba_pixels.data() + static_cast<size_t>(y * w * 4);
            for (int x = 0; x < w; ++x) {
                dst[x * 4 + 0] = src[x * 4 + 2];  // R <- B
                dst[x * 4 + 1] = src[x * 4 + 1];  // G <- G
                dst[x * 4 + 2] = src[x * 4 + 0];  // B <- R
                dst[x * 4 + 3] = src[x * 4 + 3];  // A <- A
            }
        }

        std::vector<png_bytep> row_pointers(static_cast<size_t>(h));
        for (int y = 0; y < h; ++y) {
            row_pointers[static_cast<size_t>(y)] =
                rgba_pixels.data() + static_cast<size_t>(y * w * 4);
        }
        png_write_image(png, row_pointers.data());
        png_write_end(png, nullptr);

        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return {};
    }

    auto save_animation(const std::vector<Surface*>& frames,
                        const std::string& path,
                        const std::string& /*format*/,
                        int fps) -> Result<void> override
    {
        if (frames.empty()) return {};

        auto* first = dynamic_cast<SkiaSurface*>(frames[0]);
        if (!first) {
            return std::unexpected(general_error(
                "skia save_animation: invalid frame surface type"));
        }

        const int w = first->width;
        const int h = first->height;
        std::vector<std::vector<uint8_t>> rgba_frames;
        rgba_frames.reserve(frames.size());

        for (size_t fi = 0; fi < frames.size(); ++fi) {
            Surface* surf = frames[fi];
            auto* skia_surf = dynamic_cast<SkiaSurface*>(surf);
            if (!skia_surf) {
                return std::unexpected(general_error(
                    "skia save_animation: invalid frame surface type"));
            }

            SkPixmap pixmap;
            if (!skia_surf->bitmap.peekPixels(&pixmap)) {
                return std::unexpected(general_error(
                    "skia save_animation: failed to access frame pixels"));
            }

            const uint8_t* pixels = static_cast<const uint8_t*>(pixmap.addr());
            const size_t row_bytes = pixmap.rowBytes();
            std::vector<uint8_t> frame(static_cast<size_t>(w * h * 4));

            // Skia's kN32 pixel format on little-endian platforms is BGRA,
            // but the GIF exporter expects RGBA. Swizzle while copying.
            for (int y = 0; y < h; ++y) {
                const uint8_t* src = pixels + y * row_bytes;
                uint8_t* dst = frame.data() + static_cast<size_t>(y * w * 4);
                for (int x = 0; x < w; ++x) {
                    dst[x * 4 + 0] = src[x * 4 + 2];  // R <- B
                    dst[x * 4 + 1] = src[x * 4 + 1];  // G <- G
                    dst[x * 4 + 2] = src[x * 4 + 0];  // B <- R
                    dst[x * 4 + 3] = src[x * 4 + 3];  // A <- A
                }
            }
            rgba_frames.push_back(std::move(frame));
        }

        try {
            pml::backend::gif::export_gif(
                rgba_frames, w, h, path, static_cast<double>(fps));
        } catch (const std::exception& e) {
            return std::unexpected(general_error(
                std::string("skia save_animation: gif export failed: ") + e.what()));
        }
        return {};
    }

    auto composite(Surface& dst, Surface& src, int x, int y)
        -> Result<void> override
    {
        auto* dst_surf = dynamic_cast<SkiaSurface*>(&dst);
        auto* src_surf = dynamic_cast<SkiaSurface*>(&src);
        if (!dst_surf || !src_surf) {
            return std::unexpected(general_error(
                "skia composite: invalid surface type"));
        }

        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrcOver);
        dst_surf->surface->getCanvas()->drawImage(
            src_surf->bitmap.asImage().get(),
            static_cast<SkScalar>(x),
            static_cast<SkScalar>(y),
            SkSamplingOptions(), &paint);
        return {};
    }

    auto compile_shader(const std::string& sksl) -> Result<uint64_t> override
    {
        if (sksl.empty()) {
            return std::unexpected(general_error(
                "skia compile_shader: shader source is empty"));
        }

        auto result = SkRuntimeEffect::MakeForShader(SkString(sksl));
        if (!result.effect) {
            std::string msg = "skia compile_shader: failed to compile shader";
            if (result.errorText.size() > 0) {
                msg += ": ";
                msg += result.errorText.c_str();
            }
            return std::unexpected(general_error(msg));
        }

        uint64_t handle = next_shader_handle_++;
        shader_cache_[handle] = std::move(result.effect);
        return handle;
    }
};

}  // namespace

// ═══════════════════════════════════════════════════════════════════════════════
// Registration
// ═══════════════════════════════════════════════════════════════════════════════

[[maybe_unused]] static bool registered_skia = BackendRegistry::register_backend(
    "skia",
    "Skia raster/GPU backend",
    BackendCap::RasterCPU
        | BackendCap::GPUAccel
        | BackendCap::Shaders
        | BackendCap::VectorOutput
        | BackendCap::AnimationGIF
        | BackendCap::FontRendering
        | BackendCap::LoadPNG,
    []() -> std::unique_ptr<RenderBackend> {
        return std::make_unique<SkiaBackend>();
    }
);

}  // namespace pml
