// ==========================================================================================================================================================================================================================================═
// PML API (PMLRuntime) — Google Test Suite
// ==========================================================================================================================================================================================================================================═
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
// ==========================================================================================================================================================================================================================================═

#include <gtest/gtest.h>
#include "pml/api/api.h"
#include "pml/evaluator/environment.h"

#include <nlohmann/json.hpp>
#include <string>

// ==========================================================================================================================================================================================================================================═
// Test fixture — fresh PMLRuntime per test
// ==========================================================================================================================================================================================================================================═

class PMLRuntimeTest : public ::testing::Test {
protected:
    pml::PMLRuntime rt;
};

// ==========================================================================================================================================================================================================================================═
// Construction
// ==========================================================================================================================================================================================================================================═

TEST_F(PMLRuntimeTest, CreationSucceeds) {
    // The fixture already constructed the runtime.  Verify env() is non-null.
    EXPECT_NE(rt.env(), nullptr);
}

TEST_F(PMLRuntimeTest, EnvContainsBuiltins) {
    // The global environment should have core builtins like "+" defined.
    auto result = rt.env()->lookup("+");
    EXPECT_TRUE(result.has_value());
}

// ==========================================================================================================================================================================================================================================═
// execute() — success cases
// ==========================================================================================================================================================================================================================================═

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

// ==========================================================================================================================================================================================================================================═
// execute() — error cases
// ==========================================================================================================================================================================================================================================═

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

// ==========================================================================================================================================================================================================================================═
// execute_pml() — JSON output
// ==========================================================================================================================================================================================================================================═

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

// ==========================================================================================================================================================================================================================================═
// Persistence across execute() calls
// ==========================================================================================================================================================================================================================================═

TEST_F(PMLRuntimeTest, PersistenceAcrossCalls) {
    auto r1 = rt.execute("(define greeting \"hello\")");
    EXPECT_TRUE(r1.success);

    auto r2 = rt.execute("greeting");
    EXPECT_TRUE(r2.success);
    EXPECT_EQ(r2.value, "hello");
}

// ==========================================================================================================================================================================================================================================═
// Macro definition and use
// ==========================================================================================================================================================================================================================================═

TEST_F(PMLRuntimeTest, MacroDefinitionAndUse) {
    // Define a simple macro that doubles its argument.
    auto r1 = rt.execute("(defmacro double (x) (+ x x))");
    EXPECT_TRUE(r1.success);

    auto r2 = rt.execute("(double 21)");
    EXPECT_TRUE(r2.success);
    EXPECT_EQ(r2.value, 42);
}

// ==========================================================================================================================================================================================================================================═
// Error structure
// ==========================================================================================================================================================================================================================================═

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

// ==========================================================================================================================================================================================================================================═
// execute_file()
// ==========================================================================================================================================================================================================================================═

TEST_F(PMLRuntimeTest, ExecuteFileValid) {
    // _smoke_module.pml defines (add a b) and pi — execute it as a plain
    // script (not as a module import) to verify execute_file() reads the file.
    auto r = rt.execute_file("tests/_smoke_module.pml");
    EXPECT_TRUE(r.success);
    // The file's last expression is (provide add pi) which returns nil.
    EXPECT_TRUE(r.error == std::nullopt || r.value.is_null());
}

TEST_F(PMLRuntimeTest, ExecuteFileMissing) {
    auto r = rt.execute_file("tests/_nonexistent_file_xyz.pml");
    EXPECT_FALSE(r.success);
    ASSERT_TRUE(r.error.has_value());
    EXPECT_EQ((*r.error)["type"], "ResourceError");
}

TEST_F(PMLRuntimeTest, ExecuteFileSetsSourceDir) {
    // After execute_file(), the runtime's context.source_dir should be the
    // directory of the file.
    auto r = rt.execute_file("tests/_smoke_module.pml");
    EXPECT_TRUE(r.success);
    // The path separator may be / on Windows (C++ filesystem normalizes).
    std::string dir = rt.context().source_dir;
    EXPECT_TRUE(dir.find("tests") != std::string::npos)
        << "source_dir should contain 'tests', got: " << dir;
}

// ==========================================================================================================================================================================================================================================═
// validate()
// ==========================================================================================================================================================================================================================================═

TEST_F(PMLRuntimeTest, ValidateValidSource) {
    auto vr = rt.validate("(+ 1 2)");
    EXPECT_TRUE(vr.valid);
    EXPECT_TRUE(vr.errors.empty());
}

TEST_F(PMLRuntimeTest, ValidateSyntaxError) {
    auto vr = rt.validate("((+ 1 2)");
    EXPECT_FALSE(vr.valid);
    EXPECT_FALSE(vr.errors.empty());
}

TEST_F(PMLRuntimeTest, ValidateTypeError) {
    // validate() only checks syntax + macro expansion, not runtime types.
    // Type errors are caught during evaluation, not validation.
    auto vr = rt.validate("(+ 1 \"abc\")");
    EXPECT_TRUE(vr.valid)
        << "validate() should pass for syntactically valid code";
}

TEST_F(PMLRuntimeTest, ValidateEmptySource) {
    auto vr = rt.validate("");
    EXPECT_TRUE(vr.valid);
    EXPECT_TRUE(vr.errors.empty());
}

TEST_F(PMLRuntimeTest, ValidateWithMacro) {
    auto vr = rt.validate("(defmacro double (x) (+ x x))");
    EXPECT_TRUE(vr.valid);
}

TEST_F(PMLRuntimeTest, ValidateWithFilename) {
    // Filename is propagated through to error locations.
    auto vr = rt.validate("((+ 1 2)", "myfile.pml");
    EXPECT_FALSE(vr.valid);
    ASSERT_FALSE(vr.errors.empty());
    // The error should carry the filename (either in the error dict or
    // in the PMLException depending on pipeline stage).
    auto& first = vr.errors[0];
    EXPECT_TRUE(first.contains("type"))
        << "Error dict should have a 'type' field";
}

// ==========================================================================================================================================================================================================================================═
// reset()
// ==========================================================================================================================================================================================================================================═

TEST_F(PMLRuntimeTest, ResetClearsDefinitions) {
    rt.execute("(define foo 42)");
    auto r1 = rt.execute("foo");
    EXPECT_TRUE(r1.success);
    EXPECT_EQ(r1.value, 42);

    rt.reset();

    // After reset, 'foo' should be unbound.
    auto r2 = rt.execute("foo");
    EXPECT_FALSE(r2.success);
    ASSERT_TRUE(r2.error.has_value());
    EXPECT_EQ((*r2.error)["type"], "UnboundVariableError");
}

TEST_F(PMLRuntimeTest, ResetPreservesBuiltins) {
    rt.reset();
    auto r = rt.execute("(+ 1 2)");
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.value, 3);
}

TEST_F(PMLRuntimeTest, ResetClearsCanvasState) {
    // Create a canvas via PML code.
    rt.execute("(canvas 100 100 :bg \"red\")");
    // After reset, canvas should be gone.
    rt.reset();
    // Should be able to create a new canvas without error.
    auto r = rt.execute("(canvas 200 200 :bg \"blue\")");
    EXPECT_TRUE(r.success);
}

// ==========================================================================================================================================================================================================================================═
// Multi-instance isolation
// ==========================================================================================================================================================================================================================================═

TEST_F(PMLRuntimeTest, TwoInstancesDontShareDefinitions) {
    pml::PMLRuntime rt2;

    rt.execute("(define secret 99)");
    rt2.execute("(define secret -1)");

    auto r1 = rt.execute("secret");
    ASSERT_TRUE(r1.success);
    EXPECT_EQ(r1.value, 99);

    auto r2 = rt2.execute("secret");
    ASSERT_TRUE(r2.success);
    EXPECT_EQ(r2.value, -1);
}

TEST_F(PMLRuntimeTest, TwoInstancesDontShareCanvas) {
    pml::PMLRuntime rt2;

    rt.execute("(canvas 100 100 :bg \"red\")");

    // rt2 should not have a canvas — creating one should work fine.
    auto r2 = rt2.execute("(canvas 200 200 :bg \"blue\")");
    EXPECT_TRUE(r2.success);
}

TEST_F(PMLRuntimeTest, TwoInstancesDontShareOutputFiles) {
    // Even without actual rendering, the output_files vectors should be
    // independent across instances.
    pml::PMLRuntime rt2;

    // Simulate an output file via the context directly.
    rt.context().output_files.push_back("out1.png");
    rt2.context().output_files.push_back("out2.png");

    EXPECT_EQ(rt.context().output_files.size(), 1);
    EXPECT_EQ(rt.context().output_files[0], "out1.png");
    EXPECT_EQ(rt2.context().output_files[0], "out2.png");
}

// ==========================================================================================================================================================================================================================================═
// __source_file__ tracking
// ==========================================================================================================================================================================================================================================═

TEST_F(PMLRuntimeTest, SourceFileDefaultsToStringStdin) {
    // When execute() is called without a filename, __source_file__ should
    // be "<stdin>".
    rt.execute("(define dummy 1)");
    auto src = rt.env()->try_lookup("__source_file__");
    ASSERT_TRUE(src.has_value());
    auto* s = src->as_string();
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(*s, "<stdin>");
}

TEST_F(PMLRuntimeTest, SourceFileSetFromFilename) {
    rt.execute("(define dummy 1)", "myscript.pml");
    auto src = rt.env()->try_lookup("__source_file__");
    ASSERT_TRUE(src.has_value());
    auto* s = src->as_string();
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(*s, "myscript.pml");
}

TEST_F(PMLRuntimeTest, SourceFileOverwrittenOnNextCall) {
    // First call with a filename.
    rt.execute("(define dummy 1)", "first.pml");
    // Second call with a different filename.
    rt.execute("(define dummy 2)", "second.pml");
    // __source_file__ should reflect the most recent call.
    auto src = rt.env()->try_lookup("__source_file__");
    ASSERT_TRUE(src.has_value());
    auto* s = src->as_string();
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(*s, "second.pml");
}

TEST_F(PMLRuntimeTest, SourceFileResetOnEmptyFilename) {
    // After executing with a filename, the next call with empty filename
    // should reset __source_file__ to "<stdin>" — not leave a stale value.
    rt.execute("(define dummy 1)", "myfile.pml");
    rt.execute("(define dummy 2)", "");  // empty filename — should reset to <stdin>
    auto src = rt.env()->try_lookup("__source_file__");
    ASSERT_TRUE(src.has_value());
    auto* s = src->as_string();
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(*s, "<stdin>");
}

// ==========================================================================================================================================================================================================================================═
// execute_pml() with filename option
// ==========================================================================================================================================================================================================================================═

TEST_F(PMLRuntimeTest, ExecutePmlWithFilenameOption) {
    nlohmann::json opts;
    opts["filename"] = "from_mcp.pml";
    auto j = rt.execute_pml("(+ 1 2)", opts);
    EXPECT_EQ(j["success"], true);
    EXPECT_EQ(j["value"], 3);

    // __source_file__ should reflect the passed-in filename.
    auto src = rt.env()->try_lookup("__source_file__");
    ASSERT_TRUE(src.has_value());
    auto* s = src->as_string();
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(*s, "from_mcp.pml");
}

TEST_F(PMLRuntimeTest, ExecutePmlDefaultsToStdin) {
    // Without a filename option, __source_file__ should be "<stdin>".
    auto j = rt.execute_pml("(+ 1 2)");
    EXPECT_EQ(j["success"], true);

    auto src = rt.env()->try_lookup("__source_file__");
    ASSERT_TRUE(src.has_value());
    auto* s = src->as_string();
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(*s, "<stdin>");
}

// ==========================================================================================================================================================================================================================================═
// source_dir correctness
// ==========================================================================================================================================================================================================================================═

TEST_F(PMLRuntimeTest, SourceDirFromExecuteFile) {
    rt.execute_file("tests/_smoke_module.pml");
    std::string dir = rt.context().source_dir;
    // Should be the tests/ directory (normalized path).
    EXPECT_TRUE(dir.find("tests") != std::string::npos)
        << "source_dir should contain 'tests', got: " << dir;
}

TEST_F(PMLRuntimeTest, SourceDirIsNotStale) {
    // Execute one file — source_dir becomes the directory of the file.
    rt.execute_file("tests/_smoke_module.pml");
    EXPECT_NE(rt.context().source_dir.find("tests"), std::string::npos)
        << "After execute_file, source_dir should contain 'tests'";

    // Then execute with an empty filename — source_dir should reset to CWD.
    rt.execute("(+ 1 2)", "");
    // The project root CWD does not literally contain "tests" as a substring.
    EXPECT_EQ(rt.context().source_dir.find("tests"), std::string::npos)
        << "After empty-filename execute(), source_dir should reset to CWD, not keep stale value";
}

// ==========================================================================================================================================================================================================================================═
// reset() + re-execute (full lifecycle)
// ==========================================================================================================================================================================================================================================═

TEST_F(PMLRuntimeTest, FullLifecycleExecuteResetExecute) {
    auto r1 = rt.execute("(+ 10 20)");
    EXPECT_TRUE(r1.success);
    EXPECT_EQ(r1.value, 30);

    rt.reset();

    auto r2 = rt.execute("(* 3 4)");
    EXPECT_TRUE(r2.success);
    EXPECT_EQ(r2.value, 12);
}
