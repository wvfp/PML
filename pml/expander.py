"""PML macro expander — pre-evaluation pass that expands macro calls."""

from __future__ import annotations

from typing import Any

from pml.environment import Environment
from pml.errors import MacroExpansionDepthError
from pml.types import Expr, Macro, Symbol


class Expander:
    """Macro expander that runs before evaluation.

    Recursively expands macro calls in the AST. Non-hygienic:
    macro authors must use (gensym) for unique variable names.

    The expansion algorithm:
    1. If expr is not a list or is empty → return as-is
    2. If head is a Symbol resolving to a Macro → expand and re-expand result
    3. Otherwise → recursively expand all sub-expressions
    """

    def __init__(self, env: Environment, max_depth: int = 256) -> None:
        self.env = env
        self.max_depth = max_depth

    def expand(self, expr: Expr, depth: int = 0) -> Expr:
        """Expand a single expression, recursively expanding macro calls."""
        if depth > self.max_depth:
            raise MacroExpansionDepthError(
                f"Macro expansion exceeded maximum depth of {self.max_depth}"
            )

        # Atoms and empty lists pass through unchanged
        if not isinstance(expr, list) or len(expr) == 0:
            return expr

        head = expr[0]

        # Check if head is a macro
        if isinstance(head, Symbol):
            val = self.env.try_lookup(head.name)
            if isinstance(val, Macro):
                # Expand: substitute unevaluated args into macro body
                expanded = val.expand(expr[1:])
                # Re-expand the result (macros can produce more macro calls)
                return self.expand(expanded, depth + 1)

        # Not a macro call: recursively expand sub-expressions
        # But preserve special forms — don't expand inside quote/quasiquote
        if isinstance(head, Symbol):
            if head.name == "quote":
                return expr  # Don't expand inside quotes
            if head.name in ("lambda", "defmacro"):
                # Expand body but not parameter list
                expanded_body = [self.expand(sub, depth) for sub in expr[2:]]
                return [expr[0], expr[1]] + expanded_body
            if head.name in ("define",):
                # For (define (name params) body...), expand body only
                if len(expr) >= 3 and isinstance(expr[1], list):
                    expanded_body = [self.expand(sub, depth) for sub in expr[2:]]
                    return [expr[0], expr[1]] + expanded_body

        # Default: recursively expand all sub-expressions
        return [self.expand(sub, depth) for sub in expr]

    def expand_all(self, ast: list[Expr]) -> list[Expr]:
        """Expand macros in a list of top-level expressions."""
        return [self.expand(expr) for expr in ast]
