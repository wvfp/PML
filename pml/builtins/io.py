"""IO and debugging built-in functions."""

from __future__ import annotations

from pml.environment import Environment
from pml.errors import PMLAssertionError, PMLError, PMLTypeError
from pml.types import BuiltinProcedure, Keyword, Procedure, Symbol


def _pml_repr(value) -> str:
    """Convert a PML value to its display representation."""
    if value is True:
        return "#t"
    if value is False:
        return "#f"
    if value is None:
        return "()"
    if isinstance(value, str):
        return f'"{value}"'
    if isinstance(value, Symbol):
        return value.name
    if isinstance(value, Keyword):
        return str(value)
    if isinstance(value, list):
        inner = " ".join(_pml_repr(v) for v in value)
        return f"({inner})"
    if isinstance(value, (Procedure, BuiltinProcedure)):
        return repr(value)
    # Lazy imports for optional subsystems
    try:
        from pml.graphics.objects import GraphicObject
        if isinstance(value, GraphicObject):
            return repr(value)
    except ImportError:
        pass
    try:
        from pml.transform import AffineTransform
        if isinstance(value, AffineTransform):
            return repr(value)
    except ImportError:
        pass
    try:
        from pml.graphics.canvas import Canvas
        if isinstance(value, Canvas):
            return f"<Canvas {value.width}x{value.height}>"
    except ImportError:
        pass
    try:
        from pml.sprites.style import StyleDescriptor
        if isinstance(value, StyleDescriptor):
            return f"<Style {value.shading}>"
    except ImportError:
        pass
    return str(value)


def _print(*args) -> None:
    print(" ".join(_pml_repr(a) for a in args), end="")
    return None


def _println(*args) -> None:
    print(" ".join(_pml_repr(a) for a in args))
    return None


def _error(msg: str):
    raise PMLError(str(msg))


def _assert(cond, msg: str = "Assertion failed"):
    if not cond:
        raise PMLAssertionError(msg)
    return None


def _typeof(value) -> Symbol:
    if isinstance(value, bool):
        return Symbol("boolean")
    if isinstance(value, int):
        return Symbol("integer")
    if isinstance(value, float):
        return Symbol("float")
    if isinstance(value, str):
        return Symbol("string")
    if isinstance(value, Symbol):
        return Symbol("symbol")
    if isinstance(value, Keyword):
        return Symbol("keyword")
    if isinstance(value, list):
        return Symbol("list")
    if isinstance(value, (Procedure, BuiltinProcedure)):
        return Symbol("procedure")
    if value is None:
        return Symbol("nil")
    # Optional subsystem types
    try:
        from pml.graphics.objects import GraphicObject
        if isinstance(value, GraphicObject):
            return Symbol("graphic-object")
    except ImportError:
        pass
    try:
        from pml.transform import AffineTransform
        if isinstance(value, AffineTransform):
            return Symbol("matrix")
    except ImportError:
        pass
    try:
        from pml.graphics.canvas import Canvas
        if isinstance(value, Canvas):
            return Symbol("canvas")
    except ImportError:
        pass
    return Symbol("unknown")


def register(env: Environment) -> None:
    fns = {
        "print": _print,
        "println": _println,
        "error": _error,
        "assert": _assert,
        "typeof": _typeof,
    }
    for name, fn in fns.items():
        env.define(name, BuiltinProcedure(name, fn))
