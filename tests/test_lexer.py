"""Tests for the PML lexer."""

import pytest

from pml.errors import PMLSyntaxError
from pml.lexer import Lexer, TokenType


class TestBasicTokens:
    def test_integers(self):
        tokens = Lexer("42 -10 +7").tokenize()
        assert tokens[0].type == TokenType.INTEGER
        assert tokens[0].value == "42"
        assert tokens[1].type == TokenType.INTEGER
        assert tokens[1].value == "-10"
        assert tokens[2].type == TokenType.INTEGER
        assert tokens[2].value == "+7"

    def test_floats(self):
        tokens = Lexer("3.14 -0.5").tokenize()
        assert tokens[0].type == TokenType.FLOAT
        assert tokens[0].value == "3.14"
        assert tokens[1].type == TokenType.FLOAT
        assert tokens[1].value == "-0.5"

    def test_strings(self):
        tokens = Lexer('"hello" "world"').tokenize()
        assert tokens[0].type == TokenType.STRING
        assert tokens[0].value == "hello"
        assert tokens[1].type == TokenType.STRING
        assert tokens[1].value == "world"

    def test_string_escapes(self):
        tokens = Lexer(r'"a\"b" "c\\d"').tokenize()
        assert tokens[0].value == 'a"b'
        assert tokens[1].value == "c\\d"

    def test_symbols(self):
        tokens = Lexer("circle my-var + ->").tokenize()
        assert tokens[0].type == TokenType.SYMBOL
        assert tokens[0].value == "circle"
        assert tokens[1].value == "my-var"
        assert tokens[2].value == "+"
        assert tokens[3].value == "->"

    def test_booleans(self):
        tokens = Lexer("#t #f").tokenize()
        assert tokens[0].type == TokenType.BOOLEAN
        assert tokens[0].value == "#t"
        assert tokens[1].type == TokenType.BOOLEAN
        assert tokens[1].value == "#f"

    def test_keywords(self):
        tokens = Lexer(":fill :stroke-width").tokenize()
        assert tokens[0].type == TokenType.KEYWORD
        assert tokens[0].value == "fill"
        assert tokens[1].type == TokenType.KEYWORD
        assert tokens[1].value == "stroke-width"

    def test_parens(self):
        tokens = Lexer("(+ 1 2)").tokenize()
        assert tokens[0].type == TokenType.LPAREN
        assert tokens[-2].type == TokenType.RPAREN
        assert tokens[-1].type == TokenType.EOF


class TestComments:
    def test_line_comment(self):
        tokens = Lexer("42 ; this is a comment\n10").tokenize()
        assert len(tokens) == 3  # 42, 10, EOF
        assert tokens[0].value == "42"
        assert tokens[1].value == "10"

    def test_only_comment(self):
        tokens = Lexer("; just a comment").tokenize()
        assert len(tokens) == 1
        assert tokens[0].type == TokenType.EOF


class TestSyntaxSugar:
    def test_quote(self):
        tokens = Lexer("'hello").tokenize()
        assert tokens[0].type == TokenType.QUOTE

    def test_quasiquote(self):
        tokens = Lexer("`hello").tokenize()
        assert tokens[0].type == TokenType.QUASIQUOTE

    def test_unquote(self):
        tokens = Lexer(",x").tokenize()
        assert tokens[0].type == TokenType.UNQUOTE

    def test_unquote_splicing(self):
        tokens = Lexer(",@xs").tokenize()
        assert tokens[0].type == TokenType.UNQUOTE_SPLICE


class TestLineTracking:
    def test_multiline(self):
        source = "(define x 1)\n(define y 2)"
        tokens = Lexer(source).tokenize()
        # First token on line 1
        assert tokens[0].line == 1
        # "define" on line 2
        define_token = [t for t in tokens if t.value == "define"][1]
        assert define_token.line == 2


class TestErrors:
    def test_unterminated_string(self):
        with pytest.raises(PMLSyntaxError, match="Unterminated string"):
            Lexer('"hello').tokenize()
