#include <gtest/gtest.h>
#include "pml/api/api.h"
#include "pml/evaluator/environment.h"
#include "pml/module/embedded_stdlib.h"

// ---- Threading --------------------------------------------------------------------------------------------------------------------

TEST(StdlibLoad, ThreadFirst) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(-> 10 (+ 2) (* 3))");
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.value.get<int64_t>(), 36);
}

TEST(StdlibLoad, ThreadLast) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(->> (list 1 2 3) (map (lambda (x) (* x 2))))");
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.value.is_array());
    EXPECT_EQ(r.value.size(), 3);
}

// ---- enumerate / zip ----------------------------------------------------------------------------------------------------─

TEST(StdlibLoad, EnumerateList) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(enumerate '(a b c))");
    EXPECT_TRUE(r.success);
    ASSERT_TRUE(r.value.is_array());
    ASSERT_EQ(r.value.size(), 3);
    EXPECT_EQ(r.value[0][0].get<int64_t>(), 0);
    EXPECT_EQ(r.value[1][0].get<int64_t>(), 1);
    EXPECT_EQ(r.value[2][0].get<int64_t>(), 2);
}

TEST(StdlibLoad, ZipTwoLists) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(zip '(1 2 3) '(a b c))");
    EXPECT_TRUE(r.success);
    ASSERT_TRUE(r.value.is_array());
    EXPECT_EQ(r.value.size(), 3);
}

TEST(StdlibLoad, ZipShortestTruncates) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(zip '(1 2) '(a b c))");
    EXPECT_TRUE(r.success);
    ASSERT_TRUE(r.value.is_array());
    EXPECT_EQ(r.value.size(), 2);
}

// ---- range ------------------------------------------------------------------------------------------------------------------------─

TEST(StdlibLoad, RangeOneArg) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(range 5)");
    EXPECT_TRUE(r.success);
    ASSERT_TRUE(r.value.is_array());
    EXPECT_EQ(r.value.size(), 5);
    EXPECT_EQ(r.value[0].get<int64_t>(), 0);
    EXPECT_EQ(r.value[4].get<int64_t>(), 4);
}

TEST(StdlibLoad, RangeTwoArgs) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(range 5 10)");
    EXPECT_TRUE(r.success);
    ASSERT_TRUE(r.value.is_array());
    EXPECT_EQ(r.value.size(), 5);
    EXPECT_EQ(r.value[0].get<int64_t>(), 5);
}

TEST(StdlibLoad, RangeThreeArgs) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(range 0 10 3)");
    EXPECT_TRUE(r.success);
    ASSERT_TRUE(r.value.is_array());
    EXPECT_EQ(r.value.size(), 4);
    EXPECT_EQ(r.value[0].get<int64_t>(), 0);
    EXPECT_EQ(r.value[3].get<int64_t>(), 9);
}

// ---- get (polymorphic) ----------------------------------------------------------------------------------------------------

TEST(StdlibLoad, GetList) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(get '(10 20 30) 0)");
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.value.get<int64_t>(), 10);
}

TEST(StdlibLoad, GetVector) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(get (list->vector '(100 200 300)) 1)");
    EXPECT_TRUE(r.success) << "error: " << (r.error.has_value() ? r.error->dump() : "none");
    EXPECT_EQ(r.value.get<int64_t>(), 200);
}

TEST(StdlibLoad, GetHash) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(get (make-hash '((\"x\" 42))) \"x\")");
    EXPECT_TRUE(r.success) << "error: " << (r.error.has_value() ? r.error->dump() : "none");
    EXPECT_EQ(r.value.get<int64_t>(), 42);
}

// ---- set-at! (polymorphic) --------------------------------------------------------------------------------------------

TEST(StdlibLoad, SetAtList) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(set-at! '(1 2 3) 0 99)");
    EXPECT_TRUE(r.success);
    ASSERT_TRUE(r.value.is_array());
    EXPECT_EQ(r.value.size(), 3);
    EXPECT_EQ(r.value[0].get<int64_t>(), 99);
}

TEST(StdlibLoad, SetAtVector) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(define v (list->vector '(1 2 3))) (set-at! v 1 999) (get v 1)");
    EXPECT_TRUE(r.success) << "error: " << (r.error.has_value() ? r.error->dump() : "none");
    EXPECT_EQ(r.value.get<int64_t>(), 999);
}

// ---- defn + defconst (from stdlib) ------------------------------------------------------------------------─

TEST(StdlibLoad, Defn) {
    pml::PMLRuntime rt;
    pml::load_embedded_stdlib(rt.env());
    auto r = rt.execute("(defn double (x) (* x 2)) (double 21)");
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.value.get<int64_t>(), 42);
}

TEST(StdlibLoad, Defconst) {
    pml::PMLRuntime rt;
    pml::load_embedded_stdlib(rt.env());
    auto r = rt.execute("(defconst PI 3.14) (* PI 2)");
    EXPECT_TRUE(r.success);
    EXPECT_NEAR(r.value.get<double>(), 6.28, 0.001);
}

// ---- let is now sequential --------------------------------------------------------------------------------------------

TEST(StdlibLoad, LetIsNowSequential) {
    pml::PMLRuntime rt;
    // let should now allow forward references (like let*)
    auto r = rt.execute("(let ((x 1) (y (+ x 1))) y)");
    EXPECT_TRUE(r.success) << "error: " << (r.error.has_value() ? r.error->dump() : "none");
    EXPECT_TRUE(r.value.is_number());
}

// ---- let-par is parallel ------------------------------------------------------------------------------------------------

TEST(StdlibLoad, LetParIsParallel) {
    pml::PMLRuntime rt;
    // let-par should NOT allow forward references
    auto r = rt.execute("(let-par ((x 1) (y x)) y)");
    EXPECT_FALSE(r.success) << "let-par should fail when referencing earlier bindings (parallel)";
}

TEST(StdlibLoad, LetParWorks) {
    pml::PMLRuntime rt;
    // let-par should work for independent bindings
    auto r = rt.execute("(let-par ((x 1) (y 2)) (+ x y))");
    EXPECT_TRUE(r.success) << "error: " << (r.error.has_value() ? r.error->dump() : "none");
    EXPECT_EQ(r.value.get<int64_t>(), 3);
}

// ---- let* still sequential --------------------------------------------------------------------------------------------

TEST(StdlibLoad, LetStarStillSequential) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(let* ((x 1) (y (+ x 1))) y)");
    EXPECT_TRUE(r.success) << "error: " << (r.error.has_value() ? r.error->dump() : "none");
    EXPECT_EQ(r.value.get<int64_t>(), 2);
}

// ---- Keyword aliases --------------------------------------------------------------------------------------------------------─

TEST(StdlibLoad, KeywordAliasFill) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(rect 0 0 10 10 :f \"red\")");
    EXPECT_TRUE(r.success) << "error: " << (r.error.has_value() ? r.error->dump() : "none");
}

TEST(StdlibLoad, KeywordAliasStroke) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(rect 0 0 10 10 :s \"blue\")");
    EXPECT_TRUE(r.success);
}

TEST(StdlibLoad, KeywordAliasStrokeWidth) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(rect 0 0 10 10 :sw 3)");
    EXPECT_TRUE(r.success);
}

TEST(StdlibLoad, KeywordAliasOpacity) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(rect 0 0 10 10 :op 0.5)");
    EXPECT_TRUE(r.success);
}

// ---- Implicit begin in let/let*/letrec/begin bodies ------------------------------------

TEST(StdlibLoad, LetMultiBody) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(let ((x 1)) (define y (+ x 1)) (+ x y))");
    EXPECT_TRUE(r.success) << "error: " << (r.error.has_value() ? r.error->dump() : "none");
    EXPECT_EQ(r.value.get<int64_t>(), 3);
}

TEST(StdlibLoad, LetStarMultiBody) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(let* ((x 1) (y (+ x 1))) (define z 3) (+ x y z))");
    EXPECT_TRUE(r.success) << "error: " << (r.error.has_value() ? r.error->dump() : "none");
    EXPECT_EQ(r.value.get<int64_t>(), 6);
}

TEST(StdlibLoad, BeginMultiExpr) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(begin (define x 1) (define y 2) (+ x y))");
    EXPECT_TRUE(r.success) << "error: " << (r.error.has_value() ? r.error->dump() : "none");
    EXPECT_EQ(r.value.get<int64_t>(), 3);
}

TEST(StdlibLoad, LetrecMultiBody) {
    pml::PMLRuntime rt;
    auto r = rt.execute("(letrec ((x 1) (y 2)) (define z 3) (+ x y z))");
    EXPECT_TRUE(r.success) << "error: " << (r.error.has_value() ? r.error->dump() : "none");
    EXPECT_EQ(r.value.get<int64_t>(), 6);
}
