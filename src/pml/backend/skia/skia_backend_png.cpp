// ═══════════════════════════════════════════════════════════════════════════════
// PML Skia render backend — PNG loading via libpng
// ═══════════════════════════════════════════════════════════════════════════════
//
// The prebuilt skia.lib in this tree does not expose a working SkPngDecoder,
// so we decode PNGs through libpng and upload the pixels into an SkBitmap.
// ═══════════════════════════════════════════════════════════════════════════════

#include "skia_backend_internal.h"

#include <format>
#include "pml/core/platform.h"

namespace pml {

Result<std::unique_ptr<Surface>> load_png_with_libpng(
    const std::string& path)
{
    FILE* fp = nullptr;
    if (pml_fopen(fp, path.c_str(), "rb")) {
        return std::unexpected(resource_error(
            std::format("skia: cannot open image file: {}", path)));
    }

    constexpr size_t png_sig_size = 8;
    std::array<uint8_t, png_sig_size> sig{};
    if (fread(sig.data(), 1, png_sig_size, fp) != png_sig_size) {
        fclose(fp);
        return std::unexpected(resource_error(
            std::format("skia: cannot read image file: {}", path)));
    }
    if (png_sig_cmp(sig.data(), 0, png_sig_size) != 0) {
        fclose(fp);
        return std::unexpected(resource_error(
            std::format("skia: not a PNG file: {}", path)));
    }

    png_structp png = png_create_read_struct(
        PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        return std::unexpected(resource_error(
            "skia: png_create_read_struct failed"));
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        fclose(fp);
        return std::unexpected(resource_error(
            "skia: png_create_info_struct failed"));
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return std::unexpected(resource_error(
            std::format("skia: libpng decode error: {}", path)));
    }

    png_init_io(png, fp);
    png_set_sig_bytes(png, static_cast<int>(png_sig_size));
    png_read_info(png, info);

    int w = static_cast<int>(png_get_image_width(png, info));
    int h = static_cast<int>(png_get_image_height(png, info));
    png_byte bit_depth = png_get_bit_depth(png, info);
    png_byte color_type = png_get_color_type(png, info);

    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }
    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }
    if (bit_depth == 16) {
        png_set_strip_16(png);
    }
    if (color_type == PNG_COLOR_TYPE_RGB) {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }

    png_read_update_info(png, info);

    const size_t row_bytes = png_get_rowbytes(png, info);
    std::vector<uint8_t> temp_row(static_cast<size_t>(row_bytes));

    SkImageInfo sk_info = SkImageInfo::MakeN32Premul(w, h);
    SkBitmap bitmap;
    if (!bitmap.tryAllocPixels(sk_info)) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return std::unexpected(resource_error(
            "skia: failed to allocate image pixels"));
    }

    uint8_t* dst_pixels = static_cast<uint8_t*>(bitmap.getPixels());
    const size_t dst_row_bytes = bitmap.rowBytes();

    for (int y = 0; y < h; ++y) {
        png_read_row(png, temp_row.data(), nullptr);
        uint8_t* dst = dst_pixels + y * dst_row_bytes;
        for (int x = 0; x < w; ++x) {
            // libpng gives RGBA; Skia kN32 on little-endian is BGRA.
            dst[x * 4 + 0] = temp_row[x * 4 + 2];  // B <- R
            dst[x * 4 + 1] = temp_row[x * 4 + 1];  // G <- G
            dst[x * 4 + 2] = temp_row[x * 4 + 0];  // R <- B
            dst[x * 4 + 3] = temp_row[x * 4 + 3];  // A <- A
        }
    }

    png_read_end(png, nullptr);
    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);

    auto skia_surf = std::make_unique<SkiaSurface>(std::move(bitmap));
    return std::unique_ptr<Surface>(std::move(skia_surf));
}

}  // namespace pml
