#pragma once

#include "pml/backend/backend.h"

#include <memory>
#include <vector>

namespace pml {

struct SpriteSheetFrame {
    Surface* surface;
    int x;
    int y;
    int width;
    int height;
};

struct SpriteSheet {
    std::unique_ptr<Surface> surface;
    int frame_width;
    int frame_height;
    int columns;
    int rows;
    std::vector<SpriteSheetFrame> frames;
};

class SpriteSheetPacker {
public:
    explicit SpriteSheetPacker(RenderBackend& backend, int padding = 0);

    [[nodiscard]] Result<SpriteSheet> pack(
        const std::vector<std::unique_ptr<Surface>>& frames, int cols_hint = 0);

private:
    RenderBackend& m_backend;
    int m_padding;
};

} // namespace pml
