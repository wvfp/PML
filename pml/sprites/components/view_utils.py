"""Shared view constants and helpers for sprite components."""

from __future__ import annotations

from typing import Any

from pml.types import Symbol

VIEW_NAMES = ["front", "side", "back", "three-quarter"]


def sym_str(v: Any) -> str:
    """Convert Symbol / str / None to string for param validation."""
    if isinstance(v, Symbol):
        return v.name
    return str(v) if v is not None else ""


def view_width(
    base: float,
    view: str,
    *,
    front: float = 1.0,
    side: float = 0.5,
    back: float = 1.0,
    three_quarter: float = 0.7,
) -> float:
    """Scale *base* width by view-specific multiplier.

    Each component passes its own per-view multipliers as keyword args.
    """
    m = {"front": front, "side": side, "back": back, "three-quarter": three_quarter}
    return base * m.get(view, front)
