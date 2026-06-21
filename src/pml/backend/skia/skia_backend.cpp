// ═══════════════════════════════════════════════════════════════════════════════
// PML Skia render backend — SkiaBackend class + I/O + shaders + registration
// ═══════════════════════════════════════════════════════════════════════════════
//
// Member function definitions for the SkiaBackend class (core rendering, I/O,
// compositing, and shader operations).  Draw primitives live in
// skia_backend_draw.cpp, PNG loading in skia_backend_png.cpp, and filter
// operations in skia_backend_filter.cpp.
// ═══════════════════════════════════════════════════════════════════════════════

#include "skia_backend_internal.h"

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Drawing
// ═══════════════════════════════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════════════════════════════
// Output — save_image (PNG via libpng)
// ═══════════════════════════════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════════════════════════════
// Output — save_animation (GIF export)
// ═══════════════════════════════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════════════════════════════
// Image loading
// ═══════════════════════════════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════════════════════════════
// Compositing
// ═══════════════════════════════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════════════════════════════
// Shaders
// ═══════════════════════════════════════════════════════════════════════════════

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
                                      int tile_w, int tile_h) -> Result<uint64_t>
{
    SkISize tile_size = (tile_w > 0 && tile_h > 0)
        ? SkISize{tile_w, tile_h}
        : SkISize::MakeEmpty();

    sk_sp<SkShader> noise_shader;

    if (type == NoiseType::Fractal) {
        noise_shader = SkShaders::MakeFractalNoise(
            base_freq_x, base_freq_y, octaves, seed,
            tile_size.isEmpty() ? nullptr : &tile_size);
    } else {
        noise_shader = SkShaders::MakeTurbulence(
            base_freq_x, base_freq_y, octaves, seed,
            tile_size.isEmpty() ? nullptr : &tile_size);
    }

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

// ═══════════════════════════════════════════════════════════════════════════════
// Registration
// ═══════════════════════════════════════════════════════════════════════════════

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
