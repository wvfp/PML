"""Arithmetic built-in functions."""

from __future__ import annotations

import math
import random as _random

from pml.environment import Environment
from pml.types import BuiltinProcedure


def _add(*args: float) -> float:
    return sum(args)


def _sub(*args: float) -> float:
    if len(args) == 0:
        raise TypeError("- requires at least 1 argument")
    if len(args) == 1:
        return -args[0]
    return args[0] - sum(args[1:])


def _mul(*args: float) -> float:
    result = 1
    for a in args:
        result *= a
    return result


def _div(*args: float) -> float:
    if len(args) == 0:
        raise TypeError("/ requires at least 1 argument")
    if len(args) == 1:
        return 1.0 / args[0]
    result = args[0]
    for a in args[1:]:
        result /= a
    return result


def _mod(a: float, b: float) -> float:
    return a % b


def register(env: Environment) -> None:
    """Register arithmetic functions."""
    fns = {
        "+": _add,
        "-": _sub,
        "*": _mul,
        "/": _div,
        "%": _mod,
        "abs": abs,
        "min": min,
        "max": max,
        "floor": lambda x: math.floor(x),
        "ceil": lambda x: math.ceil(x),
        "round": lambda x: round(x),
        "sqrt": math.sqrt,
        "pow": pow,
        "sin": math.sin,
        "cos": math.cos,
        "tan": math.tan,
        "atan2": math.atan2,
        # LLM-friendly Scheme-standard arithmetic
        "quotient": lambda a, b: int(a / b),
        "remainder": lambda a, b: int(a % b),
        "modulo": lambda a, b: int(((a % b) + b) % b),
        "even?": lambda a: a % 2 == 0,
        "odd?": lambda a: a % 2 != 0,
        "clamp": lambda x, lo, hi: max(min(x, hi), lo),
        "random": lambda: _random.random(),
        "randint": lambda a, b: _random.randint(int(a), int(b)),
    }
    for name, fn in fns.items():
        env.define(name, BuiltinProcedure(name, fn))
