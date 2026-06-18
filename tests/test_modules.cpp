// tests/test_modules.cpp — Tests for macros, module system, letrec, higher-order fns
//
// Uses the test helper at tests/test_helpers.h which provides:
//   pml::test::eval(source, env), pml::test::eval_int(source, env),
//   pml::test::eval_bool(source, env), pml::test::eval_string(source, env),
//   pml::test::make_env()

#include <gtest/gtest.h>
#include "test_helpers.h"
#include "pml/module/module_loader.h"

#include <string>

// ═══════════════════════════════════════════════════════════════════════════════
// Macro tests — (defmacro name (params) body...)
//
// The macro body is a template: parameters are textually substituted with
// unevaluated argument expressions, then the result is evaluated.
// ═══════════════════════════════════════════════════════════════════════════════

TEST(Macros, SimpleMacro) {
    // A simple when-style macro: body (if cond body) becomes (if #t 42)
    auto env = pml::test::make_env();
    auto r = pml::test::eval(
        "(begin"
        "  (defmacro my-when (cond body) (if cond body))"
        "  (my-when #t 42))",
        env);
    ASSERT_TRUE(r.has_value()) << r.error().message;
    EXPECT_EQ(std::get<int64_t>(*r), 42);
}

TEST(Macros, SimpleMacroFalseCondition) {
    // A macro that falls back to #f on false condition.
    auto env = pml::test::make_env();
    auto r = pml::test::eval(
        "(begin"
        "  (defmacro my-when (cond body) (if cond body #f))"
        "  (my-when #f 42))",
        env);
    ASSERT_TRUE(r.has_value()) << r.error().message;
    // When condition is false, should return #f (bool false)
    EXPECT_TRUE(std::holds_alternative<bool>(*r));
    if (std::holds_alternative<bool>(*r)) {
        EXPECT_FALSE(std::get<bool>(*r));
    }
}

TEST(Macros, MultiParamMacro) {
    // (my-and a b) → (if a b #f)
    auto env = pml::test::make_env();
    auto r = pml::test::eval(
        "(begin"
        "  (defmacro my-and (a b) (if a b #f))"
        "  (my-and #t #t))",
        env);
    ASSERT_TRUE(r.has_value()) << r.error().message;
    EXPECT_TRUE(std::get<bool>(*r));
}

TEST(Macros, MultiParamMacroFalse) {
    auto env = pml::test::make_env();
    auto r = pml::test::eval(
        "(begin"
        "  (defmacro my-and (a b) (if a b #f))"
        "  (my-and #t #f))",
        env);
    ASSERT_TRUE(r.has_value()) << r.error().message;
    EXPECT_FALSE(std::get<bool>(*r));
}

TEST(Macros, CodeGeneratingMacro) {
    // (square x) → (* x x)
    auto env = pml::test::make_env();
    auto r = pml::test::eval_int(
        "(begin"
        "  (defmacro square (x) (* x x))"
        "  (square 5))",
        env);
    EXPECT_EQ(r, 25);
}

TEST(Macros, CodeGeneratingMacroWithExpression) {
    // The macro should work with sub-expressions too
    auto env = pml::test::make_env();
    auto r = pml::test::eval_int(
        "(begin"
        "  (defmacro square (x) (* x x))"
        "  (square (+ 2 3)))",
        env);
    EXPECT_EQ(r, 25);
}

TEST(Macros, RestParams) {
    // Macro with rest params: args collects all remaining args as a list
    auto env = pml::test::make_env();
    auto r = pml::test::eval(
        "(begin"
        "  (defmacro my-list (. args) args)"
        "  (my-list 1 2 3))",
        env);
    ASSERT_TRUE(r.has_value()) << r.error().message;
    // Result should be a list (1 2 3)
    EXPECT_TRUE(pml::is_list(*r));
    auto vl = std::get<std::shared_ptr<pml::ValueList>>(*r);
    ASSERT_NE(vl, nullptr);
    EXPECT_EQ(vl->elements.size(), 3u);
    EXPECT_EQ(std::get<int64_t>(vl->elements[0]), 1);
    EXPECT_EQ(std::get<int64_t>(vl->elements[1]), 2);
    EXPECT_EQ(std::get<int64_t>(vl->elements[2]), 3);
}

TEST(Macros, NestedMacroUsage) {
    // Define one macro, then use it inside another macro's expansion
    auto env = pml::test::make_env();
    auto r = pml::test::eval_int(
        "(begin"
        "  (defmacro double (x) (* 2 x))"
        "  (defmacro quadruple (x) (double (double x)))"
        "  (quadruple 3))",
        env);
    EXPECT_EQ(r, 12);
}

TEST(Macros, MacroUsedInDefine) {
    // Define a macro, then use it inside a function definition
    auto env = pml::test::make_env();
    auto r = pml::test::eval_int(
        "(begin"
        "  (defmacro square (x) (* x x))"
        "  (define (sum-of-squares a b) (+ (square a) (square b)))"
        "  (sum-of-squares 3 4))",
        env);
    EXPECT_EQ(r, 25);
}

TEST(Macros, MacroWithLetBinding) {
    // Macro that introduces a local binding
    auto env = pml::test::make_env();
    auto r = pml::test::eval_int(
        "(begin"
        "  (defmacro with-temp (val expr) (let ((temp val)) expr))"
        "  (with-temp 10 (+ temp 5)))",
        env);
    EXPECT_EQ(r, 15);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Gensym tests
// ═══════════════════════════════════════════════════════════════════════════════

TEST(Gensym, ReturnsUniqueSymbol) {
    auto env = pml::test::make_env();
    auto r = pml::test::eval("(gensym)", env);
    ASSERT_TRUE(r.has_value()) << r.error().message;
    EXPECT_TRUE(pml::is_symbol(*r));
}

TEST(Gensym, TwoCallsReturnDifferentSymbols) {
    auto env = pml::test::make_env();
    auto r = pml::test::eval(
        "(list (gensym) (gensym))",
        env);
    ASSERT_TRUE(r.has_value()) << r.error().message;
    ASSERT_TRUE(pml::is_list(*r));
    auto vl = std::get<std::shared_ptr<pml::ValueList>>(*r);
    ASSERT_NE(vl, nullptr);
    ASSERT_EQ(vl->elements.size(), 2u);

    auto& sym1 = std::get<pml::Symbol>(vl->elements[0]);
    auto& sym2 = std::get<pml::Symbol>(vl->elements[1]);
    EXPECT_NE(sym1, sym2);
}

TEST(Gensym, CustomPrefix) {
    auto env = pml::test::make_env();
    auto r = pml::test::eval("(gensym \"tmp\")", env);
    ASSERT_TRUE(r.has_value()) << r.error().message;
    ASSERT_TRUE(pml::is_symbol(*r));
    auto& sym = std::get<pml::Symbol>(*r);
    // Should start with the custom prefix
    EXPECT_EQ(sym.name.substr(0, 3), "tmp");
}

// ═══════════════════════════════════════════════════════════════════════════════
// Assert tests
// ═══════════════════════════════════════════════════════════════════════════════

TEST(Assert, TrueAssertionSucceeds) {
    auto env = pml::test::make_env();
    auto r = pml::test::eval("(assert #t)", env);
    ASSERT_TRUE(r.has_value()) << r.error().message;
    // assert returns #t on success
    EXPECT_TRUE(std::get<bool>(*r));
}

TEST(Assert, FalseAssertionFails) {
    auto env = pml::test::make_env();
    auto r = pml::test::eval("(assert #f)", env);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, pml::ErrorCode::PMLAssertionError);
}

TEST(Assert, AssertionWithExpression) {
    auto env = pml::test::make_env();
    auto r = pml::test::eval("(assert (= 1 1))", env);
    ASSERT_TRUE(r.has_value()) << r.error().message;
    EXPECT_TRUE(std::get<bool>(*r));
}

TEST(Assert, FalseAssertionWithExpression) {
    auto env = pml::test::make_env();
    auto r = pml::test::eval("(assert (= 1 2))", env);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, pml::ErrorCode::PMLAssertionError);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Macroexpand tests
// ═══════════════════════════════════════════════════════════════════════════════

TEST(Macroexpand, ReturnsExpandedForm) {
    auto env = pml::test::make_env();
    // macroexpand returns the form (as a Value); for a non-macro call
    // it should return the form as-is
    auto r = pml::test::eval("(macroexpand (+ 1 2))", env);
    ASSERT_TRUE(r.has_value()) << r.error().message;
    // The result should be a list
    EXPECT_TRUE(pml::is_list(*r));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Provide / module system tests
// ═══════════════════════════════════════════════════════════════════════════════

TEST(Provide, MarksSymbolsAsExported) {
    auto env = pml::test::make_env();
    auto r = pml::test::eval(
        "(begin"
        "  (define x 10)"
        "  (define y 20)"
        "  (provide x y))",
        env);
    ASSERT_TRUE(r.has_value()) << r.error().message;

    // Check that the environment's exports set contains x and y
    EXPECT_TRUE(env->exports.count("x") > 0);
    EXPECT_TRUE(env->exports.count("y") > 0);
    EXPECT_FALSE(env->exports.count("z") > 0);
}

TEST(Provide, DoesNotAffectBinding) {
    // provide should mark exports but not change the bindings
    auto env = pml::test::make_env();
    auto r = pml::test::eval_int(
        "(begin"
        "  (define secret 42)"
        "  (provide secret)"
        "  secret)",
        env);
    EXPECT_EQ(r, 42);
}

TEST(Module, ModuleClassGetExported) {
    // Test Module::get() for exported symbols
    auto inner_env = pml::test::make_env();
    inner_env->define("foo", pml::Value(int64_t(100)));
    inner_env->define("bar", pml::Value(int64_t(200)));
    inner_env->exports.insert("foo");

    auto mod = std::make_shared<pml::Module>(
        "test-mod", inner_env, inner_env->exports);

    // foo is exported — should succeed
    auto foo_result = mod->get("foo");
    ASSERT_TRUE(foo_result.has_value());
    EXPECT_EQ(std::get<int64_t>(*foo_result), 100);

    // bar is not exported — should fail with AccessError
    auto bar_result = mod->get("bar");
    EXPECT_FALSE(bar_result.has_value());
}

TEST(Module, ModuleHasCheck) {
    auto inner_env = pml::test::make_env();
    inner_env->define("exported-fn", pml::Value(int64_t(1)));
    inner_env->exports.insert("exported-fn");

    auto mod = std::make_shared<pml::Module>(
        "test-mod", inner_env, inner_env->exports);

    EXPECT_TRUE(mod->has("exported-fn"));
    EXPECT_FALSE(mod->has("private-fn"));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Letrec tests
// ═══════════════════════════════════════════════════════════════════════════════

TEST(Letrec, MutualRecursion) {
    // Classic even?/odd? mutual recursion
    auto env = pml::test::make_env();
    auto r = pml::test::eval(
        "(letrec"
        "  ((even? (lambda (n) (if (= n 0) #t (odd? (- n 1)))))"
        "   (odd?  (lambda (n) (if (= n 0) #f (even? (- n 1))))))"
        "  (even? 4))",
        env);
    ASSERT_TRUE(r.has_value()) << r.error().message;
    EXPECT_TRUE(std::get<bool>(*r));
}

TEST(Letrec, MutualRecursionOdd) {
    auto env = pml::test::make_env();
    auto r = pml::test::eval(
        "(letrec"
        "  ((even? (lambda (n) (if (= n 0) #t (odd? (- n 1)))))"
        "   (odd?  (lambda (n) (if (= n 0) #f (even? (- n 1))))))"
        "  (odd? 5))",
        env);
    ASSERT_TRUE(r.has_value()) << r.error().message;
    EXPECT_TRUE(std::get<bool>(*r));
}

TEST(Letrec, SelfRecursiveFunction) {
    // Factorial via letrec
    auto env = pml::test::make_env();
    auto r = pml::test::eval_int(
        "(letrec"
        "  ((fact (lambda (n) (if (= n 0) 1 (* n (fact (- n 1)))))))"
        "  (fact 6))",
        env);
    EXPECT_EQ(r, 720);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Higher-order function tests
// ═══════════════════════════════════════════════════════════════════════════════

TEST(HigherOrder, PassFunctionAsArgument) {
    auto env = pml::test::make_env();
    auto r = pml::test::eval_int(
        "(begin"
        "  (define (apply-twice f x) (f (f x)))"
        "  (define (inc n) (+ n 1))"
        "  (apply-twice inc 5))",
        env);
    EXPECT_EQ(r, 7);
}

TEST(HigherOrder, ReturnFunctionFromFunction) {
    auto env = pml::test::make_env();
    auto r = pml::test::eval_int(
        "(begin"
        "  (define (make-adder n) (lambda (x) (+ n x)))"
        "  (define add5 (make-adder 5))"
        "  (add5 10))",
        env);
    EXPECT_EQ(r, 15);
}

TEST(HigherOrder, ClosureOverMutableState) {
    // Closure that captures and mutates a variable
    auto env = pml::test::make_env();
    auto r = pml::test::eval_int(
        "(begin"
        "  (define counter 0)"
        "  (define (tick) (begin (set! counter (+ counter 1)) counter))"
        "  (tick)"
        "  (tick)"
        "  (tick))",
        env);
    EXPECT_EQ(r, 3);
}

TEST(HigherOrder, ComposeFunctions) {
    auto env = pml::test::make_env();
    auto r = pml::test::eval_int(
        "(begin"
        "  (define (compose f g) (lambda (x) (f (g x))))"
        "  (define (double x) (* x 2))"
        "  (define (inc x) (+ x 1))"
        "  (define double-then-inc (compose inc double))"
        "  (double-then-inc 5))",
        env);
    // double(5) = 10, inc(10) = 11
    EXPECT_EQ(r, 11);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Begin — multiple return values
// ═══════════════════════════════════════════════════════════════════════════════

TEST(Begin, ReturnsLastExpression) {
    auto env = pml::test::make_env();
    auto r = pml::test::eval_int(
        "(begin 1 2 3 4 5)",
        env);
    EXPECT_EQ(r, 5);
}

TEST(Begin, SideEffectsInSequence) {
    auto env = pml::test::make_env();
    auto r = pml::test::eval_int(
        "(begin"
        "  (define x 0)"
        "  (set! x (+ x 1))"
        "  (set! x (+ x 10))"
        "  x)",
        env);
    EXPECT_EQ(r, 11);
}

TEST(Begin, NestedBegin) {
    auto env = pml::test::make_env();
    auto r = pml::test::eval_int(
        "(begin"
        "  (define a 1)"
        "  (begin"
        "    (define b 2)"
        "    (+ a b)))",
        env);
    EXPECT_EQ(r, 3);
}
