// ═══════════════════════════════════════════════════════════════════════
// PML Triangulation — poly2tri-based Constrained Delaunay Triangulation
// ═══════════════════════════════════════════════════════════════════════
// Uses poly2tri (https://github.com/jhasse/poly2tri) for robust CDT.
// Handles simple polygons with optional holes (supports Steiner points).
// ═══════════════════════════════════════════════════════════════════════

#include "pml/graphics/triangulation.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <vector>

#include "poly2tri/poly2tri.h"
#include "pml/core/error.h"
#include "pml/core/types.h"
#include "pml/graphics/objects.h"
#include "pml/graphics/transform.h"

namespace pml {
namespace {

// ── Helper: convert a numeric Value to double (handles int and double) ─────

[[nodiscard]] double as_double(const Value& v) {
    if (v.is_int()) return static_cast<double>(v.int_val());
    if (v.is_double()) return v.double_val();
    return 0.0;
}

// ── Helper: collect contour points from a GraphicObject ────────────────────

std::vector<Vec2> collect_contour(const GraphicObject& obj) {
    const std::string& st = obj.shape_type;
    if (st == "polygon") {
        std::vector<Vec2> result;
        auto* pts_param = obj.params.find(ParamKey::points);
        if (pts_param) {
            if (const auto* lst = pts_param->as_list()) {
                if (*lst) {
                    const auto& elems = (*lst)->elements;
                    // Detect flat list [x0 y0 x1 y1 ...] vs list of pairs.
                    bool flat = true;
                    if (!elems.empty() && elems[0].as_list()) {
                        flat = false;
                    }
                    if (flat) {
                        for (size_t i = 0; i + 1 < elems.size(); i += 2) {
                            double x = 0.0, y = 0.0;
                            if (elems[i].is_number()) x = as_double(elems[i]);
                            if (elems[i + 1].is_number()) y = as_double(elems[i + 1]);
                            result.push_back({x, y});
                        }
                    } else {
                        for (const auto& elem : elems) {
                            if (const auto* pair = elem.as_list()) {
                                if (pair && *pair && (*pair)->elements.size() >= 2) {
                                    double x = 0.0, y = 0.0;
                                    const auto& a = (*pair)->elements[0];
                                    const auto& b = (*pair)->elements[1];
                                    if (a.is_number()) x = as_double(a);
                                    if (b.is_number()) y = as_double(b);
                                    result.push_back({x, y});
                                }
                            }
                        }
                    }
                }
            }
        }
        return result;
    }
    if (st == "path") {
        std::vector<Vec2> result;
        auto* cmds_param = obj.params.find(ParamKey::path_commands);
        if (!cmds_param) return result;
        if (const auto* lst = cmds_param->as_list()) {
            if (!*lst) return result;
            Vec2 current{0.0, 0.0};
            Vec2 start{0.0, 0.0};
            for (const auto& cmd : (*lst)->elements) {
                if (const auto* cmd_lst = cmd.as_list()) {
                    if (!cmd_lst || (*cmd_lst)->elements.empty()) continue;
                    const auto& op = (*cmd_lst)->elements[0];
                    std::string op_str;
                    if (op.is_string()) op_str = *op.as_string();
                    else if (op.is_symbol()) op_str = op.as_symbol()->name;
                    else continue;
                    if (op_str == "M" || op_str == "m") {
                        if ((*cmd_lst)->elements.size() >= 3) {
                            current = {as_double((*cmd_lst)->elements[1]),
                                       as_double((*cmd_lst)->elements[2])};
                            start = current;
                            result.push_back(current);
                        }
                    } else if (op_str == "L" || op_str == "l") {
                        if ((*cmd_lst)->elements.size() >= 3) {
                            current = {as_double((*cmd_lst)->elements[1]),
                                       as_double((*cmd_lst)->elements[2])};
                            result.push_back(current);
                        }
                    } else if (op_str == "Z" || op_str == "z") {
                        current = start;
                    } else if (op_str == "C" || op_str == "c") {
                        if ((*cmd_lst)->elements.size() >= 7) {
                            Vec2 c0 = current;
                            Vec2 c1{as_double((*cmd_lst)->elements[1]),
                                    as_double((*cmd_lst)->elements[2])};
                            Vec2 c2{as_double((*cmd_lst)->elements[3]),
                                    as_double((*cmd_lst)->elements[4])};
                            Vec2 c3{as_double((*cmd_lst)->elements[5]),
                                    as_double((*cmd_lst)->elements[6])};
                            // Adaptive subdivision based on control-point span.
                            double cx_min = std::min({c0.x, c1.x, c2.x, c3.x});
                            double cx_max = std::max({c0.x, c1.x, c2.x, c3.x});
                            double cy_min = std::min({c0.y, c1.y, c2.y, c3.y});
                            double cy_max = std::max({c0.y, c1.y, c2.y, c3.y});
                            double span = std::max(cx_max - cx_min, cy_max - cy_min);
                            int segs = std::max(8, static_cast<int>(std::ceil(span / 8.0)));
                            for (int i = 1; i <= segs; ++i) {
                                double t = static_cast<double>(i) / segs;
                                double u = 1.0 - t;
                                double x = u*u*u*c0.x + 3*u*u*t*c1.x +
                                           3*u*t*t*c2.x + t*t*t*c3.x;
                                double y = u*u*u*c0.y + 3*u*u*t*c1.y +
                                           3*u*t*t*c2.y + t*t*t*c3.y;
                                result.push_back({x, y});
                            }
                            current = c3;
                        }
                    } else if (op_str == "Q" || op_str == "q") {
                        if ((*cmd_lst)->elements.size() >= 5) {
                            Vec2 c0 = current;
                            Vec2 c1{as_double((*cmd_lst)->elements[1]),
                                    as_double((*cmd_lst)->elements[2])};
                            Vec2 c2{as_double((*cmd_lst)->elements[3]),
                                    as_double((*cmd_lst)->elements[4])};
                            double qx_min = std::min({c0.x, c1.x, c2.x});
                            double qx_max = std::max({c0.x, c1.x, c2.x});
                            double qy_min = std::min({c0.y, c1.y, c2.y});
                            double qy_max = std::max({c0.y, c1.y, c2.y});
                            double qspan = std::max(qx_max - qx_min, qy_max - qy_min);
                            int qsegs = std::max(8, static_cast<int>(std::ceil(qspan / 8.0)));
                            for (int i = 1; i <= qsegs; ++i) {
                                double t = static_cast<double>(i) / qsegs;
                                double u = 1.0 - t;
                                double x = u*u*c0.x + 2*u*t*c1.x + t*t*c2.x;
                                double y = u*u*c0.y + 2*u*t*c1.y + t*t*c2.y;
                                result.push_back({x, y});
                            }
                            current = c2;
                        }
                    }
                }
            }
        }
        return result;
    }
    if (st == "rect") {
        auto param_d = [](const GraphicObject& o, ParamKey k, double d) {
            auto* v = o.params.find(k);
            if (!v) return d;
            if (v->is_int()) return static_cast<double>(v->int_val());
            if (v->is_double()) return v->double_val();
            return d;
        };
        double w = param_d(obj, ParamKey::w, 100.0);
        double h = param_d(obj, ParamKey::h, 100.0);
        double x = param_d(obj, ParamKey::x, 0.0);
        double y = param_d(obj, ParamKey::y, 0.0);
        return std::vector<Vec2>{{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}};
    }
    if (st == "ellipse" || st == "circle") {
        auto param_d = [](const GraphicObject& o, ParamKey k, double d) {
            auto* v = o.params.find(k);
            if (!v) return d;
            if (v->is_int()) return static_cast<double>(v->int_val());
            if (v->is_double()) return v->double_val();
            return d;
        };
        double cx = param_d(obj, ParamKey::cx, 0.0);
        double cy = param_d(obj, ParamKey::cy, 0.0);
        double rx = (st == "circle") ? param_d(obj, ParamKey::r, 50.0)
                                     : param_d(obj, ParamKey::rx, 50.0);
        double ry = (st == "circle") ? param_d(obj, ParamKey::r, 50.0)
                                     : param_d(obj, ParamKey::ry, 50.0);
        std::vector<Vec2> pts;
        // Subdivision scales with radius: more segments for larger circles.
        double max_r = std::max(rx, ry);
        int segs = std::max(24, static_cast<int>(std::ceil(max_r / 4.0)));
        segs = std::min(segs, 128); // cap to avoid excessive verts
        for (int i = 0; i < segs; ++i) {
            double a = 2.0 * std::numbers::pi_v<double> * i / segs;
            pts.push_back({cx + rx * std::cos(a), cy + ry * std::sin(a)});
        }
        return pts;
    }
    return {};
}

// ── RAII wrapper for poly2tri point vectors ────────────────────────────
// poly2tri's API requires raw p2t::Point* pointers, but we want automatic
// cleanup on scope exit (exception-safe).

struct P2tPointVec {
    std::vector<p2t::Point*> points;

    explicit P2tPointVec(const std::vector<Vec2>& pts) {
        points.reserve(pts.size());
        for (const auto& p : pts) {
            points.push_back(new p2t::Point(p.x, p.y));
        }
    }

    ~P2tPointVec() {
        for (auto* p : points) delete p;
    }
    // Non-copyable, movable.
    P2tPointVec(const P2tPointVec&) = delete;
    P2tPointVec& operator=(const P2tPointVec&) = delete;
    P2tPointVec(P2tPointVec&&) = default;
    P2tPointVec& operator=(P2tPointVec&&) = default;

    auto begin() const { return points.begin(); }
    auto end() const { return points.end(); }
    size_t size() const { return points.size(); }
};

} // anonymous namespace

// ── triangulate_polygon ────────────────────────────────────────────────────
//
//  Uses poly2tri to perform Constrained Delaunay Triangulation.
//  Supports outer contour + holes.
//

Result<TriangulatedMesh> triangulate_polygon(
    const std::vector<Vec2>& outer_contour,
    const std::vector<std::vector<Vec2>>& holes) {

    if (outer_contour.size() < 3) {
        return std::unexpected(
            pml::type_error("triangulate_polygon: need at least 3 vertices, got " +
                            std::to_string(outer_contour.size()))
        );
    }

    // Convert outer contour to poly2tri points (RAII-managed).
    P2tPointVec outer_pts(outer_contour);

    // Create CDT with outer contour.
    p2t::CDT cdt(outer_pts.points);

    // Add holes.
    std::vector<P2tPointVec> hole_vecs;
    for (const auto& hole : holes) {
        if (hole.size() >= 3) {
            hole_vecs.emplace_back(hole);
            cdt.AddHole(hole_vecs.back().points);
        }
    }

    // Run triangulation.
    cdt.Triangulate();

    // Extract triangles.
    std::vector<p2t::Triangle*> triangles = cdt.GetTriangles();

    if (triangles.empty()) {
        return std::unexpected(
            pml::type_error("triangulate_polygon: poly2tri produced no triangles")
        );
    }

    // Build mesh: collect all unique vertices, then emit indices.
    // poly2tri may introduce Steiner points, so we need to deduplicate.
    std::vector<Vec2> vertices;
    std::vector<int> contour_map;
    // Map from p2t::Point* to vertex index.
    std::unordered_map<p2t::Point*, uint32_t> point_to_idx;

    auto get_or_add_vertex = [&](p2t::Point* p, int original_idx) {
        auto it = point_to_idx.find(p);
        if (it != point_to_idx.end()) return it->second;
        uint32_t idx = static_cast<uint32_t>(vertices.size());
        vertices.push_back({p->x, p->y});
        contour_map.push_back(original_idx);
        point_to_idx[p] = idx;
        return idx;
    };

    std::vector<uint32_t> indices;
    indices.reserve(triangles.size() * 3);

    for (p2t::Triangle* tri : triangles) {
        if (!tri->IsInterior()) continue;  // skip exterior triangles
        for (int i = 0; i < 3; ++i) {
            p2t::Point* p = tri->GetPoint(i);
            // Try to find original index (heuristic: if point is on contour)
            int original_idx = -1;
            for (size_t j = 0; j < outer_contour.size(); ++j) {
                if (std::abs(p->x - outer_contour[j].x) < 1e-6 &&
                    std::abs(p->y - outer_contour[j].y) < 1e-6) {
                    original_idx = static_cast<int>(j);
                    break;
                }
            }
            uint32_t idx = get_or_add_vertex(p, original_idx);
            indices.push_back(idx);
        }
    }

    TriangulatedMesh mesh;
    mesh.vertices = std::move(vertices);
    mesh.indices = std::move(indices);
    mesh.contour_map = std::move(contour_map);
    return mesh;
}

// ── triangulate_shape ──────────────────────────────────────────────────────

Result<TriangulatedMesh> triangulate_shape(const GraphicObject& obj) {
    auto contour = collect_contour(obj);
    if (contour.empty()) {
        return std::unexpected(
            pml::type_error("triangulate_shape: shape type '" + obj.shape_type +
                            "' is not closed or has no contour")
        );
    }
    return triangulate_polygon(contour);
}

} // namespace pml
