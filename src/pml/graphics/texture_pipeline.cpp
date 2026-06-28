// ==========================================================================================================================================================================================================================================═
// PML Texture Pipeline — Implementation
// ==========================================================================================================================================================================================================================================═

#include "pml/graphics/texture_pipeline.h"

#include "pml/core/texture.h"
#include "pml/core/types.h"
#include "pml/graphics/objects.h"
#include "pml/graphics/triangulation.h"
#include "pml/graphics/uv_solver.h"

namespace pml {

TexturePipelineParams pipeline_extract_params(const GraphicObject& obj) {
    TexturePipelineParams p;
    const Value* uv_val = obj.params.find(ParamKey::uv);
    if (!uv_val) return p;
    const auto* tex_ptr = uv_val->as_texture();
    if (!tex_ptr || !*tex_ptr) return p;
    p.texture = *tex_ptr;

    if (const Value* mode_val = obj.params.find(ParamKey::uv_mode)) {
        if (mode_val->is_number()) {
            p.uv_mode = static_cast<int>(mode_val->to_double());
        }
    }

    auto val_to_int = [](const Value* v, int def) -> int {
        if (!v || !v->is_number()) return def;
        return static_cast<int>(v->to_double());
    };

    WrapMode wx = p.texture->wrap_x;
    WrapMode wy = p.texture->wrap_y;
    FilterMode fm = p.texture->filter;
    if (const Value* v = obj.params.find(ParamKey::wrap_x))
        wx = static_cast<WrapMode>(val_to_int(v, static_cast<int>(wx)));
    if (const Value* v = obj.params.find(ParamKey::wrap_y))
        wy = static_cast<WrapMode>(val_to_int(v, static_cast<int>(wy)));
    if (const Value* v = obj.params.find(ParamKey::filter))
        fm = static_cast<FilterMode>(val_to_int(v, static_cast<int>(fm)));

    p.wrap_x = wx;
    p.wrap_y = wy;
    p.filter = fm;
    return p;
}

UVResult pipeline_solve_uv(
    const TriangulatedMesh& mesh,
    int uv_mode,
    const GraphicObject& obj)
{
    switch (uv_mode) {
        case 0:
            return solve_planar_uv(mesh.vertices);
        case 2: {
            std::vector<Vec2> explicit_uvs;
            if (const Value* uvs_val = obj.params.find(ParamKey::uv_vertices)) {
                const auto* lst = uvs_val->as_list();
                if (lst && *lst) {
                    const auto& elems = (*lst)->elements;
                    for (size_t i = 0; i + 1 < elems.size(); i += 2) {
                        double u = elems[i].to_double();
                        double v = elems[i + 1].to_double();
                        explicit_uvs.push_back({u, v});
                    }
                }
            }
            return apply_explicit_uv(explicit_uvs, mesh.vertices);
        }
        default:
            return solve_harmonic_uv(mesh.vertices, mesh.indices, mesh.contour_map);
    }
}

} // namespace pml
