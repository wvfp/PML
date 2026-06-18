"""PML parser — converts a token stream into nested Python lists (AST)."""

from __future__ import annotations

from pml.errors import PMLSyntaxError
from pml.lexer import Token, TokenType
from pml.types import Expr, Keyword, Symbol


class Parser:
    """Parse a list of Tokens into a list of PML expressions."""

    def __init__(self, tokens: list[Token], filename: str = "<stdin>") -> None:
        self.tokens = tokens
        self.filename = filename
        self.pos = 0

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def parse(self) -> list[Expr]:
        """Parse the token stream and return top-level expressions."""
        exprs: list[Expr] = []
        while not self._at_end():
            exprs.append(self._parse_expr())
        return exprs

    # ------------------------------------------------------------------
    # Expression parsing
    # ------------------------------------------------------------------

    def _parse_expr(self) -> Expr:
        token = self._current()

        # List: ( ... )
        if token.type == TokenType.LPAREN:
            return self._parse_list()

        # Quote sugar: 'expr → (quote expr)
        if token.type == TokenType.QUOTE:
            self._advance()
            return [Symbol("quote"), self._parse_expr()]

        # Quasiquote sugar: `expr → (quasiquote expr)
        if token.type == TokenType.QUASIQUOTE:
            self._advance()
            return [Symbol("quasiquote"), self._parse_expr()]

        # Unquote-splicing: ,@expr → (unquote-splicing expr)
        if token.type == TokenType.UNQUOTE_SPLICE:
            self._advance()
            return [Symbol("unquote-splicing"), self._parse_expr()]

        # Unquote: ,expr → (unquote expr)
        if token.type == TokenType.UNQUOTE:
            self._advance()
            return [Symbol("unquote"), self._parse_expr()]

        # Atom
        return self._parse_atom()

    def _parse_list(self) -> list[Expr]:
        """Parse ( expr ... )"""
        open_token = self._current()
        self._advance()  # skip (
        items: list[Expr] = []
        while not self._at_end() and self._current().type != TokenType.RPAREN:
            items.append(self._parse_expr())
        if self._at_end():
            # Count unmatched parens after this position to estimate surplus
            open_count = 1
            extra_close = 0
            for tok in self.tokens[self.pos:]:
                if tok.type == TokenType.LPAREN:
                    open_count += 1
                elif tok.type == TokenType.RPAREN:
                    open_count -= 1
                    if open_count < 0:
                        extra_close += 1
                        open_count = 0
            if extra_close > 0:
                hint = f"可能多了 {extra_close} 个 ')'"
            elif open_count > 0:
                hint = f"可能少了 {open_count} 个 ')'"
            else:
                hint = ""
            raise PMLSyntaxError(
                f"Unmatched '(' at line {open_token.line} — expected closing ')'"
                + (f" — {hint}" if hint else ""),
                line=open_token.line,
                column=open_token.column,
                filename=self.filename,
            )
        self._advance()  # skip )
        return items

    def _parse_atom(self) -> Expr:
        """Parse an atomic value: number, string, boolean, symbol, keyword."""
        token = self._current()
        self._advance()

        if token.type == TokenType.INTEGER:
            return int(token.value)
        if token.type == TokenType.FLOAT:
            return float(token.value)
        if token.type == TokenType.STRING:
            return token.value
        if token.type == TokenType.BOOLEAN:
            return token.value == "#t"
        if token.type == TokenType.KEYWORD:
            return Keyword(token.value)
        if token.type == TokenType.SYMBOL:
            return Symbol(token.value)

        raise PMLSyntaxError(
            f"Unexpected token: {token.type.name} ({token.value!r})",
            line=token.line,
            column=token.column,
            filename=self.filename,
        )

    # ------------------------------------------------------------------
    # Cursor helpers
    # ------------------------------------------------------------------

    def _current(self) -> Token:
        return self.tokens[self.pos]

    def _advance(self) -> Token:
        token = self.tokens[self.pos]
        self.pos += 1
        return token

    def _at_end(self) -> bool:
        return self._current().type == TokenType.EOF
