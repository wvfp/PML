// ═══════════════════════════════════════════════════════════════════════════════
// PML Parser — Recursive Descent S-expression Parser
// ═══════════════════════════════════════════════════════════════════════════════
//
// Matches the Python PML parser (pml/parser.py) semantics exactly.
// ═══════════════════════════════════════════════════════════════════════════════

#include "parser.h"

#include <format>
#include <utility>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════════

Parser::Parser(std::vector<Token> tokens_, std::string filename_)
    : tokens(std::move(tokens_))
    , filename(std::move(filename_)) {}

// ═══════════════════════════════════════════════════════════════════════════════
// Public API
// ═══════════════════════════════════════════════════════════════════════════════

auto Parser::parse() -> Result<std::vector<Expr>> {
    std::vector<Expr> exprs;
    while (!at_end()) {
        auto expr = parse_expr();
        if (!expr) {
            return std::unexpected(expr.error());
        }
        exprs.push_back(std::move(*expr));
    }
    return exprs;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Expression parsing
// ═══════════════════════════════════════════════════════════════════════════════

auto Parser::parse_expr() -> Result<Expr> {
    const Token& token = current();

    switch (token.type) {
        // List: ( ... )
        case TokenType::LPAREN:
            return parse_list();

        // Quote sugar: 'expr → (quote expr)
        case TokenType::QUOTE: {
            (void)advance();
            auto expr = parse_expr();
            if (!expr) {
                return std::unexpected(expr.error());
            }
            return make_list({Symbol("quote"), std::move(*expr)});
        }

        // Quasiquote sugar: `expr → (quasiquote expr)
        case TokenType::QUASIQUOTE: {
            (void)advance();
            auto expr = parse_expr();
            if (!expr) {
                return std::unexpected(expr.error());
            }
            return make_list({Symbol("quasiquote"), std::move(*expr)});
        }

        // Unquote-splicing: ,@expr → (unquote-splicing expr)
        case TokenType::UNQUOTE_SPLICE: {
            (void)advance();
            auto expr = parse_expr();
            if (!expr) {
                return std::unexpected(expr.error());
            }
            return make_list({Symbol("unquote-splicing"), std::move(*expr)});
        }

        // Unquote: ,expr → (unquote expr)
        case TokenType::UNQUOTE: {
            (void)advance();
            auto expr = parse_expr();
            if (!expr) {
                return std::unexpected(expr.error());
            }
            return make_list({Symbol("unquote"), std::move(*expr)});
        }

        // Atom (number, string, boolean, symbol, keyword)
        default:
            return parse_atom();
    }
}

auto Parser::parse_list() -> Result<Expr> {
    // Save the opening ( position for error reporting
    const Token& open_token = current();
    (void)advance();  // skip (

    std::vector<Expr> items;

    while (!at_end() && current().type != TokenType::RPAREN) {
        auto expr = parse_expr();
        if (!expr) {
            return std::unexpected(expr.error());
        }
        items.push_back(std::move(*expr));
    }

    // Unmatched '(' — missing closing ')'
    if (at_end()) {
        return std::unexpected(syntax_error(
            SourceLocation{open_token.line, open_token.column, filename},
            "Unmatched '(' — expected closing ')'"
        ));
    }

    (void)advance();  // skip )
    return make_list(std::move(items));
}

auto Parser::parse_atom() -> Result<Expr> {
    const Token& token = current();
    (void)advance();

    switch (token.type) {
        case TokenType::INTEGER:
            return Expr{static_cast<int64_t>(std::stoll(token.value))};

        case TokenType::FLOAT:
            return Expr{std::stod(token.value)};

        case TokenType::STRING:
            return Expr{token.value};

        case TokenType::BOOLEAN:
            return Expr{token.value == "#t"};

        case TokenType::KEYWORD:
            return Expr{Keyword{token.value}};

        case TokenType::SYMBOL:
            return Expr{Symbol{token.value}};

        default:
            return std::unexpected(syntax_error(
                SourceLocation{token.line, token.column, filename},
                std::format("Unexpected token: {}", token.value)
            ));
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Cursor helpers
// ═══════════════════════════════════════════════════════════════════════════════

auto Parser::current() const -> const Token& {
    return tokens[pos];
}

auto Parser::advance() -> const Token& {
    return tokens[pos++];
}

auto Parser::at_end() const -> bool {
    return current().type == TokenType::END_OF_FILE;
}

}  // namespace pml
