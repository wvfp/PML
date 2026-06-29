#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Lexer — Hand-written Scanner for S-expression Tokens
// ==========================================================================================================================================================================================================================================═

#include "error.h"

#include <string>
#include <vector>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// TokenType — every token class recognized by the PML lexer
// ==========================================================================================================================================================================================================================================═

enum class TokenType {
    INTEGER,        // 42, -7
    FLOAT,          // 3.14, -0.5
    STRING,         // "hello world"
    SYMBOL,         // foo, +, set!, hello-world?
    BOOLEAN,        // #t, #f
    KEYWORD,        // :fill, :bg
    LPAREN,         // (
    RPAREN,         // )
    QUOTE,          // '
    QUASIQUOTE,     // `
    UNQUOTE,        // ,
    UNQUOTE_SPLICE, // ,@
    FNLIT,          // #(   — anonymous function literal
    END_OF_FILE     // end of file
};

// ==========================================================================================================================================================================================================================================═
// Token — a single lexeme with source location
// ==========================================================================================================================================================================================================================================═

struct Token {
    TokenType type;
    std::string value;   // lexeme text (without surrounding quotes for strings)
    int line;            // 1-based
    int column;          // 1-based (position of first char)
};

// ==========================================================================================================================================================================================================================================═
// Lexer — tokenizes PML source into a vector of tokens
// ==========================================================================================================================================================================================================================================═

class Lexer {
public:
    /// Construct a lexer for the given source string.
    /// @param source  The PML source code.
    /// @param filename  Optional filename for error reporting (default "<stdin>").
    explicit Lexer(std::string source, std::string filename = "<stdin>");

    /// Tokenize the entire source, returning tokens plus END_OF_FILE.
    /// Returns an error (PMLSyntaxError) on unterminated strings, unterminated
    /// escapes, or unexpected characters.
    [[nodiscard]] auto tokenize() -> Result<std::vector<Token>>;

private:
    std::string source;
    std::string filename;
    size_t pos = 0;
    int line = 1;
    int column = 1;

    /// Skip whitespace (spaces, tabs, newlines, CR) and line comments starting
    /// with ';' until the next non-whitespace, non-comment character or EOF.
    void skip_whitespace_and_comments();

    /// Read the next token from the source, dispatching on the first character.
    /// Returns either a Token or a PMLSyntaxError.
    [[nodiscard]] auto read_token() -> Result<Token>;

    /// Read a double-quoted string, processing escape sequences.
    /// Expects the current position to be at the opening '"'.
    [[nodiscard]] auto read_string() -> Result<Token>;

    /// Read an atom: number, boolean, keyword, or symbol.
    /// Expects the current position to be at the start of the atom.
    [[nodiscard]] auto read_atom() -> Result<Token>;

    /// Check whether the given text represents a valid number literal
    /// (integer or floating-point).
    [[nodiscard]] static auto looks_like_number(const std::string& text) -> bool;

    /// Advance one character forward, tracking line and column numbers.
    void advance();
};

}  // namespace pml
