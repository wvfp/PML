#pragma once

// ==================================================================================================================================================================================================================═
// PML Texture Blend (Skia backend) — Compose multiple texture layers
// ==================================================================================================================================================================================================================═

#include <include/core/SkBlendMode.h>
#include <include/core/SkRefCnt.h>
#include <vector>

#include "pml/core/texture.h"

class SkShader;

namespace pml {

// ----─ TextureLayer ----------------------------------------------------------------------------------------------------------------─
// Represents one texture layer with its placement and blend mode.
// ----------------------------------------------------------------------------------------------------------------------------------------------------─
struct TextureLayer {
    TextureBox texture;          // texture content + offset
    double    x = 0, y = 0;    // placement offset within shape (shape coords)
    double    width  = -1;       // override width  (-1 = use texture width)
    double    height = -1;       // override height (-1 = use texture height)
    SkBlendMode blend_mode = SkBlendMode::kSrcOver;  // how this layer blends with the one below
};

// ----─ compose_texture_layers --------------------------------------------------------------------------------------------─
// Given a list of texture layers, return a single SkShader that samples
// all layers and blends them using their blend modes.
//
// Strategy:
//   1 layer  → SkShaders::Image (direct)
//   2 layers → SkShaders::Blend(mode, bottom, top)
//   3+ layers→ nest SkShaders::Blend (bottom, blend(rest))
//
// The caller must ensure the returned shader is used with a local matrix
// that maps shape UV [0,1] to the correct texture coords.
// ----------------------------------------------------------------------------------------------------------------------------------------------------─
sk_sp<SkShader> compose_texture_layers(const std::vector<TextureLayer>& layers);

}  // namespace pml
