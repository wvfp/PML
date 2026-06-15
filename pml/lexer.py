"""PML lexer — hand-written scanner for S-expression tokens."""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum, auto

from pml.errors import PMLSyntaxError


class TokenType(Enum):
    INTEGER = auto()
    FLOAT = auto()
    STRING = auto()
    SYMBOL = auto()
    BOOLEAN = auto()
    KEYWORD = auto()
    LPAREN = auto()
    RPAREN = auto()
    QUOTE = auto()
    QUASIQUOTE = auto()
    UNQUOTE = auto()
    UNQUOTE_SPLICE = auto()
    EOF = auto()


@dataclass(slots=True)
class Token:
    type: TokenType
    value: str
    line: int
    column: int

    def __repr__(self) -> str:
        return f"Token({self.type.name}, {self.value!r}, {self.line}:{self.column})"


# Characters that terminate a symbol (besides whitespace)
_DELIMITERS = set("()\"`; \t\n\r")

# Characters that cannot appear in symbols
_SYMBOL_SPECIAL = set("-?*+<=>!_/")


class Lexer:
    """Tokenize PML source code into a list of Tokens."""

    def __init__(self, source: str, filename: str = "<stdin>") -> None:
        self.source = source
        self.filename = filename
        self.pos = 0
        self.line = 1
        self.column = 1

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def tokenize(self) -> list[Token]:
        """Scan the entire source and return a list of tokens."""
        tokens: list[Token] = []
        while self.pos < len(self.source):
            self._skip_whitespace_and_comments()
            if self.pos >= len(self.source):
                break
            token = self._read_token()
            if token is not None:
                tokens.append(token)
        tokens.append(Token(TokenType.EOF, "", self.line, self.column))
        return tokens

    # ------------------------------------------------------------------
    # Whitespace & comments
    # ------------------------------------------------------------------

    def _skip_whitespace_and_comments(self) -> None:
        while self.pos < len(self.source):
            ch = self.source[self.pos]
            if ch in " \t\n\r":
                self._advance()
            elif ch == ";":
                # Line comment — skip to end of line
                while self.pos < len(self.source) and self.source[self.pos] != "\n":
                    self._advance()
            else:
                break

    # ------------------------------------------------------------------
    # Token reading
    # ------------------------------------------------------------------

    def _read_token(self) -> Token | None:
        ch = self.source[self.pos]
        line, col = self.line, self.column

        # Parentheses
        if ch == "(":
            self._advance()
            return Token(TokenType.LPAREN, "(", line, col)
        if ch == ")":
            self._advance()
            return Token(TokenType.RPAREN, ")", line, col)

        # String literal
        if ch == '"':
            return self._read_string()

        # Syntax sugar prefixes
        if ch == "'":
            self._advance()
            return Token(TokenType.QUOTE, "'", line, col)
        if ch == "`":
            self._advance()
            return Token(TokenType.QUASIQUOTE, "`", line, col)
        if ch == ",":
            self._advance()
            # Check for ,@  (unquote-splicing)
            if self.pos < len(self.source) and self.source[self.pos] == "@":
                self._advance()
                return Token(TokenType.UNQUOTE_SPLICE, ",@", line, col)
            return Token(TokenType.UNQUOTE, ",", line, col)

        # Number or symbol
        return self._read_atom()

    # ------------------------------------------------------------------
    # Atoms
    # ------------------------------------------------------------------

    def _read_string(self) -> Token:
        """Read a double-quoted string with escape sequences."""
        line, col = self.line, self.column
        self._advance()  # skip opening "
        chars: list[str] = []
        while self.pos < len(self.source):
            ch = self.source[self.pos]
            if ch == "\\":
                self._advance()
                if self.pos >= len(self.source):
                    raise PMLSyntaxError(
                        "Unterminated string escape",
                        line=line,
                        column=col,
                        filename=self.filename,
                    )
                esc = self.source[self.pos]
                escape_map = {"n": "\n", "t": "\t", "\\": "\\", '"': '"'}
                chars.append(escape_map.get(esc, esc))
                self._advance()
            elif ch == '"':
                self._advance()  # skip closing "
                return Token(TokenType.STRING, "".join(chars), line, col)
            else:
                chars.append(ch)
                self._advance()
        raise PMLSyntaxError(
            "Unterminated string literal",
            line=line,
            column=col,
            filename=self.filename,
        )

    def _read_atom(self) -> Token:
        """Read a number, boolean, keyword, or symbol."""
        line, col = self.line, self.column
        start = self.pos
        while self.pos < len(self.source) and self.source[self.pos] not in _DELIMITERS:
            self._advance()
        text = self.source[start : self.pos]

        if not text:
            raise PMLSyntaxError(
                f"Unexpected character: {self.source[self.pos]!r}",
                line=line,
                column=col,
                filename=self.filename,
            )

        # Booleans
        if text == "#t":
            return Token(TokenType.BOOLEAN, "#t", line, col)
        if text == "#f":
            return Token(TokenType.BOOLEAN, "#f", line, col)

        # Keyword (starts with ':')
        if text.startswith(":"):
            return Token(TokenType.KEYWORD, text[1:], line, col)

        # Number: try integer then float
        if self._looks_like_number(text):
            if "." in text:
                return Token(TokenType.FLOAT, text, line, col)
            return Token(TokenType.INTEGER, text, line, col)

        # Symbol
        return Token(TokenType.SYMBOL, text, line, col)

    @staticmethod
    def _looks_like_number(text: str) -> bool:
        """Check whether text represents a number."""
        if not text:
            return False
        # Strip leading sign
        t = text
        if t[0] in "+-":
            t = t[1:]
        if not t:
            return False
        # Must start with digit or dot-digit
        if t[0] == ".":
            t = t[1:]
            if not t:
                return False
        # All remaining chars must be digits (with at most one dot)
        dot_seen = False
        for ch in t:
            if ch == ".":
                if dot_seen:
                    return False
                dot_seen = True
            elif not ch.isdigit():
                return False
        return True

    # ------------------------------------------------------------------
    # Cursor helpers
    # ------------------------------------------------------------------

    def _advance(self) -> None:
        """Move one character forward, tracking line/column."""
        if self.pos < len(self.source):
            if self.source[self.pos] == "\n":
                self.line += 1
                self.column = 1
            else:
                self.column += 1
            self.pos += 1
