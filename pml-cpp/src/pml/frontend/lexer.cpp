// ═══════════════════════════════════════════════════════════════════════════════
// PML Lexer — Hand-written Scanner for S-expression Tokens
// ═══════════════════════════════════════════════════════════════════════════════
//
// Matches the Python PML lexer (pml/lexer.py) semantics exactly.
// ═══════════════════════════════════════════════════════════════════════════════

#include "lexer.h"

#include <cctype>
#include <format>
#include <utility>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════════

Lexer::Lexer(std::string source_, std::string filename_)
    : source(std::move(source_))
    , filename(std::move(filename_)) {}

// ═══════════════════════════════════════════════════════════════════════════════
// Public API
// ═══════════════════════════════════════════════════════════════════════════════

auto Lexer::tokenize() -> Result<std::vector<Token>> {
    std::vector<Token> tokens;
    while (pos < source.size()) {
        skip_whitespace_and_comments();
        if (pos >= source.size()) {
            break;
        }
        auto tok = read_token();
        if (!tok) {
            return std::unexpected(tok.error());
        }
        tokens.push_back(std::move(*tok));
    }
    tokens.push_back(Token{TokenType::END_OF_FILE, "", line, column});
    return tokens;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Whitespace & comments
// ═══════════════════════════════════════════════════════════════════════════════

void Lexer::skip_whitespace_and_comments() {
    while (pos < source.size()) {
        const char ch = source[pos];
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            advance();
        } else if (ch == ';') {
            // Line comment — skip to end of line
            while (pos < source.size() && source[pos] != '\n') {
                advance();
            }
        } else {
            break;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Token reading dispatcher
// ═══════════════════════════════════════════════════════════════════════════════

auto Lexer::read_token() -> Result<Token> {
    const char ch = source[pos];
    const int cur_line = line;
    const int cur_col = column;

    // Parentheses
    if (ch == '(') {
        advance();
        return Token{TokenType::LPAREN, "(", cur_line, cur_col};
    }
    if (ch == ')') {
        advance();
        return Token{TokenType::RPAREN, ")", cur_line, cur_col};
    }

    // String literal
    if (ch == '"') {
        return read_string();
    }

    // Syntax sugar prefixes
    if (ch == '\'') {
        advance();
        return Token{TokenType::QUOTE, "'", cur_line, cur_col};
    }
    if (ch == '`') {
        advance();
        return Token{TokenType::QUASIQUOTE, "`", cur_line, cur_col};
    }
    if (ch == ',') {
        advance();
        // Check for ,@  (unquote-splicing)
        if (pos < source.size() && source[pos] == '@') {
            advance();
            return Token{TokenType::UNQUOTE_SPLICE, ",@", cur_line, cur_col};
        }
        return Token{TokenType::UNQUOTE, ",", cur_line, cur_col};
    }

    // Number or symbol
    return read_atom();
}

// ═══════════════════════════════════════════════════════════════════════════════
// String literal reader
// ═══════════════════════════════════════════════════════════════════════════════

auto Lexer::read_string() -> Result<Token> {
    const int str_line = line;
    const int str_col = column;
    advance();  // skip opening "

    std::string chars;
    while (pos < source.size()) {
        const char ch = source[pos];
        if (ch == '\\') {
            advance();
            if (pos >= source.size()) {
                return std::unexpected(syntax_error(
                    SourceLocation{str_line, str_col, filename},
                    "Unterminated string escape"
                ));
            }
            const char esc = source[pos];
            switch (esc) {
                case 'n':  chars.push_back('\n'); break;
                case 't':  chars.push_back('\t'); break;
                case '\\': chars.push_back('\\'); break;
                case '"':  chars.push_back('"');  break;
                default:   chars.push_back(esc);  break;  // unknown escape: keep as-is
            }
            advance();
        } else if (ch == '"') {
            advance();  // skip closing "
            return Token{TokenType::STRING, std::move(chars), str_line, str_col};
        } else {
            chars.push_back(ch);
            advance();
        }
    }

    return std::unexpected(syntax_error(
        SourceLocation{str_line, str_col, filename},
        "Unterminated string literal"
    ));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Atom reader (number, boolean, keyword, or symbol)
// ═══════════════════════════════════════════════════════════════════════════════

auto Lexer::read_atom() -> Result<Token> {
    const int atom_line = line;
    const int atom_col = column;
    const size_t start = pos;

    // Characters that terminate an atom (besides whitespace)
    // Must match the Python _DELIMITERS set: set("()\"`; \t\n\r")
    const auto is_delimiter = [](char ch) -> bool {
        return ch == '(' || ch == ')' || ch == '"' || ch == '`' || ch == ';'
            || ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
    };

    while (pos < source.size() && !is_delimiter(source[pos])) {
        advance();
    }

    const std::string text = source.substr(start, pos - start);

    if (text.empty()) {
        return std::unexpected(syntax_error(
            SourceLocation{atom_line, atom_col, filename},
            std::format("Unexpected character: {}", source[pos])
        ));
    }

    // Booleans
    if (text == "#t") {
        return Token{TokenType::BOOLEAN, "#t", atom_line, atom_col};
    }
    if (text == "#f") {
        return Token{TokenType::BOOLEAN, "#f", atom_line, atom_col};
    }

    // Keyword (starts with ':')
    if (text[0] == ':') {
        return Token{TokenType::KEYWORD, text.substr(1), atom_line, atom_col};
    }

    // Number: try integer then float
    if (looks_like_number(text)) {
        if (text.find('.') != std::string::npos) {
            return Token{TokenType::FLOAT, text, atom_line, atom_col};
        }
        return Token{TokenType::INTEGER, text, atom_line, atom_col};
    }

    // Symbol
    return Token{TokenType::SYMBOL, text, atom_line, atom_col};
}

// ═══════════════════════════════════════════════════════════════════════════════
// Number detection
// ═══════════════════════════════════════════════════════════════════════════════

auto Lexer::looks_like_number(const std::string& text) -> bool {
    if (text.empty()) {
        return false;
    }

    // Strip leading sign
    size_t i = 0;
    if (text[0] == '+' || text[0] == '-') {
        i = 1;
    }
    if (i >= text.size()) {
        return false;
    }

    // Must start with digit or dot-digit
    if (text[i] == '.') {
        ++i;
        if (i >= text.size()) {
            return false;
        }
    }

    // All remaining chars must be digits (with at most one dot)
    bool dot_seen = false;
    for (/* i already set */; i < text.size(); ++i) {
        const char ch = text[i];
        if (ch == '.') {
            if (dot_seen) {
                return false;
            }
            dot_seen = true;
        } else if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return false;
        }
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Cursor helpers
// ═══════════════════════════════════════════════════════════════════════════════

void Lexer::advance() {
    if (pos < source.size()) {
        if (source[pos] == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
        ++pos;
    }
}

}  // namespace pml
