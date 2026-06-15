"""PML animation easing functions.

Each easing function maps t ∈ [0, 1] → [0, 1], where:
- t=0 → start
- t=1 → end

Matching the spec in development.md section 3.4.1.
"""

from __future__ import annotations

import math
from typing import Callable


def _linear(t: float) -> float:
    return t


def _quad_in(t: float) -> float:
    return t * t


def _quad_out(t: float) -> float:
    return t * (2 - t)


def _quad_in_out(t: float) -> float:
    if t < 0.5:
        return 2 * t * t
    return -1 + (4 - 2 * t) * t


def _cubic_in(t: float) -> float:
    return t ** 3


def _cubic_out(t: float) -> float:
    return (t - 1) ** 3 + 1


def _cubic_in_out(t: float) -> float:
    if t < 0.5:
        return 4 * t ** 3
    return (2 * t - 2) ** 3 / 2 + 1


def _sin_in(t: float) -> float:
    return 1 - math.cos(t * math.pi / 2)


def _sin_out(t: float) -> float:
    return math.sin(t * math.pi / 2)


def _sin_in_out(t: float) -> float:
    return -(math.cos(math.pi * t) - 1) / 2


def _bounce(t: float) -> float:
    """Bounce easing — simulates a ball bouncing."""
    if t < 1 / 2.75:
        return 7.5625 * t * t
    elif t < 2 / 2.75:
        t -= 1.5 / 2.75
        return 7.5625 * t * t + 0.75
    elif t < 2.5 / 2.75:
        t -= 2.25 / 2.75
        return 7.5625 * t * t + 0.9375
    else:
        t -= 2.625 / 2.75
        return 7.5625 * t * t + 0.984375


def _elastic(t: float) -> float:
    """Elastic easing — spring-like overshoot."""
    if t == 0 or t == 1:
        return t
    p = 0.3
    s = p / 4
    return 2 ** (-10 * t) * math.sin((t - s) * (2 * math.pi) / p) + 1


EASING_FUNCTIONS: dict[str, Callable[[float], float]] = {
    "linear": _linear,
    "quad-in": _quad_in,
    "quad-out": _quad_out,
    "quad-in-out": _quad_in_out,
    "cubic-in": _cubic_in,
    "cubic-out": _cubic_out,
    "cubic-in-out": _cubic_in_out,
    "sin-in": _sin_in,
    "sin-out": _sin_out,
    "sin-in-out": _sin_in_out,
    "bounce": _bounce,
    "elastic": _elastic,
}


def get_easing(name: str) -> Callable[[float], float]:
    """Return an easing function by name.

    Falls back to 'linear' if the name is not recognized.
    Accepts both Symbol objects (with .name) and plain strings.
    """
    if hasattr(name, "name"):
        name = name.name
    return EASING_FUNCTIONS.get(str(name), _linear)


def list_easings() -> list[str]:
    """Return all available easing function names."""
    return sorted(EASING_FUNCTIONS.keys())
