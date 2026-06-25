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
#include <vector>

#include "poly2tri/poly2tri.h"
#include "pml/core/error.h"
#include "pml/core/types.h"
#include "pml/graphics/objects.h"
#include "pml/graphics/transform.h"

namespace pml {
namespace {

// ── Helper: collect contour points from a GraphicObject ────────────────────

std::vector<Vec2> collect_contour(const GraphicObject& obj) {
    const std::string& st = obj.shape_type;
    if (st == "polygon") {
        std::vector<Vec2> result;
        auto* pts_param = obj.params.find(ParamKey::points);
        if (pts_param) {
            if (const auto* lst = pts_param->as_list()) {
                if (*lst) {
                    for (const auto& elem : (*lst)->elements) {
                        if (const auto* pair = elem.as_list()) {
                            if (pair && *pair && (*pair)->elements.size() >= 2) {
                                double x = 0.0, y = 0.0;
                                const auto& a = (*pair)->elements[0];
                                const auto& b = (*pair)->elements[1];
                                if (a.is_number()) x = a.as_double();
                                if (b.is_number()) y = b.as_double();
                                result.push_back({x, y});
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
                            current = {(*cmd_lst)->elements[1].as_double(),
                                       (*cmd_lst)->elements[2].as_double()};
                            start = current;
                            result.push_back(current);
                        }
                    } else if (op_str == "L" || op_str == "l") {
                        if ((*cmd_lst)->elements.size() >= 3) {
                            current = {(*cmd_lst)->elements[1].as_double(),
                                       (*cmd_lst)->elements[2].as_double()};
                            result.push_back(current);
                        }
                    } else if (op_str == "Z" || op_str == "z") {
                        current = start;
                    } else if (op_str == "C" || op_str == "c") {
                        if ((*cmd_lst)->elements.size() >= 7) {
                            Vec2 c0 = current;
                            Vec2 c1{(*cmd_lst)->elements[1].as_double(),
                                    (*cmd_lst)->elements[2].as_double()};
                            Vec2 c2{(*cmd_lst)->elements[3].as_double(),
                                    (*cmd_lst)->elements[4].as_double()};
                            Vec2 c3{(*cmd_lst)->elements[5].as_double(),
                                    (*cmd_lst)->elements[6].as_double()};
                            constexpr int N = 8;
                            for (int i = 1; i <= N; ++i) {
                                double t = static_cast<double>(i) / N;
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
                            Vec2 c1{(*cmd_lst)->elements[1].as_double(),
                                    (*cmd_lst)->elements[2].as_double()};
                            Vec2 c2{(*cmd_lst)->elements[3].as_double(),
                                    (*cmd_lst)->elements[4].as_double()};
                            constexpr int N = 8;
                            for (int i = 1; i <= N; ++i) {
                                double t = static_cast<double>(i) / N;
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
        constexpr int N = 32;
        for (int i = 0; i < N; ++i) {
            double a = 2.0 * std::numbers::pi_v<double> * i / N;
            pts.push_back({cx + rx * std::cos(a), cy + ry * std::sin(a)});
        }
        return pts;
    }
    return {};
}

// ── Helper: convert Vec2 to p2t::Point* (caller owns memory) ─────────────

std::vector<p2t::Point*> to_p2t_points(const std::vector<Vec2>& pts) {
    std::vector<p2t::Point*> result;
    result.reserve(pts.size());
    for (const auto& p : pts) {
        result.push_back(new p2t::Point(p.x, p.y));
    }
    return result;
}

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

    // Convert outer contour to poly2tri points.
    std::vector<p2t::Point*> outer_pts = to_p2t_points(outer_contour);

    // Create CDT with outer contour.
    p2t::CDT cdt(outer_pts);

    // Add holes.
    std::vector<std::vector<p2t::Point*>> hole_ptrs;
    for (const auto& hole : holes) {
        if (hole.size() >= 3) {
            hole_ptrs.push_back(to_p2t_points(hole));
            cdt.AddHole(hole_ptrs.back());
        }
    }

    // Run triangulation.
    cdt.Triangulate();

    // Extract triangles.
    std::vector<p2t::Triangle*> triangles = cdt.GetTriangles();

    if (triangles.empty()) {
        // Cleanup
        for (auto* p : outer_pts) delete p;
        for (const auto& hole : hole_ptrs) {
            for (auto* p : hole) delete p;
        }
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

    // Cleanup poly2tri points.
    for (auto* p : outer_pts) delete p;
    for (const auto& hole : hole_ptrs) {
        for (auto* p : hole) delete p;
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
