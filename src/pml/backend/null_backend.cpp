// ═══════════════════════════════════════════════════════════════════════════════
// NullBackend — no-op renderer that records all calls
// ═══════════════════════════════════════════════════════════════════════════════
//
// Always compiled and registered as "null" so that BackendRegistry::active()
// always returns a valid backend reference, even in headless/test environments.
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/backend/backend.h"
#include "pml/backend/registry.h"

#include <string>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// NullSurface
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

struct NullSurface final : Surface {
    using Surface::Surface;
};

// ═══════════════════════════════════════════════════════════════════════════════
// NullBackend
// ═══════════════════════════════════════════════════════════════════════════════

class NullBackend final : public RenderBackend {
public:
    NullBackend() = default;

    // ── Metadata ────────────────────────────────────────────────────

    [[nodiscard]] auto info() const -> BackendInfo override
    {
        return BackendInfo{
            .name = "null",
            .description = "Null backend — records all calls, performs no rendering",
            .capabilities = BackendCap::RasterCPU,
        };
    }

    // ── Surface lifecycle ───────────────────────────────────────────

    [[nodiscard]] auto create_surface(int width, int height, uint32_t bg_color)
        -> std::unique_ptr<Surface> override
    {
        ++create_count_;
        last_width_ = width;
        last_height_ = height;
        last_bg_color_ = bg_color;

        auto surf = std::make_unique<NullSurface>();
        surf->width = width;
        surf->height = height;
        return surf;
    }

    // ── Drawing ─────────────────────────────────────────────────────

    auto draw(Surface& /*surface*/, const GraphicObject& /*obj*/)
        -> Result<void> override
    {
        ++draw_count_;
        last_draw_calls_.push_back("draw(GraphicObject)");
        return {};
    }

    // ── Output ──────────────────────────────────────────────────────

    auto save_image(Surface& /*surface*/,
                    const std::string& path,
                    const std::string& format) -> Result<void> override
    {
        ++save_count_;
        last_save_path_ = path;
        last_save_format_ = format;
        return {};
    }

    auto save_animation(const std::vector<Surface*>& /*frames*/,
                        const std::string& path,
                        const std::string& format,
                        int fps) -> Result<void> override
    {
        ++anim_count_;
        last_anim_path_ = path;
        last_anim_format_ = format;
        last_anim_fps_ = fps;
        return {};
    }

    // ── Compositing ─────────────────────────────────────────────────

    auto composite(Surface& /*dst*/, Surface& /*src*/,
                   int /*x*/, int /*y*/) -> Result<void> override
    {
        ++composite_count_;
        return {};
    }

    // ── Shaders ─────────────────────────────────────────────────────

    auto compile_shader(const std::string& glsl) -> Result<uint64_t> override
    {
        ++shader_count_;
        last_glsl_ = glsl;
        return uint64_t{1};  // dummy handle
    }

    // ── Test helpers ────────────────────────────────────────────────

    [[nodiscard]] auto draw_count() const noexcept -> int { return draw_count_; }
    [[nodiscard]] auto create_count() const noexcept -> int { return create_count_; }
    [[nodiscard]] auto save_count() const noexcept -> int { return save_count_; }
    [[nodiscard]] auto shader_count() const noexcept -> int { return shader_count_; }
    [[nodiscard]] auto anim_count() const noexcept -> int { return anim_count_; }

    [[nodiscard]] auto last_draw_calls() const -> const std::vector<std::string>&
    {
        return last_draw_calls_;
    }

    [[nodiscard]] auto last_width() const noexcept -> int { return last_width_; }
    [[nodiscard]] auto last_height() const noexcept -> int { return last_height_; }
    [[nodiscard]] auto last_bg_color() const noexcept -> uint32_t { return last_bg_color_; }
    [[nodiscard]] auto last_save_path() const -> const std::string& { return last_save_path_; }
    [[nodiscard]] auto last_save_format() const -> const std::string& { return last_save_format_; }
    [[nodiscard]] auto last_glsl() const -> const std::string& { return last_glsl_; }

private:
    int draw_count_{0};
    int create_count_{0};
    int save_count_{0};
    int shader_count_{0};
    int anim_count_{0};
    int composite_count_{0};

    int last_width_{0};
    int last_height_{0};
    uint32_t last_bg_color_{0};
    std::string last_save_path_;
    std::string last_save_format_;
    std::string last_anim_path_;
    std::string last_anim_format_;
    int last_anim_fps_{0};
    std::string last_glsl_;

    std::vector<std::string> last_draw_calls_;
};

}  // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// Static registration — always available
// ═══════════════════════════════════════════════════════════════════════════════

[[maybe_unused]] static bool registered_null = BackendRegistry::register_backend(
    "null",
    "Null backend — records all calls, performs no rendering",
    BackendCap::RasterCPU,
    []() -> std::unique_ptr<RenderBackend> {
        return std::make_unique<NullBackend>();
    }
);

}  // namespace pml
