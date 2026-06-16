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

    /// Parse the entire token stream into a vector of top-level Expr AST nodes.
    [[nodiscard]] auto parse() -> Result<std::vector<Expr>>;

private:
    std::vector<Token> tokens;
    std::string filename;
    size_t pos = 0;

    /// Parse a single expression from the current position.
    [[nodiscard]] auto parse_expr() -> Result<Expr>;

    /// Parse a parenthesized list: ( expr ... )
    /// Expects current token to be LPAREN.
    [[nodiscard]] auto parse_list() -> Result<Expr>;

    /// Parse an atomic value: integer, float, string, boolean, symbol, keyword.
    [[nodiscard]] auto parse_atom() -> Result<Expr>;

    /// Get the current token without consuming it.
    [[nodiscard]] auto current() const -> const Token&;

    /// Consume and return the current token, advancing position.
    [[nodiscard]] auto advance() -> const Token&;

    /// Check if we've reached the end of the token stream.
    [[nodiscard]] auto at_end() const -> bool;
};

}  // namespace pml
