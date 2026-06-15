"""Type predicate built-in functions."""

from __future__ import annotations

from pml.environment import Environment
from pml.types import BuiltinProcedure, Keyword, Procedure, Symbol


def register(env: Environment) -> None:
    fns = {
        "number?": lambda x: isinstance(x, (int, float)) and not isinstance(x, bool),
        "string?": lambda x: isinstance(x, str),
        "symbol?": lambda x: isinstance(x, Symbol),
        "boolean?": lambda x: isinstance(x, bool),
        "procedure?": lambda x: isinstance(x, (Procedure, BuiltinProcedure)) or callable(x),
        "keyword?": lambda x: isinstance(x, Keyword),
    }
    for name, fn in fns.items():
        env.define(name, BuiltinProcedure(name, fn))
