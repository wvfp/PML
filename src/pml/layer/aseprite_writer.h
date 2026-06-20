#pragma once

#include "spritesheet.h"

#include <string>
#include <vector>

namespace pml {

struct AsepriteFrameTag {
    std::string name;
    int from;
    int to;
    std::string direction = "forward";
};

struct AsepriteMeta {
    std::string app = "pml-cpp";
    std::string version = "1.0";
    std::string image;
    std::string format = "RGBA8888";
    int frame_width;
    int frame_height;
    int columns;
    int rows;
    int frame_duration_ms = 100;
    std::vector<AsepriteFrameTag> frame_tags;
};

[[nodiscard]] std::string write_aseprite_json(
    const SpriteSheet& sheet, const AsepriteMeta& meta);

} // namespace pml
