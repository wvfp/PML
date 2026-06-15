"""PML 2D affine transform matrix.

Represented as a 6-element vector (a, b, c, d, e, f):

    | a  c  e |     x' = a*x + c*y + e
    | b  d  f |     y' = b*x + d*y + f
    | 0  0  1 |
"""

from __future__ import annotations

import math
from dataclasses import dataclass


@dataclass(frozen=True, slots=True)
class AffineTransform:
    """Immutable 2D affine transformation matrix."""

    a: float = 1.0
    b: float = 0.0
    c: float = 0.0
    d: float = 1.0
    e: float = 0.0
    f: float = 0.0

    @staticmethod
    def identity() -> AffineTransform:
        return AffineTransform(1, 0, 0, 1, 0, 0)

    @staticmethod
    def translate(dx: float, dy: float) -> AffineTransform:
        return AffineTransform(1, 0, 0, 1, dx, dy)

    @staticmethod
    def rotate(angle: float) -> AffineTransform:
        c = math.cos(angle)
        s = math.sin(angle)
        return AffineTransform(c, s, -s, c, 0, 0)

    @staticmethod
    def scale(sx: float, sy: float | None = None) -> AffineTransform:
        if sy is None:
            sy = sx
        return AffineTransform(sx, 0, 0, sy, 0, 0)

    @staticmethod
    def shear(shx: float, shy: float) -> AffineTransform:
        return AffineTransform(1, shy, shx, 1, 0, 0)

    def compose(self, other: AffineTransform) -> AffineTransform:
        """Return self · other (apply other first, then self)."""
        return AffineTransform(
            a=self.a * other.a + self.c * other.b,
            b=self.b * other.a + self.d * other.b,
            c=self.a * other.c + self.c * other.d,
            d=self.b * other.c + self.d * other.d,
            e=self.a * other.e + self.c * other.f + self.e,
            f=self.b * other.e + self.d * other.f + self.f,
        )

    def inverse(self) -> AffineTransform:
        """Return the inverse matrix. Raises ValueError if singular."""
        det = self.a * self.d - self.b * self.c
        if abs(det) < 1e-12:
            raise ValueError("Singular matrix — cannot invert")
        inv_det = 1.0 / det
        a = self.d * inv_det
        b = -self.b * inv_det
        c = -self.c * inv_det
        d = self.a * inv_det
        e = (self.c * self.f - self.d * self.e) * inv_det
        f = (self.b * self.e - self.a * self.f) * inv_det
        return AffineTransform(a, b, c, d, e, f)

    def apply(self, x: float, y: float) -> tuple[float, float]:
        """Apply transform to point (x, y) → (x', y')."""
        return (
            self.a * x + self.c * y + self.e,
            self.b * x + self.d * y + self.f,
        )

    def apply_points(self, points: list[tuple[float, float]]) -> list[tuple[float, float]]:
        """Apply transform to a list of points."""
        return [self.apply(x, y) for x, y in points]

    def to_pillow(self) -> tuple[float, float, float, float, float, float]:
        """Convert to Pillow's affine_transform format (a, b, c, d, e, f).

        Note: Pillow uses a DIFFERENT convention for Image.affine_transform:
        it expects the INVERSE matrix as (a, b, c, d, e, f) such that
        x_src = a*x_dst + b*y_dst + c, y_src = d*x_dst + e*y_dst + f.
        So this returns the inverse matrix coefficients.
        """
        inv = self.inverse()
        return (inv.a, inv.b, inv.e, inv.c, inv.d, inv.f)

    def is_identity(self) -> bool:
        return (
            abs(self.a - 1) < 1e-9
            and abs(self.b) < 1e-9
            and abs(self.c) < 1e-9
            and abs(self.d - 1) < 1e-9
            and abs(self.e) < 1e-9
            and abs(self.f) < 1e-9
        )

    def to_list(self) -> list[float]:
        """Convert to PML list representation [a, b, c, d, e, f]."""
        return [self.a, self.b, self.c, self.d, self.e, self.f]

    def __repr__(self) -> str:
        return f"<Matrix ({self.a:.3f} {self.b:.3f} {self.c:.3f} {self.d:.3f} {self.e:.3f} {self.f:.3f})>"
