#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Canvas — Drawing surface and global state
// ───────────────────────────────────────────────────────────────────────────────
// A Canvas collects GraphicObjects for rendering.  Factories _canvas() and
// _sprite_canvas() create instances and set the global current canvas, which
// is then consumed by the render pipeline.
// ═══════════════════════════════════════════════════════════════════════════════

#include "objects.h"
#include "pml/api/context.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

/// A drawing surface that collects GraphicObjects for rendering.
///
/// Canvases are not themselves immutable — objects are added incrementally
/// via add().  The factory functions set the global current canvas so that
/// the render pipeline can find it.
struct Canvas {
    int width;                          ///< Canvas width in pixels.
    int height;                         ///< Canvas height in pixels.
    std::string bg_color{"#FFFFFF"};    ///< Background color string.
    std::vector<GraphicObject> objects; ///< Collected objects (z-order).
    std::string anchor{"top-left"};     ///< Anchor point for sprite canvases.
    int padding{0};                     ///< Padding around content.
    bool is_sprite{false};              ///< True if created via sprite-canvas.

    Canvas(int width_,
           int height_,
           std::string bg_color_ = "#FFFFFF",
           std::string anchor_ = "top-left",
           int padding_ = 0,
           bool is_sprite_ = false);

    /// Add a graphic object to the canvas (copied; takes ownership).
    void add(GraphicObject obj);
};

// ── Global current canvas (context-scoped) ───────────────────────────────

/// The current active canvas, set by _canvas() / _sprite_canvas() and
/// consumed by the render pipeline.  Mirrors Python's _current_canvas.
///
/// Access is routed through the current PMLContext so that multiple
/// PMLRuntime instances can coexist without sharing mutable canvas state.
[[nodiscard]] inline std::shared_ptr<Canvas>& current_canvas_ref() {
    return PMLContext::current().current_canvas;
}

// ── Factory functions ───────────────────────────────────────────────────

/// Create a regular canvas for static image rendering.
/// Sets @p g_current_canvas to the new instance.
///
/// @p kwargs may contain:
///   - "bg": background color string (default "#FFFFFF")
[[nodiscard]] std::shared_ptr<Canvas>
_canvas(int w, int h, const std::unordered_map<std::string, Value>& kwargs = {});

/// Create a sprite canvas with transparent background and anchor point.
/// Sets @p g_current_canvas to the new instance.
///
/// @p kwargs may contain:
///   - "bg": background color string (default "transparent")
///   - "anchor": anchor symbol or string (default Symbol("center-bottom"))
///   - "padding": integer padding (default 0)
[[nodiscard]] std::shared_ptr<Canvas>
_sprite_canvas(int w, int h, const std::unordered_map<std::string, Value>& kwargs = {});

/// Retrieve the current active canvas, or nullptr if none has been set.
[[nodiscard]] inline std::shared_ptr<Canvas> get_current_canvas() {
    return PMLContext::current().current_canvas;
}

} // namespace pml
