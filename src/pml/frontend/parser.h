#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Parser — Recursive Descent S-expression Parser
// ═══════════════════════════════════════════════════════════════════════════════
//
// Converts a token stream (produced by Lexer::tokenize()) into a vector of
// Expr AST nodes. Matches the Python PML parser (pml/parser.py) exactly.
//
// Parsing stages:
//   parse()           — top-level: loop over expressions until EOF
//   parse_expr()      — dispatch: list, quote sugar, or atom
//   parse_list()      — parenthesized ( ... ) S-expression
//   parse_atom()      — number, string, boolean, symbol, keyword
// ═══════════════════════════════════════════════════════════════════════════════

#include "error.h"
#include "lexer.h"
#include "types.h"

#include <string>
#include <vector>

namespace pml {

class Parser {
public:
    /// Construct a parser over the given token list.
    /// @param tokens   Token list produced by Lexer::tokenize().
    /// @param filename Optional filename for error reporting.
    explicit Parser(std::vector<Token> tokens, std::string filename = "<stdin>");

    /// Result of parsing that may contain both successfully parsed expressions
    /// and a list of collected errors.
    struct ParseResult {
        std::vector<Expr> expressions;
        std::vector<PMLException> errors;

        [[nodiscard]] bool success() const noexcept { return errors.empty(); }
    };

    /// Parse the entire token stream into a vector of top-level Expr AST nodes.
    /// Returns the first error encountered (legacy API).
    [[nodiscard]] auto parse() -> Result<std::vector<Expr>>;

    /// Parse the entire token stream, collecting as many expressions and
    /// errors as possible using panic-mode error recovery.
    [[nodiscard]] auto parse_all() -> ParseResult;

private:
    std::vector<Token> tokens;
    std::string filename;
    size_t pos = 0;
    std::vector<PMLException> errors_;  ///< Errors collected during parse_all

    /// Parse a single expression from the current position.
    [[nodiscard]] auto parse_expr() -> Result<Expr>;

    /// Parse a parenthesized list: ( expr ... )
    /// Expects current token to be LPAREN.
    [[nodiscard]] auto parse_list() -> Result<Expr>;

    /// Parse an atomic value: integer, float, string, boolean, symbol, keyword.
    [[nodiscard]] auto parse_atom() -> Result<Expr>;

    /// Panic-mode recovery: skip tokens until a safe top-level expression
    /// boundary (start of next expression or a closing parenthesis).
    void synchronize_top_level();

    /// Skip tokens until the matching ')' of the current list.
    void skip_to_closing_rparen();

    /// Return true if the token type can start a PML expression.
    [[nodiscard]] static bool is_expr_start(TokenType type);

    /// Get the current token without consuming it.
    [[nodiscard]] auto current() const -> const Token&;

    /// Consume and return the current token, advancing position.
    [[nodiscard]] auto advance() -> const Token&;

    /// Check if we've reached the end of the token stream.
    [[nodiscard]] auto at_end() const -> bool;
};

}  // namespace pml
