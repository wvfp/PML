// ==========================================================================================================================================================================================================================================═
// PML Parser Tests — Google Test suite for pml::Parser
// ==========================================================================================================================================================================================================================================═

#include <gtest/gtest.h>
#include "pml/frontend/lexer.h"
#include "pml/frontend/parser.h"
#include "pml/core/types.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace pml {
namespace {

// ---- Helpers ------------------------------------------------------------------------------------------------------------------------------------

// Tokenize + parse source; asserts success and returns the Expr vector.
std::vector<Expr> parse_ok(const std::string& source) {
    auto tokens = Lexer(source, "<test>").tokenize();
    if (!tokens) {
        ADD_FAILURE() << "tokenize() failed: " << tokens.error().message;
        return {};
    }
    auto ast = Parser(std::move(*tokens), "<test>").parse();
    EXPECT_TRUE(ast.has_value())
        << "parse() failed: " << ast.error().message;
    return std::move(*ast);
}

// Tokenize + parse source; asserts failure and returns the PMLException.
PMLException parse_err(const std::string& source) {
    auto tokens = Lexer(source, "<test>").tokenize();
    if (!tokens) {
        ADD_FAILURE() << "tokenize() failed unexpectedly";
        return tokens.error();
    }
    auto ast = Parser(std::move(*tokens), "<test>").parse();
    EXPECT_FALSE(ast.has_value())
        << "parse() succeeded but was expected to fail";
    return ast.error();
}

const ArenaExprVector& list_elements(const Expr& e) {
    return std::get<std::shared_ptr<ListExpr>>(e)->elements;
}

// ---- Integer ------------------------------------------------------------------------------------------------------------------------------------

TEST(Parser, ParseInteger) {
    auto exprs = parse_ok("42");
    ASSERT_EQ(exprs.size(), 1u);
    ASSERT_TRUE(is_integer(exprs[0]));
    EXPECT_EQ(std::get<int64_t>(exprs[0]), 42);
}

// ---- Float ----------------------------------------------------------------------------------------------------------------------------------------

TEST(Parser, ParseFloat) {
    auto exprs = parse_ok("3.14");
    ASSERT_EQ(exprs.size(), 1u);
    ASSERT_TRUE(is_float(exprs[0]));
    EXPECT_DOUBLE_EQ(std::get<double>(exprs[0]), 3.14);
}

// ---- String ------------------------------------------------------------------------------------------------------------------------------------─

TEST(Parser, ParseString) {
    auto exprs = parse_ok("\"hello\"");
    ASSERT_EQ(exprs.size(), 1u);
    ASSERT_TRUE(is_string(exprs[0]));
    EXPECT_EQ(std::get<std::string>(exprs[0]), "hello");
}

// ---- Boolean ------------------------------------------------------------------------------------------------------------------------------------

TEST(Parser, ParseBooleanTrue) {
    auto exprs = parse_ok("#t");
    ASSERT_EQ(exprs.size(), 1u);
    ASSERT_TRUE(is_bool(exprs[0]));
    EXPECT_EQ(std::get<bool>(exprs[0]), true);
}

// ---- Symbol ------------------------------------------------------------------------------------------------------------------------------------─

TEST(Parser, ParseSymbol) {
    auto exprs = parse_ok("foo");
    ASSERT_EQ(exprs.size(), 1u);
    ASSERT_TRUE(is_symbol(exprs[0]));
    EXPECT_EQ(std::get<Symbol>(exprs[0]).name, "foo");
}

// ---- Keyword ------------------------------------------------------------------------------------------------------------------------------------

TEST(Parser, ParseKeyword) {
    auto exprs = parse_ok(":key");
    ASSERT_EQ(exprs.size(), 1u);
    ASSERT_TRUE(is_keyword(exprs[0]));
    EXPECT_EQ(std::get<Keyword>(exprs[0]).name, "key");
}

// ---- Simple list ----------------------------------------------------------------------------------------------------------------------------

TEST(Parser, ParseSimpleList) {
    auto exprs = parse_ok("(+ 1 2)");
    ASSERT_EQ(exprs.size(), 1u);
    ASSERT_TRUE(is_list(exprs[0]));

    const auto& elems = list_elements(exprs[0]);
    ASSERT_EQ(elems.size(), 3u);

    // First element: Symbol "+"
    ASSERT_TRUE(is_symbol(elems[0]));
    EXPECT_EQ(std::get<Symbol>(elems[0]).name, "+");

    // Second element: integer 1
    ASSERT_TRUE(is_integer(elems[1]));
    EXPECT_EQ(std::get<int64_t>(elems[1]), 1);

    // Third element: integer 2
    ASSERT_TRUE(is_integer(elems[2]));
    EXPECT_EQ(std::get<int64_t>(elems[2]), 2);
}

// ---- Nested list ----------------------------------------------------------------------------------------------------------------------------

TEST(Parser, ParseNestedList) {
    auto exprs = parse_ok("(a (b c))");
    ASSERT_EQ(exprs.size(), 1u);
    ASSERT_TRUE(is_list(exprs[0]));

    const auto& outer = list_elements(exprs[0]);
    ASSERT_EQ(outer.size(), 2u);

    // First element: Symbol "a"
    ASSERT_TRUE(is_symbol(outer[0]));
    EXPECT_EQ(std::get<Symbol>(outer[0]).name, "a");

    // Second element: nested list (b c)
    ASSERT_TRUE(is_list(outer[1]));
    const auto& inner = list_elements(outer[1]);
    ASSERT_EQ(inner.size(), 2u);
    ASSERT_TRUE(is_symbol(inner[0]));
    EXPECT_EQ(std::get<Symbol>(inner[0]).name, "b");
    ASSERT_TRUE(is_symbol(inner[1]));
    EXPECT_EQ(std::get<Symbol>(inner[1]).name, "c");
}

// ---- Empty list ----------------------------------------------------------------------------------------------------------------------------─

TEST(Parser, ParseEmptyList) {
    auto exprs = parse_ok("()");
    ASSERT_EQ(exprs.size(), 1u);
    ASSERT_TRUE(is_list(exprs[0]));

    const auto& elems = list_elements(exprs[0]);
    EXPECT_EQ(elems.size(), 0u);
}

// ---- Multiple top-level expressions ------------------------------------------------------------------------------------

TEST(Parser, MultipleTopLevel) {
    auto exprs = parse_ok("1 2 3");
    ASSERT_EQ(exprs.size(), 3u);

    ASSERT_TRUE(is_integer(exprs[0]));
    EXPECT_EQ(std::get<int64_t>(exprs[0]), 1);

    ASSERT_TRUE(is_integer(exprs[1]));
    EXPECT_EQ(std::get<int64_t>(exprs[1]), 2);

    ASSERT_TRUE(is_integer(exprs[2]));
    EXPECT_EQ(std::get<int64_t>(exprs[2]), 3);
}

// ---- Quote sugar ----------------------------------------------------------------------------------------------------------------------------

TEST(Parser, QuoteSugar) {
    // 'x → (quote x)
    auto exprs = parse_ok("'x");
    ASSERT_EQ(exprs.size(), 1u);
    ASSERT_TRUE(is_list(exprs[0]));

    const auto& elems = list_elements(exprs[0]);
    ASSERT_EQ(elems.size(), 2u);

    ASSERT_TRUE(is_symbol(elems[0]));
    EXPECT_EQ(std::get<Symbol>(elems[0]).name, "quote");

    ASSERT_TRUE(is_symbol(elems[1]));
    EXPECT_EQ(std::get<Symbol>(elems[1]).name, "x");
}

// ---- Quasiquote with unquote ------------------------------------------------------------------------------------------------─

TEST(Parser, QuasiquoteWithUnquote) {
    // `(1 ,x) → (quasiquote (1 (unquote x)))
    auto exprs = parse_ok("`(1 ,x)");
    ASSERT_EQ(exprs.size(), 1u);
    ASSERT_TRUE(is_list(exprs[0]));

    const auto& outer = list_elements(exprs[0]);
    ASSERT_EQ(outer.size(), 2u);

    // First: Symbol "quasiquote"
    ASSERT_TRUE(is_symbol(outer[0]));
    EXPECT_EQ(std::get<Symbol>(outer[0]).name, "quasiquote");

    // Second: list (1 (unquote x))
    ASSERT_TRUE(is_list(outer[1]));
    const auto& body = list_elements(outer[1]);
    ASSERT_EQ(body.size(), 2u);

    // body[0]: integer 1
    ASSERT_TRUE(is_integer(body[0]));
    EXPECT_EQ(std::get<int64_t>(body[0]), 1);

    // body[1]: list (unquote x)
    ASSERT_TRUE(is_list(body[1]));
    const auto& unq = list_elements(body[1]);
    ASSERT_EQ(unq.size(), 2u);
    ASSERT_TRUE(is_symbol(unq[0]));
    EXPECT_EQ(std::get<Symbol>(unq[0]).name, "unquote");
    ASSERT_TRUE(is_symbol(unq[1]));
    EXPECT_EQ(std::get<Symbol>(unq[1]).name, "x");
}

// ---- Error: unmatched open paren ----------------------------------------------------------------------------------------─

TEST(Parser, UnmatchedOpenParen) {
    auto err = parse_err("(");
    EXPECT_EQ(err.code, ErrorCode::PMLSyntaxError);
}

// ---- Error: unmatched close paren ----------------------------------------------------------------------------------------

TEST(Parser, UnmatchedCloseParen) {
    auto err = parse_err(")");
    EXPECT_EQ(err.code, ErrorCode::PMLSyntaxError);
}

}  // namespace
}  // namespace pml
