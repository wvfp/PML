#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Render Dispatch — coordinates BackendRegistry, Canvas, and GraphicObject
// to produce output files (PNG, JPG, GIF, SVG, etc.)
// ═══════════════════════════════════════════════════════════════════════════════

#include "objects.h"
#include "canvas.h"
#include "types.h"
#include "error.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// SVG Path Parsing (pure function)
// ═══════════════════════════════════════════════════════════════════════════════

/// A single SVG path command segment.
struct PathCommand {
    enum class Type : uint8_t {
        MoveTo,             // M  x y
        LineTo,             // L  x y
        HorizontalLineTo,   // H  x
        VerticalLineTo,     // V  y
        CurveTo,            // C  x1 y1 x2 y2 x y
        SmoothCurveTo,      // S  x2 y2 x y
        QuadTo,             // Q  x1 y1 x y
        SmoothQuadTo,       // T  x y
        ArcTo,              // A  rx ry x-axis-rotation large-arc-flag sweep-flag x y
        ClosePath,          // Z  (no params)
    };

    Type type;
    bool relative{false};        ///< true for lowercase command variants
    std::vector<double> args;    ///< Command arguments (varies by type)
};

/// Parse an SVG path data string (the "d" attribute) into a sequence of
/// PathCommand structs. This is a pure function with no dependencies on
/// any rendering backend.
///
/// Supports all standard SVG path commands: M/m, L/l, H/h, V/v, C/c, S/s,
/// Q/q, T/t, A/a, Z/z. Implicit commands after repeated identical commands
/// are handled per the SVG spec.
///
/// @param svg_path  The SVG path data string (e.g. "M10 10 L20 20 Z").
/// @return A vector of parsed path commands.
/// @throws PMLException (syntax_error) on malformed input.
[[nodiscard]] auto parse_svg_path(const std::string& svg_path)
    -> Result<std::vector<PathCommand>>;

// ═══════════════════════════════════════════════════════════════════════════════
// Render Functions
// ═══════════════════════════════════════════════════════════════════════════════

/// Render the current canvas contents to a file.
///
/// Flow:
///   1. Get active backend from BackendRegistry::active()
///   2. Create a surface at canvas dimensions with bg_color
///   3. For sprite canvases: auto-center content via AffineTransform
///   4. Draw each canvas object via backend.draw()
///   5. Save via backend.save_image()
///   6. For sprite canvases: write a sidecar .meta.json
///
/// @param filename  Output file path.
/// @param fmt       Output format ("PNG", "JPG", "GIF", "SVG", etc.).
///                  If empty, detected from filename extension.
/// @param canvas    The canvas to render. If null, uses get_current_canvas().
/// @param fps       Frames per second for animation output. If > 0, an
///                  animated image (e.g. GIF) is produced using the global
///                  timeline. Defaults to 0 (static image).
/// @return The output filename on success, or an error.
[[nodiscard]] auto render(
    const std::string& filename,
    const std::string& fmt = "",
    std::shared_ptr<Canvas> canvas = nullptr,
    int fps = 0)
    -> Result<std::string>;

/// Render a GraphicObject at multiple scales.
///
/// Creates @p scales.size() files, each suffixed with @Nx:
///   - "name.png"       (1x, if 1 is in scales)
///   - "name@2x.png"    (2x)
///   - "name@4x.png"    (4x)
///
/// @param name        Base output name (without suffix/extension).
/// @param content     The GraphicObject to render at each scale.
/// @param scales      List of scale factors (e.g. {1, 2, 4}).
/// @param base_width  Base width in pixels.
/// @param base_height Base height in pixels.
/// @param fmt         Output format (default "PNG").
/// @return List of output filenames created.
[[nodiscard]] auto render_set(
    const std::string& name,
    const GraphicObject& content,
    const std::vector<int>& scales,
    int base_width = 64,
    int base_height = 64,
    const std::string& fmt = "PNG")
    -> Result<std::vector<std::string>>;

/// Render multiple sprites into a grid-based spritesheet.
///
/// Args after filename are treated as GraphicObjects placed in row-major
/// order into a grid with the given dimensions.
///
/// Produces:
///   - The spritesheet image (PNG)
///   - A sidecar .spritesheet.json metadata file
///
/// @param filename    Output file path (extension determines format).
/// @param sprites     The GraphicObjects to tile.
/// @param cols        Number of columns in the grid (default 4).
/// @param cell_width  Width of each cell in pixels (default 64).
/// @param cell_height Height of each cell in pixels (default 64).
/// @param padding     Padding between cells in pixels (default 2).
/// @param bg          Background colour (default "transparent").
/// @return The output filename on success, or an error.
[[nodiscard]] auto render_spritesheet(
    const std::string& filename,
    const std::vector<GraphicObject>& sprites,
    int cols = 4,
    int cell_width = 64,
    int cell_height = 64,
    int padding = 2,
    const std::string& bg = "transparent")
    -> Result<std::string>;

// ═══════════════════════════════════════════════════════════════════════════════
// Environment Registration
// ═══════════════════════════════════════════════════════════════════════════════

/// Bind render functions (render, render-set, render-spritesheet) into a
/// PML environment as BuiltinProcedure values.
///
/// @param env  The environment to define symbols in.
void register_render(std::shared_ptr<Environment> env);

}  // namespace pml
