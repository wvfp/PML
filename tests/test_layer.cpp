#include <gtest/gtest.h>
#include "pml/layer/layer.h"
#include "pml/layer/composition.h"

using namespace pml;

static std::shared_ptr<GraphicObject> dummy_shape() {
    return std::make_shared<GraphicObject>(pml::ShapeType::Circle);
}

TEST(LayerTest, LeafHasNoChildren) {
    auto go = dummy_shape();
    Layer layer(LayerProperties{"leaf"}, go);
    EXPECT_TRUE(layer.is_leaf());
    EXPECT_FALSE(layer.is_group());
    EXPECT_EQ(layer.properties().name, "leaf");
}

TEST(LayerTest, GroupContainsChildren) {
    auto child = std::make_shared<Layer>(LayerProperties{"child"}, dummy_shape());
    Layer group(LayerProperties{"group"}, std::vector{child});
    EXPECT_TRUE(group.is_group());
    EXPECT_EQ(group.children().size(), 1u);
}

TEST(LayerTest, ImmutableOverride) {
    Layer a(LayerProperties{"a"}, dummy_shape());
    Layer b = a.with_opacity(0.5f);
    EXPECT_FLOAT_EQ(a.properties().opacity, 1.0f);
    EXPECT_FLOAT_EQ(b.properties().opacity, 0.5f);
}

TEST(LayerTest, AnchorKeywordRoundTrip) {
    EXPECT_EQ(std::string(anchor_to_keyword(*anchor_from_keyword("center"))), "center");
    EXPECT_EQ(std::string(anchor_to_keyword(*anchor_from_keyword("bottom-right"))), "bottom-right");
}

TEST(BlendModeTest, KeywordRoundTrip) {
    EXPECT_EQ(std::string(blend_mode_to_keyword(*blend_mode_from_keyword("multiply"))), "multiply");
    EXPECT_EQ(std::string(blend_mode_to_keyword(*blend_mode_from_keyword("overlay"))), "overlay");
}

TEST(CompositionTest, AppendLayer) {
    Composition c("test", Size2D{64, 64});
    EXPECT_EQ(c.layers().size(), 0u);
    auto c2 = c.with_layer_appended(std::make_shared<Layer>(LayerProperties{"l"}, dummy_shape()));
    EXPECT_EQ(c2.layers().size(), 1u);
}
