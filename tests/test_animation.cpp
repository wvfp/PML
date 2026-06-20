// tests/test_animation.cpp — Tests for easing functions, timeline, and _apply_modifications
//
// Covers:
//   - Easing function correctness (linear, quad_in, boundary, lookup)
//   - Animation struct construction
//   - Timeline add, duration, evaluate_at, state machine, seek, reset
//   - _apply_modifications on GraphicObject

#include <gtest/gtest.h>
#include "test_helpers.h"
#include "pml/animation/easing.h"
#include "pml/animation/timeline.h"
#include "pml/graphics/params.h"

#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

// ═══════════════════════════════════════════════════════════════════════════════
// Easing function tests
// ═══════════════════════════════════════════════════════════════════════════════

TEST(Easing, LinearAtZero) {
    EXPECT_DOUBLE_EQ(pml::easing_linear(0.0), 0.0);
}

TEST(Easing, LinearAtHalf) {
    EXPECT_DOUBLE_EQ(pml::easing_linear(0.5), 0.5);
}

TEST(Easing, LinearAtOne) {
    EXPECT_DOUBLE_EQ(pml::easing_linear(1.0), 1.0);
}

TEST(Easing, QuadInAtZero) {
    EXPECT_DOUBLE_EQ(pml::easing_quad_in(0.0), 0.0);
}

TEST(Easing, QuadInAtOne) {
    EXPECT_DOUBLE_EQ(pml::easing_quad_in(1.0), 1.0);
}

TEST(Easing, QuadInAtHalf) {
    // quad_in(t) = t*t → 0.5*0.5 = 0.25
    EXPECT_NEAR(pml::easing_quad_in(0.5), 0.25, 1e-9);
}

TEST(Easing, BoundaryValuesAllEasings) {
    // Every easing function should map 0→0 and 1→1
    auto names = pml::list_easings();
    ASSERT_FALSE(names.empty());

    for (const auto& name : names) {
        auto fn = pml::get_easing(name);
        ASSERT_TRUE(fn) << "get_easing(\"" << name << "\") returned null";
        EXPECT_NEAR(fn(0.0), 0.0, 1e-6)
            << "easing \"" << name << "\" at t=0 should be 0";
        EXPECT_NEAR(fn(1.0), 1.0, 1e-6)
            << "easing \"" << name << "\" at t=1 should be 1";
    }
}

TEST(Easing, MonotonicityLinear) {
    // Linear should be monotonically non-decreasing
    double prev = pml::easing_linear(0.0);
    for (int i = 1; i <= 10; ++i) {
        double t = static_cast<double>(i) / 10.0;
        double cur = pml::easing_linear(t);
        EXPECT_GE(cur, prev) << "linear not monotone at t=" << t;
        prev = cur;
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Easing lookup tests
// ═══════════════════════════════════════════════════════════════════════════════

TEST(EasingLookup, GetLinearByName) {
    auto fn = pml::get_easing("linear");
    ASSERT_TRUE(fn);
    EXPECT_DOUBLE_EQ(fn(0.5), 0.5);
}

TEST(EasingLookup, GetQuadInByName) {
    auto fn = pml::get_easing("quad-in");
    ASSERT_TRUE(fn);
    EXPECT_NEAR(fn(0.5), 0.25, 1e-9);
}

TEST(EasingLookup, UnknownFallsBackToLinear) {
    auto fn = pml::get_easing("nonexistent-easing-xyz");
    ASSERT_TRUE(fn);
    // Should behave as linear
    EXPECT_DOUBLE_EQ(fn(0.5), 0.5);
}

TEST(EasingLookup, ListEasingsNonEmpty) {
    auto names = pml::list_easings();
    EXPECT_FALSE(names.empty());
}

TEST(EasingLookup, ListEasingsSorted) {
    auto names = pml::list_easings();
    ASSERT_GE(names.size(), 2u);
    for (size_t i = 1; i < names.size(); ++i) {
        EXPECT_LE(names[i - 1], names[i])
            << "list_easings() not sorted: \"" << names[i - 1]
            << "\" > \"" << names[i] << "\"";
    }
}

TEST(EasingLookup, ListContainsLinear) {
    auto names = pml::list_easings();
    bool found = false;
    for (const auto& n : names) {
        if (n == "linear") { found = true; break; }
    }
    EXPECT_TRUE(found) << "list_easings() should contain \"linear\"";
}

// ═══════════════════════════════════════════════════════════════════════════════
// Animation struct tests
// ═══════════════════════════════════════════════════════════════════════════════

TEST(Animation, Construction) {
    pml::Animation anim(42, "x", pml::Value(0.0), pml::Value(100.0), 2.0f, pml::easing_linear);
    EXPECT_EQ(anim.target_id, 42u);
    EXPECT_EQ(anim.property_name, "x");
    EXPECT_DOUBLE_EQ(anim.from_value.double_val(), 0.0);
    EXPECT_DOUBLE_EQ(anim.to_value.double_val(), 100.0);
    EXPECT_FLOAT_EQ(anim.duration, 2.0f);
    // Default easing should be linear
    EXPECT_DOUBLE_EQ(anim.easing(0.5), 0.5);
}

TEST(Animation, DefaultEasingIsLinear) {
    pml::Animation anim(1, "y", pml::Value(10.0), pml::Value(50.0), 1.0f);
    EXPECT_DOUBLE_EQ(anim.easing(0.25), 0.25);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Timeline tests — each test resets the global timeline first
// ═══════════════════════════════════════════════════════════════════════════════

class TimelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        pml::Timeline::reset();
    }
};

TEST_F(TimelineTest, AddAnimation) {
    auto tl = pml::get_or_create_timeline();
    ASSERT_NE(tl, nullptr);
    EXPECT_TRUE(tl->animations.empty());

    auto anim = std::make_shared<pml::Animation>(1, "x", pml::Value(0.0), pml::Value(100.0), 2.0f);
    tl->add(anim);

    EXPECT_EQ(tl->animations.size(), 1u);
    EXPECT_EQ(tl->animations[0]->target_id, 1u);
    EXPECT_EQ(tl->animations[0]->property_name, "x");
}

TEST_F(TimelineTest, TotalDurationEmpty) {
    auto tl = pml::get_or_create_timeline();
    EXPECT_DOUBLE_EQ(tl->get_total_duration(), 0.0);
}

TEST_F(TimelineTest, TotalDurationSingleAnimation) {
    auto tl = pml::get_or_create_timeline();
    tl->add(std::make_shared<pml::Animation>(1, "x", pml::Value(0.0), pml::Value(100.0), 3.0f));
    EXPECT_DOUBLE_EQ(tl->get_total_duration(), 3.0);
}

TEST_F(TimelineTest, TotalDurationMultipleAnimations) {
    auto tl = pml::get_or_create_timeline();
    tl->add(std::make_shared<pml::Animation>(1, "x", pml::Value(0.0), pml::Value(100.0), 2.0f));
    tl->add(std::make_shared<pml::Animation>(2, "y", pml::Value(0.0), pml::Value(50.0), 5.0f));
    tl->add(std::make_shared<pml::Animation>(3, "r", pml::Value(0.0), pml::Value(360.0), 1.0f));
    // Total duration is the max
    EXPECT_DOUBLE_EQ(tl->get_total_duration(), 5.0);
}

TEST_F(TimelineTest, EvaluateAtMidpoint) {
    auto tl = pml::get_or_create_timeline();
    tl->add(std::make_shared<pml::Animation>(1, "x", pml::Value(0.0), pml::Value(100.0), 2.0f, pml::easing_linear));

    auto results = tl->evaluate_at(1.0);  // midpoint
    ASSERT_EQ(results.size(), 1u);

    auto& [tid, prop, val] = results[0];
    EXPECT_EQ(tid, 1u);
    EXPECT_EQ(prop, "x");
    // At t=1.0 out of 2.0s, linear interpolation → 50.0
    EXPECT_NEAR(val.double_val(), 50.0, 1e-6);
}

TEST_F(TimelineTest, EvaluateAtStart) {
    auto tl = pml::get_or_create_timeline();
    tl->add(std::make_shared<pml::Animation>(1, "x", pml::Value(10.0), pml::Value(110.0), 2.0f, pml::easing_linear));

    auto results = tl->evaluate_at(0.0);
    ASSERT_EQ(results.size(), 1u);

    auto& [tid, prop, val] = results[0];
    EXPECT_NEAR(val.double_val(), 10.0, 1e-6);  // from_value
}

TEST_F(TimelineTest, EvaluateAtEnd) {
    auto tl = pml::get_or_create_timeline();
    tl->add(std::make_shared<pml::Animation>(1, "x", pml::Value(10.0), pml::Value(110.0), 2.0f, pml::easing_linear));

    auto results = tl->evaluate_at(2.0);
    ASSERT_EQ(results.size(), 1u);

    auto& [tid, prop, val] = results[0];
    EXPECT_NEAR(val.double_val(), 110.0, 1e-6);  // to_value
}

TEST_F(TimelineTest, EvaluateAtMultipleAnimations) {
    auto tl = pml::get_or_create_timeline();
    tl->add(std::make_shared<pml::Animation>(1, "x", pml::Value(0.0), pml::Value(100.0), 2.0f, pml::easing_linear));
    tl->add(std::make_shared<pml::Animation>(2, "y", pml::Value(0.0), pml::Value(200.0), 4.0f, pml::easing_linear));

    // At t=1.0: first is at midpoint (50), second at quarter (50)
    auto results = tl->evaluate_at(1.0);
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(TimelineTest, StateMachineIdleToPlaying) {
    auto tl = pml::get_or_create_timeline();
    EXPECT_EQ(tl->state, pml::TimelineState::Idle);

    tl->play();
    EXPECT_EQ(tl->state, pml::TimelineState::Playing);
}

TEST_F(TimelineTest, StateMachinePlayingToPaused) {
    auto tl = pml::get_or_create_timeline();
    tl->play();
    EXPECT_EQ(tl->state, pml::TimelineState::Playing);

    tl->pause();
    EXPECT_EQ(tl->state, pml::TimelineState::Paused);
}

TEST_F(TimelineTest, StateMachinePausedToPlaying) {
    auto tl = pml::get_or_create_timeline();
    tl->play();
    tl->pause();
    EXPECT_EQ(tl->state, pml::TimelineState::Paused);

    tl->play();
    EXPECT_EQ(tl->state, pml::TimelineState::Playing);
}

TEST_F(TimelineTest, StateMachineStopResets) {
    auto tl = pml::get_or_create_timeline();
    tl->add(std::make_shared<pml::Animation>(1, "x", pml::Value(0.0), pml::Value(100.0), 2.0f));
    tl->play();
    tl->seek(1.0);
    EXPECT_EQ(tl->state, pml::TimelineState::Playing);

    tl->stop();
    // stop() sets state to Stopped and resets time to 0
    EXPECT_EQ(tl->state, pml::TimelineState::Stopped);
    EXPECT_DOUBLE_EQ(tl->current_time, 0.0);
}

TEST_F(TimelineTest, SeekForward) {
    auto tl = pml::get_or_create_timeline();
    tl->seek(1.5);
    EXPECT_DOUBLE_EQ(tl->current_time, 1.5);
}

TEST_F(TimelineTest, SeekClampsNegative) {
    auto tl = pml::get_or_create_timeline();
    tl->seek(-1.0);
    EXPECT_GE(tl->current_time, 0.0);
}

TEST_F(TimelineTest, ResetClearsState) {
    auto tl = pml::get_or_create_timeline();
    tl->add(std::make_shared<pml::Animation>(1, "x", pml::Value(0.0), pml::Value(100.0), 2.0f));
    tl->play();
    tl->seek(1.0);

    pml::Timeline::reset();

    // After reset, getting the timeline should give a fresh one
    auto tl2 = pml::get_or_create_timeline();
    EXPECT_EQ(tl2->state, pml::TimelineState::Idle);
    EXPECT_TRUE(tl2->animations.empty());
    EXPECT_DOUBLE_EQ(tl2->current_time, 0.0);
}

TEST_F(TimelineTest, StateToString) {
    EXPECT_EQ(pml::timeline_state_to_string(pml::TimelineState::Idle), "idle");
    EXPECT_EQ(pml::timeline_state_to_string(pml::TimelineState::Playing), "playing");
    EXPECT_EQ(pml::timeline_state_to_string(pml::TimelineState::Paused), "paused");
    EXPECT_EQ(pml::timeline_state_to_string(pml::TimelineState::Stopped), "stopped");
    EXPECT_EQ(pml::timeline_state_to_string(pml::TimelineState::Finished), "finished");
}

// ═══════════════════════════════════════════════════════════════════════════════
// _apply_modifications tests
// ═══════════════════════════════════════════════════════════════════════════════

TEST(ApplyModifications, ChangeXParam) {
    pml::GraphicObject obj("circle", pml::Params{{pml::ParamKey::x, pml::Value(int64_t(10))}});

    std::unordered_map<std::string, pml::Value> mods = {{"x", pml::Value(50.0)}};
    auto result = pml::_apply_modifications(obj, mods);

    // The result should have x=50 in its params
    const pml::Value* v = result.params.find(pml::ParamKey::x);
    ASSERT_NE(v, nullptr);
    // Value could be stored as int64_t or double depending on implementation
    if (v->is_int()) {
        EXPECT_EQ(v->int_val(), 50);
    } else {
        EXPECT_DOUBLE_EQ(v->double_val(), 50.0);
    }
}

TEST(ApplyModifications, ChangeTransformTx) {
    pml::GraphicObject obj("rect", {});

    std::unordered_map<std::string, pml::Value> mods = {{"transform.tx", pml::Value(30.0)}};
    auto result = pml::_apply_modifications(obj, mods);

    // transform.tx maps to AffineTransform.e
    EXPECT_DOUBLE_EQ(result.transform.e, 30.0);
}

TEST(ApplyModifications, ChangeTransformTy) {
    pml::GraphicObject obj("rect", {});

    std::unordered_map<std::string, pml::Value> mods = {{"transform.ty", pml::Value(40.0)}};
    auto result = pml::_apply_modifications(obj, mods);

    EXPECT_DOUBLE_EQ(result.transform.f, 40.0);
}

TEST(ApplyModifications, OriginalUnchanged) {
    pml::GraphicObject obj("circle", pml::Params{{pml::ParamKey::x, pml::Value(int64_t(10))}});
    double orig_e = obj.transform.e;

    std::unordered_map<std::string, pml::Value> mods = {{"x", pml::Value(99.0)}, {"transform.tx", pml::Value(5.0)}};
    auto result = pml::_apply_modifications(obj, mods);

    // Original should be unchanged (immutable pattern)
    const pml::Value* v = obj.params.find(pml::ParamKey::x);
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->int_val(), 10);
    EXPECT_DOUBLE_EQ(obj.transform.e, orig_e);
}

TEST(ApplyModifications, EmptyMods) {
    pml::GraphicObject obj("circle", pml::Params{{pml::ParamKey::x, pml::Value(int64_t(10))}});

    std::unordered_map<std::string, pml::Value> mods;
    auto result = pml::_apply_modifications(obj, mods);

    // With empty mods, result should match original
    EXPECT_EQ(result.shape_type, "circle");
    const pml::Value* v = result.params.find(pml::ParamKey::x);
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->int_val(), 10);
}

TEST(ApplyModifications, ChangeFill) {
    pml::GraphicObject obj("circle", {}, "#ff0000");

    // String fill values update the fill field directly.
    std::unordered_map<std::string, pml::Value> mods = {{"fill", pml::Value(std::string("#00ff00"))}};
    auto result = pml::_apply_modifications(obj, mods);

    ASSERT_TRUE(result.fill.has_value());
    EXPECT_EQ(*result.fill, "#00ff00");
}

TEST(ApplyModifications, ChangeStroke) {
    pml::GraphicObject obj("circle", {}, std::nullopt, "#000000");

    std::unordered_map<std::string, pml::Value> mods = {{"stroke", pml::Value(std::string("#ffffff"))}};
    auto result = pml::_apply_modifications(obj, mods);

    ASSERT_TRUE(result.stroke.has_value());
    EXPECT_EQ(*result.stroke, "#ffffff");
}
