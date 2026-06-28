#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Texture Pipeline — Parameter extraction and UV solving stages for
// texture-mapped shape rendering.
//
// These functions form the Skia-agnostic front half of the texture pipeline.
// They are extracted from the anonymous namespace in textured_draw.cpp so
// they can be unit-tested independently of the Skia backend.
// ==========================================================================================================================================================================================================================================═

#include <memory>

#include "pml/core/error.h"
#include "pml/core/texture.h"
#include "pml/core/types.h"
#include "pml/graphics/graphics_types.h"
#include "pml/graphics/triangulation.h"
#include "pml/graphics/uv_solver.h"

namespace pml {

/// Parameters extracted from a GraphicObject's :uv / :uv-mode / wrap/filter
/// attributes.  The caller converts WrapMode/FilterMode to backend-specific
/// types (e.g. SkTileMode / SkFilterMode) when entering the draw stage.
struct TexturePipelineParams {
    /// The texture to map onto the shape.
    std::shared_ptr<TextureBox> texture;

    /// UV projection mode: 0 = planar, 1 = harmonic, 2 = explicit.
    int uv_mode{1};

    /// Wrapping mode per axis.
    WrapMode wrap_x{WrapMode::Clamp};
    WrapMode wrap_y{WrapMode::Clamp};

    /// Sampling filter.
    FilterMode filter{FilterMode::Linear};
};

/// Stage 1: extract texture and UV-mode params from the GraphicObject.
///
/// Reads the :uv, :uv-mode, :wrap-x, :wrap-y, and :filter attributes
/// and returns a populated TexturePipelineParams.  When no :uv attribute
/// is present the returned params.texture is null (caller should skip
/// texturing).
TexturePipelineParams pipeline_extract_params(const GraphicObject& obj);

/// Stage 3: solve UV coordinates from a triangulated mesh.
///
/// Dispatches to solve_planar_uv, solve_harmonic_uv, or apply_explicit_uv
/// based on uv_mode.  Explicit UV coordinates are read from the object's
/// :uv-vertices attribute.
UVResult pipeline_solve_uv(
    const TriangulatedMesh& mesh,
    int uv_mode,
    const GraphicObject& obj
);

}  // namespace pml
