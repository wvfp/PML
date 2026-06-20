#include "spritesheet.h"

#include <algorithm>
#include <cmath>

namespace pml {

SpriteSheetPacker::SpriteSheetPacker(RenderBackend& backend, int padding)
    : m_backend(backend), m_padding(padding) {}

Result<SpriteSheet> SpriteSheetPacker::pack(
    const std::vector<std::unique_ptr<Surface>>& frames, int cols_hint) {
    if (frames.empty()) {
        return std::unexpected(composition_error({}, "cannot pack empty frame list"));
    }

    int frame_w = 0;
    int frame_h = 0;
    for (const auto& f : frames) {
        if (!f) continue;
        frame_w = std::max(frame_w, f->width);
        frame_h = std::max(frame_h, f->height);
    }

    int n = static_cast<int>(frames.size());
    int cols = cols_hint;
    if (cols <= 0) {
        cols = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(n))));
    }
    cols = std::clamp(cols, 1, std::max(n, 1));
    int rows = static_cast<int>(std::ceil(static_cast<double>(n) / cols));

    int sheet_w = cols * frame_w + (cols - 1) * m_padding;
    int sheet_h = rows * frame_h + (rows - 1) * m_padding;

    auto sheet = m_backend.create_surface(sheet_w, sheet_h, 0x00000000);
    if (!sheet) {
        return std::unexpected(resource_error("failed to create sprite sheet surface"));
    }

    SpriteSheet result;
    result.surface = std::move(sheet);
    result.frame_width = frame_w;
    result.frame_height = frame_h;
    result.columns = cols;
    result.rows = rows;

    for (int i = 0; i < n; ++i) {
        int col = i % cols;
        int row = i / cols;
        int x = col * (frame_w + m_padding);
        int y = row * (frame_h + m_padding);

        if (frames[i]) {
            auto composite_result = m_backend.composite(*result.surface, *frames[i], x, y);
            if (!composite_result) return std::unexpected(composite_result.error());
        }

        result.frames.push_back(SpriteSheetFrame{
            frames[i].get(), x, y, frame_w, frame_h});
    }

    return result;
}

} // namespace pml
