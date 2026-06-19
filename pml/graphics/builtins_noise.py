"""PML noise texture builtins — noise-fill for organic polygon fills."""

from __future__ import annotations

from typing import Any

from pml.environment import Environment
from pml.graphics.objects import GraphicObject
from pml.transform import AffineTransform
from pml.types import BuiltinProcedure


def _noise_fill(vertices: list, **kwargs: Any) -> GraphicObject:
    """(noise-fill vertices :fill \"#color\" :intensity 0.15 :scale 12.0 :seed 42)

    Fill a polygon defined by vertices with noise-modulated color variation.
    vertices: list of (x y) pairs, e.g. (list (list 128 30) (list 238 85) ...)
    :fill — base hex color
    :intensity — noise strength 0.0-1.0 (default 0.15)
    :scale — noise grid scale (default 12.0, smaller = finer)
    :seed — random seed for reproducibility (default 0)
    """
    params: dict[str, Any] = {"vertices": vertices}
    for k in ("intensity", "scale", "seed"):
        if k in kwargs:
            params[k] = kwargs[k]

    return GraphicObject(
        shape_type="noise-fill",
        params=params,
        fill=kwargs.get("fill", "#888888"),
        opacity=kwargs.get("opacity", 1.0),
        blend_mode=kwargs.get("blend-mode", "normal"),
        transform=kwargs.get("transform", AffineTransform.identity()),
    )


def register_noise_builtins(env: Environment) -> None:
    """Register all noise texture PML builtins."""
    env.define("noise-fill", BuiltinProcedure("noise-fill", _noise_fill, accepts_kwargs=True))
