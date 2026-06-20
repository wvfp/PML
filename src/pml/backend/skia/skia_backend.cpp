// ═══════════════════════════════════════════════════════════════════════════════
// PML Skia render backend — real implementation
// ═══════════════════════════════════════════════════════════════════════════════
//
// Uses the pre-built Skia static library. All Skia headers are included through
// the generated aggregate header <skia.h>.

#include "pml/api/context.h"
#include "pml/asset/asset_cache.h"
#include "pml/backend/backend.h"
#include "pml/backend/registry.h"
#include "pml/backend/color_helpers.h"
#include "pml/backend/gif/gif_exporter.h"
#include "pml/graphics/objects.h"
#include "pml/graphics/transform.h"
#include "pml/graphics3d/camera3d.h"
#include "pml/graphics3d/mesh3d.h"
#include "pml/graphics3d/transform3d.h"
#include "pml/layer/blend_mode_skia.h"

#include <skia.h>
#include <codec/SkCodec.h>
#include <effects/SkPerlinNoiseShader.h>

#include <png.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

namespace {

// ═══════════════════════════════════════════════════════════════════════════════
// SkiaSurface
// ═══════════════════════════════════════════════════════════════════════════════

struct SkiaSurface final : Surface {
    // Bitmap must outlive `surface`, therefore declare it first.
    SkBitmap bitmap;
    sk_sp<SkSurface> surface;

    SkiaSurface(int w, int h, uint32_t bg_color)
    {
        width = w;
        height = h;
        bitmap.allocN32Pixels(w, h);
        bitmap.eraseColor(static_cast<SkColor>(bg_color));
        surface = SkSurfaces::WrapPixels(
            bitmap.info(), bitmap.getPixels(), bitmap.rowBytes());
    }

    // Construct from an already-decoded SkBitmap.
    explicit SkiaSurface(SkBitmap decoded)
    {
        width = decoded.width();
        height = decoded.height();
        bitmap = std::move(decoded);
        surface = SkSurfaces::WrapPixels(
            bitmap.info(), bitmap.getPixels(), bitmap.rowBytes());
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// Color helpers
// ═══════════════════════════════════════════════════════════════════════════════

inline SkColor to_sk_color(uint32_t argb)
{
    // parse_color() returns 0xAARRGGBB, which matches SkColor layout.
    return static_cast<SkColor>(argb);
}

std::optional<SkColor> parse_sk_color(const std::string& s)
{
    auto c = parse_color(s);
    if (!c) return std::nullopt;
    return to_sk_color(*c);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Paint helpers
// ═══════════════════════════════════════════════════════════════════════════════

void configure_fill_paint(
    SkPaint& paint,
    const std::optional<std::string>& fill,
    double /*stroke_width*/,
    sk_sp<SkShader> shader = nullptr)
{
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);
    if (shader) {
        paint.setShader(std::move(shader));
        paint.setColor(SK_ColorWHITE);
    } else if (fill) {
        if (auto c = parse_sk_color(*fill)) {
            paint.setColor(*c);
        } else {
            paint.setColor(SK_ColorTRANSPARENT);
        }
    } else {
        paint.setColor(SK_ColorTRANSPARENT);
    }
}

void configure_stroke_paint(
    SkPaint& paint,
    const std::optional<std::string>& stroke,
    double stroke_width)
{
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(static_cast<SkScalar>(stroke_width));
    if (stroke) {
        if (auto c = parse_sk_color(*stroke)) {
            paint.setColor(*c);
        } else {
            paint.setColor(SK_ColorTRANSPARENT);
        }
    } else {
        paint.setColor(SK_ColorTRANSPARENT);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Param extraction
// ═══════════════════════════════════════════════════════════════════════════════

[[nodiscard]] double get_double(
    const Params& params,
    ParamKey key,
    double default_val)
{
    const Value* v = params.find(key);
    if (!v) return default_val;
    if (v->is_int()) {
        return static_cast<double>(v->int_val());
    }
    if (v->is_double()) {
        return v->double_val();
    }
    return default_val;
}

[[nodiscard]] std::string get_string(
    const Params& params,
    ParamKey key,
    const std::string& default_val)
{
    const Value* v = params.find(key);
    if (!v) return default_val;
    if (const auto* s = v->as_string()) {
        return *s;
    }
    return default_val;
}

[[nodiscard]] std::optional<SkRect> parse_rect_value(const Value& v)
{
    const auto* lst = v.as_list();
    if (!lst || !*lst) return std::nullopt;
    const auto& elems = (*lst)->elements;
    if (elems.size() < 4) return std::nullopt;

    double vals[4] = {0.0, 0.0, 0.0, 0.0};
    for (int i = 0; i < 4; ++i) {
        if (elems[i].is_int()) {
            vals[i] = static_cast<double>(elems[i].int_val());
        } else if (elems[i].is_double()) {
            vals[i] = elems[i].double_val();
        } else {
            return std::nullopt;
        }
    }
    return SkRect::MakeXYWH(
        static_cast<SkScalar>(vals[0]),
        static_cast<SkScalar>(vals[1]),
        static_cast<SkScalar>(vals[2]),
        static_cast<SkScalar>(vals[3]));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Recursive drawing
// ═══════════════════════════════════════════════════════════════════════════════

using ShaderLookup = std::function<sk_sp<SkShader>(int64_t)>;

Result<void> draw_object(SkCanvas* canvas, const GraphicObject& obj,
                         sk_sp<SkShader> shader, const ShaderLookup& lookup);
Result<void> draw_mesh3d(SkCanvas* canvas, const GraphicObject& obj,
                         const ShaderLookup& lookup);

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

Result<void> draw_mesh3d(SkCanvas* canvas, const GraphicObject& obj,
                         const ShaderLookup& lookup) {
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
        // Our local winding produces front-faces with positive signed area
        // in screen coordinates (Y down), so cull faces with negative area.
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

// ═══════════════════════════════════════════════════════════════════════════════
// PNG loading helper (libpng)
// ═══════════════════════════════════════════════════════════════════════════════
//
// The prebuilt skia.lib in this tree does not expose a working SkPngDecoder,
// so we decode PNGs through libpng and upload the pixels into an SkBitmap.

static Result<std::unique_ptr<Surface>> load_png_with_libpng(
    const std::string& path)
{
    FILE* fp = nullptr;
    if (fopen_s(&fp, path.c_str(), "rb") != 0 || !fp) {
        return std::unexpected(resource_error(
            std::format("skia: cannot open image file: {}", path)));
    }

    constexpr size_t png_sig_size = 8;
    std::array<uint8_t, png_sig_size> sig{};
    if (fread(sig.data(), 1, png_sig_size, fp) != png_sig_size) {
        fclose(fp);
        return std::unexpected(resource_error(
            std::format("skia: cannot read image file: {}", path)));
    }
    if (png_sig_cmp(sig.data(), 0, png_sig_size) != 0) {
        fclose(fp);
        return std::unexpected(resource_error(
            std::format("skia: not a PNG file: {}", path)));
    }

    png_structp png = png_create_read_struct(
        PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        return std::unexpected(resource_error(
            "skia: png_create_read_struct failed"));
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        fclose(fp);
        return std::unexpected(resource_error(
            "skia: png_create_info_struct failed"));
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return std::unexpected(resource_error(
            std::format("skia: libpng decode error: {}", path)));
    }

    png_init_io(png, fp);
    png_set_sig_bytes(png, static_cast<int>(png_sig_size));
    png_read_info(png, info);

    int w = static_cast<int>(png_get_image_width(png, info));
    int h = static_cast<int>(png_get_image_height(png, info));
    png_byte bit_depth = png_get_bit_depth(png, info);
    png_byte color_type = png_get_color_type(png, info);

    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }
    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }
    if (bit_depth == 16) {
        png_set_strip_16(png);
    }
    if (color_type == PNG_COLOR_TYPE_RGB) {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }

    png_read_update_info(png, info);

    const size_t row_bytes = png_get_rowbytes(png, info);
    std::vector<uint8_t> temp_row(static_cast<size_t>(row_bytes));

    SkImageInfo sk_info = SkImageInfo::MakeN32Premul(w, h);
    SkBitmap bitmap;
    if (!bitmap.tryAllocPixels(sk_info)) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return std::unexpected(resource_error(
            "skia: failed to allocate image pixels"));
    }

    uint8_t* dst_pixels = static_cast<uint8_t*>(bitmap.getPixels());
    const size_t dst_row_bytes = bitmap.rowBytes();

    for (int y = 0; y < h; ++y) {
        png_read_row(png, temp_row.data(), nullptr);
        uint8_t* dst = dst_pixels + y * dst_row_bytes;
        for (int x = 0; x < w; ++x) {
            // libpng gives RGBA; Skia kN32 on little-endian is BGRA.
            dst[x * 4 + 0] = temp_row[x * 4 + 2];  // B <- R
            dst[x * 4 + 1] = temp_row[x * 4 + 1];  // G <- G
            dst[x * 4 + 2] = temp_row[x * 4 + 0];  // R <- B
            dst[x * 4 + 3] = temp_row[x * 4 + 3];  // A <- A
        }
    }

    png_read_end(png, nullptr);
    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);

    auto skia_surf = std::make_unique<SkiaSurface>(std::move(bitmap));
    return std::unique_ptr<Surface>(std::move(skia_surf));
}

// ═══════════════════════════════════════════════════════════════════════════════
// SkiaBackend
// ═══════════════════════════════════════════════════════════════════════════════

class SkiaBackend final : public RenderBackend {
public:
    [[nodiscard]] auto info() const -> BackendInfo override
    {
        return BackendInfo{
            .name = "skia",
            .description = "Skia raster/GPU backend",
            .capabilities =
                BackendCap::RasterCPU
              | BackendCap::GPUAccel
              | BackendCap::Shaders
              | BackendCap::VectorOutput
              | BackendCap::AnimationGIF
              | BackendCap::FontRendering
              | BackendCap::LoadPNG
              | BackendCap::LoadImage,
        };
    }

    [[nodiscard]] auto create_surface(int width, int height, uint32_t bg_color)
        -> std::unique_ptr<Surface> override
    {
        return std::make_unique<SkiaSurface>(width, height, bg_color);
    }

    [[nodiscard]] sk_sp<SkShader> lookup_shader(int64_t handle) const
    {
        if (handle <= 0) return nullptr;

        // Check runtime effect cache (handles 1 - 999999)
        auto it = shader_cache_.find(static_cast<uint64_t>(handle));
        if (it != shader_cache_.end() && it->second) {
            // No uniforms for now; pass empty uniforms data and no children.
            return it->second->makeShader(SkData::MakeEmpty(), {}, nullptr);
        }

        // Check pre-built shader cache (handles >= 1000000)
        auto pit = preshader_cache_.find(static_cast<uint64_t>(handle));
        if (pit != preshader_cache_.end() && pit->second) {
            return pit->second;
        }

        return nullptr;
    }

private:
    uint64_t next_shader_handle_{1};
    uint64_t next_preshader_handle_{1000000};
    std::unordered_map<uint64_t, sk_sp<SkRuntimeEffect>> shader_cache_;
    std::unordered_map<uint64_t, sk_sp<SkShader>> preshader_cache_;

    auto draw(Surface& surface, const GraphicObject& obj)
        -> Result<void> override
    {
        auto* skia_surf = dynamic_cast<SkiaSurface*>(&surface);
        if (!skia_surf) {
            return std::unexpected(general_error(
                "skia draw: invalid surface type"));
        }
        ShaderLookup lookup = [this](int64_t h) { return lookup_shader(h); };
        return draw_object(skia_surf->surface->getCanvas(), obj, nullptr, lookup);
    }

    auto save_image(Surface& surface, const std::string& path,
                    const std::string& format) -> Result<void> override
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

    auto save_animation(const std::vector<Surface*>& frames,
                        const std::string& path,
                        const std::string& /*format*/,
                        int fps) -> Result<void> override
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

    [[nodiscard]] auto load_image(const std::string& path)
        -> Result<std::unique_ptr<Surface>> override
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

    auto composite(Surface& dst, Surface& src, int x, int y)
        -> Result<void> override
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

    [[nodiscard]] auto draw_to_new_surface(
        const GraphicObject& obj, int w, int h, uint32_t bg)
        -> Result<std::unique_ptr<Surface>> override
    {
        auto surface = create_surface(w, h, bg);
        if (!surface) {
            return std::unexpected(resource_error("failed to create layer surface"));
        }
        auto result = draw(*surface, obj);
        if (!result) return std::unexpected(result.error());
        return surface;
    }

    auto composite_with_blend(Surface& dst, Surface& src, int x, int y,
                              BlendMode blend, float opacity)
        -> Result<void> override
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

    auto apply_mask(Surface& dst, Surface& mask) -> Result<void> override {
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

    [[nodiscard]] auto supports_blend_mode() const noexcept -> bool override { return true; }
    [[nodiscard]] auto supports_layer_compositing() const noexcept -> bool override { return true; }

    auto compile_shader(const std::string& sksl) -> Result<uint64_t> override
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

    auto create_noise_shader(NoiseType type,
                            float base_freq_x, float base_freq_y,
                            int octaves, float seed,
                            int tile_w, int tile_h) -> Result<uint64_t> override
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

    auto create_shader_with_uniforms(uint64_t shader_handle,
                                     const std::vector<uint8_t>& uniform_data)
        -> Result<uint64_t> override
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

    // ── FilterBackend ─────────────────────────────────────────────────

    [[nodiscard]] Result<void> apply_sk_image_filter(
        SkiaSurface& surf, sk_sp<SkImageFilter> filter)
    {
        if (!filter) return {};

        auto tmp = SkSurfaces::Raster(surf.bitmap.info());
        if (!tmp) {
            return std::unexpected(filter_error(
                {}, "skia: failed to create filter scratch surface"));
        }

        SkPaint paint;
        paint.setImageFilter(std::move(filter));
        tmp->getCanvas()->drawImage(
            surf.bitmap.asImage().get(), 0, 0, SkSamplingOptions(), &paint);

        SkPixmap pixmap;
        if (!tmp->peekPixels(&pixmap)) {
            return std::unexpected(filter_error(
                {}, "skia: failed to read filtered pixels"));
        }
        if (!surf.bitmap.writePixels(pixmap)) {
            return std::unexpected(filter_error(
                {}, "skia: failed to copy filtered pixels back"));
        }
        return {};
    }

    Result<void> apply_color_matrix(
        Surface& s, const std::array<float,20>& m) override
    {
        auto* surf = dynamic_cast<SkiaSurface*>(&s);
        if (!surf) {
            return std::unexpected(filter_error(
                {}, "skia: invalid surface for color matrix"));
        }
        sk_sp<SkColorFilter> cf = SkColorFilters::Matrix(m.data());
        sk_sp<SkImageFilter> imf = SkImageFilters::ColorFilter(
            std::move(cf), nullptr);
        return apply_sk_image_filter(*surf, std::move(imf));
    }

    Result<void> apply_blur(
        Surface& s, float rx, float ry, BlurType type) override
    {
        auto* surf = dynamic_cast<SkiaSurface*>(&s);
        if (!surf) {
            return std::unexpected(filter_error(
                {}, "skia: invalid surface for blur"));
        }
        if (type == BlurType::Gaussian) {
            auto imf = SkImageFilters::Blur(
                rx, ry, SkTileMode::kDecal, nullptr);
            return apply_sk_image_filter(*surf, std::move(imf));
        }
        return std::unexpected(filter_not_supported(
            {}, "skia: only gaussian blur is supported natively"));
    }

    Result<void> apply_convolution(
        Surface& s, const ConvolutionKernel& k) override
    {
        auto* surf = dynamic_cast<SkiaSurface*>(&s);
        if (!surf) {
            return std::unexpected(filter_error(
                {}, "skia: invalid surface for convolution"));
        }

        SkISize size{k.width, k.height};
        std::vector<SkScalar> kernel(k.values.begin(), k.values.end());
        SkScalar gain = k.divisor != 0.0f ? 1.0f / k.divisor : 1.0f;
        SkScalar bias = static_cast<SkScalar>(k.offset);
        SkIPoint target{
            k.anchor_x >= 0 ? k.anchor_x : k.width / 2,
            k.anchor_y >= 0 ? k.anchor_y : k.height / 2};

        auto imf = SkImageFilters::MatrixConvolution(
            size, kernel.data(), gain, bias, target,
            SkTileMode::kDecal, false, nullptr);
        return apply_sk_image_filter(*surf, std::move(imf));
    }

    Result<void> apply_color_table(
        Surface& s,
        const std::array<uint8_t,256>& r,
        const std::array<uint8_t,256>& g,
        const std::array<uint8_t,256>& b,
        const std::array<uint8_t,256>& a) override
    {
        auto* surf = dynamic_cast<SkiaSurface*>(&s);
        if (!surf) {
            return std::unexpected(filter_error(
                {}, "skia: invalid surface for color table"));
        }
        sk_sp<SkColorFilter> cf = SkColorFilters::TableARGB(
            const_cast<uint8_t*>(a.data()),
            const_cast<uint8_t*>(r.data()),
            const_cast<uint8_t*>(g.data()),
            const_cast<uint8_t*>(b.data()));
        sk_sp<SkImageFilter> imf = SkImageFilters::ColorFilter(
            std::move(cf), nullptr);
        return apply_sk_image_filter(*surf, std::move(imf));
    }

    Result<void> apply_offset(Surface& s, float dx, float dy) override
    {
        auto* surf = dynamic_cast<SkiaSurface*>(&s);
        if (!surf) {
            return std::unexpected(filter_error(
                {}, "skia: invalid surface for offset"));
        }
        auto imf = SkImageFilters::Offset(dx, dy, nullptr);
        return apply_sk_image_filter(*surf, std::move(imf));
    }

    Result<void> apply_drop_shadow(
        Surface& s, float dx, float dy,
        float blur_x, float blur_y, uint32_t color) override
    {
        auto* surf = dynamic_cast<SkiaSurface*>(&s);
        if (!surf) {
            return std::unexpected(filter_error(
                {}, "skia: invalid surface for drop shadow"));
        }
        auto imf = SkImageFilters::DropShadow(
            dx, dy, blur_x, blur_y, to_sk_color(color), nullptr);
        return apply_sk_image_filter(*surf, std::move(imf));
    }

    Result<void> apply_inner_shadow(
        Surface& s, float dx, float dy, float blur, uint32_t color) override
    {
        auto* surf = dynamic_cast<SkiaSurface*>(&s);
        if (!surf) {
            return std::unexpected(filter_error(
                {}, "skia: invalid surface for inner shadow"));
        }

        // 1. Render the shadow outside the shape (DropShadowOnly).
        // 2. Mask it by the source alpha so it only appears inside the shape.
        // 3. Composite the inner shadow on top of the original source.
        auto shadow = SkImageFilters::DropShadowOnly(
            dx, dy, blur, blur, to_sk_color(color), nullptr);
        auto masked = SkImageFilters::Blend(
            SkBlendMode::kSrcIn, nullptr, std::move(shadow));
        auto merge = SkImageFilters::Merge(nullptr, std::move(masked));
        return apply_sk_image_filter(*surf, std::move(merge));
    }

    Result<void> apply_outer_glow(
        Surface& s, float blur, uint32_t color) override
    {
        auto* surf = dynamic_cast<SkiaSurface*>(&s);
        if (!surf) {
            return std::unexpected(filter_error(
                {}, "skia: invalid surface for outer glow"));
        }

        // Glow is a colored blur behind the source.
        auto glow = SkImageFilters::DropShadowOnly(
            0, 0, blur, blur, to_sk_color(color), nullptr);
        auto merge = SkImageFilters::Merge(std::move(glow), nullptr);
        return apply_sk_image_filter(*surf, std::move(merge));
    }

    Result<void> apply_inner_glow(
        Surface& s, float blur, uint32_t color) override
    {
        auto* surf = dynamic_cast<SkiaSurface*>(&s);
        if (!surf) {
            return std::unexpected(filter_error(
                {}, "skia: invalid surface for inner glow"));
        }

        // Same as inner shadow but centered (no offset).
        auto glow = SkImageFilters::DropShadowOnly(
            0, 0, blur, blur, to_sk_color(color), nullptr);
        auto masked = SkImageFilters::Blend(
            SkBlendMode::kSrcIn, nullptr, std::move(glow));
        auto merge = SkImageFilters::Merge(nullptr, std::move(masked));
        return apply_sk_image_filter(*surf, std::move(merge));
    }

    Result<void> apply_bevel_emboss(
        Surface& s, float angle, float altitude, float blur,
        uint32_t highlight, uint32_t shadow) override
    {
        auto* surf = dynamic_cast<SkiaSurface*>(&s);
        if (!surf) {
            return std::unexpected(filter_error(
                {}, "skia: invalid surface for bevel/emboss"));
        }

        // Convert spherical angle/altitude to a normalized light direction.
        const double deg2rad = 3.14159265358979323846 / 180.0;
        const double a = angle * deg2rad;
        const double alt = altitude * deg2rad;
        const float z = static_cast<float>(std::sin(alt));
        const float xy = static_cast<float>(std::cos(alt));
        SkPoint3 dir = SkPoint3::Make(
            xy * static_cast<float>(std::cos(a)),
            xy * static_cast<float>(std::sin(a)),
            z);
        dir.normalize();
        SkPoint3 opp = -dir;

        // Use Skia's diffuse lighting filters with the source alpha as bump map.
        // Two opposite lights give highlight and shadow edges.
        const SkScalar surface_scale = std::max<SkScalar>(1.0f, blur / 2.0f);
        auto highlight_lit = SkImageFilters::DistantLitDiffuse(
            dir, to_sk_color(highlight), surface_scale, 1.0f, nullptr);
        auto shadow_lit = SkImageFilters::DistantLitDiffuse(
            opp, to_sk_color(shadow), surface_scale, 1.0f, nullptr);

        sk_sp<SkImageFilter> filters[] = {
            nullptr,
            std::move(shadow_lit),
            std::move(highlight_lit),
        };
        auto merge = SkImageFilters::Merge(filters, 3);
        return apply_sk_image_filter(*surf, std::move(merge));
    }
};

}  // namespace

// ═══════════════════════════════════════════════════════════════════════════════
// Registration
// ═══════════════════════════════════════════════════════════════════════════════

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
