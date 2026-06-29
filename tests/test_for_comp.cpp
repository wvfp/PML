#include <gtest/gtest.h>
#include "test_helpers.h"

TEST(ForComp, SingleBinding) {
    auto r = pml::test::eval("(for-comp (x in (list 1 2 3)) (* x 2))");
    ASSERT_TRUE(r.has_value()) << (r.has_value() ? "" : r.error().message);
    ASSERT_TRUE(r->is_vector());
    auto* vec = r->as_vector();
    ASSERT_NE(vec, nullptr);
    ASSERT_EQ((*vec)->elements.size(), 3);
    EXPECT_EQ((*vec)->elements[0].int_val(), 2);
    EXPECT_EQ((*vec)->elements[1].int_val(), 4);
    EXPECT_EQ((*vec)->elements[2].int_val(), 6);
}

TEST(ForComp, NestedBindings) {
    auto r = pml::test::eval("(for-comp (x in (list 1 2)) (y in (list 3 4)) (list x y))");
    ASSERT_TRUE(r.has_value()) << (r.has_value() ? "" : r.error().message);
    ASSERT_TRUE(r->is_vector());
    auto* vec = r->as_vector();
    ASSERT_NE(vec, nullptr);
    EXPECT_EQ((*vec)->elements.size(), 4);
}

TEST(ForComp, EmptyList) {
    auto r = pml::test::eval("(for-comp (x in (list)) x)");
    ASSERT_TRUE(r.has_value()) << (r.has_value() ? "" : r.error().message);
    ASSERT_TRUE(r->is_vector());
    auto* vec = r->as_vector();
    ASSERT_NE(vec, nullptr);
    EXPECT_TRUE((*vec)->elements.empty());
}

TEST(ForComp, RangeBinding) {
    auto r = pml::test::eval("(for-comp (x in 0 5) x)");
    ASSERT_TRUE(r.has_value()) << (r.has_value() ? "" : r.error().message);
    ASSERT_TRUE(r->is_vector());
    auto* vec = r->as_vector();
    ASSERT_NE(vec, nullptr);
    ASSERT_EQ((*vec)->elements.size(), 5);
    EXPECT_EQ((*vec)->elements[0].int_val(), 0);
    EXPECT_EQ((*vec)->elements[1].int_val(), 1);
    EXPECT_EQ((*vec)->elements[2].int_val(), 2);
    EXPECT_EQ((*vec)->elements[3].int_val(), 3);
    EXPECT_EQ((*vec)->elements[4].int_val(), 4);
}

TEST(ForComp, RangeBindingMultiply) {
    auto r = pml::test::eval("(for-comp (x in 0 5) (* x 2))");
    ASSERT_TRUE(r.has_value()) << (r.has_value() ? "" : r.error().message);
    ASSERT_TRUE(r->is_vector());
    auto* vec = r->as_vector();
    ASSERT_NE(vec, nullptr);
    ASSERT_EQ((*vec)->elements.size(), 5);
    EXPECT_EQ((*vec)->elements[0].int_val(), 0);
    EXPECT_EQ((*vec)->elements[1].int_val(), 2);
    EXPECT_EQ((*vec)->elements[2].int_val(), 4);
    EXPECT_EQ((*vec)->elements[3].int_val(), 6);
    EXPECT_EQ((*vec)->elements[4].int_val(), 8);
}
