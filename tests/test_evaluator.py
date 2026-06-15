"""Tests for the PML evaluator."""

import pytest

from pml.environment import Environment
from pml.errors import ArityError, PMLTypeError, UnboundVariableError
from pml.evaluator import evaluate
from pml.lexer import Lexer
from pml.parser import Parser
from pml.repl import create_global_env
from pml.types import Keyword, Symbol


def run(source: str, env: Environment | None = None):
    """Parse and evaluate a PML expression, returning the last result."""
    if env is None:
        env = create_global_env()
    tokens = Lexer(source).tokenize()
    ast = Parser(tokens).parse()
    result = None
    for expr in ast:
        result = evaluate(expr, env)
    return result, env


class TestArithmetic:
    def test_addition(self):
        result, _ = run("(+ 1 2)")
        assert result == 3

    def test_nested_arithmetic(self):
        result, _ = run("(* (+ 1 2) (- 10 3))")
        assert result == 21

    def test_subtraction_unary(self):
        result, _ = run("(- 5)")
        assert result == -5

    def test_division(self):
        result, _ = run("(/ 10 2)")
        assert result == 5.0

    def test_modulo(self):
        result, _ = run("(% 10 3)")
        assert result == 1


class TestVariables:
    def test_define_and_lookup(self):
        result, _ = run("(define x 42)\nx")
        assert result == 42

    def test_set(self):
        result, _ = run("(define x 1)\n(set! x 99)\nx")
        assert result == 99

    def test_unbound_error(self):
        with pytest.raises(UnboundVariableError):
            run("unknown-var")


class TestFunctions:
    def test_lambda(self):
        result, _ = run("((lambda (x y) (+ x y)) 3 4)")
        assert result == 7

    def test_define_function(self):
        result, _ = run("(define (square x) (* x x))\n(square 5)")
        assert result == 25

    def test_recursion(self):
        result, _ = run("""
            (define (factorial n)
              (if (= n 0) 1
                  (* n (factorial (- n 1)))))
            (factorial 10)
        """)
        assert result == 3628800

    def test_closure(self):
        result, _ = run("""
            (define (make-adder n)
              (lambda (x) (+ n x)))
            (define add5 (make-adder 5))
            (add5 10)
        """)
        assert result == 15

    def test_arity_error(self):
        with pytest.raises(ArityError):
            run("(define (f x y) (+ x y))\n(f 1)")


class TestConditionals:
    def test_if_true(self):
        result, _ = run("(if #t 1 2)")
        assert result == 1

    def test_if_false(self):
        result, _ = run("(if #f 1 2)")
        assert result == 2

    def test_cond(self):
        result, _ = run("""
            (define x 5)
            (cond
              ((> x 10) "big")
              ((> x 3)  "medium")
              (else     "small"))
        """)
        assert result == "medium"

    def test_and_short_circuit(self):
        result, _ = run("(and #t 42 #f 99)")
        assert result is False

    def test_or_short_circuit(self):
        result, _ = run("(or #f #f 42)")
        assert result == 42


class TestLet:
    def test_let_basic(self):
        result, _ = run("(let ((x 10) (y 20)) (+ x y))")
        assert result == 30

    def test_let_star(self):
        result, _ = run("(let* ((x 10) (y (* x 2))) y)")
        assert result == 20

    def test_let_scoping(self):
        """let bindings don't leak to outer scope."""
        result, _ = run("(define x 1)\n(let ((x 99)) x)\nx")
        assert result == 1


class TestLists:
    def test_list_construction(self):
        result, _ = run("(list 1 2 3)")
        assert result == [1, 2, 3]

    def test_car_cdr(self):
        result, _ = run("(car (list 10 20 30))")
        assert result == 10
        result, _ = run("(cdr (list 10 20 30))")
        assert result == [20, 30]

    def test_cons(self):
        result, _ = run("(cons 1 (list 2 3))")
        assert result == [1, 2, 3]

    def test_length(self):
        result, _ = run("(length (list 1 2 3 4))")
        assert result == 4

    def test_map(self):
        result, _ = run("(map (lambda (x) (* x x)) (list 1 2 3))")
        assert result == [1, 4, 9]

    def test_filter(self):
        result, _ = run("(filter (lambda (x) (> x 2)) (list 1 2 3 4 5))")
        assert result == [3, 4, 5]

    def test_reduce(self):
        result, _ = run("(reduce + 0 (list 1 2 3 4 5))")
        assert result == 15


class TestStrings:
    def test_string_append(self):
        result, _ = run('(string-append "hello" " " "world")')
        assert result == "hello world"

    def test_format(self):
        result, _ = run('(format "x=~a, y=~a" 10 20)')
        assert result == "x=10, y=20"

    def test_number_to_string(self):
        result, _ = run("(number->string 42)")
        assert result == "42"


class TestTypePredicates:
    def test_number_pred(self):
        result, _ = run("(number? 42)")
        assert result is True
        result, _ = run('(number? "hello")')
        assert result is False

    def test_typeof(self):
        result, _ = run("(typeof 42)")
        assert isinstance(result, Symbol)
        assert result.name == "integer"


class TestDo:
    def test_do_loop(self):
        result, _ = run("""
            (do ((i 0 (+ i 1))
                 (sum 0 (+ sum i)))
                ((>= i 10) sum))
        """)
        assert result == 45


class TestQuasiquote:
    def test_simple_quasiquote(self):
        result, _ = run("`(1 2 3)")
        assert result == [1, 2, 3]

    def test_unquote(self):
        result, _ = run("(define x 42)\n`(a ,x b)")
        lst = result
        assert isinstance(lst[0], Symbol)
        assert lst[0].name == "a"
        assert lst[1] == 42
        assert isinstance(lst[2], Symbol)
        assert lst[2].name == "b"

    def test_unquote_splicing(self):
        result, _ = run("(define xs (list 1 2 3))\n`(a ,@xs b)")
        lst = result
        assert isinstance(lst[0], Symbol)
        assert lst[0].name == "a"
        assert lst[1] == 1
        assert lst[2] == 2
        assert lst[3] == 3
        assert isinstance(lst[4], Symbol)
        assert lst[4].name == "b"
