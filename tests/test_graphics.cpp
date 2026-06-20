#include <gtest/gtest.h>
#include "test_helpers.h"
#include "pml/graphics/transform.h"
#include "pml/graphics/objects.h"
#include "pml/graphics/canvas.h"
#include "pml/sprites/style.h"
#include "pml/sprites/palette.h"

#include <cmath>
#include <memory>

// AffineTransform is defined in the global namespace (not pml::).
// types.h forward-declares pml::AffineTransform, so using namespace pml
// would cause ambiguity. We use pml::AffineTransform explicitly and qualify
// pml:: types where needed.

// ============================================================================
// AffineTransform — Static Factories
// ============================================================================

TEST(AffineTransformTest, Identity) {
    auto t = pml::AffineTransform::identity();
    EXPECT_TRUE(t.is_identity());
    EXPECT_DOUBLE_EQ(t.a, 1.0);
    EXPECT_DOUBLE_EQ(t.b, 0.0);
    EXPECT_DOUBLE_EQ(t.c, 0.0);
    EXPECT_DOUBLE_EQ(t.d, 1.0);
    EXPECT_DOUBLE_EQ(t.e, 0.0);
    EXPECT_DOUBLE_EQ(t.f, 0.0);
}

TEST(AffineTransformTest, Translate) {
    auto t = pml::AffineTransform::translate(10.0, 20.0);
    EXPECT_DOUBLE_EQ(t.a, 1.0);
    EXPECT_DOUBLE_EQ(t.d, 1.0);
    EXPECT_DOUBLE_EQ(t.e, 10.0);
    EXPECT_DOUBLE_EQ(t.f, 20.0);
}

TEST(AffineTransformTest, Scale) {
    auto t = pml::AffineTransform::scale(2.0, 3.0);
    EXPECT_DOUBLE_EQ(t.a, 2.0);
    EXPECT_DOUBLE_EQ(t.b, 0.0);
    EXPECT_DOUBLE_EQ(t.c, 0.0);
    EXPECT_DOUBLE_EQ(t.d, 3.0);
    EXPECT_DOUBLE_EQ(t.e, 0.0);
    EXPECT_DOUBLE_EQ(t.f, 0.0);
}

TEST(AffineTransformTest, Rotate) {
    auto t = pml::AffineTransform::rotate(90.0);
    // cos(90deg) ~ 0, sin(90deg) ~ 1
    EXPECT_NEAR(t.a, 0.0, 1e-9);
    EXPECT_NEAR(t.b, 1.0, 1e-9);
    EXPECT_NEAR(t.c, -1.0, 1e-9);
    EXPECT_NEAR(t.d, 0.0, 1e-9);
}

TEST(AffineTransformTest, Shear) {
    auto t = pml::AffineTransform::shear(0.5, 0.25);
    EXPECT_DOUBLE_EQ(t.a, 1.0);
    EXPECT_DOUBLE_EQ(t.b, 0.25);
    EXPECT_DOUBLE_EQ(t.c, 0.5);
    EXPECT_DOUBLE_EQ(t.d, 1.0);
}

// ============================================================================
// AffineTransform — Operations
// ============================================================================

TEST(AffineTransformTest, ComposeTranslateThenRotate) {
    auto translate = pml::AffineTransform::translate(5.0, 0.0);
    auto rotate = pml::AffineTransform::rotate(90.0);
    auto composed = rotate.compose(translate);

    // After composing: first translate (5,0) then rotate 90 degrees
    // Point (0,0) -> translate -> (5,0) -> rotate 90 -> (0,5)
    auto [x, y] = composed.apply(0.0, 0.0);
    EXPECT_NEAR(x, 0.0, 1e-9);
    EXPECT_NEAR(y, 5.0, 1e-9);
}

TEST(AffineTransformTest, Inverse) {
    auto t = pml::AffineTransform::translate(10.0, 20.0);
    auto inv = t.inverse();
    ASSERT_TRUE(inv.has_value());

    // Composing with inverse should yield identity
    auto result = t.compose(*inv);
    EXPECT_TRUE(result.is_identity());
}

TEST(AffineTransformTest, InverseScale) {
    auto t = pml::AffineTransform::scale(2.0, 4.0);
    auto inv = t.inverse();
    ASSERT_TRUE(inv.has_value());

    auto result = t.compose(*inv);
    EXPECT_NEAR(result.a, 1.0, 1e-9);
    EXPECT_NEAR(result.d, 1.0, 1e-9);
}

TEST(AffineTransformTest, ApplyPoint) {
    auto t = pml::AffineTransform::translate(3.0, 7.0);
    auto [x, y] = t.apply(1.0, 2.0);
    EXPECT_DOUBLE_EQ(x, 4.0);
    EXPECT_DOUBLE_EQ(y, 9.0);
}

TEST(AffineTransformTest, ApplyScalePoint) {
    auto t = pml::AffineTransform::scale(3.0, 2.0);
    auto [x, y] = t.apply(4.0, 5.0);
    EXPECT_DOUBLE_EQ(x, 12.0);
    EXPECT_DOUBLE_EQ(y, 10.0);
}

TEST(AffineTransformTest, SingularMatrixNoInverse) {
    // A matrix with all zeros has determinant 0 — no inverse.
    pml::AffineTransform singular{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    auto inv = singular.inverse();
    EXPECT_FALSE(inv.has_value());
}

TEST(AffineTransformTest, OperatorStarCompose) {
    auto t1 = pml::AffineTransform::translate(1.0, 2.0);
    auto t2 = pml::AffineTransform::scale(3.0, 3.0);
    auto composed = t1 * t2;
    auto manual = t1.compose(t2);
    EXPECT_DOUBLE_EQ(composed.a, manual.a);
    EXPECT_DOUBLE_EQ(composed.d, manual.d);
    EXPECT_DOUBLE_EQ(composed.e, manual.e);
    EXPECT_DOUBLE_EQ(composed.f, manual.f);
}

// ============================================================================
// GraphicObject
// ============================================================================

TEST(GraphicObjectTest, Creation) {
    pml::GraphicObject obj("circle", pml::Params{{pml::ParamKey::r, pml::Value(int64_t{50})}}, "#ff0000");
    EXPECT_EQ(obj.shape_type, "circle");
    EXPECT_EQ(obj.fill.value_or(""), "#ff0000");
    EXPECT_TRUE(obj.params.contains(pml::ParamKey::r));
    const pml::Value* v = obj.params.find(pml::ParamKey::r);
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->int_val(), 50);
}

TEST(GraphicObjectTest, UniqueId) {
    pml::GraphicObject obj1("rect");
    pml::GraphicObject obj2("rect");
    EXPECT_NE(obj1.id, obj2.id);
}

TEST(GraphicObjectTest, WithTransformReturnsNewObject) {
    pml::GraphicObject obj("circle", pml::Params{{pml::ParamKey::r, pml::Value(int64_t{10})}}, "#00ff00");
    auto t = pml::AffineTransform::translate(100.0, 200.0);

    auto transformed = obj.with_transform(t);

    // Original should be unchanged
    EXPECT_TRUE(obj.transform.is_identity());
    // Transformed should have the new transform
    EXPECT_DOUBLE_EQ(transformed.transform.e, 100.0);
    EXPECT_DOUBLE_EQ(transformed.transform.f, 200.0);
    // with_transform preserves identity (same logical object, different transform)
    EXPECT_EQ(transformed.id, obj.id);
    // But the shape type and fill are preserved
    EXPECT_EQ(transformed.shape_type, "circle");
    EXPECT_EQ(transformed.fill.value_or(""), "#00ff00");
}

TEST(GraphicObjectTest, WithFillReturnsNewObject) {
    pml::GraphicObject obj("rect", {}, "#ff0000");
    auto modified = obj.with_fill("#0000ff");
    EXPECT_EQ(obj.fill.value_or(""), "#ff0000");
    EXPECT_EQ(modified.fill.value_or(""), "#0000ff");
}

TEST(GraphicObjectTest, WithStrokeReturnsNewObject) {
    pml::GraphicObject obj("rect", {}, "#ff0000", "#000000");
    auto modified = obj.with_stroke("#ffffff");
    EXPECT_EQ(obj.stroke.value_or(""), "#000000");
    EXPECT_EQ(modified.stroke.value_or(""), "#ffffff");
}

TEST(GraphicObjectTest, ChildrenComposition) {
    pml::GraphicObject child1("circle", {{"radius", int64_t{5}}});
    pml::GraphicObject child2("circle", {{"radius", int64_t{10}}});
    pml::GraphicObject group("group", {}, std::nullopt, std::nullopt, 1.0,
                              pml::AffineTransform::identity(),
                              {child1, child2});
    EXPECT_EQ(group.children.size(), 2);
    EXPECT_EQ(group.children[0].shape_type, "circle");
    EXPECT_EQ(group.children[1].shape_type, "circle");
}

// ============================================================================
// Canvas
// ============================================================================

TEST(CanvasTest, Creation) {
    pml::Canvas canvas(800, 600, "#FFFFFF");
    EXPECT_EQ(canvas.width, 800);
    EXPECT_EQ(canvas.height, 600);
    EXPECT_EQ(canvas.bg_color, "#FFFFFF");
    EXPECT_TRUE(canvas.objects.empty());
    EXPECT_FALSE(canvas.is_sprite);
}

TEST(CanvasTest, SpriteCanvas) {
    pml::Canvas canvas(256, 256, "transparent", "center-bottom", 10, true);
    EXPECT_EQ(canvas.width, 256);
    EXPECT_EQ(canvas.height, 256);
    EXPECT_TRUE(canvas.is_sprite);
    EXPECT_EQ(canvas.anchor, "center-bottom");
    EXPECT_EQ(canvas.padding, 10);
}

TEST(CanvasTest, AddObject) {
    pml::Canvas canvas(100, 100);
    pml::GraphicObject obj("circle", pml::Params{{pml::ParamKey::r, pml::Value(int64_t{20})}}, "#ff0000");
    canvas.add(obj);
    EXPECT_EQ(canvas.objects.size(), 1);
    EXPECT_EQ(canvas.objects[0].shape_type, "circle");
}

TEST(CanvasTest, AddMultipleObjects) {
    pml::Canvas canvas(100, 100);
    canvas.add(pml::GraphicObject("circle"));
    canvas.add(pml::GraphicObject("rect"));
    canvas.add(pml::GraphicObject("line"));
    EXPECT_EQ(canvas.objects.size(), 3);
    // Objects are added in z-order (last on top)
    EXPECT_EQ(canvas.objects[0].shape_type, "circle");
    EXPECT_EQ(canvas.objects[1].shape_type, "rect");
    EXPECT_EQ(canvas.objects[2].shape_type, "line");
}

// ============================================================================
// Style Registry
// ============================================================================

TEST(StyleTest, PredefinedCelStyle) {
    auto& registry = pml::StyleRegistry::instance();
    EXPECT_TRUE(registry.has("cel"));
    auto cel = registry.get("cel");
    EXPECT_GT(cel.outline_width, 0.0f);
}

TEST(StyleTest, PredefinedPixelStyle) {
    auto& registry = pml::StyleRegistry::instance();
    EXPECT_TRUE(registry.has("pixel"));
    auto pixel = registry.get("pixel");
    EXPECT_GT(pixel.pixel_size, 1);
    EXPECT_FALSE(pixel.anti_alias);
}

TEST(StyleTest, PredefinedFlatStyle) {
    auto& registry = pml::StyleRegistry::instance();
    EXPECT_TRUE(registry.has("flat"));
    auto flat = registry.get("flat");
    EXPECT_NEAR(flat.outline_width, 0.0f, 1e-6);
}

TEST(StyleTest, DefineCustomStyle) {
    auto& registry = pml::StyleRegistry::instance();
    pml::StyleDescriptor custom;
    custom.outline_width = 5.0f;
    custom.shading = "cel";
    registry.define("test-custom", custom);
    EXPECT_TRUE(registry.has("test-custom"));
    auto retrieved = registry.get("test-custom");
    EXPECT_NEAR(retrieved.outline_width, 5.0f, 1e-6);
}

TEST(StyleTest, UnknownStyleFallsBackToCel) {
    auto& registry = pml::StyleRegistry::instance();
    auto style = registry.get("nonexistent-style");
    auto cel = registry.get("cel");
    EXPECT_NEAR(style.outline_width, cel.outline_width, 1e-6);
    EXPECT_EQ(style.shading, cel.shading);
}

TEST(StyleTest, MergeOverrides) {
    pml::StyleDescriptor base;
    base.outline_width = 2.0f;
    base.shading = "flat";

    pml::StyleDescriptor overrides;
    overrides.outline_width = 5.0f;

    auto merged = base.merge(overrides);
    EXPECT_NEAR(merged.outline_width, 5.0f, 1e-6);
    EXPECT_EQ(merged.shading, "flat");
}

// ============================================================================
// Palette
// ============================================================================

TEST(PaletteTest, CreateAndLookup) {
    pml::Palette pal("test-palette", {
        {"skin", "#FFCC99"},
        {"hair", "#332211"},
        {"eyes", "#4488FF"},
    });
    EXPECT_EQ(pal.get("skin"), "#FFCC99");
    EXPECT_EQ(pal.get("hair"), "#332211");
    EXPECT_EQ(pal.get("eyes"), "#4488FF");
}

TEST(PaletteTest, UnknownKeyReturnsFallback) {
    pml::Palette pal("test-palette", {{"skin", "#FFCC99"}});
    // Unknown keys should return "#808080"
    EXPECT_EQ(pal.get("nonexistent"), "#808080");
}

TEST(PaletteTest, RegisterAndRetrieve) {
    auto& mgr = pml::PaletteManager::instance();
    auto pal = std::make_shared<pml::Palette>(
        "cpp-test-palette",
        std::unordered_map<std::string, std::string>{
            {"bg", "#000000"},
            {"fg", "#FFFFFF"},
        });
    mgr.define("cpp-test-palette", pal);

    auto retrieved = mgr.get("cpp-test-palette");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->get("bg"), "#000000");
    EXPECT_EQ(retrieved->get("fg"), "#FFFFFF");
}

TEST(PaletteTest, SetActivePalette) {
    auto& mgr = pml::PaletteManager::instance();
    auto pal = std::make_shared<pml::Palette>(
        "active-test",
        std::unordered_map<std::string, std::string>{{"main", "#AABBCC"}});
    mgr.define("active-test", pal);
    mgr.set_active(pal);

    auto active = mgr.active();
    ASSERT_NE(active, nullptr);
    EXPECT_EQ(active->get("main"), "#AABBCC");
}
