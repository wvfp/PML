"""PML runtime types: Symbol, Keyword, Procedure, BuiltinProcedure."""

from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING, Any, Callable

if TYPE_CHECKING:
    from pml.environment import Environment

# AST type: nested Python lists with atomic values
Atom = int | float | str | bool | None
Expr = Atom | list["Expr"]


@dataclass(frozen=True, slots=True)
class Symbol:
    """An interned symbol (identifier) in PML."""

    name: str

    def __repr__(self) -> str:
        return f"Symbol({self.name})"

    def __str__(self) -> str:
        return self.name


@dataclass(frozen=True, slots=True)
class Keyword:
    """A keyword like :fill, :stroke. Stored without the leading colon."""

    name: str

    def __repr__(self) -> str:
        return f":{self.name}"

    def __str__(self) -> str:
        return f":{self.name}"


class Procedure:
    """A user-defined PML function (closure)."""

    __slots__ = ("params", "body", "closure_env", "name")

    def __init__(
        self,
        params: list[str],
        body: list[Expr],
        closure_env: Environment,
        name: str | None = None,
    ) -> None:
        self.params = params
        self.body = body
        self.closure_env = closure_env
        self.name = name

    def __repr__(self) -> str:
        label = self.name or "lambda"
        return f"<Procedure {label} ({' '.join(self.params)})>"


class BuiltinProcedure:
    """A built-in function implemented in Python."""

    __slots__ = ("name", "fn", "accepts_kwargs")

    def __init__(
        self,
        name: str,
        fn: Callable[..., Any],
        *,
        accepts_kwargs: bool = False,
    ) -> None:
        self.name = name
        self.fn = fn
        self.accepts_kwargs = accepts_kwargs

    def __repr__(self) -> str:
        return f"<BuiltinProcedure {self.name}>"


class Macro:
    """A user-defined PML macro (non-hygienic, expanded before evaluation)."""

    __slots__ = ("name", "params", "rest_param", "body", "closure_env")

    def __init__(
        self,
        name: str,
        params: list[str],
        body: list[Expr],
        closure_env: Environment,
        rest_param: str | None = None,
    ) -> None:
        self.name = name
        self.params = params
        self.rest_param = rest_param
        self.body = body
        self.closure_env = closure_env

    def expand(self, args: list[Expr]) -> Expr:
        """Expand the macro body by substituting parameters with unevaluated args."""
        # Build parameter bindings
        bindings: dict[str, Expr] = {}
        for i, param in enumerate(self.params):
            if i < len(args):
                bindings[param] = args[i]
            else:
                bindings[param] = []

        # Rest parameter collects remaining args — wrap in (list ...) for evaluation
        rest_names: set[str] = set()
        if self.rest_param:
            rest_args = args[len(self.params):]
            # Wrap as (list arg1 arg2 ...) so it evaluates to a PML list
            bindings[self.rest_param] = [Symbol("list")] + rest_args
            rest_names.add(self.rest_param)

        # Substitute into body (take last body expression as result)
        result: Expr = []
        for expr in self.body:
            result = _substitute(expr, bindings, rest_names)

        return result

    def __repr__(self) -> str:
        return f"<Macro {self.name} ({' '.join(self.params)})>"


def _substitute(expr: Expr, bindings: dict[str, Expr], rest_names: set[str] | None = None) -> Expr:
    """Recursively substitute symbols in an expression with bound values.

    This is a simple non-hygienic substitution — symbols matching binding
    keys are replaced with their bound expressions.
    """
    if isinstance(expr, Symbol):
        if expr.name in bindings:
            return bindings[expr.name]
        return expr

    if not isinstance(expr, list):
        return expr

    if len(expr) == 0:
        return expr

    # Handle quasiquote-like patterns within macro body:
    # (unquote x) -> substitute x
    # (unquote-splicing x) -> splice list
    head = expr[0] if isinstance(expr[0], Symbol) else None

    if head and head.name == "unquote" and len(expr) == 2:
        inner = expr[1]
        if isinstance(inner, Symbol) and inner.name in bindings:
            return bindings[inner.name]
        return _substitute(inner, bindings)

    if head and head.name == "unquote-splicing" and len(expr) == 2:
        inner = expr[1]
        if isinstance(inner, Symbol) and inner.name in bindings:
            val = bindings[inner.name]
            if isinstance(val, list):
                return val
            return [val]
        return _substitute(inner, bindings)

    # Recursively substitute all sub-expressions
    result: list[Expr] = []
    for sub in expr:
        substituted = _substitute(sub, bindings)
        # Handle splicing: if we encounter (unquote-splicing x) at list level
        if (isinstance(sub, list) and len(sub) == 2
                and isinstance(sub[0], Symbol) and sub[0].name == "unquote-splicing"
                and isinstance(substituted, list)):
            result.extend(substituted)
        else:
            result.append(substituted)
    return result
