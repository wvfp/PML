// ==========================================================================================================================================================================================================================================═
// Filter engine unit tests
// ==========================================================================================================================================================================================================================================═

#include <gtest/gtest.h>

#include "pml/backend/backend.h"
#include "pml/filter/adjustments.h"
#include "pml/filter/convolution.h"
#include "pml/filter/filter_backend.h"
#include "pml/filter/image_filter.h"
#include "pml/filter/layer_style.h"
#include "pml/graphics/graphic_object.h"
#include "pml/layer/layer.h"

using namespace pml;

struct DummySurface : Surface {
    using Surface::Surface;
};

class MockFilterBackend : public FilterBackend {
public:
    mutable bool color_matrix_called = false;
    mutable std::array<float, 20> last_matrix{};

    mutable bool blur_called = false;
    mutable float last_blur_x = 0.0f;
    mutable float last_blur_y = 0.0f;
    mutable BlurType last_blur_type = BlurType::Gaussian;

    mutable bool convolution_called = false;
    mutable ConvolutionKernel last_kernel{};

    mutable bool color_table_called = false;
    mutable std::array<uint8_t, 256> last_r{};
    mutable std::array<uint8_t, 256> last_g{};
    mutable std::array<uint8_t, 256> last_b{};
    mutable std::array<uint8_t, 256> last_a{};

    mutable bool drop_shadow_called = false;
    mutable bool inner_shadow_called = false;
    mutable bool outer_glow_called = false;
    mutable bool inner_glow_called = false;
    mutable bool bevel_emboss_called = false;

    void reset() const {
        color_matrix_called = false;
        blur_called = false;
        convolution_called = false;
        color_table_called = false;
        drop_shadow_called = false;
        inner_shadow_called = false;
        outer_glow_called = false;
        inner_glow_called = false;
        bevel_emboss_called = false;
        last_matrix = {};
        last_kernel = {};
        last_r = last_g = last_b = last_a = {};
    }

    [[nodiscard]] Result<void> apply_color_matrix(
        Surface&, const std::array<float, 20>& matrix) override {
        color_matrix_called = true;
        last_matrix = matrix;
        return {};
    }

    [[nodiscard]] Result<void> apply_blur(
        Surface&, float radius_x, float radius_y, BlurType type) override {
        blur_called = true;
        last_blur_x = radius_x;
        last_blur_y = radius_y;
        last_blur_type = type;
        return {};
    }

    [[nodiscard]] Result<void> apply_convolution(
        Surface&, const ConvolutionKernel& kernel) override {
        convolution_called = true;
        last_kernel = kernel;
        return {};
    }

    [[nodiscard]] Result<void> apply_color_table(
        Surface&,
        const std::array<uint8_t, 256>& r,
        const std::array<uint8_t, 256>& g,
        const std::array<uint8_t, 256>& b,
        const std::array<uint8_t, 256>& a) override {
        color_table_called = true;
        last_r = r;
        last_g = g;
        last_b = b;
        last_a = a;
        return {};
    }

    [[nodiscard]] Result<void> apply_offset(Surface&, float, float) override {
        return {};
    }

    [[nodiscard]] Result<void> apply_drop_shadow(
        Surface&, float, float, float, float, uint32_t) override {
        drop_shadow_called = true;
        return {};
    }

    [[nodiscard]] Result<void> apply_inner_shadow(
        Surface&, float, float, float, uint32_t) override {
        inner_shadow_called = true;
        return {};
    }

    [[nodiscard]] Result<void> apply_outer_glow(
        Surface&, float, uint32_t) override {
        outer_glow_called = true;
        return {};
    }

    [[nodiscard]] Result<void> apply_inner_glow(
        Surface&, float, uint32_t) override {
        inner_glow_called = true;
        return {};
    }

    [[nodiscard]] Result<void> apply_bevel_emboss(
        Surface&, float, float, float, uint32_t, uint32_t) override {
        bevel_emboss_called = true;
        return {};
    }
};

TEST(FilterTest, FilterChainAppliesFiltersInOrder) {
    MockFilterBackend backend;
    DummySurface surface(16, 16);

    auto chain = std::make_shared<FilterChain>(std::vector<std::shared_ptr<ImageFilter>>{
        std::make_shared<BlurFilter>(3.0f, BlurFilterType::Gaussian, 0.0f),
        std::make_shared<ThresholdFilter>(128)
    });

    auto result = chain->apply(backend, surface);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(backend.blur_called);
    EXPECT_TRUE(backend.color_table_called);
}

TEST(FilterTest, ColorMatrixFilterCallsBackend) {
    MockFilterBackend backend;
    DummySurface surface(8, 8);

    ColorAdjustParams params;
    params.contrast = 2.0;
    ColorMatrixFilter filter(params);

    auto result = filter.apply(backend, surface);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(backend.color_matrix_called);
    EXPECT_FLOAT_EQ(backend.last_matrix[0], 2.0f);
    EXPECT_FLOAT_EQ(backend.last_matrix[6], 2.0f);
    EXPECT_FLOAT_EQ(backend.last_matrix[12], 2.0f);
}

TEST(FilterTest, BlurFilterGaussianCallsBackend) {
    MockFilterBackend backend;
    DummySurface surface(8, 8);

    BlurFilter filter(5.0f, BlurFilterType::Gaussian, 0.0f);
    auto result = filter.apply(backend, surface);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(backend.blur_called);
    EXPECT_FLOAT_EQ(backend.last_blur_x, 5.0f);
    EXPECT_FLOAT_EQ(backend.last_blur_y, 5.0f);
    EXPECT_EQ(backend.last_blur_type, BlurType::Gaussian);
}

TEST(FilterTest, SharpenFilterUsesConvolutionKernel) {
    MockFilterBackend backend;
    DummySurface surface(8, 8);

    SharpenFilter filter(1.5f, 1.0f);
    auto result = filter.apply(backend, surface);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(backend.convolution_called);
    EXPECT_EQ(backend.last_kernel.width, 3);
    EXPECT_EQ(backend.last_kernel.height, 3);
    EXPECT_FLOAT_EQ(backend.last_kernel.values[4], 7.0f);
}

TEST(FilterTest, DropShadowFilterCallsBackend) {
    MockFilterBackend backend;
    DummySurface surface(8, 8);

    DropShadowFilter filter(3.0f, 4.0f, 5.0f, 5.0f, 0xFF112233);
    auto result = filter.apply(backend, surface);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(backend.drop_shadow_called);
}

TEST(FilterTest, InnerShadowFilterCallsBackend) {
    MockFilterBackend backend;
    DummySurface surface(8, 8);

    InnerShadowFilter filter(2.0f, 3.0f, 4.0f, 0xFF000000);
    auto result = filter.apply(backend, surface);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(backend.inner_shadow_called);
}

TEST(FilterTest, OuterGlowFilterCallsBackend) {
    MockFilterBackend backend;
    DummySurface surface(8, 8);

    OuterGlowFilter filter(6.0f, 0xFFFFAA00);
    auto result = filter.apply(backend, surface);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(backend.outer_glow_called);
}

TEST(FilterTest, InnerGlowFilterCallsBackend) {
    MockFilterBackend backend;
    DummySurface surface(8, 8);

    InnerGlowFilter filter(5.0f, 0xFFFFFFFF);
    auto result = filter.apply(backend, surface);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(backend.inner_glow_called);
}

TEST(FilterTest, BevelEmbossFilterCallsBackend) {
    MockFilterBackend backend;
    DummySurface surface(8, 8);

    BevelEmbossFilter filter(120.0f, 30.0f, 4.0f, 0xFFFFFFFF, 0xFF000000);
    auto result = filter.apply(backend, surface);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(backend.bevel_emboss_called);
}

TEST(FilterTest, LayerWithFilterStoresFilter) {
    Layer layer(LayerProperties{"l"},
                std::make_shared<GraphicObject>("circle"));

    auto filter = std::make_shared<BlurFilter>(2.0f, BlurFilterType::Box, 0.0f);
    Layer updated = layer.with_filter(filter);

    EXPECT_TRUE(updated.properties().filters.size() == 1u);
    EXPECT_EQ(updated.properties().filters[0]->name(), "blur");
    EXPECT_TRUE(layer.properties().filters.empty()); // immutable
}

TEST(FilterTest, LayerWithFiltersReplacesFilterList) {
    Layer layer(LayerProperties{"l"},
                std::make_shared<GraphicObject>("rect"));

    auto filters = std::vector<std::shared_ptr<ImageFilter>>{
        std::make_shared<BlurFilter>(1.0f, BlurFilterType::Gaussian, 0.0f),
        std::make_shared<ColorMatrixFilter>(ColorAdjustParams{})
    };
    Layer updated = layer.with_filters(filters);

    EXPECT_EQ(updated.properties().filters.size(), 2u);
    EXPECT_EQ(updated.properties().filters[0]->name(), "blur");
    EXPECT_EQ(updated.properties().filters[1]->name(), "color-adjust");
}

TEST(FilterTest, FilterChainNameAndEmptyApply) {
    FilterChain chain({});
    EXPECT_EQ(chain.name(), "filter-chain");

    MockFilterBackend backend;
    DummySurface surface(4, 4);
    auto result = chain.apply(backend, surface);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(backend.color_matrix_called);
    EXPECT_FALSE(backend.blur_called);
}
