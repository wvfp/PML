#include <gtest/gtest.h>
#include "test_helpers.h"

// All pml:: types and pml::test:: helpers are fully qualified below.

// ============================================================================
// Arithmetic Operations
// ============================================================================

TEST(EvaluatorArithmetic, Addition) {
    EXPECT_EQ(pml::test::eval_int("(+ 1 2)"), 3);
}

TEST(EvaluatorArithmetic, Subtraction) {
    EXPECT_EQ(pml::test::eval_int("(- 10 3)"), 7);
}

TEST(EvaluatorArithmetic, Multiplication) {
    EXPECT_EQ(pml::test::eval_int("(* 4 5)"), 20);
}

TEST(EvaluatorArithmetic, Division) {
    // Division always returns double in PML C++ (matching Python PML behavior)
    EXPECT_DOUBLE_EQ(pml::test::eval_double("(/ 10 2)"), 5.0);
}

TEST(EvaluatorArithmetic, MultiArgAddition) {
    EXPECT_EQ(pml::test::eval_int("(+ 1 2 3)"), 6);
}

TEST(EvaluatorArithmetic, MultiArgMultiplication) {
    EXPECT_EQ(pml::test::eval_int("(* 2 3 4)"), 24);
}

// ============================================================================
// Variable Definition and Mutation
// ============================================================================

TEST(EvaluatorDefine, BasicDefine) {
    EXPECT_EQ(pml::test::eval_int("(define x 5) x"), 5);
}

TEST(EvaluatorSet, BasicSet) {
    EXPECT_EQ(pml::test::eval_int("(define x 1) (set! x 42) x"), 42);
}

// ============================================================================
// Lambda and Functions
// ============================================================================

TEST(EvaluatorLambda, BasicLambda) {
    EXPECT_EQ(pml::test::eval_int("(define f (lambda (x) (* x 2))) (f 5)"), 10);
}

TEST(EvaluatorLambda, Recursion) {
    EXPECT_EQ(pml::test::eval_int(
        "(define fact (lambda (n) (if (= n 0) 1 (* n (fact (- n 1)))))) "
        "(fact 5)"), 120);
}

TEST(EvaluatorLambda, Closure) {
    EXPECT_EQ(pml::test::eval_int(
        "(define make-adder (lambda (n) (lambda (x) (+ n x)))) "
        "(define add5 (make-adder 5)) "
        "(add5 3)"), 8);
}

// ============================================================================
// Conditionals
// ============================================================================

TEST(EvaluatorIf, TrueBranch) {
    EXPECT_EQ(pml::test::eval_string("(if #t \"yes\" \"no\")"), "yes");
}

TEST(EvaluatorIf, FalseBranch) {
    EXPECT_EQ(pml::test::eval_string("(if #f \"yes\" \"no\")"), "no");
}

TEST(EvaluatorCond, BasicCond) {
    EXPECT_EQ(pml::test::eval_int("(cond (#f 1) (#t 2))"), 2);
}

// ============================================================================
// Logical Operations
// ============================================================================

TEST(EvaluatorAnd, BothTrue) {
    EXPECT_TRUE(pml::test::eval_bool("(and #t #t)"));
}

TEST(EvaluatorAnd, OneFalse) {
    EXPECT_FALSE(pml::test::eval_bool("(and #t #f)"));
}

TEST(EvaluatorOr, OneTrue) {
    EXPECT_TRUE(pml::test::eval_bool("(or #f #t)"));
}

TEST(EvaluatorOr, BothFalse) {
    EXPECT_FALSE(pml::test::eval_bool("(or #f #f)"));
}

// ============================================================================
// Let Bindings
// ============================================================================

TEST(EvaluatorLet, ParallelBindings) {
    EXPECT_EQ(pml::test::eval_int("(let ((x 1) (y 2)) (+ x y))"), 3);
}

TEST(EvaluatorLetStar, SequentialBindings) {
    EXPECT_EQ(pml::test::eval_int("(let* ((x 1) (y (+ x 1))) y)"), 2);
}

// ============================================================================
// Begin
// ============================================================================

TEST(EvaluatorBegin, ReturnsLast) {
    EXPECT_EQ(pml::test::eval_int("(begin 1 2 3)"), 3);
}

// ============================================================================
// List Operations
// ============================================================================

TEST(EvaluatorList, Car) {
    EXPECT_EQ(pml::test::eval_int("(car (list 1 2 3))"), 1);
}

TEST(EvaluatorList, Cdr) {
    auto result = pml::test::eval("(cdr (list 1 2 3))");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<pml::ValueList>>(*result));
    auto list = std::get<std::shared_ptr<pml::ValueList>>(*result);
    EXPECT_EQ(list->elements.size(), 2);
}

TEST(EvaluatorList, Cons) {
    auto result = pml::test::eval("(cons 1 (list 2 3))");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<pml::ValueList>>(*result));
    auto list = std::get<std::shared_ptr<pml::ValueList>>(*result);
    EXPECT_EQ(list->elements.size(), 3);
}

TEST(EvaluatorList, ListCreation) {
    auto result = pml::test::eval("(list 1 2 3)");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<pml::ValueList>>(*result));
    auto list = std::get<std::shared_ptr<pml::ValueList>>(*result);
    EXPECT_EQ(list->elements.size(), 3);
}

TEST(EvaluatorList, Append) {
    auto result = pml::test::eval("(append (list 1 2) (list 3 4))");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<pml::ValueList>>(*result));
    auto list = std::get<std::shared_ptr<pml::ValueList>>(*result);
    ASSERT_EQ(list->elements.size(), 4);
    EXPECT_EQ(std::get<int64_t>(list->elements[0]), 1);
    EXPECT_EQ(std::get<int64_t>(list->elements[1]), 2);
    EXPECT_EQ(std::get<int64_t>(list->elements[2]), 3);
    EXPECT_EQ(std::get<int64_t>(list->elements[3]), 4);
}

TEST(EvaluatorList, Reverse) {
    auto result = pml::test::eval("(reverse (list 1 2 3 4))");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<pml::ValueList>>(*result));
    auto list = std::get<std::shared_ptr<pml::ValueList>>(*result);
    ASSERT_EQ(list->elements.size(), 4);
    EXPECT_EQ(std::get<int64_t>(list->elements[0]), 4);
    EXPECT_EQ(std::get<int64_t>(list->elements[1]), 3);
    EXPECT_EQ(std::get<int64_t>(list->elements[2]), 2);
    EXPECT_EQ(std::get<int64_t>(list->elements[3]), 1);
}

// ============================================================================
// String Operations
// ============================================================================

TEST(EvaluatorString, Append) {
    EXPECT_EQ(pml::test::eval_string("(string-append \"hello\" \" \" \"world\")"),
              "hello world");
}

TEST(EvaluatorString, Length) {
    EXPECT_EQ(pml::test::eval_int("(string-length \"hello\")"), 5);
}

// ============================================================================
// Type Predicates
// ============================================================================

TEST(EvaluatorPredicates, NumberPredicate) {
    EXPECT_TRUE(pml::test::eval_bool("(number? 42)"));
}

TEST(EvaluatorPredicates, StringPredicate) {
    EXPECT_TRUE(pml::test::eval_bool("(string? \"hi\")"));
}

TEST(EvaluatorPredicates, SymbolPredicate) {
    EXPECT_TRUE(pml::test::eval_bool("(symbol? 'x)"));
}

// ============================================================================
// Do Loop
// ============================================================================

TEST(EvaluatorDo, BasicLoop) {
    EXPECT_EQ(pml::test::eval_int("(do ((i 0 (+ i 1))) ((= i 5) i))"), 5);
}

// ============================================================================
// Quasiquote
// ============================================================================

TEST(EvaluatorQuasiquote, QuotedList) {
    // Test quote (') which produces a list of literal values
    auto result = pml::test::eval("'(1 2 3)");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<pml::ValueList>>(*result));
    auto list = std::get<std::shared_ptr<pml::ValueList>>(*result);
    ASSERT_EQ(list->elements.size(), 3);
    EXPECT_EQ(std::get<int64_t>(list->elements[0]), 1);
    EXPECT_EQ(std::get<int64_t>(list->elements[1]), 2);
    EXPECT_EQ(std::get<int64_t>(list->elements[2]), 3);
}

// ============================================================================
// Error Handling
// ============================================================================

TEST(EvaluatorErrors, ArityError) {
    // Calling a 2-arg lambda with only 1 arg should produce an error
    auto result = pml::test::eval(
        "(define f (lambda (x y) (+ x y))) (f 1)");
    EXPECT_FALSE(result.has_value());
}
