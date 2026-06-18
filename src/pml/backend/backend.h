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

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Forward declarations
// ═══════════════════════════════════════════════════════════════════════════════

class GraphicObject;  ///< Defined in pml/graphics/graphic_object.h (Task 10)

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
class RenderBackend {
public:
    virtual ~RenderBackend() = default;

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

    // ── Compositing ────────────────────────────────────────────────────

    /// Composite a source surface onto a destination surface at (x, y).
    /// The source surface is blended onto the destination using alpha
    /// compositing.  Used by spritesheet rendering.
    virtual auto composite(
        Surface& dst, Surface& src, int x, int y) -> Result<void> = 0;

    // ── Shaders ─────────────────────────────────────────────────────

    /// Compile a GLSL shader string. Returns a backend-specific shader
    /// handle (opaque integer) on success, or an error on failure.
    virtual auto compile_shader(const std::string& glsl) -> Result<uint64_t> = 0;
};

}  // namespace pml
