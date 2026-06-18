// ═══════════════════════════════════════════════════════════════════════════════
// Cairo Render Backend — Implementation
// ═══════════════════════════════════════════════════════════════════════════════
//
// Dispatches GraphicObject shapes to Cairo drawing commands.  Groups are
// handled recursively with cairo_save/cairo_restore and transform stacking.
// PNG output via cairo_surface_write_to_png().
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/backend/cairo/cairo_backend.h"
#include "pml/backend/color_helpers.h"
#include "pml/backend/registry.h"
#include "pml/graphics/graphic_object.h"

#include <cairo.h>
#include <cctype>
#include <cmath>
#include <numbers>
#include <string>
#include <vector>

namespace pml {

// ======================================================================
// Internal helpers (anonymous namespace)
// ======================================================================
namespace {

/// Safely extract a numeric parameter from a GraphicObject's params map.
/// Handles both double and int64_t Value types.
[[nodiscard]] std::optional<double> get_num_param(
    const std::unordered_map<std::string, Value>& params,
    const std::string& key)
{
    auto it = params.find(key);
    if (it == params.end()) return std::nullopt;
    if (auto* d = std::get_if<double>(&it->second)) return *d;
    if (auto* i = std::get_if<int64_t>(&it->second)) return static_cast<double>(*i);
    return std::nullopt;
}

/// Safely extract a string parameter from a GraphicObject's params map.
[[nodiscard]] std::optional<std::string> get_str_param(
    const std::unordered_map<std::string, Value>& params,
    const std::string& key)
{
    auto it = params.find(key);
    if (it == params.end()) return std::nullopt;
    if (auto* s = std::get_if<std::string>(&it->second)) return *s;
    return std::nullopt;
}

/// Extract polygon points from a Value containing a list of [x, y] pairs.
[[nodiscard]] std::vector<std::pair<double, double>> extract_points(
    const Value& val)
{
    std::vector<std::pair<double, double>> points;
    auto* list_ptr = std::get_if<std::shared_ptr<ValueList>>(&val);
    if (!list_ptr) return points;

    for (const auto& elem : (*list_ptr)->elements) {
        auto* pt_ptr = std::get_if<std::shared_ptr<ValueList>>(&elem);
        if (!pt_ptr || (*pt_ptr)->elements.size() < 2) continue;

        double x = 0.0, y = 0.0;
        if (auto* d = std::get_if<double>(&(*pt_ptr)->elements[0])) {
            x = *d;
        } else if (auto* i = std::get_if<int64_t>(&(*pt_ptr)->elements[0])) {
            x = static_cast<double>(*i);
        }
        if (auto* d = std::get_if<double>(&(*pt_ptr)->elements[1])) {
            y = *d;
        } else if (auto* i = std::get_if<int64_t>(&(*pt_ptr)->elements[1])) {
            y = static_cast<double>(*i);
        }
        points.emplace_back(x, y);
    }
    return points;
}

/// Convert ARGB uint32 (from color_helpers parse_color) to Cairo RGBA doubles.
/// Color is stored as AARRGGBB (alpha in bits 24-31).
[[nodiscard]] inline double argb_alpha(uint32_t argb) noexcept
{
    return ((argb >> 24) & 0xFF) / 255.0;
}
[[nodiscard]] inline double argb_red(uint32_t argb) noexcept
{
    return ((argb >> 16) & 0xFF) / 255.0;
}
[[nodiscard]] inline double argb_green(uint32_t argb) noexcept
{
    return ((argb >> 8) & 0xFF) / 255.0;
}
[[nodiscard]] inline double argb_blue(uint32_t argb) noexcept
{
    return (argb & 0xFF) / 255.0;
}

/// Apply a parsed ARGB color as Cairo source.
void cairo_set_source_argb(cairo_t* cr, uint32_t argb)
{
    cairo_set_source_rgba(cr,
        argb_red(argb), argb_green(argb), argb_blue(argb), argb_alpha(argb));
}

// ======================================================================
// SVG path parsing (simplified, absolute coords only)
// ======================================================================

/// Tokenize SVG path data string into commands and numeric tokens.
[[nodiscard]] std::vector<std::string> tokenize_svg_path(const std::string& d)
{
    std::vector<std::string> tokens;
    size_t i = 0;
    while (i < d.size()) {
        const char c = d[i];
        if (std::isspace(static_cast<unsigned char>(c))) {
            ++i;
            continue;
        }
        // Command letter
        if (std::isalpha(static_cast<unsigned char>(c))) {
            tokens.push_back(std::string(1, c));
            ++i;
            continue;
        }
        // Numeric: optional sign, digits, optional decimal point
        if (c == '+' || c == '-' || std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            size_t start = i;
            if (c == '+' || c == '-') ++i;
            while (i < d.size() && (std::isdigit(static_cast<unsigned char>(d[i])) || d[i] == '.')) {
                ++i;
            }
            // Handle case like "-.5" — second char is '.'
            if (start + 1 == i && (d[start] == '-' || d[start] == '+') &&
                i < d.size() && d[i] == '.')
            {
                ++i;
                while (i < d.size() && std::isdigit(static_cast<unsigned char>(d[i]))) ++i;
            }
            tokens.push_back(d.substr(start, i - start));
            continue;
        }
        // Comma — skip (used as separator in SVG)
        if (c == ',') {
            ++i;
            continue;
        }
        // Unknown character — skip
        ++i;
    }
    return tokens;
}

/// Parse SVG path data into polylines.
/// Supports: M (moveto), L (lineto), H (horizontal lineto),
/// V (vertical lineto), Z/z (closepath).  Absolute coordinates only.
[[nodiscard]] std::vector<std::vector<std::pair<double, double>>> parse_svg_path(
    const std::string& d)
{
    const auto tokens = tokenize_svg_path(d);
    std::vector<std::vector<std::pair<double, double>>> polylines;
    std::vector<std::pair<double, double>> current;
    double cx = 0.0, cy = 0.0;
    size_t i = 0;

    while (i < tokens.size()) {
        const std::string& cmd = tokens[i];
        if (cmd == "M") {
            if (!current.empty()) {
                polylines.push_back(std::move(current));
                current.clear();
            }
            if (i + 2 < tokens.size()) {
                cx = std::stod(tokens[i + 1]);
                cy = std::stod(tokens[i + 2]);
                current.emplace_back(cx, cy);
                i += 3;
            } else {
                ++i;
            }
        } else if (cmd == "L") {
            if (i + 2 < tokens.size()) {
                cx = std::stod(tokens[i + 1]);
                cy = std::stod(tokens[i + 2]);
                current.emplace_back(cx, cy);
                i += 3;
            } else {
                ++i;
            }
        } else if (cmd == "H") {
            if (i + 1 < tokens.size()) {
                cx = std::stod(tokens[i + 1]);
                current.emplace_back(cx, cy);
                i += 2;
            } else {
                ++i;
            }
        } else if (cmd == "V") {
            if (i + 1 < tokens.size()) {
                cy = std::stod(tokens[i + 1]);
                current.emplace_back(cx, cy);
                i += 2;
            } else {
                ++i;
            }
        } else if (cmd == "Z" || cmd == "z") {
            if (!current.empty()) {
                // Close path by adding first point again (Cairo close_path
                // draws a straight line back to the start)
                current.push_back(current.front());
                polylines.push_back(std::move(current));
                current.clear();
            }
            ++i;
        } else if (cmd.size() == 1 && std::isalpha(static_cast<unsigned char>(cmd[0]))) {
            // Unknown command — skip
            ++i;
        } else {
            // Numeric token without preceding command — treated as L continuation
            if (!current.empty() && i + 1 < tokens.size()) {
                cx = std::stod(tokens[i]);
                cy = std::stod(tokens[i + 1]);
                current.emplace_back(cx, cy);
                i += 2;
            } else {
                ++i;
            }
        }
    }

    if (!current.empty()) {
        polylines.push_back(std::move(current));
    }

    return polylines;
}

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// Metadata
// ═══════════════════════════════════════════════════════════════════════════════

auto CairoBackend::info() const -> BackendInfo
{
    return BackendInfo{
        .name = "cairo",
        .description = "Cairo 2D render backend — CPU raster, toy-text font",
        .capabilities = BackendCap::RasterCPU
                      | BackendCap::FontRendering,
    };
}

// ═══════════════════════════════════════════════════════════════════════════════
// Surface lifecycle
// ═══════════════════════════════════════════════════════════════════════════════

auto CairoBackend::create_surface(int width, int height, uint32_t bg_color)
    -> std::unique_ptr<Surface>
{
    cairo_surface_t* surf = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status(surf) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surf);
        return nullptr;
    }

    cairo_t* cr = cairo_create(surf);
    if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        cairo_destroy(cr);
        cairo_surface_destroy(surf);
        return nullptr;
    }

    // Fill with background colour
    cairo_set_source_argb(cr, bg_color);
    cairo_paint(cr);

    return std::make_unique<CairoSurface>(width, height, surf, cr);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Drawing
// ═══════════════════════════════════════════════════════════════════════════════

auto CairoBackend::draw(Surface& surface, const GraphicObject& obj)
    -> Result<void>
{
    auto& cs = static_cast<CairoSurface&>(surface);
    if (!cs.cr) {
        return std::unexpected(
            general_error("CairoBackend: Cairo context is null"));
    }
    draw_shape(cs.cr, obj);
    return {};
}

void CairoBackend::draw_shape(cairo_t* cr, const GraphicObject& obj)
{
    if (obj.shape_type == "group") {
        // Groups: apply group transform, draw each child
        cairo_save(cr);
        if (!obj.transform.is_identity()) {
            auto m = obj.transform.to_cairo_matrix();
            cairo_transform(cr, &m);
        }
        for (const auto& child : obj.children) {
            draw_shape(cr, child);
        }
        cairo_restore(cr);
        return;
    }

    // Non-group shapes: save, transform, build path, fill/stroke, restore
    cairo_save(cr);

    if (!obj.transform.is_identity()) {
        auto m = obj.transform.to_cairo_matrix();
        cairo_transform(cr, &m);
    }

    build_path(cr, obj);
    apply_fill_stroke(cr, obj);

    cairo_restore(cr);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Path building
// ═══════════════════════════════════════════════════════════════════════════════

void CairoBackend::build_path(cairo_t* cr, const GraphicObject& obj)
{
    const auto& type = obj.shape_type;

    if (type == "circle") {
        build_circle(cr, obj);
    } else if (type == "rect") {
        build_rect(cr, obj);
    } else if (type == "ellipse") {
        build_ellipse(cr, obj);
    } else if (type == "line") {
        build_line(cr, obj);
    } else if (type == "polygon") {
        build_polygon(cr, obj);
    } else if (type == "path") {
        build_path_shape(cr, obj);
    } else if (type == "text") {
        // Text: render directly (no path needed)
        auto text = get_str_param(obj.params, "text");
        if (text.has_value()) {
            auto x = get_num_param(obj.params, "x").value_or(0.0);
            auto y = get_num_param(obj.params, "y").value_or(0.0);
            auto font_size = get_num_param(obj.params, "font_size").value_or(12.0);

            cairo_select_font_face(cr, "sans-serif",
                CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
            cairo_set_font_size(cr, font_size);
            cairo_move_to(cr, x, y);

            if (obj.fill.has_value()) {
                auto argb = parse_color(*obj.fill);
                if (argb.has_value()) {
                    cairo_set_source_argb(cr, *argb);
                }
            }
            cairo_show_text(cr, text->c_str());
        }
    } else if (type == "image") {
        auto src = get_str_param(obj.params, "src");
        if (src.has_value()) {
            auto x = get_num_param(obj.params, "x").value_or(0.0);
            auto y = get_num_param(obj.params, "y").value_or(0.0);

#if CAIRO_HAS_PNG_FUNCTIONS
            cairo_surface_t* img = cairo_image_surface_create_from_png(
                src->c_str());
            if (cairo_surface_status(img) == CAIRO_STATUS_SUCCESS) {
                cairo_set_source_surface(cr, img, x, y);
                cairo_paint(cr);
                cairo_surface_destroy(img);
            } else {
                cairo_surface_destroy(img);
            }
#else
            // Cairo built without PNG support — skip image shapes.
            (void)src; (void)x; (void)y;
#endif
        }
    }
}

void CairoBackend::build_circle(cairo_t* cr, const GraphicObject& obj)
{
    auto x = get_num_param(obj.params, "x").value_or(0.0);
    auto y = get_num_param(obj.params, "y").value_or(0.0);
    auto r = get_num_param(obj.params, "r").value_or(1.0);

    cairo_arc(cr, x, y, r, 0.0, 2.0 * std::numbers::pi_v<double>);
}

void CairoBackend::build_rect(cairo_t* cr, const GraphicObject& obj)
{
    auto x = get_num_param(obj.params, "x").value_or(0.0);
    auto y = get_num_param(obj.params, "y").value_or(0.0);
    auto w = get_num_param(obj.params, "w").value_or(0.0);
    auto h = get_num_param(obj.params, "h").value_or(0.0);

    cairo_rectangle(cr, x, y, w, h);
}

void CairoBackend::build_ellipse(cairo_t* cr, const GraphicObject& obj)
{
    auto cx = get_num_param(obj.params, "cx").value_or(0.0);
    auto cy = get_num_param(obj.params, "cy").value_or(0.0);
    auto rx = get_num_param(obj.params, "rx").value_or(1.0);
    auto ry = get_num_param(obj.params, "ry").value_or(1.0);

    // Draw unit circle, scale/translate to ellipse
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_scale(cr, rx, ry);
    cairo_arc(cr, 0.0, 0.0, 1.0, 0.0, 2.0 * std::numbers::pi_v<double>);
    cairo_restore(cr);
}

void CairoBackend::build_line(cairo_t* cr, const GraphicObject& obj)
{
    auto x1 = get_num_param(obj.params, "x1").value_or(0.0);
    auto y1 = get_num_param(obj.params, "y1").value_or(0.0);
    auto x2 = get_num_param(obj.params, "x2").value_or(0.0);
    auto y2 = get_num_param(obj.params, "y2").value_or(0.0);

    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
}

void CairoBackend::build_polygon(cairo_t* cr, const GraphicObject& obj)
{
    auto it = obj.params.find("points");
    if (it == obj.params.end()) return;

    auto points = extract_points(it->second);
    if (points.empty()) return;

    cairo_move_to(cr, points[0].first, points[0].second);
    for (size_t i = 1; i < points.size(); ++i) {
        cairo_line_to(cr, points[i].first, points[i].second);
    }
    cairo_close_path(cr);
}

void CairoBackend::build_path_shape(cairo_t* cr, const GraphicObject& obj)
{
    auto d = get_str_param(obj.params, "d");
    if (!d.has_value()) return;

    auto polylines = parse_svg_path(*d);
    for (const auto& polyline : polylines) {
        if (polyline.empty()) continue;
        cairo_move_to(cr, polyline[0].first, polyline[0].second);
        for (size_t i = 1; i < polyline.size(); ++i) {
            cairo_line_to(cr, polyline[i].first, polyline[i].second);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Fill / Stroke
// ═══════════════════════════════════════════════════════════════════════════════

void CairoBackend::apply_fill_stroke(cairo_t* cr, const GraphicObject& obj)
{
    const bool has_fill = obj.fill.has_value() && !obj.fill->empty();
    const bool has_stroke = obj.stroke.has_value() && !obj.stroke->empty();

    if (!has_fill && !has_stroke) {
        cairo_new_path(cr);
        return;
    }

    // ── Fill ─────────────────────────────────────────────────────────
    if (has_fill) {
        auto argb = parse_color(*obj.fill);
        if (argb.has_value()) {
            cairo_set_source_argb(cr, *argb);
            if (has_stroke) {
                cairo_fill_preserve(cr);
            } else {
                cairo_fill(cr);
                return;
            }
        } else if (!has_stroke) {
            cairo_new_path(cr);
            return;
        }
    }

    // ── Stroke ───────────────────────────────────────────────────────
    if (has_stroke) {
        auto argb = parse_color(*obj.stroke);
        if (argb.has_value()) {
            cairo_set_source_argb(cr, *argb);
            cairo_set_line_width(cr, obj.stroke_width);
            cairo_stroke(cr);
        } else {
            cairo_new_path(cr);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Output
// ═══════════════════════════════════════════════════════════════════════════════

auto CairoBackend::save_image(
    Surface& surface, const std::string& path, const std::string& format)
    -> Result<void>
{
    (void)format;  // Cairo always writes PNG; caller must pass "png"
    auto& cs = static_cast<CairoSurface&>(surface);
    if (!cs.surface) {
        return std::unexpected(
            general_error("CairoBackend: surface is null"));
    }

#if CAIRO_HAS_PNG_FUNCTIONS
    cairo_status_t status = cairo_surface_write_to_png(cs.surface, path.c_str());
    if (status != CAIRO_STATUS_SUCCESS) {
        return std::unexpected(resource_error(
            "Failed to write PNG: " + std::string(cairo_status_to_string(status))));
    }
    return {};
#else
    (void)path;
    return std::unexpected(
        general_error("CairoBackend: cairo built without PNG support — "
                      "use the Pillow/Skia backend, or rebuild cairo with -Dpng=enabled"));
#endif
}

auto CairoBackend::save_animation(
    const std::vector<Surface*>& /*frames*/,
    const std::string& /*path*/,
    const std::string& /*format*/,
    int /*fps*/) -> Result<void>
{
    return std::unexpected(
        general_error("Cairo backend does not support animation output"));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Compositing
// ═══════════════════════════════════════════════════════════════════════════════

auto CairoBackend::composite(
    Surface& dst, Surface& src, int x, int y) -> Result<void>
{
    auto& dst_cs = static_cast<CairoSurface&>(dst);
    auto& src_cs = static_cast<CairoSurface&>(src);

    if (!dst_cs.cr || !src_cs.surface) {
        return std::unexpected(
            general_error("CairoBackend: invalid surface for compositing"));
    }

    cairo_save(dst_cs.cr);
    cairo_set_source_surface(dst_cs.cr, src_cs.surface,
        static_cast<double>(x), static_cast<double>(y));
    cairo_paint(dst_cs.cr);
    cairo_restore(dst_cs.cr);

    return {};
}

// ═══════════════════════════════════════════════════════════════════════════════
// Shaders (not supported)
// ═══════════════════════════════════════════════════════════════════════════════

auto CairoBackend::compile_shader(const std::string& /*glsl*/)
    -> Result<uint64_t>
{
    return std::unexpected(
        general_error("Cairo backend does not support shader compilation"));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Static registration — auto-registers "cairo" backend
// ═══════════════════════════════════════════════════════════════════════════════

[[maybe_unused]] static bool _registered_cairo = BackendRegistry::register_backend(
    "cairo",
    "Cairo 2D render backend — CPU raster, toy-text font",
    BackendCap::RasterCPU | BackendCap::FontRendering,
    []() -> std::unique_ptr<RenderBackend> {
        return std::make_unique<CairoBackend>();
    }
);

} // namespace pml
