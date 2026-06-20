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
    auto result = parse_all();
    if (!result.success()) {
        return std::unexpected(result.errors[0]);
    }
    return result.expressions;
}

auto Parser::parse_all() -> ParseResult {
    ParseResult result;
    errors_.clear();

    while (!at_end()) {
        auto expr = parse_expr();
        if (!expr) {
            result.errors.push_back(expr.error());
            synchronize_top_level();
        } else {
            result.expressions.push_back(std::move(*expr));
        }
    }

    // Append any errors recorded by nested recovery (e.g. inside lists).
    if (!errors_.empty()) {
        result.errors.insert(
            result.errors.end(),
            std::make_move_iterator(errors_.begin()),
            std::make_move_iterator(errors_.end()));
        errors_.clear();
    }

    return result;
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
            const Token& token = current();
            SourceLocation loc{token.line, token.column, filename};
            (void)advance();
            auto expr = parse_expr();
            if (!expr) {
                return std::unexpected(expr.error());
            }
            return make_list({Symbol("quote"), std::move(*expr)}, loc);
        }

        // Quasiquote sugar: `expr → (quasiquote expr)
        case TokenType::QUASIQUOTE: {
            const Token& token = current();
            SourceLocation loc{token.line, token.column, filename};
            (void)advance();
            auto expr = parse_expr();
            if (!expr) {
                return std::unexpected(expr.error());
            }
            return make_list({Symbol("quasiquote"), std::move(*expr)}, loc);
        }

        // Unquote-splicing: ,@expr → (unquote-splicing expr)
        case TokenType::UNQUOTE_SPLICE: {
            const Token& token = current();
            SourceLocation loc{token.line, token.column, filename};
            (void)advance();
            auto expr = parse_expr();
            if (!expr) {
                return std::unexpected(expr.error());
            }
            return make_list({Symbol("unquote-splicing"), std::move(*expr)}, loc);
        }

        // Unquote: ,expr → (unquote expr)
        case TokenType::UNQUOTE: {
            const Token& token = current();
            SourceLocation loc{token.line, token.column, filename};
            (void)advance();
            auto expr = parse_expr();
            if (!expr) {
                return std::unexpected(expr.error());
            }
            return make_list({Symbol("unquote"), std::move(*expr)}, loc);
        }

        // Atom (number, string, boolean, symbol, keyword)
        default:
            return parse_atom();
    }
}

auto Parser::parse_list() -> Result<Expr> {
    // Save the opening ( position for error reporting
    const Token& open_token = current();
    SourceLocation list_loc{open_token.line, open_token.column, filename};
    (void)advance();  // skip (

    std::vector<Expr> items;
    std::vector<SourceLocation> item_locs;

    while (!at_end() && current().type != TokenType::RPAREN) {
        const Token& elem_token = current();
        SourceLocation elem_loc{elem_token.line, elem_token.column, filename};
        auto expr = parse_expr();
        if (!expr) {
            // Panic-mode recovery: record the error and skip to the matching
            // ')' so that parsing can continue with the next expression.
            errors_.push_back(expr.error());
            skip_to_closing_rparen();
            if (!at_end() && current().type == TokenType::RPAREN) {
                (void)advance();  // consume )
            }
            return make_list(std::move(items), list_loc, std::move(item_locs));
        }
        items.push_back(std::move(*expr));
        item_locs.push_back(elem_loc);
    }

    // Unmatched '(' — missing closing ')'
    if (at_end()) {
        return std::unexpected(syntax_error(
            list_loc,
            "Unmatched '(' — expected closing ')'"
        ));
    }

    (void)advance();  // skip )
    return make_list(std::move(items), list_loc, std::move(item_locs));
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

// ═══════════════════════════════════════════════════════════════════════════════
// Panic-mode error recovery
// ═══════════════════════════════════════════════════════════════════════════════

bool Parser::is_expr_start(TokenType type) {
    switch (type) {
        case TokenType::LPAREN:
        case TokenType::QUOTE:
        case TokenType::QUASIQUOTE:
        case TokenType::UNQUOTE:
        case TokenType::UNQUOTE_SPLICE:
        case TokenType::INTEGER:
        case TokenType::FLOAT:
        case TokenType::STRING:
        case TokenType::BOOLEAN:
        case TokenType::KEYWORD:
        case TokenType::SYMBOL:
            return true;
        default:
            return false;
    }
}

void Parser::synchronize_top_level() {
    while (!at_end()) {
        if (is_expr_start(current().type)) {
            return;
        }
        if (current().type == TokenType::RPAREN) {
            (void)advance();  // skip stray )
            return;
        }
        (void)advance();
    }
}

void Parser::skip_to_closing_rparen() {
    int depth = 0;
    while (!at_end()) {
        const Token& tok = current();
        if (tok.type == TokenType::LPAREN) {
            ++depth;
        } else if (tok.type == TokenType::RPAREN) {
            if (depth == 0) {
                return;
            }
            --depth;
        }
        (void)advance();
    }
}

}  // namespace pml
