"""String operation built-in functions."""

from __future__ import annotations

from pml.environment import Environment
from pml.errors import PMLTypeError
from pml.types import BuiltinProcedure


def _string_append(*args: str) -> str:
    return "".join(str(a) for a in args)


def _string_length(s: str) -> int:
    if not isinstance(s, str):
        raise PMLTypeError("string-length: expected string")
    return len(s)


def _substring(s: str, start: int, end: int) -> str:
    if not isinstance(s, str):
        raise PMLTypeError("substring: expected string")
    return s[int(start) : int(end)]


def _string_ref(s: str, i: int) -> str:
    if not isinstance(s, str):
        raise PMLTypeError("string-ref: expected string")
    return s[int(i)]


def _number_to_string(n) -> str:
    return str(n)


def _string_to_number(s: str):
    if not isinstance(s, str):
        raise PMLTypeError("string->number: expected string")
    try:
        if "." in s:
            return float(s)
        return int(s)
    except ValueError:
        raise PMLTypeError(f"string->number: cannot convert {s!r}")


def _format(fmt: str, *args) -> str:
    """Simple format: ~a is replaced by args in order."""
    result = fmt
    for arg in args:
        result = result.replace("~a", str(arg), 1)
    return result


def register(env: Environment) -> None:
    fns = {
        "string-append": _string_append,
        "string-length": _string_length,
        "substring": _substring,
        "string-ref": _string_ref,
        "number->string": _number_to_string,
        "string->number": _string_to_number,
        "format": _format,
    }
    for name, fn in fns.items():
        env.define(name, BuiltinProcedure(name, fn))
