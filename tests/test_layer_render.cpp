#include <gtest/gtest.h>
#include "pml/backend/backend.h"
#include "pml/layer/blend_mode.h"
#include "pml/layer/compositor.h"
#include "pml/layer/layer.h"
#include "pml/layer/composition.h"

using namespace pml;

namespace {

struct MockSurface final : Surface {
    using Surface::Surface;
};

class MockBackend final : public RenderBackend {
public:
    BackendInfo info() const override {
        return BackendInfo{"mock", "mock", BackendCap::RasterCPU};
    }

    std::unique_ptr<Surface> create_surface(int w, int h, uint32_t) override {
        return std::make_unique<MockSurface>(w, h);
    }

    Result<void> draw(Surface&, const GraphicObject&) override { return {}; }

    Result<void> save_image(Surface&, const std::string&, const std::string&) override {
        return {};
    }

    Result<void> save_animation(const std::vector<Surface*>&, const std::string&,
                                const std::string&, int) override {
        return {};
    }

    Result<std::unique_ptr<Surface>> load_image(const std::string&) override {
        return std::unexpected(resource_error("mock backend cannot load images"));
    }

    Result<void> composite(Surface&, Surface&, int, int) override { return {}; }

    Result<void> composite_with_blend(Surface&, Surface&, int, int,
                                      BlendMode, float) override { return {}; }

    Result<void> apply_mask(Surface&, Surface&) override { return {}; }

    Result<std::unique_ptr<Surface>> draw_to_new_surface(
        const GraphicObject&, int w, int h, uint32_t bg) override {
        return create_surface(w, h, bg);
    }

    bool supports_blend_mode() const noexcept override { return true; }
    bool supports_layer_compositing() const noexcept override { return true; }

    Result<uint64_t> compile_shader(const std::string&) override { return 1; }

    Result<void> apply_color_matrix(Surface&, const std::array<float, 20>&) override { return {}; }
    Result<void> apply_blur(Surface&, float, float, BlurType) override { return {}; }
    Result<void> apply_convolution(Surface&, const ConvolutionKernel&) override { return {}; }
    Result<void> apply_color_table(Surface&,
        const std::array<uint8_t, 256>&,
        const std::array<uint8_t, 256>&,
        const std::array<uint8_t, 256>&,
        const std::array<uint8_t, 256>&) override { return {}; }
    Result<void> apply_offset(Surface&, float, float) override { return {}; }
    Result<void> apply_drop_shadow(Surface&, float, float, float, float, uint32_t) override { return {}; }
    Result<void> apply_inner_shadow(Surface&, float, float, float, uint32_t) override { return {}; }
    Result<void> apply_outer_glow(Surface&, float, uint32_t) override { return {}; }
    Result<void> apply_inner_glow(Surface&, float, uint32_t) override { return {}; }
    Result<void> apply_bevel_emboss(Surface&, float, float, float, uint32_t, uint32_t) override { return {}; }
};

}

TEST(LayerRenderTest, MockBackendRendersComposition) {
    Composition c("test", Size2D{32, 32});
    auto layer = std::make_shared<Layer>(LayerProperties{"l"},
        std::make_shared<GraphicObject>("circle"));
    c = c.with_layer_appended(layer);

    MockBackend backend;
    Compositor compositor(backend);
    auto result = compositor.render(c);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ((*result)->width, 32);
    EXPECT_EQ((*result)->height, 32);
}
