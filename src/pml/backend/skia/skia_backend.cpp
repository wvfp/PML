// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?// PML Skia render backend 鈥?SkiaBackend class + I/O + shaders + registration
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?//
// Member function definitions for the SkiaBackend class (core rendering, I/O,
// compositing, and shader operations).  Draw primitives live in
// skia_backend_draw.cpp, PNG loading in skia_backend_png.cpp, and filter
// operations in skia_backend_filter.cpp.
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
#include "skia_backend_internal.h"

#include <filesystem>
#include <format>
#include "pml/core/platform.h"
namespace fs = std::filesystem;

namespace pml {

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?// Drawing
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
auto SkiaBackend::draw(Surface& surface, const GraphicObject& obj)
    -> Result<void>
{
    auto* skia_surf = dynamic_cast<SkiaSurface*>(&surface);
    if (!skia_surf) {
        return std::unexpected(general_error(
            "skia draw: invalid surface type"));
    }
    ShaderLookup lookup = [this](int64_t h) { return lookup_shader(h); };
    return draw_object(skia_surf->surface->getCanvas(), obj, nullptr, lookup);
}

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?// Output 鈥?save_image (PNG via libpng)
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
auto SkiaBackend::save_image(Surface& surface, const std::string& path,
                             const std::string& format) -> Result<void>
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

    // On Windows, git-checked-out PNGs carry FILE_ATTRIBUTE_READONLY,
    // causing fopen_s("wb") / CreateFile to fail with EACCES.  Rather
    // than relying on fs::permissions (which maps to _chmod on MSVC and
    // may not fully clear the attribute on all Windows configurations),
    // we delete any existing file first.  DeleteFileW handles read-only
    // files correctly on Windows, and creating a brand-new file avoids
    // any residual attribute or lock issues.
    std::error_code ec;
    fs::remove(path, ec);

    FILE* fp = nullptr;
    errno = 0;
    if (pml_fopen(fp, path.c_str(), "wb")) {
        const int os_err = errno;
        return std::unexpected(general_error(
            "skia save_image: failed to open file: " + path
            + " (errno=" + std::to_string(os_err) + ")"));
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

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?// Output 鈥?save_animation (GIF export)
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
auto SkiaBackend::save_animation(const std::vector<Surface*>& frames,
                                 const std::string& path,
                                 const std::string& /*format*/,
                                 int fps) -> Result<void>
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

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?// Image loading
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
auto SkiaBackend::load_image(const std::string& path)
    -> Result<std::unique_ptr<Surface>>
{
    // The prebuilt skia.lib does not expose a working PNG decoder, so
    // route PNGs through libpng. Other formats still try SkCodec.
    auto png_result = load_png_with_libpng(path);
    if (png_result.has_value()) {
        return png_result;
    }

    // If libpng rejected it (not a PNG / corrupt), fall back to SkCodec.
    auto data = SkData::MakeFromFileName(path.c_str());
    if (!data) {
        return std::unexpected(resource_error(
            std::format("skia: cannot read image file: {}", path)));
    }

    auto codec = SkCodec::MakeFromData(std::move(data));
    if (!codec) {
        return std::unexpected(resource_error(
            std::format("skia: cannot decode image format: {}", path)));
    }

    SkImageInfo info = codec->getInfo().makeColorType(kN32_SkColorType)
                                          .makeAlphaType(kPremul_SkAlphaType);
    SkBitmap bitmap;
    if (!bitmap.tryAllocPixels(info)) {
        return std::unexpected(resource_error(
            "skia: failed to allocate image pixels"));
    }

    SkCodec::Result decode_result = codec->getPixels(
        info, bitmap.getPixels(), bitmap.rowBytes());
    if (decode_result != SkCodec::kSuccess
        && decode_result != SkCodec::kIncompleteInput) {
        return std::unexpected(resource_error(
            std::format("skia: image decode failed with code {}",
                        static_cast<int>(decode_result))));
    }

    return std::make_unique<SkiaSurface>(std::move(bitmap));
}

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?// Compositing
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
auto SkiaBackend::composite(Surface& dst, Surface& src, int x, int y)
    -> Result<void>
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

auto SkiaBackend::draw_to_new_surface(
    const GraphicObject& obj, int w, int h, uint32_t bg)
    -> Result<std::unique_ptr<Surface>>
{
    auto surface = create_surface(w, h, bg);
    if (!surface) {
        return std::unexpected(resource_error("failed to create layer surface"));
    }
    auto result = draw(*surface, obj);
    if (!result) return std::unexpected(result.error());
    return surface;
}

auto SkiaBackend::composite_with_blend(Surface& dst, Surface& src, int x, int y,
                                       BlendMode blend, float opacity)
    -> Result<void>
{
    auto* dst_surf = dynamic_cast<SkiaSurface*>(&dst);
    auto* src_surf = dynamic_cast<SkiaSurface*>(&src);
    if (!dst_surf || !src_surf) {
        return std::unexpected(general_error(
            "skia composite_with_blend: invalid surface type"));
    }

    SkPaint paint;
    paint.setBlendMode(to_skia_blend_mode(blend));
    paint.setAlphaf(std::clamp(opacity, 0.0f, 1.0f));
    dst_surf->surface->getCanvas()->drawImage(
        src_surf->bitmap.asImage().get(),
        static_cast<SkScalar>(x),
        static_cast<SkScalar>(y),
        SkSamplingOptions(), &paint);
    return {};
}

auto SkiaBackend::apply_mask(Surface& dst, Surface& mask) -> Result<void>
{
    auto* dst_surf = dynamic_cast<SkiaSurface*>(&dst);
    auto* mask_surf = dynamic_cast<SkiaSurface*>(&mask);
    if (!dst_surf || !mask_surf) {
        return std::unexpected(general_error(
            "skia apply_mask: invalid surface type"));
    }
    if (dst_surf->width != mask_surf->width || dst_surf->height != mask_surf->height) {
        return std::unexpected(general_error(
            "skia apply_mask: mask and dst sizes must match"));
    }

    SkPixmap dst_pm;
    SkPixmap mask_pm;
    if (!dst_surf->bitmap.peekPixels(&dst_pm) ||
        !mask_surf->bitmap.peekPixels(&mask_pm)) {
        return std::unexpected(resource_error("skia apply_mask: cannot access pixels"));
    }

    for (int y = 0; y < dst_surf->height; ++y) {
        uint32_t* dst_row = static_cast<uint32_t*>(dst_pm.writable_addr32(0, y));
        const uint32_t* mask_row = static_cast<const uint32_t*>(mask_pm.addr32(0, y));
        for (int x = 0; x < dst_surf->width; ++x) {
            uint32_t d = dst_row[x];
            uint32_t m = mask_row[x];
            uint8_t da = (d >> 24) & 0xFF;
            uint8_t ma = (m >> 24) & 0xFF;
            uint8_t out_a = static_cast<uint8_t>((da * ma) / 255);
            dst_row[x] = (d & 0x00FFFFFF) | (out_a << 24);
        }
    }
    return {};
}

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?// Shaders
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
auto SkiaBackend::compile_shader(const std::string& sksl) -> Result<uint64_t>
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

auto SkiaBackend::create_noise_shader(NoiseType type,
                                      float base_freq_x, float base_freq_y,
                                      int octaves, float seed,
                                      int tile_w, int tile_h,
                                      float lacunarity,
                                      float persistence) -> Result<uint64_t>
{
    // =====================================================================
    // Frequency pre-adjustment for seamless tiling
    //
    // Ensure tile_w * freq == n (integer >= 1) so the wrapping period in
    // the custom SkSL shader is exact.  Uses round() to minimise frequency
    // distortion, with max(1, ...) to guarantee at least one lattice cell
    // per tile.
    // =====================================================================
    if (tile_w > 0 && base_freq_x > 0.0f) {
        float n = std::max(1.0f, std::round(base_freq_x * static_cast<float>(tile_w)));
        base_freq_x = n / static_cast<float>(tile_w);
    }
    if (tile_h > 0 && base_freq_y > 0.0f) {
        float n = std::max(1.0f, std::round(base_freq_y * static_cast<float>(tile_h)));
        base_freq_y = n / static_cast<float>(tile_h);
    }

    octaves = std::clamp(octaves, 1, 32);

    // =====================================================================
    // Custom SkSL Perlin noise shader
    //
    // Implements 2D Perlin noise directly in SkSL with:
    //   - Hash-based gradient generation (no lookup tables)
    //   - Gradient wrapping via mod(floor(p), period) for pixel-exact
    //     seamless tiling at tile_w × tile_h boundaries — guaranteed by
    //     integer period = floor(tile_{w,h} * freq * lacunarity^{octave} + 0.5)
    //   - FBM fractal sum with lacunarity / persistence control
    //   - Turbulence mode (abs of noise per octave)
    //   - Seed-dependent hash offset for varied patterns
    //
    // Replaces the previous Skia Shaders::MakeFractalNoise/Turbulence path.
    //
    // NOTE: octaves is baked into the SkSL source rather than a uniform
    // because this Skia build's SkSL requires constant loop bounds.
    // =====================================================================

    // ── Generate octave loop body ──
    // Each octave iteration is emitted as a separate source block with
    // its own constant period computation, so no runtime loop is needed.
    std::string octave_code;
    for (int oi = 0; oi < octaves; ++oi) {
        // Each octave i has frequency scaled by lacunarity^i
        octave_code += "{\n";
        octave_code += "    float n;\n";
        if (tile_w > 0 && tile_h > 0) {
            // Compute integer period for this octave (baked constant)
            float oc_w = static_cast<float>(tile_w) * base_freq_x
                         * std::pow(lacunarity, static_cast<float>(oi));
            float oc_h = static_cast<float>(tile_h) * base_freq_y
                         * std::pow(lacunarity, static_cast<float>(oi));
            int p_w = std::max(1, static_cast<int>(oc_w + 0.5f));
            int p_h = std::max(1, static_cast<int>(oc_h + 0.5f));
            octave_code += "    n = perlin_noise(p, float2("
                           + std::to_string(p_w) + ".0, "
                           + std::to_string(p_h) + ".0));\n";
        } else {
            octave_code += "    n = perlin_noise(p, float2(1.0e10, 1.0e10));\n";
        }
        if (type == NoiseType::Turbulence) {
            octave_code += "    n = abs(n);\n";
        }
        octave_code += "    value += amplitude * n;\n";
        octave_code += "    norm  += amplitude;\n";
        octave_code += "    p        *= u_lacunarity;\n";
        octave_code += "    amplitude *= u_persistence;\n";
        octave_code += "}\n";
    }

    std::string sksl = R"(
        uniform float u_freq_x;
        uniform float u_freq_y;
        uniform float u_lacunarity;
        uniform float u_persistence;
        uniform float u_seed;
        uniform float u_type;   // 0.0 = fractal, 1.0 = turbulence

        // Hash-based 2D gradient generator
        // Returns an approximately unit-length gradient on the unit circle.
        float2 random_grad(float2 p) {
            p += float2(u_seed * 0.771, u_seed * 0.453);
            float3 p3 = fract(float3(p.xyx) * 0.1031);
            p3 += dot(p3, p3.yzx + 33.33);
            return normalize(-1.0 + 2.0 * fract(float2(p3.x + p3.y, p3.y + p3.z)));
        }

        // Quintic fade: smoothstep with zero 2nd derivative at endpoints
        // (Perlin's improved interpolation function)
        float fade(float t) {
            return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
        }

        // Single-octave 2D Perlin noise with gradient wrapping.
        // period = (0, 0) disables wrapping.
        float perlin_noise(float2 p, float2 period) {
            float2 i = floor(p);
            float2 f = fract(p);
            float2 u = float2(fade(f.x), fade(f.y));

            // Wrap lattice coordinates modulo period for seamless tiling
            float2 i00 = float2(mod(i.x,        period.x), mod(i.y,        period.y));
            float2 i10 = float2(mod(i.x + 1.0,  period.x), mod(i.y,        period.y));
            float2 i01 = float2(mod(i.x,        period.x), mod(i.y + 1.0,  period.y));
            float2 i11 = float2(mod(i.x + 1.0,  period.x), mod(i.y + 1.0,  period.y));

            float2 g00 = random_grad(i00);
            float2 g10 = random_grad(i10);
            float2 g01 = random_grad(i01);
            float2 g11 = random_grad(i11);

            float n00 = dot(g00, f - float2(0.0, 0.0));
            float n10 = dot(g10, f - float2(1.0, 0.0));
            float n01 = dot(g01, f - float2(0.0, 1.0));
            float n11 = dot(g11, f - float2(1.0, 1.0));

            return mix(mix(n00, n10, u.x), mix(n01, n11, u.x), u.y);
        }

        half4 main(float2 xy) {
            float2 p = xy * float2(u_freq_x, u_freq_y);

            float value = 0.0;
            float norm   = 0.0;
            float amplitude = 1.0;

            // Unrolled octaves (baked at shader-generation time)
    )" + octave_code + R"(

            float v = value / max(norm, 0.0001);

            // Map Perlin noise [-1, 1] → [0, 1]; turbulence is already [0, 1]
            if (u_type < 0.5) {
                v = v * 0.5 + 0.5;
            }
            v = clamp(v, 0.0, 1.0);

            return half4(v, v, v, 1.0);
        }
    )";

    // ── Compile SkSL ──
    auto result = SkRuntimeEffect::MakeForShader(SkString(sksl));
    if (!result.effect) {
        return std::unexpected(general_error(
            "skia create_noise_shader: SkSL compilation failed: " +
            std::string(result.errorText.c_str())));
    }
    sk_sp<SkRuntimeEffect> effect = std::move(result.effect);

    // ── Build uniform data (matches SkSL declaration order) ──
    // SkSL packs scalar uniforms at 4-byte boundaries in declaration order.
    // 6 uniforms × 4 bytes = 24 bytes.
    struct NoiseUniforms {
        float freq_x;       // offset  0
        float freq_y;       // offset  4
        float lacunarity_v; // offset  8
        float persistence_v;// offset 12
        float seed_v;       // offset 16
        float type_v;        // offset 20
    };
    static_assert(sizeof(NoiseUniforms) == 24,
                  "NoiseUniforms size must match SkSL uniform layout");

    NoiseUniforms uniforms{};
    uniforms.freq_x        = base_freq_x;
    uniforms.freq_y        = base_freq_y;
    uniforms.lacunarity_v  = lacunarity;
    uniforms.persistence_v = persistence;
    uniforms.seed_v        = seed;
    uniforms.type_v        = (type == NoiseType::Turbulence) ? 1.0f : 0.0f;

    sk_sp<SkData> uniform_data = SkData::MakeWithCopy(&uniforms, sizeof(uniforms));
    if (!uniform_data) {
        return std::unexpected(general_error(
            "skia create_noise_shader: failed to allocate uniform data"));
    }

    // ── Create shader from compiled effect + uniforms ──
    sk_sp<SkShader> noise_shader = effect->makeShader(
        std::move(uniform_data), {}, nullptr);
    if (!noise_shader) {
        return std::unexpected(general_error(
            "skia create_noise_shader: failed to create noise shader"));
    }

    uint64_t handle = next_preshader_handle_++;
    preshader_cache_[handle] = std::move(noise_shader);
    return handle;
}

auto SkiaBackend::create_shader_with_uniforms(uint64_t shader_handle,
                                              const std::vector<uint8_t>& uniform_data)
    -> Result<uint64_t>
{
    auto it = shader_cache_.find(shader_handle);
    if (it == shader_cache_.end() || !it->second) {
        return std::unexpected(general_error(
            "skia create_shader_with_uniforms: invalid shader handle"));
    }

    sk_sp<SkData> data = SkData::MakeWithCopy(
        uniform_data.data(), uniform_data.size());
    if (!data) {
        return std::unexpected(general_error(
            "skia create_shader_with_uniforms: failed to allocate uniform data"));
    }

    // Create a pre-baked shader with uniforms
    sk_sp<SkShader> baked = it->second->makeShader(data, {}, nullptr);
    if (!baked) {
        return std::unexpected(general_error(
            "skia create_shader_with_uniforms: failed to create shader with uniforms"));
    }

    uint64_t handle = next_preshader_handle_++;
    preshader_cache_[handle] = std::move(baked);
    return handle;
}

auto SkiaBackend::bind_textures_to_shader(
    uint64_t shader_handle,
    const std::vector<std::pair<std::string, Value>>& textures)
    -> Result<uint64_t>
{
    // 1. Look up the raw SkRuntimeEffect from shader_cache_
    auto it = shader_cache_.find(shader_handle);
    if (it == shader_cache_.end() || !it->second) {
        return std::unexpected(general_error(
            "skia bind_textures_to_shader: invalid shader handle"));
    }
    sk_sp<SkRuntimeEffect> effect = it->second;

    // 2. If no child slots in the effect, or no textures 鈫?just return the same handle
    auto children_meta = effect->children();
    if (children_meta.empty() || textures.empty()) {
        return shader_handle;
    }

    // 3. Build children array in declaration order (index-based)
    //    We use the effect's children() span to map slot names to indices.
    std::vector<SkRuntimeEffect::ChildPtr> children(children_meta.size());

    // Helper: compute the bounding box of a GraphicObject recursively
    auto compute_bounds = [](const GraphicObject& obj) -> std::pair<double, double> {
        // Recursive lambda via std::function
        std::function<std::tuple<double, double, double, double>(const GraphicObject&)> bounds;
        bounds = [&bounds](const GraphicObject& o) -> std::tuple<double, double, double, double> {
            double min_x = 1e9, min_y = 1e9, max_x = 0, max_y = 0;
            bool found = false;

            auto get_param_double = [&](ParamKey key, double def) -> double {
                if (const Value* v = o.params.find(key)) {
                    if (v->is_double()) return v->double_val();
                    if (v->is_int()) return static_cast<double>(v->int_val());
                }
                return def;
            };

            if (o.shape_type == "rect") {
                double x = get_param_double(ParamKey::x, 0);
                double y = get_param_double(ParamKey::y, 0);
                double w = get_param_double(ParamKey::w, 0);
                double h = get_param_double(ParamKey::h, 0);
                min_x = x; min_y = y; max_x = x + w; max_y = y + h;
                found = true;
            } else if (o.shape_type == "circle") {
                double cx = get_param_double(ParamKey::cx, 0);
                double cy = get_param_double(ParamKey::cy, 0);
                double r = get_param_double(ParamKey::r, 0);
                min_x = cx - r; min_y = cy - r; max_x = cx + r; max_y = cy + r;
                found = true;
            } else if (o.shape_type == "ellipse") {
                double cx = get_param_double(ParamKey::cx, 0);
                double cy = get_param_double(ParamKey::cy, 0);
                double rx = get_param_double(ParamKey::rx, 0);
                double ry = get_param_double(ParamKey::ry, 0);
                min_x = cx - rx; min_y = cy - ry; max_x = cx + rx; max_y = cy + ry;
                found = true;
            } else if (o.shape_type == "line") {
                double x1 = get_param_double(ParamKey::x, 0);
                double y1 = get_param_double(ParamKey::y, 0);
                double x2 = get_param_double(ParamKey::x2, 0);
                double y2 = get_param_double(ParamKey::y2, 0);
                min_x = std::min(x1, x2); min_y = std::min(y1, y2);
                max_x = std::max(x1, x2); max_y = std::max(y1, y2);
                found = true;
            }

            // Also consider children (for groups and nested structures)
            for (const auto& child : o.children) {
                auto [cmin_x, cmin_y, cmax_x, cmax_y] = bounds(child);
                if (cmin_x < 1e9) {
                    min_x = std::min(min_x, cmin_x);
                    min_y = std::min(min_y, cmin_y);
                    max_x = std::max(max_x, cmax_x);
                    max_y = std::max(max_y, cmax_y);
                    found = true;
                }
            }

            if (!found) return {0.0, 0.0, 0.0, 0.0};
            return {min_x, min_y, max_x, max_y};
        };
        auto [min_x, min_y, max_x, max_y] = bounds(obj);
        return {std::max(1.0, max_x - min_x), std::max(1.0, max_y - min_y)};
    };

    // Helper: determine texture size from GraphicObject
    auto compute_texture_size = [&compute_bounds](const GraphicObject& obj) -> std::pair<int, int> {
        if (obj.shape_type == "group" || obj.shape_type.empty()) {
            auto [w, h] = compute_bounds(obj);
            return {std::max(1, static_cast<int>(std::ceil(w))),
                    std::max(1, static_cast<int>(std::ceil(h)))};
        }
        if (obj.shape_type == "rect") {
            int w = 256, h = 256;
            if (const Value* v = obj.params.find(ParamKey::w)) {
                if (v->is_int()) w = static_cast<int>(v->int_val());
                else if (v->is_double()) w = static_cast<int>(v->double_val());
            }
            if (const Value* v = obj.params.find(ParamKey::h)) {
                if (v->is_int()) h = static_cast<int>(v->int_val());
                else if (v->is_double()) h = static_cast<int>(v->double_val());
            }
            return {std::max(1, w), std::max(1, h)};
        }
        if (obj.shape_type == "circle") {
            double r = 32.0;
            if (const Value* v = obj.params.find(ParamKey::r)) {
                if (v->is_int()) r = static_cast<double>(v->int_val());
                else if (v->is_double()) r = v->double_val();
            }
            int size = std::max(1, static_cast<int>(std::ceil(r * 2.0)));
            return {size, size};
        }
        if (obj.shape_type == "ellipse") {
            double rx = 32.0, ry = 32.0;
            if (const Value* v = obj.params.find(ParamKey::rx)) {
                if (v->is_int()) rx = static_cast<double>(v->int_val());
                else if (v->is_double()) rx = v->double_val();
            }
            if (const Value* v = obj.params.find(ParamKey::ry)) {
                if (v->is_int()) ry = static_cast<double>(v->int_val());
                else if (v->is_double()) ry = v->double_val();
            }
            int w = std::max(1, static_cast<int>(std::ceil(rx * 2.0)));
            int h = std::max(1, static_cast<int>(std::ceil(ry * 2.0)));
            return {w, h};
        }
        // Default: 256 x 256
        return {256, 256};
    };

    for (const auto& [slot_name, val] : textures) {
        // Look up the child index by slot name
        const SkRuntimeEffect::Child* child_info = effect->findChild(slot_name);
        if (!child_info) {
            return std::unexpected(type_error(
                std::format("skia bind_textures_to_shader: unknown child slot '{}'", slot_name)));
        }

        const auto* go = val.as_graphic_object();
        if (!go || !*go) {
            return std::unexpected(type_error(
                std::format("skia bind_textures_to_shader: value for slot '{}' is not a GraphicObject", slot_name)));
        }

        // Determine texture dimensions
        auto [tex_w, tex_h] = compute_texture_size(**go);

        // Create temp surface, draw the GraphicObject, snapshot to SkImage
        auto info = SkImageInfo::MakeN32Premul(tex_w, tex_h);
        auto temp_surf = SkSurfaces::Raster(info);
        if (!temp_surf) {
            return std::unexpected(resource_error(
                "skia bind_textures_to_shader: failed to create temp surface"));
        }

        // We need to translate the object so it appears at (0,0) on the temp surface.
        // For objects already at origin, no translation needed.
        // Create a shader lookup that uses this backend's cache.
        ShaderLookup lookup = [this](int64_t h) { return lookup_shader(h); };
        auto dr = draw_object(temp_surf->getCanvas(), **go, nullptr, lookup);
        if (!dr) return std::unexpected(std::move(dr.error()));

        auto image = temp_surf->makeImageSnapshot();
        if (!image) {
            return std::unexpected(resource_error(
                "skia bind_textures_to_shader: failed to snapshot temp surface"));
        }

        auto child_shader = image->makeShader(
            SkTileMode::kRepeat, SkTileMode::kRepeat,
            SkSamplingOptions(SkFilterMode::kLinear));
        if (!child_shader) {
            return std::unexpected(resource_error(
                "skia bind_textures_to_shader: failed to create child shader from image"));
        }

        children[static_cast<size_t>(child_info->index)] =
            SkRuntimeEffect::ChildPtr(std::move(child_shader));
    }

    // 4. Create the baked shader with children (empty uniforms)
    sk_sp<SkData> uniform_data = SkData::MakeEmpty();
    sk_sp<SkShader> baked = effect->makeShader(
        std::move(uniform_data),
        SkSpan<const SkRuntimeEffect::ChildPtr>(children.data(), children.size()),
        nullptr);
    if (!baked) {
        return std::unexpected(general_error(
            "skia bind_textures_to_shader: failed to create shader with children"));
    }

    // 5. Cache and return new handle
    uint64_t handle = next_preshader_handle_++;
    preshader_cache_[handle] = std::move(baked);
    return handle;
}

// 鈹€鈹€ compose_with_child_shader 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€
// Compose an existing preshader (e.g. noise) as the `src` child of a new
// SkSL wrapper effect, without rasterising the source to an image.
auto SkiaBackend::compose_with_child_shader(
    uint64_t preshader_handle,
    const std::string& sksl_wrapper_src) -> Result<uint64_t>
{
    // 1. Look up the preshader from preshader_cache_
    auto pit = preshader_cache_.find(preshader_handle);
    if (pit == preshader_cache_.end() || !pit->second) {
        return std::unexpected(general_error(
            "skia compose_with_child_shader: invalid preshader handle"));
    }
    sk_sp<SkShader> source_shader = pit->second;

    // 2. Compile the wrapper SkSL
    auto opts = SkRuntimeEffect::Options{};
    auto comp_result = SkRuntimeEffect::MakeForShader(
        SkString(sksl_wrapper_src), opts);
    if (!comp_result.effect) {
        std::string msg = "skia compose_with_child_shader: "
                          "failed to compile wrapper shader";
        if (comp_result.errorText.size() > 0) {
            msg += ": ";
            msg += comp_result.errorText.c_str();
        }
        return std::unexpected(general_error(msg));
    }
    sk_sp<SkRuntimeEffect> effect = std::move(comp_result.effect);

    // 3. The wrapper must have exactly one `uniform shader src;` child slot.
    //    Bind the source shader directly as a ChildPtr (no rasterisation).
    SkRuntimeEffect::ChildPtr child(source_shader);

    // 4. Create the baked shader (empty uniforms)
    sk_sp<SkData> empty_uniforms = SkData::MakeEmpty();
    sk_sp<SkShader> baked = effect->makeShader(
        std::move(empty_uniforms),
        SkSpan<const SkRuntimeEffect::ChildPtr>(&child, 1),
        nullptr);
    if (!baked) {
        return std::unexpected(general_error(
            "skia compose_with_child_shader: "
            "failed to create composed shader"));
    }

    // 5. Cache and return new handle
    uint64_t handle = next_preshader_handle_++;
    preshader_cache_[handle] = std::move(baked);
    return handle;
}

auto SkiaBackend::compose_with_child_shaders(
    const std::vector<uint64_t>& preshader_handles,
    const std::string& sksl_wrapper_src,
    const std::vector<uint8_t>& uniform_data) -> Result<uint64_t>
{
    if (preshader_handles.empty()) {
        return std::unexpected(general_error(
            "skia compose_with_child_shaders: at least one preshader required"));
    }

    // 1. Look up all preshaders from preshader_cache_
    std::vector<sk_sp<SkShader>> child_shaders;
    child_shaders.reserve(preshader_handles.size());
    for (auto h : preshader_handles) {
        auto pit = preshader_cache_.find(h);
        if (pit == preshader_cache_.end() || !pit->second) {
            return std::unexpected(general_error(
                "skia compose_with_child_shaders: invalid preshader handle"));
        }
        child_shaders.push_back(pit->second);
    }

    // 2. Compile the wrapper SkSL
    auto opts = SkRuntimeEffect::Options{};
    auto comp_result = SkRuntimeEffect::MakeForShader(
        SkString(sksl_wrapper_src), opts);
    if (!comp_result.effect) {
        std::string msg = "skia compose_with_child_shaders: "
                          "failed to compile wrapper shader";
        if (comp_result.errorText.size() > 0) {
            msg += ": ";
            msg += comp_result.errorText.c_str();
        }
        return std::unexpected(general_error(msg));
    }
    sk_sp<SkRuntimeEffect> effect = std::move(comp_result.effect);

    // 3. Bind all child shaders as ChildPtrs (no rasterisation)
    std::vector<SkRuntimeEffect::ChildPtr> children;
    children.reserve(child_shaders.size());
    for (auto& sh : child_shaders) {
        children.emplace_back(sh);
    }

    // 4. Create the baked shader (with uniforms if provided)
    sk_sp<SkData> uniform_skdata;
    if (uniform_data.empty()) {
        uniform_skdata = SkData::MakeEmpty();
    } else {
        uniform_skdata = SkData::MakeWithCopy(
            uniform_data.data(), uniform_data.size());
    }

    sk_sp<SkShader> baked = effect->makeShader(
        std::move(uniform_skdata),
        SkSpan<const SkRuntimeEffect::ChildPtr>(children.data(), children.size()),
        nullptr);
    if (!baked) {
        return std::unexpected(general_error(
            "skia compose_with_child_shaders: "
            "failed to create composed shader"));
    }

    // 5. Cache and return new handle
    uint64_t handle = next_preshader_handle_++;
    preshader_cache_[handle] = std::move(baked);
    return handle;
}

auto SkiaBackend::create_shader_with_children(
    const std::string& src,
    const std::vector<ShaderChildInfo>& childDescs,
    const std::vector<Value>& uniforms) -> Result<Value>
{
    // 1. Compile the SkSL effect (with uniform support)
    auto opts = SkRuntimeEffect::Options{};
    auto comp_result = SkRuntimeEffect::MakeForShader(SkString(src), opts);
    if (!comp_result.effect) {
        std::string msg =
            "skia create_shader_with_children: failed to compile shader";
        if (comp_result.errorText.size() > 0) {
            msg += ": ";
            msg += comp_result.errorText.c_str();
        }
        return std::unexpected(general_error(msg));
    }

    sk_sp<SkRuntimeEffect> effect = std::move(comp_result.effect);

    // 2. Build uniform data from Value list (convert each to float)
    std::vector<uint8_t> uniform_bytes;
    uniform_bytes.reserve(uniforms.size() * sizeof(float));
    for (const auto& val : uniforms) {
        float f = 0.0f;
        if (val.is_int()) {
            f = static_cast<float>(val.int_val());
        } else if (val.is_double()) {
            f = static_cast<float>(val.double_val());
        }
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&f);
        for (size_t i = 0; i < sizeof(float); ++i) {
            uniform_bytes.push_back(p[i]);
        }
    }

    sk_sp<SkData> uniform_data = SkData::MakeWithCopy(
        uniform_bytes.data(), uniform_bytes.size());
    if (!uniform_data) {
        return std::unexpected(general_error(
            "skia create_shader_with_children: "
            "failed to allocate uniform data"));
    }

    // 3. Create child shader references from child descriptions.
    //    For each child, create a temporary surface, snapshot it, and wrap
    //    its image shader as a ChildPtr.
    std::vector<SkRuntimeEffect::ChildPtr> children;
    children.reserve(childDescs.size());
    for (const auto& desc : childDescs) {
        auto info = SkImageInfo::MakeN32Premul(desc.width, desc.height);
        auto temp_surf = SkSurfaces::Raster(info);
        if (!temp_surf) {
            return std::unexpected(general_error(
                "skia create_shader_with_children: "
                "failed to create temporary surface for child"));
        }
        auto image = temp_surf->makeImageSnapshot();
        if (!image) {
            return std::unexpected(general_error(
                "skia create_shader_with_children: "
                "failed to snapshot child surface"));
        }
        auto child_shader = image->makeShader(
            SkTileMode::kRepeat, SkTileMode::kRepeat,
            SkSamplingOptions(SkFilterMode::kLinear));
        children.emplace_back(std::move(child_shader));
    }

    // 4. Create the shader with children bound
    sk_sp<SkShader> baked = effect->makeShader(
        std::move(uniform_data),
        SkSpan<const SkRuntimeEffect::ChildPtr>(
            children.data(), children.size()),
        nullptr);
    if (!baked) {
        return std::unexpected(general_error(
            "skia create_shader_with_children: "
            "failed to create shader with children"));
    }

    // 5. Cache and return handle as Value
    uint64_t handle = next_preshader_handle_++;
    preshader_cache_[handle] = std::move(baked);
    return Value(static_cast<int64_t>(handle));
}

}  // namespace pml

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?// Registration
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
namespace pml {

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
