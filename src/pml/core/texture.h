#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Texture — Rasterised GraphicObject with sampling controls
// ==========================================================================================================================================================================================================================================═

#include <atomic>
#include <cstddef>
#include <memory>

namespace pml {

/// Texture coordinate wrapping (tiling) mode.
enum class WrapMode {
    Clamp,   ///< Clamp to edge (no tiling).
    Repeat,  ///< Repeat the texture.
    Mirror   ///< Mirror the texture on each repeat.
};

/// Texture sampling filter (interpolation) mode.
enum class FilterMode {
    Nearest, ///< Point/nearest-neighbour sampling.
    Linear   ///< Bilinear interpolation.
};

struct GraphicObject;

/// A texture is a rasterised GraphicObject with a stable identity, configurable
/// size, and sampling parameters (wrap/filter). It is a first-class PML value
/// stored in a Box.
struct TextureBox {
    /// Stable identity — assigned once at construction, never re-used.
    size_t stable_id;

    /// Hint for render-backend cache layout. Mutable so the runtime can update
    /// it without replacing the whole value. Not part of identity.
    mutable size_t cache_key_hint{0};

    /// The source graphic object that this texture rasterises.
    /// May be shared across multiple textures.
    std::shared_ptr<GraphicObject> go;

    /// Raster dimensions in pixels.
    int width{512};
    int height{512};

    /// Texture coordinate wrapping mode per axis.
    WrapMode wrap_x{WrapMode::Clamp};
    WrapMode wrap_y{WrapMode::Clamp};

    /// Texture sampling filter.
    FilterMode filter{FilterMode::Linear};

    /// Construct a texture from a GraphicObject and optional dimensions.
    explicit TextureBox(
        std::shared_ptr<GraphicObject> go_,
        int w = 512,
        int h = 512
    ) : stable_id(next_id()),
        go(std::move(go_)),
        width(w),
        height(h)
    {}

    /// Create a duplicate with a new stable identity and the same sampling
    /// parameters but sharing the source GraphicObject.
    [[nodiscard]] TextureBox duplicate() const {
        TextureBox dup(go, width, height);
        dup.wrap_x = wrap_x;
        dup.wrap_y = wrap_y;
        dup.filter = filter;
        return dup;
    }

private:
    static size_t next_id() {
        static std::atomic<size_t> counter{1};
        return counter.fetch_add(1, std::memory_order_relaxed);
    }
};

} // namespace pml
