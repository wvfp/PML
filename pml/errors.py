"""PML exception types."""

from __future__ import annotations


class PMLError(Exception):
    """Base exception for all PML errors."""

    def __init__(
        self,
        message: str,
        *,
        line: int | None = None,
        column: int | None = None,
        filename: str | None = None,
    ) -> None:
        self.pml_message = message
        self.line = line
        self.column = column
        self.filename = filename
        super().__init__(self._format())

    def _format(self) -> str:
        parts = [self.pml_message]
        loc = self._location()
        if loc:
            parts.insert(0, loc)
        return " — ".join(parts)

    def _location(self) -> str | None:
        if self.filename and self.line:
            return f"{self.filename}:{self.line}:{self.column or 0}"
        if self.line:
            return f"line {self.line}:{self.column or 0}"
        return None


class PMLSyntaxError(PMLError):
    """Lexer or parser syntax error (unmatched parens, illegal atom, etc.)."""


class PMLTypeError(PMLError):
    """Argument type or arity mismatch."""


class UnboundVariableError(PMLError):
    """Reference to a variable that doesn't exist in any scope."""


class ArityError(PMLError):
    """Wrong number of arguments passed to a function."""


class PMLAssertionError(PMLError):
    """Assertion failure from (assert ...)."""


class ImportError_(PMLError):
    """Module import error (file not found, circular dependency)."""


class CircularImportError(ImportError_):
    """Circular dependency detected during module loading."""


class MacroExpansionDepthError(PMLError):
    """Macro expansion exceeded maximum recursion depth."""


class AccessError(PMLError):
    """Attempt to access a non-exported module symbol."""


class ResourceError(PMLError):
    """Resource (image, font, etc.) not found or cannot be loaded."""


class IKNoSolutionError(PMLError):
    """IK solver failed to converge within maximum iterations."""
