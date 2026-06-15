"""Comparison built-in functions."""

from __future__ import annotations

from pml.environment import Environment
from pml.types import BuiltinProcedure


def _eq(a, b) -> bool:
    """Reference equality (for symbols, booleans, numbers)."""
    return a is b or (type(a) is type(b) and a == b)


def _equal(a, b) -> bool:
    """Structural (deep) equality."""
    if type(a) is not type(b):
        return False
    if isinstance(a, list):
        if len(a) != len(b):
            return False
        return all(_equal(x, y) for x, y in zip(a, b))
    return a == b


def register(env: Environment) -> None:
    fns = {
        "=": lambda a, b: a == b,
        "<": lambda a, b: a < b,
        ">": lambda a, b: a > b,
        "<=": lambda a, b: a <= b,
        ">=": lambda a, b: a >= b,
        "eq?": _eq,
        "equal?": _equal,
    }
    for name, fn in fns.items():
        env.define(name, BuiltinProcedure(name, fn))
