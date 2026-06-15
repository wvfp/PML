"""PML built-in functions — registration entry point."""

from __future__ import annotations

from pml.environment import Environment

from pml.builtins import (
    arithmetic,
    comparison,
    io,
    list_ops,
    string_ops,
    type_predicates,
)


def register_all(env: Environment) -> None:
    """Register all built-in functions into the given environment."""
    arithmetic.register(env)
    comparison.register(env)
    list_ops.register(env)
    string_ops.register(env)
    type_predicates.register(env)
    io.register(env)
