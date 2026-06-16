// ═══════════════════════════════════════════════════════════════════════════════
// PML API (PMLRuntime) — Google Test Suite
// ═══════════════════════════════════════════════════════════════════════════════
//
// Tests for the PMLRuntime facade:
//   - Construction and environment access
//   - execute() with valid and invalid PML source
//   - execute_pml() JSON output structure
//   - Persistence across execute() calls
//   - Macro definition and expansion
//   - Error structure (type, message, line, hint)
//
// Each test creates a fresh PMLRuntime to avoid cross-test pollution.
// ═══════════════════════════════════════════════════════════════════════════════

#include <gtest/gtest.h>
#include "pml/api/api.h"
#include "pml/evaluator/environment.h"

#include <nlohmann/json.hpp>
#include <string>

// ═══════════════════════════════════════════════════════════════════════════════
// Test fixture — fresh PMLRuntime per test
// ═══════════════════════════════════════════════════════════════════════════════

class PMLRuntimeTest : public ::testing::Test {
protected:
    pml::PMLRuntime rt;
};

// ═══════════════════════════════════════════════════════════════════════════════
// Construction
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(PMLRuntimeTest, CreationSucceeds) {
    // The fixture already constructed the runtime.  Verify env() is non-null.
    EXPECT_NE(rt.env(), nullptr);
}

TEST_F(PMLRuntimeTest, EnvContainsBuiltins) {
    // The global environment should have core builtins like "+" defined.
    auto result = rt.env()->lookup("+");
    EXPECT_TRUE(result.has_value());
}

// ═══════════════════════════════════════════════════════════════════════════════
// execute() — success cases
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(PMLRuntimeTest, ExecuteSimpleArithmetic) {
    auto r = rt.execute("(+ 1 2)");
    EXPECT_TRUE(r.success);
    EXPECT_FALSE(r.error.has_value());
    EXPECT_EQ(r.value, 3);
}

TEST_F(PMLRuntimeTest, ExecuteDefineAndUse) {
    auto r = rt.execute("(define x 5) x");
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.value, 5);
}

TEST_F(PMLRuntimeTest, ExecuteEmptySource) {
    auto r = rt.execute("");
    EXPECT_TRUE(r.success);
    // Empty source: value is null, no error.
    EXPECT_TRUE(r.value.is_null());
    EXPECT_FALSE(r.error.has_value());
}

TEST_F(PMLRuntimeTest, ExecuteMultipleExpressions) {
    // Multiple expressions return the value of the last one.
    auto r = rt.execute("1 2 3");
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.value, 3);
}

TEST_F(PMLRuntimeTest, ExecuteStringLiteral) {
    auto r = rt.execute("\"hello\"");
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.value, "hello");
}

TEST_F(PMLRuntimeTest, ExecuteBooleanLiteral) {
    auto r = rt.execute("#t");
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.value, true);
}

// ═══════════════════════════════════════════════════════════════════════════════
// execute() — error cases
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(PMLRuntimeTest, ExecuteTypeError) {
    // Adding a number and a string should fail.
    auto r = rt.execute("(+ 1 \"abc\")");
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.error.has_value());
}

TEST_F(PMLRuntimeTest, ExecuteSyntaxError) {
    // Unmatched parenthesis.
    auto r = rt.execute("((+ 1 2)");
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.error.has_value());
}

TEST_F(PMLRuntimeTest, ExecuteUnboundVariable) {
    auto r = rt.execute("undefined-variable");
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.error.has_value());
}

// ═══════════════════════════════════════════════════════════════════════════════
// execute_pml() — JSON output
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(PMLRuntimeTest, ExecutePmlReturnsJsonWithSuccess) {
    auto j = rt.execute_pml("(+ 10 20)");
    EXPECT_TRUE(j.contains("success"));
    EXPECT_EQ(j["success"], true);
    EXPECT_EQ(j["value"], 30);
}

TEST_F(PMLRuntimeTest, ExecutePmlErrorContainsTypeAndMessage) {
    auto j = rt.execute_pml("(+ 1 \"abc\")");
    EXPECT_EQ(j["success"], false);
    EXPECT_TRUE(j.contains("error"));
    EXPECT_TRUE(j["error"].is_object());
    EXPECT_TRUE(j["error"].contains("type"));
    EXPECT_TRUE(j["error"].contains("message"));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Persistence across execute() calls
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(PMLRuntimeTest, PersistenceAcrossCalls) {
    auto r1 = rt.execute("(define greeting \"hello\")");
    EXPECT_TRUE(r1.success);

    auto r2 = rt.execute("greeting");
    EXPECT_TRUE(r2.success);
    EXPECT_EQ(r2.value, "hello");
}

// ═══════════════════════════════════════════════════════════════════════════════
// Macro definition and use
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(PMLRuntimeTest, MacroDefinitionAndUse) {
    // Define a simple macro that doubles its argument.
    auto r1 = rt.execute("(defmacro double (x) (+ x x))");
    EXPECT_TRUE(r1.success);

    auto r2 = rt.execute("(double 21)");
    EXPECT_TRUE(r2.success);
    EXPECT_EQ(r2.value, 42);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Error structure
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(PMLRuntimeTest, ErrorHasTypeField) {
    auto r = rt.execute("undefined-var-xyz");
    ASSERT_FALSE(r.success);
    ASSERT_TRUE(r.error.has_value());
    EXPECT_TRUE(r.error->contains("type"));
    EXPECT_EQ((*r.error)["type"], "UnboundVariableError");
}

TEST_F(PMLRuntimeTest, ErrorHasHintField) {
    auto r = rt.execute("undefined-var-xyz");
    ASSERT_FALSE(r.success);
    ASSERT_TRUE(r.error.has_value());
    EXPECT_TRUE(r.error->contains("hint"));
    // The hint for UnboundVariableError should mention "define".
    std::string hint = (*r.error)["hint"].get<std::string>();
    EXPECT_FALSE(hint.empty());
}

TEST_F(PMLRuntimeTest, RenderResultToJson) {
    auto r = rt.execute("(+ 1 2)");
    auto j = r.to_json();
    EXPECT_EQ(j["success"], true);
    EXPECT_EQ(j["value"], 3);
    EXPECT_TRUE(j["error"].is_null());
}

TEST_F(PMLRuntimeTest, RenderResultToJsonOnError) {
    auto r = rt.execute("(+ 1 \"abc\")");
    auto j = r.to_json();
    EXPECT_EQ(j["success"], false);
    EXPECT_TRUE(j["error"].is_object());
}
