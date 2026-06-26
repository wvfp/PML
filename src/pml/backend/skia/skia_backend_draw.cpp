// ═══════════════════════════════════════════════════════════════════════════════
// PML Skia render backend — draw primitives
// ═══════════════════════════════════════════════════════════════════════════════
//
// Free functions for drawing each shape type and the main draw_object dispatcher.
// Declared in skia_backend_internal.h, implemented here.
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/core/types.h"

#include "skia_backend_internal.h"

#include "pml/api/context.h"
#include "pml/asset/asset_cache.h"
#include "pml/backend/registry.h"
#include "pml/graphics/path_types.h"
#include "pml/graphics/polygon_perturb.h"
#include "pml/core/texture.h"
#include "textured_draw.h"

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Forward declaration of per-shape draw functions
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

// ── Stroke alignment helper ─────────────────────────────────────────────
// Clip-based: draw at 2x stroke width, clip to keep only the inner or
// outer half of the stroke.  Caller must save/restore canvas when used.

inline void apply_clip_for_stroke_align(
    SkCanvas* canvas, const GraphicObject& obj, const SkPath& shape_path)
{
    if (obj.stroke_align == "inside") {
        canvas->clipPath(shape_path, SkClipOp::kIntersect, true);
    } else if (obj.stroke_align == "outside") {
        canvas->clipPath(shape_path, SkClipOp::kDifference, true);
    }
}

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
        configure_fill_paint(paint, obj.fill, obj.stroke_width, shader, obj.blend_mode);
        if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
            canvas->drawCircle(static_cast<SkScalar>(cx),
                               static_cast<SkScalar>(cy),
                               static_cast<SkScalar>(r), paint);
        }
    }
    if (obj.stroke) {
        SkPaint paint;
        configure_stroke_paint(paint, obj.stroke, obj.stroke_width, obj.blend_mode);
        if (paint.getColor() != SK_ColorTRANSPARENT) {
            SkScalar scx = static_cast<SkScalar>(cx);
            SkScalar scy = static_cast<SkScalar>(cy);
            SkScalar sr  = static_cast<SkScalar>(r);
            if (obj.stroke_align != "center") {
                canvas->save();
                SkPathBuilder path_builder;
                path_builder.addCircle(scx, scy, sr);
                apply_clip_for_stroke_align(canvas, obj, path_builder.snapshot());
                paint.setStrokeWidth(paint.getStrokeWidth() * 2.0f);
                canvas->drawCircle(scx, scy, sr, paint);
                canvas->restore();
            } else {
                canvas->drawCircle(scx, scy, sr, paint);
            }
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
        configure_fill_paint(paint, obj.fill, obj.stroke_width, shader, obj.blend_mode);
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
        configure_stroke_paint(paint, obj.stroke, obj.stroke_width, obj.blend_mode);
        if (paint.getColor() != SK_ColorTRANSPARENT) {
            if (obj.stroke_align != "center") {
                canvas->save();
                SkPathBuilder path_builder;
                if (rx > 0.0) {
                    SkRRect rrect = SkRRect::MakeRectXY(rect, static_cast<SkScalar>(rx), static_cast<SkScalar>(rx));
                    path_builder.addRRect(rrect);
                } else {
                    path_builder.addRect(rect);
                }
                apply_clip_for_stroke_align(canvas, obj, path_builder.snapshot());
                paint.setStrokeWidth(paint.getStrokeWidth() * 2.0f);
                if (rx > 0.0) {
                    canvas->drawRoundRect(rect, static_cast<SkScalar>(rx), static_cast<SkScalar>(rx), paint);
                } else {
                    canvas->drawRect(rect, paint);
                }
                canvas->restore();
            } else {
                if (rx > 0.0) {
                    canvas->drawRoundRect(rect, static_cast<SkScalar>(rx), static_cast<SkScalar>(rx), paint);
                } else {
                    canvas->drawRect(rect, paint);
                }
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
        configure_fill_paint(paint, obj.fill, obj.stroke_width, shader, obj.blend_mode);
        if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
            canvas->drawOval(oval, paint);
        }
    }
    if (obj.stroke) {
        SkPaint paint;
        configure_stroke_paint(paint, obj.stroke, obj.stroke_width, obj.blend_mode);
        if (paint.getColor() != SK_ColorTRANSPARENT) {
            if (obj.stroke_align != "center") {
                canvas->save();
                SkPathBuilder path_builder;
                path_builder.addOval(oval);
                apply_clip_for_stroke_align(canvas, obj, path_builder.snapshot());
                paint.setStrokeWidth(paint.getStrokeWidth() * 2.0f);
                canvas->drawOval(oval, paint);
                canvas->restore();
            } else {
                canvas->drawOval(oval, paint);
            }
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
        configure_stroke_paint(paint, obj.stroke, obj.stroke_width, obj.blend_mode);
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

    auto to_double = [](const Value& val) -> double {
        if (val.is_int()) {
            return static_cast<double>(val.int_val());
        }
        if (val.is_double()) {
            return val.double_val();
        }
        return 0.0;
    };

    // Build initial vertex list
    std::vector<RoughPoint> vertices;
    vertices.reserve(elems.size() / 2);
    for (size_t i = 0; i + 1 < elems.size(); i += 2) {
        vertices.push_back({to_double(elems[i]), to_double(elems[i + 1])});
    }

    // Check for corner-radius metadata
    bool has_corner_radius = obj.metadata.contains("corner_radius");
    bool has_edge_params = obj.metadata.contains("edge_seed") || 
                           obj.metadata.contains("edge_noise") ||
                           obj.metadata.contains("edge_subdivisions");

    std::vector<RoughPoint> final_points = vertices;

    if (has_corner_radius || has_edge_params) {
        // Build perturbation config from metadata
        PerturbConfig config;
        
        // Parse edge_seed
        int edge_seed = 0;
        auto seed_it = obj.metadata.find("edge_seed");
        if (seed_it != obj.metadata.end()) {
            if (seed_it->second.is_int()) edge_seed = seed_it->second.int_val();
            else if (seed_it->second.is_double()) edge_seed = static_cast<int>(seed_it->second.double_val());
        }
        config.seed = edge_seed;

        // Parse edge_noise
        auto noise_it = obj.metadata.find("edge_noise");
        if (noise_it != obj.metadata.end()) {
            if (const auto* noise_list = noise_it->second.as_list()) {
                if (*noise_list) {
                    for (const auto& elem : (*noise_list)->elements) {
                        config.edge_noise.push_back(to_double(elem));
                    }
                }
            } else {
                config.edge_noise.push_back(to_double(noise_it->second));
            }
        } else {
            config.edge_noise = {0.0};
        }

        // Parse edge_subdivisions
        auto subdiv_it = obj.metadata.find("edge_subdivisions");
        if (subdiv_it != obj.metadata.end()) {
            if (subdiv_it->second.is_int()) {
                config.edge_subdivisions = {static_cast<int>(subdiv_it->second.int_val())};
            } else if (subdiv_it->second.is_double()) {
                config.edge_subdivisions = {static_cast<int>(subdiv_it->second.double_val())};
            }
        } else {
            config.edge_subdivisions = {0};
        }

        // Parse corner_radius
        auto corner_r_it = obj.metadata.find("corner_radius");
        if (corner_r_it != obj.metadata.end()) {
            if (const auto* corner_list = corner_r_it->second.as_list()) {
                if (*corner_list) {
                    for (const auto& elem : (*corner_list)->elements) {
                        config.corner_radius.push_back(to_double(elem));
                    }
                }
            } else {
                config.corner_radius.push_back(to_double(corner_r_it->second));
            }
        } else {
            config.corner_radius = {0.0};
        }

        // Parse corner_mask (defaults to all true)
        auto corner_m_it = obj.metadata.find("corner_mask");
        if (corner_m_it != obj.metadata.end()) {
            if (const auto* mask_list = corner_m_it->second.as_list()) {
                if (*mask_list) {
                    for (const auto& elem : (*mask_list)->elements) {
                        config.corner_mask.push_back(elem.is_bool() ? elem.bool_val() : true);
                    }
                }
            } else {
                config.corner_mask.push_back(corner_m_it->second.is_bool() ? corner_m_it->second.bool_val() : true);
            }
        } else {
            config.corner_mask = {true};
        }

        // Parse edge_mask (defaults to all true)
        auto edge_m_it = obj.metadata.find("edge_mask");
        if (edge_m_it != obj.metadata.end()) {
            if (const auto* mask_list = edge_m_it->second.as_list()) {
                if (*mask_list) {
                    for (const auto& elem : (*mask_list)->elements) {
                        config.edge_mask.push_back(elem.is_bool() ? elem.bool_val() : true);
                    }
                }
            } else {
                config.edge_mask.push_back(edge_m_it->second.is_bool() ? edge_m_it->second.bool_val() : true);
            }
        } else {
            config.edge_mask = {true};
        }

        // Apply perturbation
        PerlinNoise2D noise(edge_seed);
        auto perturbed = perturb_polygon(vertices, config, noise);
        final_points = flatten_perturb_result(perturbed);
    }

    // Build path from final points
    SkPathBuilder builder;
    if (!final_points.empty()) {
        builder.moveTo(static_cast<SkScalar>(final_points[0].x),
                       static_cast<SkScalar>(final_points[0].y));
        for (size_t i = 1; i < final_points.size(); ++i) {
            builder.lineTo(static_cast<SkScalar>(final_points[i].x),
                           static_cast<SkScalar>(final_points[i].y));
        }
        builder.close();
    }
    SkPath path = builder.snapshot();

    if (obj.fill || shader) {
        SkPaint paint;
        configure_fill_paint(paint, obj.fill, obj.stroke_width, shader, obj.blend_mode);
        if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
            canvas->drawPath(path, paint);
        }
    }
    if (obj.stroke) {
        SkPaint paint;
        configure_stroke_paint(paint, obj.stroke, obj.stroke_width, obj.blend_mode);
        if (paint.getColor() != SK_ColorTRANSPARENT) {
            if (obj.stroke_align != "center") {
                canvas->save();
                apply_clip_for_stroke_align(canvas, obj, path);
                paint.setStrokeWidth(paint.getStrokeWidth() * 2.0f);
                canvas->drawPath(path, paint);
                canvas->restore();
            } else {
                canvas->drawPath(path, paint);
            }
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
    configure_fill_paint(paint, obj.fill, obj.stroke_width, shader, obj.blend_mode);
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

    // Apply blend mode via non-null SkPaint when specified
    SkPaint img_paint;
    if (obj.blend_mode.has_value()) {
        img_paint.setBlendMode(to_skia_blend_mode(*obj.blend_mode));
    }

    canvas->drawImageRect(
        skia_surf->bitmap.asImage().get(),
        src_rect,
        dst_rect,
        SkSamplingOptions(),
        obj.blend_mode.has_value() ? &img_paint : nullptr,
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
                            double stroke_width,
                            std::optional<BlendMode> blend_mode = std::nullopt)
{
    if (!stroke_color) return;
    SkPaint paint;
    configure_stroke_paint(paint, stroke_color, stroke_width, blend_mode);
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

    draw_rough_stroke_path(canvas, path, obj.stroke, obj.stroke_width, obj.blend_mode);
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
            configure_fill_paint(paint, obj.fill, obj.stroke_width, shader, obj.blend_mode);
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
        draw_rough_stroke_path(canvas, path, obj.stroke, obj.stroke_width, obj.blend_mode);
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
            configure_fill_paint(paint, obj.fill, obj.stroke_width, shader, obj.blend_mode);
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
        draw_rough_stroke_path(canvas, path, obj.stroke, obj.stroke_width, obj.blend_mode);
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
            configure_fill_paint(paint, obj.fill, obj.stroke_width, shader, obj.blend_mode);
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
        draw_rough_stroke_path(canvas, path, obj.stroke, obj.stroke_width, obj.blend_mode);
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
            configure_fill_paint(paint, obj.fill, obj.stroke_width, shader, obj.blend_mode);
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
        draw_rough_stroke_path(canvas, path, obj.stroke, obj.stroke_width, obj.blend_mode);
    }

    return {};
}

// ═══════════════════════════════════════════════════════════════════════════════
// Path helpers — parse PML command lists and build SkPath from SVG commands
// ═══════════════════════════════════════════════════════════════════════════════

static std::expected<std::vector<PathCommand>, std::string>
parse_pml_path_commands(const Value& list_val)
{
    const auto* list = list_val.as_list();
    if (!list || !*list) {
        return std::unexpected(std::string("path: expected a list of commands"));
    }

    const auto& elems = (*list)->elements;
    std::vector<PathCommand> result;
    result.reserve(elems.size());

    for (size_t i = 0; i < elems.size(); ++i) {
        const auto* cmd_list = elems[i].as_list();
        if (!cmd_list || !*cmd_list || (*cmd_list)->elements.empty()) {
            return std::unexpected(
                "path: each command must be a non-empty list (command-symbol args...)");
        }
        const auto& cmd_elems = (*cmd_list)->elements;
        const auto* sym = cmd_elems[0].as_symbol();
        if (!sym) {
            return std::unexpected("path: first element of each command must be a symbol");
        }

        auto cmd_type = string_to_cmd(sym->name);
        if (!cmd_type) {
            return std::unexpected("path: unknown command '" + sym->name + "'");
        }

        std::vector<double> args;
        args.reserve(cmd_elems.size() - 1);
        for (size_t j = 1; j < cmd_elems.size(); ++j) {
            if (cmd_elems[j].is_double()) {
                args.push_back(cmd_elems[j].double_val());
            } else if (cmd_elems[j].is_int()) {
                args.push_back(static_cast<double>(cmd_elems[j].int_val()));
            } else {
                return std::unexpected(
                    "path: command '" + sym->name + "' argument must be numeric");
            }
        }

        result.emplace_back(*cmd_type, std::move(args));
    }

    return result;
}

static SkPath build_skpath_from_commands(const std::vector<PathCommand>& cmds)
{
    SkPathBuilder builder;
    double cx = 0.0, cy = 0.0;           // current point
    double pcx = 0.0, pcy = 0.0;         // previous control point (for smooth curves)
    double subpath_start_x = 0.0, subpath_start_y = 0.0;
    bool has_prev_cp = false;

    for (const auto& cmd : cmds) {
        const auto& a = cmd.args;
        bool rel = is_relative(cmd.type);

        switch (cmd.type) {
        case PathCmdType::MoveTo:
        case PathCmdType::MoveToR: {
            double x = rel ? cx + a[0] : a[0];
            double y = rel ? cy + a[1] : a[1];
            builder.moveTo(static_cast<SkScalar>(x), static_cast<SkScalar>(y));
            cx = x; cy = y;
            subpath_start_x = x; subpath_start_y = y;
            has_prev_cp = false;
            break;
        }
        case PathCmdType::LineTo:
        case PathCmdType::LineToR: {
            double x = rel ? cx + a[0] : a[0];
            double y = rel ? cy + a[1] : a[1];
            builder.lineTo(static_cast<SkScalar>(x), static_cast<SkScalar>(y));
            cx = x; cy = y;
            has_prev_cp = false;
            break;
        }
        case PathCmdType::HLineTo:
        case PathCmdType::HLineToR: {
            double x = rel ? cx + a[0] : a[0];
            builder.lineTo(static_cast<SkScalar>(x), static_cast<SkScalar>(cy));
            cx = x;
            has_prev_cp = false;
            break;
        }
        case PathCmdType::VLineTo:
        case PathCmdType::VLineToR: {
            double y = rel ? cy + a[0] : a[0];
            builder.lineTo(static_cast<SkScalar>(cx), static_cast<SkScalar>(y));
            cy = y;
            has_prev_cp = false;
            break;
        }
        case PathCmdType::CubicTo:
        case PathCmdType::CubicToR: {
            double cx1 = rel ? cx + a[0] : a[0];
            double cy1 = rel ? cy + a[1] : a[1];
            double cx2 = rel ? cx + a[2] : a[2];
            double cy2 = rel ? cy + a[3] : a[3];
            double x   = rel ? cx + a[4] : a[4];
            double y   = rel ? cy + a[5] : a[5];
            builder.cubicTo(
                static_cast<SkScalar>(cx1), static_cast<SkScalar>(cy1),
                static_cast<SkScalar>(cx2), static_cast<SkScalar>(cy2),
                static_cast<SkScalar>(x),   static_cast<SkScalar>(y));
            pcx = cx2; pcy = cy2;
            has_prev_cp = true;
            cx = x; cy = y;
            break;
        }
        case PathCmdType::SmoothCubicTo:
        case PathCmdType::SmoothCubicToR: {
            // Reflect previous control point
            double cx1 = has_prev_cp ? 2.0 * cx - pcx : cx;
            double cy1 = has_prev_cp ? 2.0 * cy - pcy : cy;
            double cx2 = rel ? cx + a[0] : a[0];
            double cy2 = rel ? cy + a[1] : a[1];
            double x   = rel ? cx + a[2] : a[2];
            double y   = rel ? cy + a[3] : a[3];
            builder.cubicTo(
                static_cast<SkScalar>(cx1), static_cast<SkScalar>(cy1),
                static_cast<SkScalar>(cx2), static_cast<SkScalar>(cy2),
                static_cast<SkScalar>(x),   static_cast<SkScalar>(y));
            pcx = cx2; pcy = cy2;
            has_prev_cp = true;
            cx = x; cy = y;
            break;
        }
        case PathCmdType::QuadTo:
        case PathCmdType::QuadToR: {
            double qcx = rel ? cx + a[0] : a[0];
            double qcy = rel ? cy + a[1] : a[1];
            double x   = rel ? cx + a[2] : a[2];
            double y   = rel ? cy + a[3] : a[3];
            builder.quadTo(
                static_cast<SkScalar>(qcx), static_cast<SkScalar>(qcy),
                static_cast<SkScalar>(x),   static_cast<SkScalar>(y));
            pcx = qcx; pcy = qcy;
            has_prev_cp = true;
            cx = x; cy = y;
            break;
        }
        case PathCmdType::SmoothQuadTo:
        case PathCmdType::SmoothQuadToR: {
            // Reflect previous control point
            double qcx = has_prev_cp ? 2.0 * cx - pcx : cx;
            double qcy = has_prev_cp ? 2.0 * cy - pcy : cy;
            double x   = rel ? cx + a[0] : a[0];
            double y   = rel ? cy + a[1] : a[1];
            builder.quadTo(
                static_cast<SkScalar>(qcx), static_cast<SkScalar>(qcy),
                static_cast<SkScalar>(x),   static_cast<SkScalar>(y));
            pcx = qcx; pcy = qcy;
            has_prev_cp = true;
            cx = x; cy = y;
            break;
        }
        case PathCmdType::Arc:
        case PathCmdType::ArcR: {
            double rx  = a[0];
            double ry  = a[1];
            double rot = a[2];
            int laf    = static_cast<int>(a[3]);
            int sf     = static_cast<int>(a[4]);
            double x   = rel ? cx + a[5] : a[5];
            double y   = rel ? cy + a[6] : a[6];

            if (rx > 0.0 && ry > 0.0) {
                builder.arcTo(
                    SkPoint{static_cast<SkScalar>(rx), static_cast<SkScalar>(ry)},
                    static_cast<SkScalar>(rot),
                    laf != 0 ? SkPathBuilder::kLarge_ArcSize : SkPathBuilder::kSmall_ArcSize,
                    sf != 0 ? SkPathDirection::kCW : SkPathDirection::kCCW,
                    SkPoint{static_cast<SkScalar>(x), static_cast<SkScalar>(y)});
            } else {
                builder.lineTo(static_cast<SkScalar>(x), static_cast<SkScalar>(y));
            }
            cx = x; cy = y;
            has_prev_cp = false;
            break;
        }
        case PathCmdType::Close:
            builder.close();
            cx = subpath_start_x; cy = subpath_start_y;
            has_prev_cp = false;
            break;
        default:
            break;
        }
    }

    return builder.detach();
}

Result<void> draw_path(SkCanvas* canvas, const GraphicObject& obj,
                       sk_sp<SkShader> shader)
{
    // Try path_commands first (from S-expression API), then fall back to :d SVG string
    std::vector<PathCommand> cmds;
    bool has_commands = false;

    if (const Value* pc_val = obj.params.find(ParamKey::path_commands)) {
        auto parsed = parse_pml_path_commands(*pc_val);
        if (parsed) {
            cmds = std::move(*parsed);
            has_commands = true;
        } else {
            return std::unexpected(general_error(
                "draw_path: " + parsed.error()));
        }
    }

    if (!has_commands) {
        std::string d = get_string(obj.params, ParamKey::d, "");
        if (d.empty()) return {};

        auto parsed = parse_svg_path_string(d);
        if (!parsed) {
            return std::unexpected(general_error(
                "draw_path: " + parsed.error()));
        }
        cmds = std::move(*parsed);
        has_commands = true;
    }

    if (!has_commands || cmds.empty()) return {};

    SkPath path = build_skpath_from_commands(cmds);

    if (obj.fill || shader) {
        SkPaint paint;
        configure_fill_paint(paint, obj.fill, obj.stroke_width, shader, obj.blend_mode);
        if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
            canvas->drawPath(path, paint);
        }
    }
    if (obj.stroke) {
        SkPaint paint;
        configure_stroke_paint(paint, obj.stroke, obj.stroke_width, obj.blend_mode);
        if (paint.getColor() != SK_ColorTRANSPARENT) {
            if (obj.stroke_align != "center") {
                canvas->save();
                apply_clip_for_stroke_align(canvas, obj, path);
                paint.setStrokeWidth(paint.getStrokeWidth() * 2.0f);
                canvas->drawPath(path, paint);
                canvas->restore();
            } else {
                canvas->drawPath(path, paint);
            }
        }
    }
    return {};
}

// ── Rough path rendering ──────────────────────────────────────────────────────

// Sample points at regular intervals along a path for rough rendering
static std::vector<RoughPoint> sample_path_points(const SkPath& path, int samples_per_verb = 10)
{
    std::vector<RoughPoint> pts;
    SkPath::Iter iter(path, false);

    SkPoint path_pts[4];
    SkPath::Verb verb;

    while ((verb = iter.next(path_pts)) != SkPath::kDone_Verb) {
        switch (verb) {
        case SkPath::kMove_Verb: {
            pts.push_back({path_pts[0].fX, path_pts[0].fY});
            break;
        }
        case SkPath::kLine_Verb: {
            SkScalar dx = path_pts[1].fX - path_pts[0].fX;
            SkScalar dy = path_pts[1].fY - path_pts[0].fY;
            for (int i = 1; i < samples_per_verb; ++i) {
                SkScalar t = static_cast<SkScalar>(i) / samples_per_verb;
                pts.push_back({path_pts[0].fX + dx * t, path_pts[0].fY + dy * t});
            }
            pts.push_back({path_pts[1].fX, path_pts[1].fY});
            break;
        }
        case SkPath::kQuad_Verb: {
            for (int i = 1; i < samples_per_verb; ++i) {
                SkScalar t = static_cast<SkScalar>(i) / samples_per_verb;
                SkScalar t1 = 1.0f - t;
                SkScalar x = t1 * t1 * path_pts[0].fX + 2.0f * t1 * t * path_pts[1].fX + t * t * path_pts[2].fX;
                SkScalar y = t1 * t1 * path_pts[0].fY + 2.0f * t1 * t * path_pts[1].fY + t * t * path_pts[2].fY;
                pts.push_back({x, y});
            }
            pts.push_back({path_pts[2].fX, path_pts[2].fY});
            break;
        }
        case SkPath::kCubic_Verb: {
            for (int i = 1; i < samples_per_verb; ++i) {
                SkScalar t = static_cast<SkScalar>(i) / samples_per_verb;
                SkScalar t1 = 1.0f - t;
                SkScalar x = t1 * t1 * t1 * path_pts[0].fX +
                             3.0f * t1 * t1 * t * path_pts[1].fX +
                             3.0f * t1 * t * t * path_pts[2].fX +
                             t * t * t * path_pts[3].fX;
                SkScalar y = t1 * t1 * t1 * path_pts[0].fY +
                             3.0f * t1 * t1 * t * path_pts[1].fY +
                             3.0f * t1 * t * t * path_pts[2].fY +
                             t * t * t * path_pts[3].fY;
                pts.push_back({x, y});
            }
            pts.push_back({path_pts[3].fX, path_pts[3].fY});
            break;
        }
        case SkPath::kClose_Verb:
            // Close: no additional points needed, the next move starts a new subpath
            break;
        case SkPath::kDone_Verb:
        case SkPath::kConic_Verb:
        default:
            break;
        }
    }

    return pts;
}

Result<void> draw_rough_path(SkCanvas* canvas, const GraphicObject& obj,
                             sk_sp<SkShader> shader,
                             const RoughStyleParams& params,
                             RoughRandom& rng)
{
    // Build path commands
    std::vector<PathCommand> cmds;
    bool has_commands = false;

    if (const Value* pc_val = obj.params.find(ParamKey::path_commands)) {
        auto parsed = parse_pml_path_commands(*pc_val);
        if (parsed) {
            cmds = std::move(*parsed);
            has_commands = true;
        } else {
            return std::unexpected(general_error(
                "draw_path: " + parsed.error()));
        }
    }

    if (!has_commands) {
        std::string d = get_string(obj.params, ParamKey::d, "");
        if (d.empty()) return {};

        auto parsed = parse_svg_path_string(d);
        if (!parsed) {
            return std::unexpected(general_error(
                "draw_path: " + parsed.error()));
        }
        cmds = std::move(*parsed);
        has_commands = true;
    }

    if (!has_commands || cmds.empty()) return {};

    SkPath path = build_skpath_from_commands(cmds);

    // Sample path to get points for rough rendering
    auto pts = sample_path_points(path);

    // Determine if path is closed (last contour ends with kClose_Verb)
    bool is_closed = path.isLastContourClosed();

    // Fill layer
    if (obj.fill || shader) {
        if (params.fill_style == "solid" || params.fill_style.empty()) {
            auto ops = rough_linear_path(pts, is_closed, const_cast<RoughStyleParams&>(params), rng);
            SkPath fill_path = rough_ops_to_skpath(ops);
            if (is_closed) {
                SkPathBuilder b(fill_path); b.close();
                fill_path = b.detach();
            }
            SkPaint paint;
            configure_fill_paint(paint, obj.fill, obj.stroke_width, shader, obj.blend_mode);
            if (paint.getColor() != SK_ColorTRANSPARENT || paint.getShader()) {
                canvas->drawPath(fill_path, paint);
            }
        } else {
            // Pattern fill
            std::vector<RoughPoint> poly_pts;
            if (is_closed && !pts.empty()) {
                // Use all sampled points for pattern fill
                poly_pts = pts;
            }
            auto result = draw_rough_fill_impl(canvas, poly_pts, obj.fill, params, rng);
            if (!result) return result;
        }
    }

    // Stroke layer
    if (obj.stroke) {
        auto ops = rough_linear_path(pts, is_closed, const_cast<RoughStyleParams&>(params), rng);
        SkPath stroke_path = rough_ops_to_skpath(ops);
        draw_rough_stroke_path(canvas, stroke_path, obj.stroke, obj.stroke_width, obj.blend_mode);
    }

    return {};
}

// ═══════════════════════════════════════════════════════════════════════════════
// draw_object — main dispatcher
// ═══════════════════════════════════════════════════════════════════════════════
// Shape draw function registry
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

using DrawMap = std::unordered_map<std::string, DrawFn>;

DrawMap& draw_registry() {
    static DrawMap reg;
    return reg;
}

} // anonymous namespace

void register_draw_fn(const std::string& shape_type, DrawFn fn) {
    draw_registry()[shape_type] = std::move(fn);
}

// ═══════════════════════════════════════════════════════════════════════════════
// draw_object — registry-based dispatcher
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

    // Texture-mapped objects are handled separately.
    if (obj.params.find(ParamKey::uv)) {
        return draw_textured_object(canvas, obj);
    }

    // Groups iterate children recursively.
    if (obj.shape_type == "group") {
        return draw_group(canvas, obj, local_shader, lookup);
    }

    // Rough-style shapes: try the rough dispatch first.
    if (obj.shape_type.rfind("rough_", 0) == 0) {
        RoughStyleParams params;
        RoughRandom rng;
        if (extract_rough_params(obj, params, rng)) {
            auto it = draw_registry().find(obj.shape_type);
            if (it != draw_registry().end()) {
                return it->second(canvas, obj, local_shader, lookup);
            }
        }
        // If roughness is 0 or unknown rough_ prefix, strip "rough_" and
        // try the exact shape name.
        std::string exact = obj.shape_type.substr(6); // len("rough_")
        auto it = draw_registry().find(exact);
        if (it != draw_registry().end()) {
            return it->second(canvas, obj, local_shader, lookup);
        }
        return {};
    }

    // Standard shape dispatch via registry.
    auto it = draw_registry().find(obj.shape_type);
    if (it != draw_registry().end()) {
        return it->second(canvas, obj, local_shader, lookup);
    }

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

// ── Static registration of all shape draw functions ────────────────────

namespace {

[[maybe_unused]] bool registered_shapes = []() {
    // Exact shapes (shader-only, no lookup)
    register_draw_fn("circle",  [](SkCanvas* c, const GraphicObject& o, sk_sp<SkShader> s, const ShaderLookup&) { return draw_circle(c, o, s); });
    register_draw_fn("rect",    [](SkCanvas* c, const GraphicObject& o, sk_sp<SkShader> s, const ShaderLookup&) { return draw_rect(c, o, s); });
    register_draw_fn("ellipse", [](SkCanvas* c, const GraphicObject& o, sk_sp<SkShader> s, const ShaderLookup&) { return draw_ellipse(c, o, s); });
    register_draw_fn("line",    [](SkCanvas* c, const GraphicObject& o, sk_sp<SkShader> s, const ShaderLookup&) { return draw_line(c, o, s); });
    register_draw_fn("polygon", [](SkCanvas* c, const GraphicObject& o, sk_sp<SkShader> s, const ShaderLookup&) { return draw_polygon(c, o, s); });
    register_draw_fn("text",    [](SkCanvas* c, const GraphicObject& o, sk_sp<SkShader> s, const ShaderLookup&) { return draw_text(c, o, s); });
    register_draw_fn("path",    [](SkCanvas* c, const GraphicObject& o, sk_sp<SkShader> s, const ShaderLookup&) { return draw_path(c, o, s); });
    register_draw_fn("image",   [](SkCanvas* c, const GraphicObject& o, sk_sp<SkShader> s, const ShaderLookup&) { return draw_image(c, o, s); });
    register_draw_fn("mesh3d",  [](SkCanvas* c, const GraphicObject& o, sk_sp<SkShader> s, const ShaderLookup& l) { return draw_mesh3d(c, o, l); });

    // Rough shapes: extract rough params inside the lambda before dispatching.
    register_draw_fn("rough_circle",  [](SkCanvas* c, const GraphicObject& o, sk_sp<SkShader> s, const ShaderLookup&) {
        RoughStyleParams p; RoughRandom r;
        if (!extract_rough_params(o, p, r)) return Result<void>{};
        return draw_rough_circle(c, o, s, p, r);
    });
    register_draw_fn("rough_rect",    [](SkCanvas* c, const GraphicObject& o, sk_sp<SkShader> s, const ShaderLookup&) {
        RoughStyleParams p; RoughRandom r;
        if (!extract_rough_params(o, p, r)) return Result<void>{};
        return draw_rough_rect(c, o, s, p, r);
    });
    register_draw_fn("rough_ellipse", [](SkCanvas* c, const GraphicObject& o, sk_sp<SkShader> s, const ShaderLookup&) {
        RoughStyleParams p; RoughRandom r;
        if (!extract_rough_params(o, p, r)) return Result<void>{};
        return draw_rough_ellipse(c, o, s, p, r);
    });
    register_draw_fn("rough_line",    [](SkCanvas* c, const GraphicObject& o, sk_sp<SkShader> s, const ShaderLookup&) {
        RoughStyleParams p; RoughRandom r;
        if (!extract_rough_params(o, p, r)) return Result<void>{};
        return draw_rough_line(c, o, p, r);
    });
    register_draw_fn("rough_polygon", [](SkCanvas* c, const GraphicObject& o, sk_sp<SkShader> s, const ShaderLookup&) {
        RoughStyleParams p; RoughRandom r;
        if (!extract_rough_params(o, p, r)) return Result<void>{};
        return draw_rough_polygon(c, o, s, p, r);
    });
    register_draw_fn("rough_path",    [](SkCanvas* c, const GraphicObject& o, sk_sp<SkShader> s, const ShaderLookup&) {
        RoughStyleParams p; RoughRandom r;
        if (!extract_rough_params(o, p, r)) return Result<void>{};
        return draw_rough_path(c, o, s, p, r);
    });
    return true;
}();

} // namespace
}  // namespace pml
