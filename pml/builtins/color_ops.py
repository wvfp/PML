"""Color manipulation builtins — alpha, blend, lighten, desaturate."""

from __future__ import annotations

import colorsys
import re

from pml.backend.pillow import parse_color
from pml.environment import Environment
from pml.types import BuiltinProcedure


_HEX_RGB = re.compile(r"^#([0-9a-f]{2})([0-9a-f]{2})([0-9a-f]{2})", re.I)
_HEX_RGBA = re.compile(r"^#([0-9a-f]{2})([0-9a-f]{2})([0-9a-f]{2})([0-9a-f]{2})", re.I)


def _to_rgba(color: str) -> tuple[int, int, int, int]:
    """Parse any color string to (r,g,b,a) 0-255."""
    parsed = parse_color(color)
    if parsed is None:
        raise ValueError(f"color-alpha: cannot parse color '{color}'")
    return parsed


def _rgba_to_hex(r: int, g: int, b: int, a: int) -> str:
    """Convert (r,g,b,a) to #RRGGBBAA hex string."""
    return f"#{r:02x}{g:02x}{b:02x}{a:02x}"


def _color_alpha(color: str, alpha: float) -> str:
    """(color-alpha \"#ff8833\" 0.5) → \"#ff883380\" — set alpha channel."""
    r, g, b, a = _to_rgba(color)
    new_a = max(0, min(255, round(a * alpha)))
    return _rgba_to_hex(r, g, b, new_a)


def _color_blend(color1: str, color2: str, t: float) -> str:
    """(color-blend \"#ff0000\" \"#0000ff\" 0.5) → blend at ratio t."""
    r1, g1, b1, a1 = _to_rgba(color1)
    r2, g2, b2, a2 = _to_rgba(color2)
    t = max(0.0, min(1.0, t))
    r = round(r1 + (r2 - r1) * t)
    g = round(g1 + (g2 - g1) * t)
    b = round(b1 + (b2 - b1) * t)
    a = round(a1 + (a2 - a1) * t)
    return _rgba_to_hex(r, g, b, a)


def _color_lighten(color: str, amount: float) -> str:
    """(color-lighten \"#cc4444\" 30) → lighten by amount (HSB V channel)."""
    r, g, b, a = _to_rgba(color)
    h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
    v = max(0.0, min(1.0, v + amount / 255))
    r2, g2, b2 = colorsys.hsv_to_rgb(h, s, v)
    return _rgba_to_hex(round(r2 * 255), round(g2 * 255), round(b2 * 255), a)


def _color_darken(color: str, amount: float) -> str:
    """(color-darken \"#cc4444\" 30) → darken by amount (HSB V channel)."""
    return _color_lighten(color, -amount)


def _color_desaturate(color: str, amount: float) -> str:
    """(color-desaturate \"#ff8833\" 30) → reduce saturation by amount."""
    r, g, b, a = _to_rgba(color)
    h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
    s = max(0.0, min(1.0, s - amount / 100))
    r2, g2, b2 = colorsys.hsv_to_rgb(h, s, v)
    return _rgba_to_hex(round(r2 * 255), round(g2 * 255), round(b2 * 255), a)


def register_color_ops(env: Environment) -> None:
    """Register color manipulation builtins."""
    fns = {
        "color-alpha": _color_alpha,
        "color-blend": _color_blend,
        "color-lighten": _color_lighten,
        "color-darken": _color_darken,
        "color-desaturate": _color_desaturate,
    }
    for name, fn in fns.items():
        env.define(name, BuiltinProcedure(name, fn))
