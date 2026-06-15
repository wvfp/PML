"""PML animation interpolation — type-aware value interpolation.

Supports numeric lerp, color interpolation (RGB space),
AffineTransform interpolation, and dict-level interpolation.
"""

from __future__ import annotations

import math
from typing import Any

from pml.transform import AffineTransform


def lerp_numeric(a: float, b: float, t: float) -> float:
    """Linear interpolation between two numbers."""
    return a + (b - a) * t


def lerp_color(a: str, b: str, t: float) -> str:
    """Interpolate two color strings in RGB space.

    Uses the Pillow backend's parse_color for consistency.
    Returns a hex color string like '#rrggbb' or '#rrggbbaa'.
    """
    from pml.backend.pillow import parse_color

    rgba_a = parse_color(a)
    rgba_b = parse_color(b)

    if rgba_a is None:
        rgba_a = (0, 0, 0, 0)
    if rgba_b is None:
        rgba_b = (0, 0, 0, 0)

    r = int(rgba_a[0] + (rgba_b[0] - rgba_a[0]) * t)
    g = int(rgba_a[1] + (rgba_b[1] - rgba_a[1]) * t)
    b_ch = int(rgba_a[2] + (rgba_b[2] - rgba_a[2]) * t)
    alpha = int(rgba_a[3] + (rgba_b[3] - rgba_a[3]) * t)

    # Clamp to valid range
    r = max(0, min(255, r))
    g = max(0, min(255, g))
    b_ch = max(0, min(255, b_ch))
    alpha = max(0, min(255, alpha))

    if alpha == 255:
        return f"#{r:02x}{g:02x}{b_ch:02x}"
    return f"#{r:02x}{g:02x}{b_ch:02x}{alpha:02x}"


def lerp_transform(a: AffineTransform, b: AffineTransform, t: float) -> AffineTransform:
    """Interpolate two affine transforms component-wise.

    For pure translation/scale, component-wise lerp works well.
    For rotation, we extract the angle and interpolate it separately
    to avoid non-orthogonal intermediate matrices.
    """
    # Decompose: extract rotation angle from each transform
    angle_a = math.atan2(a.b, a.a)
    angle_b = math.atan2(b.b, b.a)

    # Shortest path rotation interpolation
    diff = angle_b - angle_a
    if diff > math.pi:
        diff -= 2 * math.pi
    elif diff < -math.pi:
        diff += 2 * math.pi
    angle = angle_a + diff * t

    # Interpolate scale (extracted from matrix)
    scale_a = math.sqrt(a.a * a.a + a.b * a.b)
    scale_b = math.sqrt(b.a * b.a + b.b * b.b)
    scale = scale_a + (scale_b - scale_a) * t

    # Handle negative scale
    if a.a < 0:
        scale_a = -scale_a
    if b.a < 0:
        scale_b = -scale_b

    # Interpolate translation directly
    tx = lerp_numeric(a.e, b.e, t)
    ty = lerp_numeric(a.f, b.f, t)

    # Reconstruct matrix
    cos_a = math.cos(angle) * scale
    sin_a = math.sin(angle) * scale

    return AffineTransform(
        a=cos_a,
        b=sin_a,
        c=-sin_a,
        d=cos_a,
        e=tx,
        f=ty,
    )


def lerp_value(a: Any, b: Any, t: float) -> Any:
    """Type-aware interpolation between two values.

    - int/float → numeric lerp
    - str → color lerp (assumes color strings)
    - AffineTransform → transform lerp
    - bool → discrete (switch at t=0.5)
    - other → discrete (return a if t < 0.5, else b)
    """
    # bool must be checked BEFORE int/float (bool is subclass of int)
    if isinstance(a, bool) and isinstance(b, bool):
        return b if t >= 0.5 else a

    if isinstance(a, (int, float)) and isinstance(b, (int, float)):
        result = lerp_numeric(float(a), float(b), t)
        # Preserve int type if both are ints
        if isinstance(a, int) and isinstance(b, int):
            return int(round(result))
        return result

    if isinstance(a, str) and isinstance(b, str):
        return lerp_color(a, b, t)

    if isinstance(a, AffineTransform) and isinstance(b, AffineTransform):
        return lerp_transform(a, b, t)

    # Discrete: no interpolation possible
    return b if t >= 0.5 else a
