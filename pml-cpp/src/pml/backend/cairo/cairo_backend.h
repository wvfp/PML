#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Cairo Render Backend
// ═══════════════════════════════════════════════════════════════════════════════
//
// Concrete RenderBackend implementation using Cairo for CPU-based 2D rendering.
// Supports: circle, rect, ellipse, line, polygon, path, group shapes.
// Output: PNG via cairo_surface_write_to_png().
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/backend/backend.h"

#include <cairo.h>
#include <memory>
#include <string>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// CairoSurface — concrete Surface for Cairo rendering
// ═══════════════════════════════════════════════════════════════════════════════

/// A Cairo-backed render surface wrapping a cairo_surface_t and a cairo_t.
/// Owns both objects; destroys them on destruction.
struct CairoSurface final : Surface {
    cairo_surface_t* surface{nullptr};
    cairo_t* cr{nullptr};

    CairoSurface(int w, int h, cairo_surface_t* s, cairo_t* c)
        : Surface(w, h), surface(s), cr(c) {}

    ~CairoSurface() override
    {
        if (cr) cairo_destroy(cr);
        if (surface) cairo_surface_destroy(surface);
    }

    // Non-copyable
    CairoSurface(const CairoSurface&) = delete;
    CairoSurface& operator=(const CairoSurface&) = delete;

    // Movable
    CairoSurface(CairoSurface&& other) noexcept
        : Surface(other.width, other.height),
          surface(other.surface),
          cr(other.cr)
    {
        other.surface = nullptr;
        other.cr = nullptr;
    }

    CairoSurface& operator=(CairoSurface&& other) noexcept
    {
        if (this != &other) {
            if (cr) cairo_destroy(cr);
            if (surface) cairo_surface_destroy(surface);
            width = other.width;
            height = other.height;
            surface = other.surface;
            cr = other.cr;
            other.surface = nullptr;
            other.cr = nullptr;
        }
        return *this;
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// CairoBackend
// ═══════════════════════════════════════════════════════════════════════════════

/// Render backend using Cairo for CPU-based raster rendering.
///
/// Capabilities:
///   - RasterCPU: full CPU-based rendering
///   - FontRendering: text via Cairo font API
///   - AlphaComposite: native alpha blending
///
/// Not supported:
///   - GPU acceleration, shaders, vector output, GIF animation
class CairoBackend final : public RenderBackend {
public:
    CairoBackend() = default;

    // ── Metadata ────────────────────────────────────────────────────

    [[nodiscard]] auto info() const -> BackendInfo override;

    // ── Surface lifecycle ───────────────────────────────────────────

    [[nodiscard]] auto create_surface(int width, int height, uint32_t bg_color)
        -> std::unique_ptr<Surface> override;

    // ── Drawing ─────────────────────────────────────────────────────

    auto draw(Surface& surface, const GraphicObject& obj)
        -> Result<void> override;

    // ── Output ──────────────────────────────────────────────────────

    auto save_image(Surface& surface, const std::string& path, const std::string& format)
        -> Result<void> override;

    auto save_animation(const std::vector<Surface*>& frames,
                        const std::string& path,
                        const std::string& format,
                        int fps)
        -> Result<void> override;

    // ── Compositing ─────────────────────────────────────────────────

    auto composite(Surface& dst, Surface& src, int x, int y)
        -> Result<void> override;

    // ── Shaders ─────────────────────────────────────────────────────

    auto compile_shader(const std::string& glsl)
        -> Result<uint64_t> override;

private:
    // ── Shape rendering ─────────────────────────────────────────────

    /// Draw a single GraphicObject (handles group recursively).
    void draw_shape(cairo_t* cr, const GraphicObject& obj);

    /// Build the Cairo path for the given shape.
    void build_path(cairo_t* cr, const GraphicObject& obj);

    /// Apply fill then stroke to the current Cairo path.
    void apply_fill_stroke(cairo_t* cr, const GraphicObject& obj);

    // ── Shape helpers ───────────────────────────────────────────────

    void build_circle(cairo_t* cr, const GraphicObject& obj);
    void build_rect(cairo_t* cr, const GraphicObject& obj);
    void build_ellipse(cairo_t* cr, const GraphicObject& obj);
    void build_line(cairo_t* cr, const GraphicObject& obj);
    void build_polygon(cairo_t* cr, const GraphicObject& obj);
    void build_path_shape(cairo_t* cr, const GraphicObject& obj);
};

} // namespace pml
