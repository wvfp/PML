"""Transform built-in functions — translate, rotate, scale, etc."""

from __future__ import annotations

from typing import Any

from pml.environment import Environment
from pml.errors import PMLTypeError
from pml.transform import AffineTransform
from pml.types import BuiltinProcedure


def _translate(dx: float, dy: float) -> AffineTransform:
    return AffineTransform.translate(dx, dy)


def _rotate(angle: float) -> AffineTransform:
    return AffineTransform.rotate(angle)


def _scale(sx: float, sy: float | None = None) -> AffineTransform:
    if sy is None:
        return AffineTransform.scale(sx)
    return AffineTransform.scale(sx, sy)


def _shear(shx: float, shy: float) -> AffineTransform:
    return AffineTransform.shear(shx, shy)


def _compose(m1: Any, m2: Any) -> AffineTransform:
    if not isinstance(m1, AffineTransform) or not isinstance(m2, AffineTransform):
        raise PMLTypeError("compose: both arguments must be AffineTransform")
    return m1.compose(m2)


def _transform(a: float, b: float, c: float, d: float, e: float, f: float) -> AffineTransform:
    return AffineTransform(a, b, c, d, e, f)


def _identity_matrix() -> AffineTransform:
    return AffineTransform.identity()


def _matrix_inverse(m: Any) -> AffineTransform:
    if not isinstance(m, AffineTransform):
        raise PMLTypeError("matrix-inverse: expected AffineTransform")
    return m.inverse()


def _matrix_apply(m: Any, x: float, y: float) -> list[float]:
    if not isinstance(m, AffineTransform):
        raise PMLTypeError("matrix-apply: expected AffineTransform")
    rx, ry = m.apply(x, y)
    return [rx, ry]


def register_transforms(env: Environment) -> None:
    """Register transform-related builtins."""
    fns = {
        "translate": BuiltinProcedure("translate", _translate),
        "rotate": BuiltinProcedure("rotate", _rotate),
        "scale": BuiltinProcedure("scale", _scale),
        "shear": BuiltinProcedure("shear", _shear),
        "compose": BuiltinProcedure("compose", _compose),
        "transform": BuiltinProcedure("transform", _transform),
        "identity-matrix": BuiltinProcedure("identity-matrix", _identity_matrix),
        "matrix-inverse": BuiltinProcedure("matrix-inverse", _matrix_inverse),
        "matrix-apply": BuiltinProcedure("matrix-apply", _matrix_apply),
        "matrix?": BuiltinProcedure("matrix?", lambda x: isinstance(x, AffineTransform)),
    }
    for name, proc in fns.items():
        env.define(name, proc)
