// ==========================================================================================================================================================================================================================================═
// PML Built-in Procedures — Shader + Blend Mode Regression Tests
// ==========================================================================================================================================================================================================================================═
//
// GTest regression coverage for Task 3 of the fix-shader-blend-mode-forwarding
// spec: verify that apply-shader! forwards :blend-mode to the returned
// GraphicObject and that the blend mode is actually honoured when the scene
// is rendered.
//
// These tests run through the full lexer → parser → expander → evaluator
// pipeline via PMLRuntime, then inspect the resulting GraphicObject and,
// when Skia is available, render it to a small surface and read back pixels.
// ==========================================================================================================================================================================================================================================═

#include <gtest/gtest.h>
#include "pml/api/api.h"
#include "pml/backend/backend.h"
#include "pml/backend/registry.h"
#include "pml/evaluator/environment.h"
#include "pml/graphics/objects.h"
#include "pml/layer/blend_mode.h"

#ifdef PML_USE_SKIA
#include "pml/backend/skia/skia_backend_internal.h"
#endif

#include <memory>
#include <string>

class PMLBuiltinsSmokeTest : public ::testing::Test {
protected:
    std::string saved_backend_;
    pml::PMLRuntime rt;

    void SetUp() override {
        saved_backend_ = pml::BackendRegistry::instance().active().info().name;
#ifdef PML_USE_SKIA
        // Ensure the Skia backend TU is linked and registered, then activate it.
        [[maybe_unused]] auto force_link = std::make_unique<pml::SkiaBackend>();
        ASSERT_TRUE(pml::BackendRegistry::instance().set_active("skia"));
#endif
    }

    void TearDown() override {
        pml::BackendRegistry::instance().set_active(saved_backend_);
    }
};

// Execute PML source and return the last expression's raw Value by binding it
// to a temporary variable and looking it up in the runtime environment.
static pml::Value eval_value(pml::PMLRuntime& rt, const std::string& source) {
    auto wrap = "(define __test_result__ (begin " + source + "))";
    auto r = rt.execute(wrap);
    EXPECT_TRUE(r.success);
    auto looked_up = rt.env()->try_lookup("__test_result__");
    EXPECT_TRUE(looked_up.has_value());
    return *looked_up;
}

// Extract the GraphicObject from a Value and assert it is one.
static const pml::GraphicObject& expect_graphic_object(const pml::Value& v) {
    const auto* go = v.as_graphic_object();
    EXPECT_NE(go, nullptr);
    EXPECT_NE(*go, nullptr);
    return **go;
}

#ifdef PML_USE_SKIA

// Read a pixel from a Skia-backed surface (unpremultiplied 0xAARRGGBB).
static uint32_t get_pixel(const pml::Surface& surf, int x, int y) {
    const auto* skia = dynamic_cast<const pml::SkiaSurface*>(&surf);
    if (!skia) return 0;
    return skia->bitmap.getColor(x, y);
}

// Render a GraphicObject to a freshly-allocated Skia surface.
static std::unique_ptr<pml::Surface>
render_to_surface(pml::RenderBackend& backend, const pml::GraphicObject& obj,
                  int w, int h, uint32_t bg) {
    auto surf = backend.create_surface(w, h, bg);
    EXPECT_NE(surf, nullptr);
    auto r = backend.draw(*surf, obj);
    EXPECT_TRUE(r.has_value()) << r.error().what();
    return surf;
}

static int sk_color_a(uint32_t c) { return static_cast<int>((c >> 24) & 0xFF); }
static int sk_color_r(uint32_t c) { return static_cast<int>((c >> 16) & 0xFF); }
static int sk_color_g(uint32_t c) { return static_cast<int>((c >> 8) & 0xFF); }
static int sk_color_b(uint32_t c) { return static_cast<int>(c & 0xFF); }

#endif // PML_USE_SKIA

TEST_F(PMLBuiltinsSmokeTest, ShaderAlphaDstInMaskTest) {
    // apply-shader! must forward :blend-mode "dst-in" so that an SDF-style
    // alpha mask can be used to composite against the scene below.
    auto value = eval_value(rt,
        "(define alpha-mask-shader"
        "  (shader \"half4 main(float2 xy) { return half4(1.0, 1.0, 1.0, 1.0 - xy.x / 64.0); }\"))"
        "(define masked-rect"
        "  (apply-shader! (rect 0 0 64 64) alpha-mask-shader :blend-mode \"dst-in\"))"
        "(group (circle 32 32 24 :fill \"#ff0000\") masked-rect)");

    const auto& group = expect_graphic_object(value);

    // Property check: the shader-wrapped rect carries DstIn.
    auto masked_lookup = rt.env()->try_lookup("masked-rect");
    ASSERT_TRUE(masked_lookup.has_value());
    const auto& masked_obj = expect_graphic_object(*masked_lookup);
    ASSERT_TRUE(masked_obj.blend_mode.has_value());
    EXPECT_EQ(masked_obj.blend_mode.value(), pml::BlendMode::DstIn);

#ifdef PML_USE_SKIA
    // Pixel check: render the group to a black surface.
    auto surf = render_to_surface(
        pml::BackendRegistry::instance().active(), group, 64, 64, 0xFF000000);
    ASSERT_NE(surf, nullptr);

    // Left side: mask alpha is high (1 - 16/64 = 0.75) so the red circle
    // should still be clearly visible.
    uint32_t left = get_pixel(*surf, 16, 32);
    EXPECT_GT(sk_color_r(left), 200) << "left pixel should be red-ish";
    EXPECT_GT(sk_color_a(left), 150) << "left pixel should be fairly opaque";

    // Right side: mask alpha is low (1 - 48/64 = 0.25) so the red circle
    // should be heavily attenuated / transparent.
    uint32_t right = get_pixel(*surf, 48, 32);
    EXPECT_LT(sk_color_a(right), 120) << "right pixel should be fairly transparent";
    EXPECT_LT(sk_color_r(right) * sk_color_a(right) / 255, 120)
        << "right pixel's premultiplied red should be attenuated by mask";
#endif
}

TEST_F(PMLBuiltinsSmokeTest, ShaderMultiplyBlendTest) {
    // apply-shader! must forward :blend-mode "multiply" so that a shader
    // returning a solid colour multiplies with the destination pixels.
    auto value = eval_value(rt,
        "(define blue-shader"
        "  (shader \"half4 main(float2 xy) { return half4(0.0, 0.0, 1.0, 1.0); }\"))"
        "(define blue-rect"
        "  (apply-shader! (rect 16 16 32 32) blue-shader :blend-mode \"multiply\"))"
        "(group (circle 32 32 24 :fill \"#ff0000\") blue-rect)");

    const auto& group = expect_graphic_object(value);

    // Property check: the shader-wrapped rect carries Multiply.
    auto blue_lookup = rt.env()->try_lookup("blue-rect");
    ASSERT_TRUE(blue_lookup.has_value());
    const auto& blue_obj = expect_graphic_object(*blue_lookup);
    ASSERT_TRUE(blue_obj.blend_mode.has_value());
    EXPECT_EQ(blue_obj.blend_mode.value(), pml::BlendMode::Multiply);

#ifdef PML_USE_SKIA
    // Pixel check: render the group to a black surface.
    auto surf = render_to_surface(
        pml::BackendRegistry::instance().active(), group, 64, 64, 0xFF000000);
    ASSERT_NE(surf, nullptr);

    // Inside the red circle but outside the blue rect: still red.
    uint32_t outside = get_pixel(*surf, 12, 32);
    EXPECT_GT(sk_color_r(outside), 200) << "pixel outside blue rect should stay red";

    // Inside both the red circle and the blue rect: multiply gives black.
    uint32_t inside = get_pixel(*surf, 24, 32);
    EXPECT_LT(sk_color_r(inside), 50) << "red channel should be multiplied to near zero";
    EXPECT_LT(sk_color_g(inside), 50) << "green channel should be near zero";
    EXPECT_LT(sk_color_b(inside), 50) << "blue channel should be near zero";
#endif
}
