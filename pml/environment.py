"""PML environment — scoped symbol table with lexical scoping."""

from __future__ import annotations

from typing import Any

from pml.errors import UnboundVariableError


class Environment:
    """A lexical environment mapping symbols to values, with parent chain."""

    __slots__ = ("parent", "bindings", "exports")

    def __init__(
        self,
        parent: Environment | None = None,
        bindings: dict[str, Any] | None = None,
    ) -> None:
        self.parent = parent
        self.bindings: dict[str, Any] = bindings or {}
        self.exports: set[str] = set()

    def lookup(self, name: str) -> Any:
        """Look up a symbol, searching outward through parent scopes."""
        if name in self.bindings:
            return self.bindings[name]
        if self.parent is not None:
            return self.parent.lookup(name)
        raise UnboundVariableError(f"Unbound variable: {name}")

    def define(self, name: str, value: Any) -> None:
        """Define (or shadow) a binding in the current scope."""
        self.bindings[name] = value

    def set(self, name: str, value: Any) -> None:
        """Mutate an existing binding, searching outward through parent scopes."""
        if name in self.bindings:
            self.bindings[name] = value
            return
        if self.parent is not None:
            self.parent.set(name, value)
            return
        raise UnboundVariableError(f"Cannot set! unbound variable: {name}")

    def extend(self, names: list[str], values: list[Any]) -> Environment:
        """Create a child environment with new bindings (for function calls)."""
        child = Environment(parent=self)
        for name, value in zip(names, values):
            child.define(name, value)
        return child

    def try_lookup(self, name: str) -> Any | None:
        """Look up a symbol, returning None if not found (no error)."""
        try:
            return self.lookup(name)
        except UnboundVariableError:
            return None

    def __repr__(self) -> str:
        keys = list(self.bindings.keys())
        return f"<Environment bindings={keys}>"
