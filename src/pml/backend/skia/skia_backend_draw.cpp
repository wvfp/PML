// ═══════════════════════════════════════════════════════════════════════════════
// PML Skia render backend — draw primitives
// ═══════════════════════════════════════════════════════════════════════════════
//
// Free functions for drawing each shape type and the main draw_object dispatcher.
// Declared in skia_backend_internal.h, implemented here.
// ═══════════════════════════════════════════════════════════════════════════════

#include "skia_backend_internal.h"

#include "pml/api/context.h"
#include "pml/asset/asset_cache.h"
#include "pml/backend/registry.h"

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Forward declaration of per-shape draw functions
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

Result<void> draw_group(SkCanvas* canvas, const GraphicObject& obj,
                        sk_sp<SkShader> shader, const ShaderLookup& lookup)
{
    for (const auto& child : obj.children) {
        auto r = draw_object(canvas, child, shader, lookup);
        if (!r) return r;
    }
    return {};
}

Result<void> draw_circle(SkCanvas* canvas, const GraphicObject& obj,
                         sk_sp<SkShader> shader)
{
    double cx = get_double(obj.params, ParamKey::cx, 0.0);
    double cy = get_double(obj.params, ParamKey::cy, 0.0);
    double r  = get_double(obj.params, ParamKey::r, 0.0);

    if (obj.fill || shader) {
        SkPaint paint;
        configure_fill_paint(paint, obj.fill, obj.stroke_width, shader);
        if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
            canvas->drawCircle(static_cast<SkScalar>(cx),
                               static_cast<SkScalar>(cy),
                               static_cast<SkScalar>(r), paint);
        }
    }
    if (obj.stroke) {
        SkPaint paint;
        configure_stroke_paint(paint, obj.stroke, obj.stroke_width);
        if (paint.getColor() != SK_ColorTRANSPARENT) {
            canvas->drawCircle(static_cast<SkScalar>(cx),
                               static_cast<SkScalar>(cy),
                               static_cast<SkScalar>(r), paint);
        }
    }
    return {};
}

Result<void> draw_rect(SkCanvas* canvas, const GraphicObject& obj,
                       sk_sp<SkShader> shader)
{
    double x = get_double(obj.params, ParamKey::x, 0.0);
    double y = get_double(obj.params, ParamKey::y, 0.0);
    double w = get_double(obj.params, ParamKey::w, 0.0);
    double h = get_double(obj.params, ParamKey::h, 0.0);
    double rx = get_double(obj.params, ParamKey::rx, 0.0);

    SkRect rect = SkRect::MakeXYWH(static_cast<SkScalar>(x),
                                   static_cast<SkScalar>(y),
                                   static_cast<SkScalar>(w),
                                   static_cast<SkScalar>(h));

    if (obj.fill || shader) {
        SkPaint paint;
        configure_fill_paint(paint, obj.fill, obj.stroke_width, shader);
        if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
            if (rx > 0.0) {
                canvas->drawRoundRect(rect,
                                      static_cast<SkScalar>(rx),
                                      static_cast<SkScalar>(rx), paint);
            } else {
                canvas->drawRect(rect, paint);
            }
        }
    }
    if (obj.stroke) {
        SkPaint paint;
        configure_stroke_paint(paint, obj.stroke, obj.stroke_width);
        if (paint.getColor() != SK_ColorTRANSPARENT) {
            if (rx > 0.0) {
                canvas->drawRoundRect(rect,
                                      static_cast<SkScalar>(rx),
                                      static_cast<SkScalar>(rx), paint);
            } else {
                canvas->drawRect(rect, paint);
            }
        }
    }
    return {};
}

Result<void> draw_ellipse(SkCanvas* canvas, const GraphicObject& obj,
                          sk_sp<SkShader> shader)
{
    double cx = get_double(obj.params, ParamKey::cx, 0.0);
    double cy = get_double(obj.params, ParamKey::cy, 0.0);
    double rx = get_double(obj.params, ParamKey::rx, 0.0);
    double ry = get_double(obj.params, ParamKey::ry, 0.0);

    SkRect oval = SkRect::MakeXYWH(static_cast<SkScalar>(cx - rx),
                                   static_cast<SkScalar>(cy - ry),
                                   static_cast<SkScalar>(rx * 2.0),
                                   static_cast<SkScalar>(ry * 2.0));

    if (obj.fill || shader) {
        SkPaint paint;
        configure_fill_paint(paint, obj.fill, obj.stroke_width, shader);
        if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
            canvas->drawOval(oval, paint);
        }
    }
    if (obj.stroke) {
        SkPaint paint;
        configure_stroke_paint(paint, obj.stroke, obj.stroke_width);
        if (paint.getColor() != SK_ColorTRANSPARENT) {
            canvas->drawOval(oval, paint);
        }
    }
    return {};
}

Result<void> draw_line(SkCanvas* canvas, const GraphicObject& obj,
                       sk_sp<SkShader> /*shader*/)
{
    double x1 = get_double(obj.params, ParamKey::x1, 0.0);
    double y1 = get_double(obj.params, ParamKey::y1, 0.0);
    double x2 = get_double(obj.params, ParamKey::x2, 0.0);
    double y2 = get_double(obj.params, ParamKey::y2, 0.0);

    if (obj.stroke) {
        SkPaint paint;
        configure_stroke_paint(paint, obj.stroke, obj.stroke_width);
        if (paint.getColor() != SK_ColorTRANSPARENT) {
            canvas->drawLine(static_cast<SkScalar>(x1),
                             static_cast<SkScalar>(y1),
                             static_cast<SkScalar>(x2),
                             static_cast<SkScalar>(y2), paint);
        }
    }
    return {};
}

Result<void> draw_polygon(SkCanvas* canvas, const GraphicObject& obj,
                          sk_sp<SkShader> shader)
{
    const Value* v = obj.params.find(ParamKey::points);
    if (!v) return {};

    const auto* list = v->as_list();
    if (!list || !*list) return {};

    const auto& elems = (*list)->elements;
    if (elems.size() < 4 || elems.size() % 2 != 0) return {};

    SkPathBuilder builder;
    auto to_double = [](const Value& val) -> double {
        if (val.is_int()) {
            return static_cast<double>(val.int_val());
        }
        if (val.is_double()) {
            return val.double_val();
        }
        return 0.0;
    };

    builder.moveTo(static_cast<SkScalar>(to_double(elems[0])),
                   static_cast<SkScalar>(to_double(elems[1])));
    for (size_t i = 2; i + 1 < elems.size(); i += 2) {
        builder.lineTo(static_cast<SkScalar>(to_double(elems[i])),
                       static_cast<SkScalar>(to_double(elems[i + 1])));
    }
    builder.close();
    SkPath path = builder.snapshot();

    if (obj.fill || shader) {
        SkPaint paint;
        configure_fill_paint(paint, obj.fill, obj.stroke_width, shader);
        if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
            canvas->drawPath(path, paint);
        }
    }
    if (obj.stroke) {
        SkPaint paint;
        configure_stroke_paint(paint, obj.stroke, obj.stroke_width);
        if (paint.getColor() != SK_ColorTRANSPARENT) {
            canvas->drawPath(path, paint);
        }
    }
    return {};
}

Result<void> draw_text(SkCanvas* canvas, const GraphicObject& obj,
                       sk_sp<SkShader> shader)
{
    double x = get_double(obj.params, ParamKey::x, 0.0);
    double y = get_double(obj.params, ParamKey::y, 0.0);
    std::string text = get_string(obj.params, ParamKey::text, "");
    double font_size = get_double(obj.params, ParamKey::font_size, 16.0);

    if (text.empty()) return {};

    SkPaint paint;
    configure_fill_paint(paint, obj.fill, obj.stroke_width, shader);
    if (paint.getColor() == SK_ColorTRANSPARENT && !paint.getShader()) {
        paint.setColor(SK_ColorBLACK);
    }

#ifdef _WIN32
    sk_sp<SkFontMgr> font_mgr = SkFontMgr_New_DirectWrite();
#else
    sk_sp<SkFontMgr> font_mgr = SkFontMgr::RefEmpty();
#endif

    sk_sp<SkTypeface> typeface = font_mgr->matchFamilyStyle(nullptr, SkFontStyle());
    if (!typeface) {
        typeface = font_mgr->matchFamilyStyle("Arial", SkFontStyle());
    }
    if (!typeface) {
        typeface = font_mgr->matchFamilyStyle("Microsoft YaHei", SkFontStyle());
    }
    if (!typeface) {
        return std::unexpected(general_error(
            "skia draw_text: failed to resolve a default typeface"));
    }

    SkFont font(typeface, static_cast<SkScalar>(font_size));
    font.setEdging(SkFont::Edging::kAntiAlias);
    canvas->drawSimpleText(text.c_str(), text.size(),
                           SkTextEncoding::kUTF8,
                           static_cast<SkScalar>(x),
                           static_cast<SkScalar>(y),
                           font, paint);
    return {};
}

Result<void> draw_path(SkCanvas* canvas, const GraphicObject& obj,
                       sk_sp<SkShader> /*shader*/)
{
    std::string d = get_string(obj.params, ParamKey::d, "");
    if (d.empty()) return {};

    // TODO: parse SVG path data into SkPath.
    // For now, leave as a no-op so compilation and basic shapes work.
    (void)canvas;
    (void)obj;
    return {};
}

Result<void> draw_image(SkCanvas* canvas, const GraphicObject& obj,
                        sk_sp<SkShader> /*shader*/)
{
    std::string src = get_string(obj.params, ParamKey::src, "");
    double x = get_double(obj.params, ParamKey::x, 0.0);
    double y = get_double(obj.params, ParamKey::y, 0.0);

    if (src.empty()) return {};

    auto& ctx = PMLContext::current();
    if (!ctx.assets) {
        return std::unexpected(resource_error(
            "skia draw_image: no asset cache available"));
    }

    auto& backend = BackendRegistry::instance().active();
    auto surface_result = ctx.assets->load_image(backend, src);
    if (!surface_result) {
        return std::unexpected(surface_result.error());
    }

    auto* skia_surf = dynamic_cast<SkiaSurface*>(surface_result->get());
    if (!skia_surf) {
        return std::unexpected(general_error(
            "skia draw_image: invalid decoded surface type"));
    }

    const SkScalar sx = static_cast<SkScalar>(x);
    const SkScalar sy = static_cast<SkScalar>(y);
    const SkScalar iw = static_cast<SkScalar>(skia_surf->width);
    const SkScalar ih = static_cast<SkScalar>(skia_surf->height);

    SkRect src_rect = SkRect::MakeWH(iw, ih);
    if (const Value* crop_val = obj.params.find(ParamKey::crop)) {
        if (auto rect = parse_rect_value(*crop_val)) {
            src_rect = *rect;
        }
    }

    SkRect dst_rect = SkRect::MakeXYWH(sx, sy, src_rect.width(), src_rect.height());
    if (obj.params.contains(ParamKey::w) && obj.params.contains(ParamKey::h)) {
        dst_rect = SkRect::MakeXYWH(
            sx, sy,
            static_cast<SkScalar>(get_double(obj.params, ParamKey::w, static_cast<double>(src_rect.width()))),
            static_cast<SkScalar>(get_double(obj.params, ParamKey::h, static_cast<double>(src_rect.height()))));
    }

    canvas->drawImageRect(
        skia_surf->bitmap.asImage().get(),
        src_rect,
        dst_rect,
        SkSamplingOptions(),
        nullptr,
        SkCanvas::kFast_SrcRectConstraint);
    return {};
}

}  // namespace

// ═══════════════════════════════════════════════════════════════════════════════
// Rough-style rendering functions
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

/// Convert a RoughOpSet to an SkPath.
SkPath rough_ops_to_skpath(const std::vector<RoughOp>& ops)
{
    SkPathBuilder builder;
    for (const auto& op : ops) {
        switch (op.op) {
        case RoughOpType::Move:
            builder.moveTo(static_cast<SkScalar>(op.data[0]),
                           static_cast<SkScalar>(op.data[1]));
            break;
        case RoughOpType::LineTo:
            builder.lineTo(static_cast<SkScalar>(op.data[0]),
                           static_cast<SkScalar>(op.data[1]));
            break;
        case RoughOpType::BCurveTo:
            builder.cubicTo(
                static_cast<SkScalar>(op.data[0]),
                static_cast<SkScalar>(op.data[1]),
                static_cast<SkScalar>(op.data[2]),
                static_cast<SkScalar>(op.data[3]),
                static_cast<SkScalar>(op.data[4]),
                static_cast<SkScalar>(op.data[5]));
            break;
        }
    }
    return builder.snapshot();
}

/// Extract metadata value as double, returns default_val if not found or wrong type.
double meta_double(const GraphicObject& obj, const std::string& key, double default_val)
{
    auto it = obj.metadata.find(key);
    if (it == obj.metadata.end()) return default_val;
    if (it->second.is_double()) return it->second.double_val();
    if (it->second.is_int()) return static_cast<double>(it->second.int_val());
    return default_val;
}

/// Extract metadata value as string, returns default_val if not found.
std::string meta_string(const GraphicObject& obj, const std::string& key,
                        const std::string& default_val)
{
    auto it = obj.metadata.find(key);
    if (it == obj.metadata.end()) return default_val;
    if (const auto* s = it->second.as_string()) return *s;
    return default_val;
}

/// Convert PolygonObject flat-points list to vector of RoughPoint.
std::vector<RoughPoint> polygon_to_rough_points(const GraphicObject& obj)
{
    std::vector<RoughPoint> pts;
    const Value* v = obj.params.find(ParamKey::points);
    if (!v) return pts;
    const auto* list = v->as_list();
    if (!list || !*list) return pts;
    const auto& elems = (*list)->elements;
    if (elems.size() < 4 || elems.size() % 2 != 0) return pts;
    pts.reserve(elems.size() / 2);
    auto to_double = [](const Value& val) -> double {
        if (val.is_int()) return static_cast<double>(val.int_val());
        if (val.is_double()) return val.double_val();
        return 0.0;
    };
    for (size_t i = 0; i + 1 < elems.size(); i += 2) {
        pts.push_back({to_double(elems[i]), to_double(elems[i + 1])});
    }
    return pts;
}

/// Build a closed polygon from rectangle params.
std::vector<RoughPoint> rect_to_rough_points(const GraphicObject& obj)
{
    double x = get_double(obj.params, ParamKey::x, 0.0);
    double y = get_double(obj.params, ParamKey::y, 0.0);
    double w = get_double(obj.params, ParamKey::w, 0.0);
    double h = get_double(obj.params, ParamKey::h, 0.0);
    return {
        {x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}
    };
}

/// Build a closed polygon from circle params (approximated as polygon).
std::vector<RoughPoint> circle_to_rough_points(const GraphicObject& obj)
{
    double cx = get_double(obj.params, ParamKey::cx, 0.0);
    double cy = get_double(obj.params, ParamKey::cy, 0.0);
    double r = get_double(obj.params, ParamKey::r, 0.0);
    return {{cx - r, cy - r}, {cx + r, cy - r}, {cx + r, cy + r}, {cx - r, cy + r}};
}

/// Render a rough fill pattern inside a closed polygon boundary.
Result<void> draw_rough_fill_impl(SkCanvas* canvas,
                                  const std::vector<RoughPoint>& polygon,
                                  const std::optional<std::string>& fill_color,
                                  const RoughStyleParams& params,
                                  RoughRandom& rng)
{
    if (polygon.size() < 3) return {};

    if (params.fill_style == "solid" || params.fill_style.empty()) {
        // Native solid fill — nothing to do here; handled by caller.
        return {};
    }

    // Generate fill pattern ops from rough_filler
    auto ops = generate_fill_pattern(polygon, const_cast<RoughStyleParams&>(params), rng);
    if (ops.empty()) return {};

    SkPath path = rough_ops_to_skpath(ops);

    // Only visible if we have a fill color (pattern lines use the fill color as stroke)
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    double fweight = 2.0;  // default fill line weight
    paint.setStrokeWidth(static_cast<SkScalar>(fweight));
    if (fill_color) {
        if (auto c = parse_sk_color(*fill_color)) {
            paint.setColor(*c);
        } else {
            paint.setColor(SK_ColorTRANSPARENT);
        }
    } else {
        paint.setColor(SK_ColorTRANSPARENT);
    }

    if (paint.getColor() != SK_ColorTRANSPARENT) {
        canvas->drawPath(path, paint);
    }
    return {};
}

/// Render the perturbed stroke path for a rough shape.
void draw_rough_stroke_path(SkCanvas* canvas, const SkPath& path,
                            const std::optional<std::string>& stroke_color,
                            double stroke_width)
{
    if (!stroke_color) return;
    SkPaint paint;
    configure_stroke_paint(paint, stroke_color, stroke_width);
    if (paint.getColor() != SK_ColorTRANSPARENT) {
        canvas->drawPath(path, paint);
    }
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════════
// extract_rough_params — public (declared in internal.h)
// ═══════════════════════════════════════════════════════════════════════════════

bool extract_rough_params(const GraphicObject& obj,
                          RoughStyleParams& params,
                          RoughRandom& rng)
{
    params.roughness = meta_double(obj, "roughness", 0.0);
    params.bowing = meta_double(obj, "bowing", 1.0);
    params.seed = static_cast<int>(meta_double(obj, "seed", 0.0));
    params.fill_style = meta_string(obj, "fill_style", "solid");

    if (params.roughness <= 0.0 && params.fill_style == "solid") {
        return false;  // no rough rendering needed
    }

    rng = RoughRandom(params.seed);
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Rough draw functions — public (declared in internal.h)
// ═══════════════════════════════════════════════════════════════════════════════

Result<void> draw_rough_line(SkCanvas* canvas, const GraphicObject& obj,
                             const RoughStyleParams& params,
                             RoughRandom& rng)
{
    double x1 = get_double(obj.params, ParamKey::x1, 0.0);
    double y1 = get_double(obj.params, ParamKey::y1, 0.0);
    double x2 = get_double(obj.params, ParamKey::x2, 0.0);
    double y2 = get_double(obj.params, ParamKey::y2, 0.0);

    // Generate perturbed line ops
    auto ops = rough_double_line(x1, y1, x2, y2,
                                 const_cast<RoughStyleParams&>(params), rng, false);
    SkPath path = rough_ops_to_skpath(ops);

    draw_rough_stroke_path(canvas, path, obj.stroke, obj.stroke_width);
    return {};
}

Result<void> draw_rough_rect(SkCanvas* canvas, const GraphicObject& obj,
                             sk_sp<SkShader> shader,
                             const RoughStyleParams& params,
                             RoughRandom& rng)
{
    // Build rect as a closed polygon
    auto pts = rect_to_rough_points(obj);

    // Fill layer
    if (obj.fill || shader) {
        if (params.fill_style == "solid" || params.fill_style.empty()) {
            // Native solid fill — use perturbed polygon path as fill boundary
            auto ops = rough_linear_path(pts, true, const_cast<RoughStyleParams&>(params), rng);
            SkPath fill_path = rough_ops_to_skpath(ops);
            SkPaint paint;
            configure_fill_paint(paint, obj.fill, obj.stroke_width, shader);
            if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
                canvas->drawPath(fill_path, paint);
            }
        } else {
            // Pattern fill
            auto result = draw_rough_fill_impl(canvas, pts, obj.fill, params, rng);
            if (!result) return result;
        }
    }

    // Stroke layer
    if (obj.stroke) {
        auto ops = rough_linear_path(pts, true, const_cast<RoughStyleParams&>(params), rng);
        SkPath path = rough_ops_to_skpath(ops);
        draw_rough_stroke_path(canvas, path, obj.stroke, obj.stroke_width);
    }

    return {};
}

Result<void> draw_rough_ellipse(SkCanvas* canvas, const GraphicObject& obj,
                                sk_sp<SkShader> shader,
                                const RoughStyleParams& params,
                                RoughRandom& rng)
{
    double cx = get_double(obj.params, ParamKey::cx, 0.0);
    double cy = get_double(obj.params, ParamKey::cy, 0.0);
    double rx = get_double(obj.params, ParamKey::rx, 0.0);
    double ry = get_double(obj.params, ParamKey::ry, 0.0);

    // Get ellipse points through Catmull-Rom curve
    auto ellipse_pts = compute_rough_ellipse_points(cx, cy, rx, ry, const_cast<RoughStyleParams&>(params), rng);

    // Fill layer
    if (obj.fill || shader) {
        if (params.fill_style == "solid" || params.fill_style.empty()) {
            // Native solid fill using perturbed boundary
            auto ops = rough_curve(ellipse_pts, nullptr, const_cast<RoughStyleParams&>(params), rng);
            SkPath fill_path = rough_ops_to_skpath(ops);
            { SkPathBuilder b(fill_path); b.close(); fill_path = b.detach(); }
            SkPaint paint;
            configure_fill_paint(paint, obj.fill, obj.stroke_width, shader);
            if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
                canvas->drawPath(fill_path, paint);
            }
        } else {
            // For pattern fills, build a bounding polygon from the ellipse points
            std::vector<RoughPoint> poly;
            poly.reserve(ellipse_pts.size());
            for (const auto& p : ellipse_pts) {
                poly.push_back(p);
            }
            auto result = draw_rough_fill_impl(canvas, poly, obj.fill, params, rng);
            if (!result) return result;
        }
    }

    // Stroke layer
    if (obj.stroke) {
        auto ops = rough_curve(ellipse_pts, nullptr, const_cast<RoughStyleParams&>(params), rng);
        SkPath path = rough_ops_to_skpath(ops);
        draw_rough_stroke_path(canvas, path, obj.stroke, obj.stroke_width);
    }

    return {};
}

Result<void> draw_rough_circle(SkCanvas* canvas, const GraphicObject& obj,
                               sk_sp<SkShader> shader,
                               const RoughStyleParams& params,
                               RoughRandom& rng)
{
    // Circle is rendered as a rough ellipse with rx=ry
    double cx = get_double(obj.params, ParamKey::cx, 0.0);
    double cy = get_double(obj.params, ParamKey::cy, 0.0);
    double r  = get_double(obj.params, ParamKey::r, 0.0);

    // Create temporary obj-like params for the ellipse
    auto ellipse_pts = compute_rough_ellipse_points(cx, cy, r, r, const_cast<RoughStyleParams&>(params), rng);

    // Fill layer
    if (obj.fill || shader) {
        if (params.fill_style == "solid" || params.fill_style.empty()) {
            auto ops = rough_curve(ellipse_pts, nullptr, const_cast<RoughStyleParams&>(params), rng);
            SkPath fill_path = rough_ops_to_skpath(ops);
            { SkPathBuilder b(fill_path); b.close(); fill_path = b.detach(); }
            SkPaint paint;
            configure_fill_paint(paint, obj.fill, obj.stroke_width, shader);
            if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
                canvas->drawPath(fill_path, paint);
            }
        } else {
            std::vector<RoughPoint> poly;
            poly.reserve(ellipse_pts.size());
            for (const auto& p : ellipse_pts) poly.push_back(p);
            auto result = draw_rough_fill_impl(canvas, poly, obj.fill, params, rng);
            if (!result) return result;
        }
    }

    // Stroke layer
    if (obj.stroke) {
        auto ops = rough_curve(ellipse_pts, nullptr, const_cast<RoughStyleParams&>(params), rng);
        SkPath path = rough_ops_to_skpath(ops);
        draw_rough_stroke_path(canvas, path, obj.stroke, obj.stroke_width);
    }

    return {};
}

Result<void> draw_rough_polygon(SkCanvas* canvas, const GraphicObject& obj,
                                sk_sp<SkShader> shader,
                                const RoughStyleParams& params,
                                RoughRandom& rng)
{
    auto pts = polygon_to_rough_points(obj);
    if (pts.size() < 3) return {};

    // Fill layer
    if (obj.fill || shader) {
        if (params.fill_style == "solid" || params.fill_style.empty()) {
            auto ops = rough_linear_path(pts, true, const_cast<RoughStyleParams&>(params), rng);
            SkPath fill_path = rough_ops_to_skpath(ops);
            SkPaint paint;
            configure_fill_paint(paint, obj.fill, obj.stroke_width, shader);
            if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
                canvas->drawPath(fill_path, paint);
            }
        } else {
            auto result = draw_rough_fill_impl(canvas, pts, obj.fill, params, rng);
            if (!result) return result;
        }
    }

    // Stroke layer
    if (obj.stroke) {
        auto ops = rough_linear_path(pts, true, const_cast<RoughStyleParams&>(params), rng);
        SkPath path = rough_ops_to_skpath(ops);
        draw_rough_stroke_path(canvas, path, obj.stroke, obj.stroke_width);
    }

    return {};
}

// ═══════════════════════════════════════════════════════════════════════════════
// draw_object — main dispatcher
// ═══════════════════════════════════════════════════════════════════════════════

Result<void> draw_object(SkCanvas* canvas, const GraphicObject& obj,
                         sk_sp<SkShader> parent_shader,
                         const ShaderLookup& lookup)
{
    SkAutoCanvasRestore acr(canvas, true);

    if (!obj.transform.is_identity()) {
        canvas->concat(obj.transform.to_skmatrix());
    }

    // If this object has its own shader handle, resolve it; otherwise inherit.
    sk_sp<SkShader> local_shader = parent_shader;
    if (const Value* shader_val = obj.params.find(ParamKey::shader)) {
        int64_t handle = 0;
        if (shader_val->is_int()) {
            handle = shader_val->int_val();
        } else if (shader_val->is_double()) {
            handle = static_cast<int64_t>(shader_val->double_val());
        }
        if (handle > 0) {
            local_shader = lookup(handle);
        }
    }

    if (obj.shape_type == "group") {
        return draw_group(canvas, obj, local_shader, lookup);
    }

    // ── Rough-style dispatch ──────────────────────────────────────────────
    // shape_type starting with "rough_" triggers the hand-drawn rendering path.
    if (obj.shape_type.rfind("rough_", 0) == 0) {
        RoughStyleParams params;
        RoughRandom rng;
        if (!extract_rough_params(obj, params, rng)) {
            // roughness = 0 and solid fill → fall through to exact path below
            goto exact_path;
        }
        if (obj.shape_type == "rough_circle") {
            return draw_rough_circle(canvas, obj, local_shader, params, rng);
        }
        if (obj.shape_type == "rough_rect") {
            return draw_rough_rect(canvas, obj, local_shader, params, rng);
        }
        if (obj.shape_type == "rough_ellipse") {
            return draw_rough_ellipse(canvas, obj, local_shader, params, rng);
        }
        if (obj.shape_type == "rough_line") {
            return draw_rough_line(canvas, obj, params, rng);
        }
        if (obj.shape_type == "rough_polygon") {
            return draw_rough_polygon(canvas, obj, local_shader, params, rng);
        }
        // Unknown rough_ prefix → fall through to exact path
    }

exact_path:
    if (obj.shape_type == "circle")  return draw_circle(canvas, obj, local_shader);
    if (obj.shape_type == "rect")    return draw_rect(canvas, obj, local_shader);
    if (obj.shape_type == "ellipse") return draw_ellipse(canvas, obj, local_shader);
    if (obj.shape_type == "line")    return draw_line(canvas, obj, local_shader);
    if (obj.shape_type == "polygon") return draw_polygon(canvas, obj, local_shader);
    if (obj.shape_type == "text")    return draw_text(canvas, obj, local_shader);
    if (obj.shape_type == "path")    return draw_path(canvas, obj, local_shader);
    if (obj.shape_type == "image")   return draw_image(canvas, obj, local_shader);
    if (obj.shape_type == "mesh3d")  return draw_mesh3d(canvas, obj, lookup);

    // Unknown shape type is non-fatal (matching Python's tolerant behaviour).
    return {};
}

// ═══════════════════════════════════════════════════════════════════════════════
// 3D mesh rendering
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

Result<void> draw_mesh3d_impl(SkCanvas* canvas, const GraphicObject& obj,
                              const ShaderLookup& lookup)
{
    const Value* mesh_val = obj.params.find("mesh");
    const Value* transform_val = obj.params.find("transform");
    if (!mesh_val || !transform_val) return {};

    const auto* mesh_ptr = mesh_val->as_mesh3d();
    const auto* transform_ptr = transform_val->as_transform3d();
    if (!mesh_ptr || !transform_ptr) return {};

    const auto& mesh = **mesh_ptr;
    const auto& transform = **transform_ptr;
    const Camera3D& cam = current_camera();
    const Mat4 vp = cam.projection_matrix() * cam.view_matrix();

    const int canvas_width = canvas->getBaseLayerSize().width();
    const int canvas_height = canvas->getBaseLayerSize().height();
    if (canvas_width <= 0 || canvas_height <= 0) return {};

    struct ProjectedFace {
        const Mesh3D::Face* face;
        std::vector<SkPoint> points;
        std::vector<SkPoint> uvs;
        double depth;
    };
    std::vector<ProjectedFace> visible;
    visible.reserve(mesh.faces.size());

    for (const auto& face : mesh.faces) {
        if (face.indices.size() < 3) continue;

        ProjectedFace pf;
        pf.face = &face;
        pf.points.reserve(face.indices.size());
        pf.uvs.reserve(face.indices.size());

        Vec3 world_sum(0, 0, 0);
        bool clipped = false;
        for (int idx : face.indices) {
            Vec3 world = transform.apply(mesh.vertices[idx].position);
            world_sum = world_sum + world;
            Vec3 ndc = vp.transform_point(world);

            // Simple clipping: skip faces entirely behind the near plane.
            if (ndc.z < -1.0 || ndc.z > 1.0) {
                clipped = true;
                break;
            }

            SkPoint p{
                static_cast<SkScalar>((ndc.x + 1.0) * 0.5 * canvas_width),
                static_cast<SkScalar>((1.0 - ndc.y) * 0.5 * canvas_height)};
            pf.points.push_back(p);
        }
        if (clipped || pf.points.size() < 3) continue;

        // Backface culling using signed screen-space area.
        if (cam.backface_culling) {
            double signed_area = 0.0;
            for (size_t i = 0; i < pf.points.size(); ++i) {
                const SkPoint& a = pf.points[i];
                const SkPoint& b = pf.points[(i + 1) % pf.points.size()];
                signed_area += static_cast<double>(a.x()) * b.y() -
                               static_cast<double>(b.x()) * a.y();
            }
            if (signed_area < 0.0) continue;
        }

        Vec3 center = world_sum / static_cast<double>(face.indices.size());
        Vec3 center_ndc = vp.transform_point(center);
        pf.depth = center_ndc.z;

        for (const auto& uv : face.uvs) {
            pf.uvs.push_back({static_cast<SkScalar>(uv.x),
                              static_cast<SkScalar>(uv.y)});
        }
        visible.push_back(std::move(pf));
    }

    // Painter's algorithm: draw from far to near.
    std::sort(visible.begin(), visible.end(),
              [](const ProjectedFace& a, const ProjectedFace& b) {
                  return a.depth > b.depth;
              });

    // Temporary surface shared across all face rasterizations.
    for (const auto& pf : visible) {
        const Mesh3D::Face& face = *pf.face;
        const int tex_w = std::max(1, static_cast<int>(std::ceil(face.tex_width)));
        const int tex_h = std::max(1, static_cast<int>(std::ceil(face.tex_height)));

        // Rasterize the material to a temporary surface.
        SkiaSurface mat_surface(tex_w, tex_h, 0x00000000);
        auto mat_result = draw_object(mat_surface.surface->getCanvas(),
                                      face.material, nullptr, lookup);
        if (!mat_result) return mat_result;

        sk_sp<SkShader> shader = mat_surface.bitmap.asImage()->makeShader(
            SkTileMode::kClamp, SkTileMode::kClamp, SkSamplingOptions());
        if (!shader) continue;

        // Triangulate: fan for arbitrary n-gons; our factories use quads.
        const size_t n = pf.points.size();
        if (n < 3) continue;

        std::vector<SkPoint> tri_positions;
        std::vector<SkPoint> tri_uvs;
        tri_positions.reserve((n - 2) * 3);
        tri_uvs.reserve((n - 2) * 3);

        for (size_t i = 1; i + 1 < n; ++i) {
            tri_positions.push_back(pf.points[0]);
            tri_positions.push_back(pf.points[i]);
            tri_positions.push_back(pf.points[i + 1]);

            tri_uvs.push_back({pf.uvs[0].x() * tex_w, pf.uvs[0].y() * tex_h});
            tri_uvs.push_back({pf.uvs[i].x() * tex_w, pf.uvs[i].y() * tex_h});
            tri_uvs.push_back({pf.uvs[i + 1].x() * tex_w,
                               pf.uvs[i + 1].y() * tex_h});
        }

        auto vertices = SkVertices::MakeCopy(
            SkVertices::kTriangles_VertexMode,
            static_cast<int>(tri_positions.size()),
            tri_positions.data(),
            tri_uvs.data(),
            nullptr);

        SkPaint paint;
        paint.setShader(std::move(shader));
        paint.setBlendMode(SkBlendMode::kSrcOver);
        canvas->drawVertices(vertices.get(), SkBlendMode::kSrcOver, paint);
    }

    return {};
}

}  // namespace

Result<void> draw_mesh3d(SkCanvas* canvas, const GraphicObject& obj,
                         const ShaderLookup& lookup)
{
    return draw_mesh3d_impl(canvas, obj, lookup);
}

}  // namespace pml
