// ==========================================================================================================================================================================================================================================═
// PML Lexer Tests — Google Test suite for pml::Lexer
// ==========================================================================================================================================================================================================================================═

#include <gtest/gtest.h>
#include "pml/frontend/lexer.h"

#include <string>
#include <vector>

namespace pml {
namespace {

// Helper: tokenize source and return the token vector (asserts success).
std::vector<Token> tokenize_ok(const std::string& source) {
    auto result = Lexer(source, "<test>").tokenize();
    EXPECT_TRUE(result.has_value())
        << "tokenize() failed: " << result.error().message;
    return std::move(*result);
}

// ----─ Integer tokens --------------------------------------------------------------------------------------------------------------------

TEST(Lexer, IntegerLiteral) {
    auto tokens = tokenize_ok("42");
    ASSERT_EQ(tokens.size(), 2u);  // INTEGER + EOF
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[0].value, "42");
}

TEST(Lexer, NegativeInteger) {
    auto tokens = tokenize_ok("-5");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[0].value, "-5");
}

// ----─ Float tokens ------------------------------------------------------------------------------------------------------------------------

TEST(Lexer, FloatLiteral) {
    auto tokens = tokenize_ok("3.14");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type, TokenType::FLOAT);
    EXPECT_EQ(tokens[0].value, "3.14");
}

// ----─ String tokens --------------------------------------------------------------------------------------------------------------------─

TEST(Lexer, StringLiteral) {
    auto tokens = tokenize_ok("\"hello\"");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type, TokenType::STRING);
    EXPECT_EQ(tokens[0].value, "hello");
}

TEST(Lexer, StringEscapeNewline) {
    auto tokens = tokenize_ok("\"hello\\nworld\"");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type, TokenType::STRING);
    EXPECT_EQ(tokens[0].value, "hello\nworld");
}

// ----─ Symbol tokens --------------------------------------------------------------------------------------------------------------------─

TEST(Lexer, SymbolWithDash) {
    auto tokens = tokenize_ok("foo-bar");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type, TokenType::SYMBOL);
    EXPECT_EQ(tokens[0].value, "foo-bar");
}

// ----─ Boolean tokens --------------------------------------------------------------------------------------------------------------------

TEST(Lexer, BooleanTrue) {
    auto tokens = tokenize_ok("#t");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type, TokenType::BOOLEAN);
    EXPECT_EQ(tokens[0].value, "#t");
}

TEST(Lexer, BooleanFalse) {
    auto tokens = tokenize_ok("#f");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type, TokenType::BOOLEAN);
    EXPECT_EQ(tokens[0].value, "#f");
}

// ----─ Keyword tokens --------------------------------------------------------------------------------------------------------------------

TEST(Lexer, Keyword) {
    auto tokens = tokenize_ok(":key");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type, TokenType::KEYWORD);
    EXPECT_EQ(tokens[0].value, "key");  // stored without leading ':'
}

// ----─ Parenthesized expression ------------------------------------------------------------------------------------------------

TEST(Lexer, ParenthesizedExpression) {
    auto tokens = tokenize_ok("(+ 1 2)");
    // Expected: LPAREN, SYMBOL("+"), INTEGER("1"), INTEGER("2"), RPAREN, EOF
    ASSERT_EQ(tokens.size(), 6u);
    EXPECT_EQ(tokens[0].type, TokenType::LPAREN);
    EXPECT_EQ(tokens[1].type, TokenType::SYMBOL);
    EXPECT_EQ(tokens[1].value, "+");
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[2].value, "1");
    EXPECT_EQ(tokens[3].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[3].value, "2");
    EXPECT_EQ(tokens[4].type, TokenType::RPAREN);
    EXPECT_EQ(tokens[5].type, TokenType::END_OF_FILE);
}

// ----─ Comments --------------------------------------------------------------------------------------------------------------------------------

TEST(Lexer, LineComment) {
    auto tokens = tokenize_ok("; comment\n42");
    ASSERT_EQ(tokens.size(), 2u);  // INTEGER("42") + EOF
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[0].value, "42");
}

// ----─ Quote sugar ------------------------------------------------------------------------------------------------------------------------─

TEST(Lexer, QuoteSugar) {
    auto tokens = tokenize_ok("'x");
    ASSERT_EQ(tokens.size(), 3u);  // QUOTE, SYMBOL, EOF
    EXPECT_EQ(tokens[0].type, TokenType::QUOTE);
    EXPECT_EQ(tokens[1].type, TokenType::SYMBOL);
    EXPECT_EQ(tokens[1].value, "x");
}

// ----─ Quasiquote / unquote ----------------------------------------------------------------------------------------------------─

TEST(Lexer, QuasiquoteAndUnquote) {
    auto tokens = tokenize_ok("`(1 ,x)");
    // Expected: QUASIQUOTE, LPAREN, INTEGER("1"), UNQUOTE, SYMBOL("x"), RPAREN, EOF
    ASSERT_EQ(tokens.size(), 7u);
    EXPECT_EQ(tokens[0].type, TokenType::QUASIQUOTE);
    EXPECT_EQ(tokens[1].type, TokenType::LPAREN);
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[2].value, "1");
    EXPECT_EQ(tokens[3].type, TokenType::UNQUOTE);
    EXPECT_EQ(tokens[4].type, TokenType::SYMBOL);
    EXPECT_EQ(tokens[4].value, "x");
    EXPECT_EQ(tokens[5].type, TokenType::RPAREN);
    EXPECT_EQ(tokens[6].type, TokenType::END_OF_FILE);
}

// ----─ Empty input ------------------------------------------------------------------------------------------------------------------------─

TEST(Lexer, EmptyInput) {
    auto tokens = tokenize_ok("");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::END_OF_FILE);
}

// ----─ Unterminated string --------------------------------------------------------------------------------------------------------─

TEST(Lexer, UnterminatedString) {
    auto result = Lexer("\"hello", "<test>").tokenize();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::PMLSyntaxError);
}

// ----─ Line tracking --------------------------------------------------------------------------------------------------------------------─

TEST(Lexer, LineTracking) {
    // "(a\nb)" — '(' on line 1, 'a' on line 1, 'b' on line 2, ')' on line 2
    auto tokens = tokenize_ok("(a\nb)");
    ASSERT_GE(tokens.size(), 5u);  // LPAREN, SYMBOL, SYMBOL, RPAREN, EOF

    EXPECT_EQ(tokens[0].type, TokenType::LPAREN);
    EXPECT_EQ(tokens[0].line, 1);

    EXPECT_EQ(tokens[1].type, TokenType::SYMBOL);
    EXPECT_EQ(tokens[1].value, "a");
    EXPECT_EQ(tokens[1].line, 1);

    EXPECT_EQ(tokens[2].type, TokenType::SYMBOL);
    EXPECT_EQ(tokens[2].value, "b");
    EXPECT_EQ(tokens[2].line, 2);

    EXPECT_EQ(tokens[3].type, TokenType::RPAREN);
    EXPECT_EQ(tokens[3].line, 2);
}

}  // namespace
}  // namespace pml
