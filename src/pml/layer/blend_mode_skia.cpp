#include "blend_mode_skia.h"

namespace pml {

SkBlendMode to_skia_blend_mode(BlendMode mode) noexcept {
    switch (mode) {
    case BlendMode::Normal: return SkBlendMode::kSrcOver;
    case BlendMode::Multiply: return SkBlendMode::kMultiply;
    case BlendMode::Screen: return SkBlendMode::kScreen;
    case BlendMode::Overlay: return SkBlendMode::kOverlay;
    case BlendMode::SoftLight: return SkBlendMode::kSoftLight;
    case BlendMode::HardLight: return SkBlendMode::kHardLight;
    case BlendMode::Darken: return SkBlendMode::kDarken;
    case BlendMode::Lighten: return SkBlendMode::kLighten;
    case BlendMode::ColorDodge: return SkBlendMode::kColorDodge;
    case BlendMode::ColorBurn: return SkBlendMode::kColorBurn;
    case BlendMode::Difference: return SkBlendMode::kDifference;
    case BlendMode::Exclusion: return SkBlendMode::kExclusion;
    case BlendMode::Hue: return SkBlendMode::kHue;
    case BlendMode::Saturation: return SkBlendMode::kSaturation;
    case BlendMode::Color: return SkBlendMode::kColor;
    case BlendMode::Luminosity: return SkBlendMode::kLuminosity;
    case BlendMode::Plus: return SkBlendMode::kPlus;
    case BlendMode::DstIn: return SkBlendMode::kDstIn;
    case BlendMode::SrcAtop: return SkBlendMode::kSrcATop;
    }
    return SkBlendMode::kSrcOver;
}

} // namespace pml
