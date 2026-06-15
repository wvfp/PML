"""Tests for the PML parser."""

import pytest

from pml.errors import PMLSyntaxError
from pml.lexer import Lexer
from pml.parser import Parser
from pml.types import Keyword, Symbol


def parse(source: str):
    tokens = Lexer(source).tokenize()
    return Parser(tokens).parse()


class TestAtoms:
    def test_integer(self):
        assert parse("42") == [42]

    def test_float(self):
        assert parse("3.14") == [3.14]

    def test_string(self):
        assert parse('"hello"') == ["hello"]

    def test_boolean(self):
        assert parse("#t") == [True]
        assert parse("#f") == [False]

    def test_symbol(self):
        result = parse("circle")
        assert len(result) == 1
        assert isinstance(result[0], Symbol)
        assert result[0].name == "circle"

    def test_keyword(self):
        result = parse(":fill")
        assert len(result) == 1
        assert isinstance(result[0], Keyword)
        assert result[0].name == "fill"


class TestLists:
    def test_simple_list(self):
        result = parse("(+ 1 2)")
        assert len(result) == 1
        lst = result[0]
        assert isinstance(lst, list)
        assert isinstance(lst[0], Symbol)
        assert lst[0].name == "+"
        assert lst[1] == 1
        assert lst[2] == 2

    def test_nested_list(self):
        result = parse("(+ (* 2 3) 4)")
        lst = result[0]
        assert isinstance(lst[1], list)
        assert lst[1][0].name == "*"
        assert lst[2] == 4

    def test_empty_list(self):
        result = parse("()")
        assert result == [[]]

    def test_multiple_top_level(self):
        result = parse("1 2 3")
        assert result == [1, 2, 3]


class TestSyntaxSugar:
    def test_quote(self):
        result = parse("'hello")
        lst = result[0]
        assert isinstance(lst[0], Symbol)
        assert lst[0].name == "quote"
        assert isinstance(lst[1], Symbol)
        assert lst[1].name == "hello"

    def test_quote_list(self):
        result = parse("'(1 2 3)")
        lst = result[0]
        assert lst[0].name == "quote"
        assert isinstance(lst[1], list)
        assert lst[1] == [1, 2, 3]

    def test_quasiquote(self):
        result = parse("`(a ,b)")
        lst = result[0]
        assert lst[0].name == "quasiquote"


class TestErrors:
    def test_unmatched_open(self):
        with pytest.raises(PMLSyntaxError, match="Unmatched"):
            parse("(+ 1 2")

    def test_unmatched_close(self):
        # Extra closing paren is just left as a token — parser reads it as
        # unexpected, but our current impl will fail at next parse_expr
        with pytest.raises(PMLSyntaxError):
            parse(")")
