// ==========================================================================================================================================================================================================================================═
// PML Parser — Recursive Descent S-expression Parser
// ==========================================================================================================================================================================================================================================═
//
// Matches the Python PML parser (pml/parser.py) semantics exactly.
// ==========================================================================================================================================================================================================================================═

#include "parser.h"

#include <format>
#include <functional>
#include <utility>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// Constructor
// ==========================================================================================================================================================================================================================================═

Parser::Parser(std::vector<Token> tokens_, std::string filename_)
    : tokens(std::move(tokens_))
    , filename(std::move(filename_)) {}

// ==========================================================================================================================================================================================================================================═
// Public API
// ==========================================================================================================================================================================================================================================═

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

// ==========================================================================================================================================================================================================================================═
// Expression parsing
// ==========================================================================================================================================================================================================================================═

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

        // #( — anonymous function (lambda shorthand)
        case TokenType::FNLIT:
            return parse_fnlit();

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
        // Provide context: show the last successfully parsed expression location
        std::string hint;
        if (!items.empty()) {
            const auto& last_loc = item_locs.back();
            hint = std::format(" (last complete sub-expression at line {})", last_loc.line);
        }
        return std::unexpected(syntax_error(
            list_loc,
            std::format("Unmatched '(' at line {} -- expected closing ')'", list_loc.line) + hint
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

auto Parser::parse_fnlit() -> Result<Expr> {
    const Token& token = current();  // the #( token
    SourceLocation loc{token.line, token.column, filename};
    (void)advance();  // consume FNLIT (#)

    // Expect LPAREN — the body list
    if (at_end() || current().type != TokenType::LPAREN) {
        return std::unexpected(syntax_error(loc, "Expected '(' after # in #()"));
    }

    // Parse the body list: (expr ...)
    // parse_list() returns a ListExpr with all body expressions as elements
    auto list_result = parse_list();
    if (!list_result) return std::unexpected(list_result.error());

    auto* lst = get_list(*list_result);
    if (!lst) {
        return std::unexpected(syntax_error(loc, "Internal error: fnlit body is not a list"));
    }

    // Empty body check
    if (lst->empty()) {
        return std::unexpected(syntax_error(loc, "#() requires at least one body expression"));
    }

    // Determine body expression(s):
    // - One element → unwrap to just that expression (e.g., #(42) → body is 42)
    // - Multiple elements → use the entire list as a single body expression (e.g., #(* % 2) → body is (* %2))
    std::vector<Expr> body;
    if (lst->size() == 1) {
        body.push_back(std::move((*lst)[0]));
    } else {
        body.push_back(std::move(*list_result));
    }

    // Detect % / %n placeholders in body
    int max_arg = 0;

    // Recursive walker: find %/%n symbols and replace bare % with %1
    std::function<void(Expr&)> process = [&](Expr& e) {
        if (auto* sym = std::get_if<Symbol>(&e)) {
            if (sym->name == "%") {
                sym->name = "%1";
                max_arg = std::max(max_arg, 1);
            } else if (sym->name.size() >= 2 && sym->name[0] == '%') {
                // Check if the rest is all digits (e.g., %1, %2, ... %9)
                bool all_digits = true;
                for (char c : sym->name.substr(1)) {
                    if (c < '0' || c > '9') { all_digits = false; break; }
                }
                if (all_digits) {
                    int n = std::stoi(sym->name.substr(1));
                    max_arg = std::max(max_arg, n);
                }
            }
        } else if (auto* inner_lst = get_list(e)) {
            // Don't recurse into quote forms — symbols inside quote are literal
            if (!inner_lst->empty() && is_symbol((*inner_lst)[0]) &&
                std::get<Symbol>((*inner_lst)[0]).name == "quote") {
                return;
            }
            for (auto& child : *inner_lst) {
                process(child);
            }
        }
    };

    // Walk body and discover/replace % args
    for (auto& e : body) {
        process(e);
    }

    // Build params list: (%1 %2 ... %max_n)
    std::vector<Expr> params;
    for (int i = 1; i <= max_arg; ++i) {
        params.push_back(Expr(Symbol(std::format("%{}", i))));
    }

    // Build (lambda (%1 ... %n) body...)
    std::vector<Expr> lambda_children;
    lambda_children.push_back(Expr(Symbol{"lambda"}));
    lambda_children.push_back(make_list(std::move(params), loc));
    for (auto& e : body) {
        lambda_children.push_back(std::move(e));
    }

    return make_list(std::move(lambda_children), loc);
}

// ==========================================================================================================================================================================================================================================═
// Cursor helpers
// ==========================================================================================================================================================================================================================================═

auto Parser::current() const -> const Token& {
    return tokens[pos];
}

auto Parser::advance() -> const Token& {
    return tokens[pos++];
}

auto Parser::at_end() const -> bool {
    return current().type == TokenType::END_OF_FILE;
}

// ==========================================================================================================================================================================================================================================═
// Panic-mode error recovery
// ==========================================================================================================================================================================================================================================═

bool Parser::is_expr_start(TokenType type) {
    switch (type) {
        case TokenType::LPAREN:
        case TokenType::QUOTE:
        case TokenType::QUASIQUOTE:
        case TokenType::UNQUOTE:
        case TokenType::UNQUOTE_SPLICE:
        case TokenType::FNLIT:
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
