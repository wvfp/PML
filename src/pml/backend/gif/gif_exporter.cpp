// ==========================================================================================================================================================================================================================================═
// PML GIF Exporter — giflib implementation
// ==========================================================================================================================================================================================================================================═

#include "gif_exporter.h"

#include <gif_lib.h>

// GifQuantizeBuffer is declared in getarg.h rather than gif_lib.h in giflib 5.2.x
extern "C" int GifQuantizeBuffer(unsigned int Width, unsigned int Height,
                      int *ColorMapSize, GifByteType *RedInput,
                      GifByteType *GreenInput, GifByteType *BlueInput,
                      GifByteType *OutputBuffer, GifColorType *OutputColorMap);

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace pml::backend::gif {

namespace {

// ---- RAII wrapper for GifFileType ----------------------------------------------------------------------------------------

/// Calls EGifCloseFile on destruction (ignoring close errors during unwind).
struct GifCloser {
    void operator()(GifFileType* g) const noexcept
    {
        if (g) {
            int err = 0;
            EGifCloseFile(g, &err);
        }
    }
};

using GifHandle = std::unique_ptr<GifFileType, GifCloser>;

// ---- Throw helpers --------------------------------------------------------------------------------------------------------------------─

[[noreturn]] void throw_gif_error(const std::string& context, int giflib_error)
{
    const char* msg = GifErrorString(giflib_error);
    throw std::runtime_error("GIF export: " + context + ": "
                             + (msg ? msg : "unknown error"));
}

[[noreturn]] void throw_gif_error(const std::string& context, GifFileType* gif)
{
    // gif->Error holds the last error code
    throw_gif_error(context, gif->Error);
}

} // anonymous namespace

// ==========================================================================================================================================================================================================================================═
// export_gif
// ==========================================================================================================================================================================================================================================═

void export_gif(const std::vector<std::vector<uint8_t>>& frames,
                int width, int height,
                const std::string& path,
                double fps)
{
    // ---- Preconditions --------------------------------------------------------------------------------------------------------
    if (frames.empty()) {
        throw std::runtime_error("GIF export: no frames provided");
    }

    const size_t expected_frame_size = static_cast<size_t>(width)
                                       * static_cast<size_t>(height) * 4;
    for (size_t i = 0; i < frames.size(); ++i) {
        if (frames[i].size() != expected_frame_size) {
            throw std::runtime_error(
                "GIF export: frame " + std::to_string(i) + " has size "
                + std::to_string(frames[i].size()) + ", expected "
                + std::to_string(expected_frame_size));
        }
    }

    // ---- Open output file ------------------------------------------------------------------------------------------------─
    int open_err = 0;
    GifFileType* raw_gif = EGifOpenFileName(path.c_str(), false, &open_err);
    if (!raw_gif) {
        throw_gif_error("failed to open " + path, open_err);
    }
    GifHandle gif(raw_gif);

    // ---- GIF89a header (required for animation + GCE) ----------------------------------------─
    EGifSetGifVersion(gif.get(), true);

    // ---- Logical screen descriptor --------------------------------------------------------------------------------
    // No global color map; each frame provides its own local color map.
    if (EGifPutScreenDesc(gif.get(), width, height, 8, 0, nullptr) == GIF_ERROR) {
        throw_gif_error("failed to write screen descriptor", gif.get());
    }

    // ---- Frame delay in centiseconds (giflib units) --------------------------------------------─
    const int delay_cs = std::max(1, static_cast<int>(std::round(100.0 / fps)));

    // ---- Reusable buffers ------------------------------------------------------------------------------------------------─
    const size_t pixel_count = static_cast<size_t>(width)
                               * static_cast<size_t>(height);
    std::vector<GifByteType> red_plane(pixel_count);
    std::vector<GifByteType> green_plane(pixel_count);
    std::vector<GifByteType> blue_plane(pixel_count);
    std::vector<GifPixelType> quantized(pixel_count);
    std::vector<GifColorType> palette_storage(256);

    // ---- Frame loop ------------------------------------------------------------------------------------------------------------─
    for (const auto& frame : frames) {
        // 1. Composite RGBA onto white background & separate RGB planes
        //
        //    Alpha blending formula:
        //      out = (alpha * fg + (255 - alpha) * bg) / 255
        //    bg = white (255, 255, 255)
        for (size_t i = 0; i < pixel_count; ++i) {
            const size_t off = i * 4;
            const int r = frame[off];
            const int g = frame[off + 1];
            const int b = frame[off + 2];
            const int a = frame[off + 3];
            const int inv_a = 255 - a;

            red_plane[i]   = static_cast<GifByteType>((a * r + inv_a * 255) / 255);
            green_plane[i] = static_cast<GifByteType>((a * g + inv_a * 255) / 255);
            blue_plane[i]  = static_cast<GifByteType>((a * b + inv_a * 255) / 255);
        }

        // 2. Quantize to 256-color adaptive palette
        int color_count = 256;
        if (GifQuantizeBuffer(
                static_cast<unsigned>(width),
                static_cast<unsigned>(height),
                &color_count,
                red_plane.data(),
                green_plane.data(),
                blue_plane.data(),
                quantized.data(),
                palette_storage.data()) == GIF_ERROR) {
            throw std::runtime_error("GIF export: quantization failed");
        }

        // 3. Create local color map
        // GifMakeMapObject requires the color count to be a power of two.
        // GifQuantizeBuffer may return an arbitrary count, so round up.
        int map_size = 1;
        while (map_size < color_count && map_size < 256) {
            map_size <<= 1;
        }
        if (map_size < 2) {
            map_size = 2;
        }

        ColorMapObject* local_map = GifMakeMapObject(
            map_size, palette_storage.data());
        if (!local_map) {
            throw std::runtime_error("GIF export: failed to create color map");
        }

        // 4. Write Graphics Control Extension (GCE)
        //
        //    Disposal mode 2 = restore to background color.
        //    Delay time in centiseconds = 100 / fps.
        {
            GraphicsControlBlock gcb{};
            gcb.DisposalMode = DISPOSE_BACKGROUND;
            gcb.UserInputFlag = false;
            gcb.DelayTime = delay_cs;
            gcb.TransparentColor = NO_TRANSPARENT_COLOR;

            GifByteType gce_ext[4];
            EGifGCBToExtension(&gcb, gce_ext);
            if (EGifPutExtension(gif.get(), GRAPHICS_EXT_FUNC_CODE,
                                 4, gce_ext) == GIF_ERROR) {
                GifFreeMapObject(local_map);
                throw_gif_error("failed to write GCE extension", gif.get());
            }
        }

        // 5. Write image descriptor with local color map
        if (EGifPutImageDesc(gif.get(), 0, 0, width, height,
                             false, local_map) == GIF_ERROR) {
            GifFreeMapObject(local_map);
            throw_gif_error("failed to write image descriptor", gif.get());
        }

        // 6. Write quantized pixel data
        if (EGifPutLine(gif.get(), quantized.data(),
                        static_cast<int>(pixel_count)) == GIF_ERROR) {
            GifFreeMapObject(local_map);
            throw_gif_error("failed to write pixel data", gif.get());
        }

        GifFreeMapObject(local_map);
    }

    // ---- Close file (writes GIF trailer) --------------------------------------------------------------------
    // GifHandle destructor calls EGifCloseFile automatically.
    // On normal exit, release the handle to close explicitly and check errors.
    int close_err = 0;
    if (EGifCloseFile(raw_gif, &close_err) == GIF_ERROR) {
        // Prevent double-close in deleter
        static_cast<void>(gif.release());
        throw_gif_error("failed to close " + path, close_err);
    }
    // Release so the deleter does not double-close
    static_cast<void>(gif.release());
}

} // namespace pml::backend::gif
