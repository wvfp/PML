#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Render Backend — Abstract Base Class
// ═══════════════════════════════════════════════════════════════════════════════
//
// All render backends (Null, Skia, Cairo, etc.) inherit from RenderBackend
// and implement the pure virtual methods below.
//
// Surface is a base struct with virtual destructor; backends subclass it to
// store their own pixel/raster/context data.
// ═══════════════════════════════════════════════════════════════════════════════

#include "capabilities.h"
#include "pml/core/error.h"
#include "pml/filter/filter_backend.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Forward declarations
// ═══════════════════════════════════════════════════════════════════════════════

class GraphicObject;  ///< Defined in pml/graphics/graphic_object.h (Task 10)
enum class BlendMode : uint8_t; ///< Defined in pml/layer/blend_mode.h

// ═══════════════════════════════════════════════════════════════════════════════
// Surface — abstract render target
// ═══════════════════════════════════════════════════════════════════════════════

/// Abstract render target (canvas / raster image).
/// Each backend defines its own surface by inheriting from this struct.
struct Surface {
    int width{};     ///< Surface width in pixels
    int height{};    ///< Surface height in pixels

    Surface() = default;
    Surface(int w, int h) : width(w), height(h) {}
    virtual ~Surface() = default;

    // Non-copyable, movable.
    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;
    Surface(Surface&&) = default;
    Surface& operator=(Surface&&) = default;
};

// ═══════════════════════════════════════════════════════════════════════════════
// RenderBackend — abstract base class
// ═══════════════════════════════════════════════════════════════════════════════

/// Abstract base class for all render backends.
///
/// Methods use `Result<T>` (std::expected) for error reporting. A method
/// returning `Result<void>` succeeds with an empty expected or fails with
/// a PMLException describing what went wrong.
class RenderBackend : public FilterBackend {
public:
    ~RenderBackend() override = default;

    // ── Metadata ────────────────────────────────────────────────────

    /// Return metadata describing this backend (name, description, capabilities).
    [[nodiscard]] virtual auto info() const -> BackendInfo = 0;

    // ── Surface lifecycle ───────────────────────────────────────────

    /// Create a new surface (render target) with the given dimensions
    /// and background colour (packed as 0xRRGGBBAA).
    [[nodiscard]] virtual auto create_surface(
        int width, int height, uint32_t bg_color) -> std::unique_ptr<Surface> = 0;

    // ── Drawing ─────────────────────────────────────────────────────

    /// Draw a GraphicObject onto the given surface.
    virtual auto draw(Surface& surface, const GraphicObject& obj) -> Result<void> = 0;

    // ── Output ──────────────────────────────────────────────────────

    /// Save a surface to an image file. Format hint (e.g. "png", "jpg")
    /// may be backend-dependent.
    virtual auto save_image(
        Surface& surface,
        const std::string& path,
        const std::string& format) -> Result<void> = 0;

    /// Save a sequence of surfaces as an animated image (e.g. GIF).
    virtual auto save_animation(
        const std::vector<Surface*>& frames,
        const std::string& path,
        const std::string& format,
        int fps) -> Result<void> = 0;

    /// Load an image file into a backend-specific Surface.
    /// The caller owns the returned surface.
    [[nodiscard]] virtual auto load_image(
        const std::string& path) -> Result<std::unique_ptr<Surface>> = 0;

    // ── Compositing ────────────────────────────────────────────────────

    /// Composite a source surface onto a destination surface at (x, y).
    /// The source surface is blended onto the destination using alpha
    /// compositing.  Used by spritesheet rendering.
    virtual auto composite(
        Surface& dst, Surface& src, int x, int y) -> Result<void> = 0;

    /// Composite with a specific blend mode and global opacity.
    virtual auto composite_with_blend(
        Surface& dst, Surface& src, int x, int y,
        BlendMode blend, float opacity) -> Result<void> = 0;

    /// Use the alpha channel of `mask` to modulate the alpha of `dst`.
    /// dst pixel alpha becomes dst_alpha * (mask_alpha / 255).
    virtual auto apply_mask(Surface& dst, Surface& mask) -> Result<void> = 0;

    /// Draw a GraphicObject into a fresh surface of the requested size.
    /// Used by the compositor to capture a layer as a raster image.
    [[nodiscard]] virtual auto draw_to_new_surface(
        const GraphicObject& obj,
        int width, int height,
        uint32_t bg_color) -> Result<std::unique_ptr<Surface>> = 0;

    /// True if this backend supports non-Normal blend modes.
    [[nodiscard]] virtual auto supports_blend_mode() const noexcept -> bool = 0;

    /// True if this backend can render layers to off-screen surfaces.
    [[nodiscard]] virtual auto supports_layer_compositing() const noexcept -> bool = 0;

    // ── Shaders ─────────────────────────────────────────────────────

    /// Compile a GLSL shader string. Returns a backend-specific shader
    /// handle (opaque integer) on success, or an error on failure.
    virtual auto compile_shader(const std::string& glsl) -> Result<uint64_t> = 0;

    // ── Noise Shaders ────────────────────────────────────────────────

    /// Noise shader type.
    enum class NoiseType {
        Fractal,    ///< Fractal noise (smooth, continuous)
        Turbulence,  ///< Turbulence noise (harsher, more variation)
    };

    /// Create a Perlin noise shader.
    /// @param type       Noise type (Fractal or Turbulence)
    /// @param base_freq_x  Base frequency X (usually 0.0 - 1.0)
    /// @param base_freq_y  Base frequency Y (usually 0.0 - 1.0)
    /// @param octaves    Number of octaves (1-8 typical)
    /// @param seed       Random seed
    /// @param tile_w    Tile width for seamless noise (0 = no tiling)
    /// @param tile_h    Tile height for seamless noise (0 = no tiling)
    /// @return          Shader handle on success
    virtual auto create_noise_shader(NoiseType type,
                                    float base_freq_x, float base_freq_y,
                                    int octaves, float seed,
                                    int tile_w, int tile_h)
        -> Result<uint64_t>
    {
        return std::unexpected(general_error(
            "noise shaders not supported by this backend"));
    }

    // ── Shader Uniforms ─────────────────────────────────────────────

    /// Create a shader with uniform values bound.
    /// SkSL uniforms are passed as a raw byte vector (layout matches SkRuntimeEffect).
    /// @param shader_handle  Handle from compile_shader
    /// @param uniform_data  Raw uniform data (floats aligned to 4 bytes)
    /// @return              New shader handle with uniforms bound
    virtual auto create_shader_with_uniforms(uint64_t shader_handle,
                                             const std::vector<uint8_t>& uniform_data)
        -> Result<uint64_t>
    {
        return std::unexpected(general_error(
            "shader uniforms not supported by this backend"));
    }

};

}  // namespace pml
