#include "blend_mode.h"

#include <unordered_map>

namespace pml {

namespace {
const std::unordered_map<std::string_view, BlendMode> kBlendMap = {
    {"normal", BlendMode::Normal}, {"multiply", BlendMode::Multiply},
    {"screen", BlendMode::Screen}, {"overlay", BlendMode::Overlay},
    {"soft-light", BlendMode::SoftLight}, {"hard-light", BlendMode::HardLight},
    {"darken", BlendMode::Darken}, {"lighten", BlendMode::Lighten},
    {"color-dodge", BlendMode::ColorDodge}, {"color-burn", BlendMode::ColorBurn},
    {"difference", BlendMode::Difference}, {"exclusion", BlendMode::Exclusion},
    {"hue", BlendMode::Hue}, {"saturation", BlendMode::Saturation},
    {"color", BlendMode::Color}, {"luminosity", BlendMode::Luminosity},
    {"plus", BlendMode::Plus}, {"dst-in", BlendMode::DstIn},
    {"src-atop", BlendMode::SrcAtop},
};
}

std::optional<BlendMode> blend_mode_from_keyword(std::string_view name) {
    auto it = kBlendMap.find(name);
    if (it == kBlendMap.end()) return std::nullopt;
    return it->second;
}

const char* blend_mode_to_keyword(BlendMode mode) noexcept {
    switch (mode) {
    case BlendMode::Normal: return "normal";
    case BlendMode::Multiply: return "multiply";
    case BlendMode::Screen: return "screen";
    case BlendMode::Overlay: return "overlay";
    case BlendMode::SoftLight: return "soft-light";
    case BlendMode::HardLight: return "hard-light";
    case BlendMode::Darken: return "darken";
    case BlendMode::Lighten: return "lighten";
    case BlendMode::ColorDodge: return "color-dodge";
    case BlendMode::ColorBurn: return "color-burn";
    case BlendMode::Difference: return "difference";
    case BlendMode::Exclusion: return "exclusion";
    case BlendMode::Hue: return "hue";
    case BlendMode::Saturation: return "saturation";
    case BlendMode::Color: return "color";
    case BlendMode::Luminosity: return "luminosity";
    case BlendMode::Plus: return "plus";
    case BlendMode::DstIn: return "dst-in";
    case BlendMode::SrcAtop: return "src-atop";
    }
    return "normal";
}

} // namespace pml
