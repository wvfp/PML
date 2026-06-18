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


def _list_ref(lst, index):
    if not isinstance(lst, list):
        raise PMLTypeError("list-ref: expected list")
    if index < 0 or index >= len(lst):
        raise PMLTypeError(f"list-ref: index {index} out of range [0, {len(lst)})")
    return lst[index]


def _range(*args):
    if len(args) == 2:
        return list(range(int(args[0]), int(args[1])))
    elif len(args) == 3:
        return list(range(int(args[0]), int(args[1]), int(args[2])))
    raise PMLTypeError("range: expects 2 or 3 arguments (start end [step])")


def _list_tail(lst, k):
    if not isinstance(lst, list):
        raise PMLTypeError("list-tail: expected list")
    n = int(k)
    if n < 0 or n > len(lst):
        raise PMLTypeError(f"list-tail: index {n} out of range [0, {len(lst)}]")
    return lst[n:]


def _memq(item, lst):
    if not isinstance(lst, list):
        raise PMLTypeError("memq: second argument must be list")
    for x in lst:
        if x is item:
            return [x] + lst[lst.index(x) + 1:]
    return False


def _assoc(key, alist):
    if not isinstance(alist, list):
        raise PMLTypeError("assoc: second argument must be list")
    for pair in alist:
        if isinstance(pair, list) and len(pair) > 0 and pair[0] == key:
            return pair
    return False


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
        "list-ref": _list_ref,
        "list-tail": _list_tail,
        "range": _range,
        "null?": lambda x: isinstance(x, list) and len(x) == 0,
        "list?": lambda x: isinstance(x, list),
        "memq": _memq,
        "assoc": _assoc,
    }
    for name, fn in fns.items():
        env.define(name, BuiltinProcedure(name, fn))

    # map, filter, reduce need access to evaluator — registered separately
    # via the evaluator-aware registration in the REPL/main entry point
