"""List operation built-in functions."""

from __future__ import annotations

from pml.environment import Environment
from pml.errors import PMLTypeError
from pml.types import BuiltinProcedure


def _cons(a, b):
    if isinstance(b, list):
        return [a] + b
    return [a, b]


def _car(lst):
    if not isinstance(lst, list) or len(lst) == 0:
        raise PMLTypeError("car: expected non-empty list")
    return lst[0]


def _cdr(lst):
    if not isinstance(lst, list) or len(lst) == 0:
        raise PMLTypeError("cdr: expected non-empty list")
    return lst[1:]


def _lst(*args):
    return list(args)


def _length(lst):
    if not isinstance(lst, list):
        raise PMLTypeError("length: expected list")
    return len(lst)


def _append(*lsts):
    result = []
    for lst in lsts:
        if not isinstance(lst, list):
            raise PMLTypeError("append: expected list")
        result.extend(lst)
    return result


def _reverse(lst):
    if not isinstance(lst, list):
        raise PMLTypeError("reverse: expected list")
    return lst[::-1]


def _nth(lst, i):
    if not isinstance(lst, list):
        raise PMLTypeError("nth: first argument must be list")
    return lst[int(i)]


def _range(*args):
    if len(args) == 2:
        return list(range(int(args[0]), int(args[1])))
    elif len(args) == 3:
        return list(range(int(args[0]), int(args[1]), int(args[2])))
    raise PMLTypeError("range: expects 2 or 3 arguments (start end [step])")


def register(env: Environment) -> None:
    fns = {
        "cons": _cons,
        "car": _car,
        "cdr": _cdr,
        "list": _lst,
        "length": _length,
        "append": _append,
        "reverse": _reverse,
        "nth": _nth,
        "range": _range,
        "null?": lambda x: isinstance(x, list) and len(x) == 0,
        "list?": lambda x: isinstance(x, list),
    }
    for name, fn in fns.items():
        env.define(name, BuiltinProcedure(name, fn))

    # map, filter, reduce need access to evaluator — registered separately
    # via the evaluator-aware registration in the REPL/main entry point
