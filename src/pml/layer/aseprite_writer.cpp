#include "aseprite_writer.h"

#include <nlohmann/json.hpp>

namespace pml {

std::string write_aseprite_json(const SpriteSheet& sheet, const AsepriteMeta& meta) {
    using json = nlohmann::json;

    json root;
    json frames = json::object();

    for (size_t i = 0; i < sheet.frames.size(); ++i) {
        const auto& f = sheet.frames[i];
        std::string name = meta.image + " " + std::to_string(i) + ".aseprite";
        frames[name] = {
            {"frame", {{"x", f.x}, {"y", f.y}, {"w", f.width}, {"h", f.height}}},
            {"spriteSourceSize", {{"x", 0}, {"y", 0}, {"w", f.width}, {"h", f.height}}},
            {"sourceSize", {{"w", f.width}, {"h", f.height}}},
            {"duration", meta.frame_duration_ms}
        };
    }
    root["frames"] = frames;

    json meta_json = {
        {"app", meta.app},
        {"version", meta.version},
        {"image", meta.image},
        {"format", meta.format},
        {"size", {{"w", sheet.surface->width}, {"h", sheet.surface->height}}},
        {"scale", "1"},
        {"frameTags", json::array()}
    };

    for (const auto& tag : meta.frame_tags) {
        meta_json["frameTags"].push_back({
            {"name", tag.name},
            {"from", tag.from},
            {"to", tag.to},
            {"direction", tag.direction}
        });
    }

    root["meta"] = meta_json;
    return root.dump(2);
}

} // namespace pml
