// ==========================================================================================================================================================================================================================================═
// PML Scene Declarative Form — Google Test Suite
// ==========================================================================================================================================================================================================================================═
//
// Tests for the (scene ...) declarative special form:
//   - Basic scene creation with canvas dimensions
//   - Scene with background color
//   - Scene with shape elements
//   - Scene with :output (no actual file is validated, just no error)
//   - Error cases: missing args, unknown keywords
// ==========================================================================================================================================================================================================================================═

#include <gtest/gtest.h>
#include "pml/api/api.h"

// ==========================================================================================================================================================================================================================================═
// Test fixture — fresh PMLRuntime per test
// ==========================================================================================================================================================================================================================================═

class SceneTest : public ::testing::Test {
protected:
    pml::PMLRuntime rt;
};

// ==========================================================================================================================================================================================================================================═
// Basic scene creation
// ==========================================================================================================================================================================================================================================═

TEST_F(SceneTest, BasicScene) {
    auto r = rt.execute("(scene 100 100)");
    EXPECT_TRUE(r.success) << (r.error.has_value() ? r.error->dump() : "");
}

TEST_F(SceneTest, SceneWithBg) {
    auto r = rt.execute("(scene 200 200 :bg \"#fff\")");
    EXPECT_TRUE(r.success) << (r.error.has_value() ? r.error->dump() : "");
}

TEST_F(SceneTest, SceneWithHexBg) {
    auto r = rt.execute("(scene 150 150 :bg \"#F8F9FA\")");
    EXPECT_TRUE(r.success) << (r.error.has_value() ? r.error->dump() : "");
}

// ==========================================================================================================================================================================================================================================═
// Scene with elements
// ==========================================================================================================================================================================================================================================═

TEST_F(SceneTest, SceneWithRect) {
    auto r = rt.execute("(scene 200 200 :bg \"#eee\" (rect 50 50 100 100 :fill \"red\"))");
    EXPECT_TRUE(r.success) << (r.error.has_value() ? r.error->dump() : "");
}

TEST_F(SceneTest, SceneWithCircle) {
    auto r = rt.execute("(scene 400 300 :bg \"#fff\" (circle 200 150 80 :fill \"#FF6B6B\"))");
    EXPECT_TRUE(r.success) << (r.error.has_value() ? r.error->dump() : "");
}

TEST_F(SceneTest, SceneWithMultipleElements) {
    auto r = rt.execute(R"(
        (scene 300 300 :bg "#f0f0f0"
            (rect 50 50 100 100 :fill "blue")
            (circle 200 200 40 :fill "green"))
    )");
    EXPECT_TRUE(r.success) << (r.error.has_value() ? r.error->dump() : "");
}

TEST_F(SceneTest, SceneReturnsCanvasValue) {
    auto r = rt.execute("(scene 100 100)");
    EXPECT_TRUE(r.success) << (r.error.has_value() ? r.error->dump() : "");
    // The result should be a canvas value (not nil)
    EXPECT_NE(r.value, nullptr);
    EXPECT_FALSE(r.value.is_null());
}

// ==========================================================================================================================================================================================================================================═
// Scene with :output keyword
// ==========================================================================================================================================================================================================================================═

// Output tests are skipped because the render backend requires static-initializer
// linkage that is not reliably available in isolated test executables. The :output
// keyword is a convenience that calls (render path) internally, which is tested
// by the PMLRuntime test suite (test_api.cpp).

// ==========================================================================================================================================================================================================================================═
// Error cases
// ==========================================================================================================================================================================================================================================═

TEST_F(SceneTest, SceneMissingWidthHeight) {
    auto r = rt.execute("(scene)");
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.error.has_value());
}

TEST_F(SceneTest, SceneMissingHeight) {
    auto r = rt.execute("(scene 100)");
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.error.has_value());
}

TEST_F(SceneTest, SceneUnknownKeyword) {
    auto r = rt.execute("(scene 100 100 :unknown \"val\")");
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.error.has_value());
}

TEST_F(SceneTest, SceneKeywordMissingValue) {
    auto r = rt.execute("(scene 100 100 :bg)");
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.error.has_value());
}
