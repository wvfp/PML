"""PML sketch/hand-drawn shape builtins — pencil, watercolor, hatch, charcoal."""

from __future__ import annotations

from typing import Any

from pml.environment import Environment
from pml.graphics.objects import GraphicObject
from pml.transform import AffineTransform
from pml.types import BuiltinProcedure


def _extract_transform(kwargs: dict[str, Any]) -> AffineTransform:
    t = kwargs.get("transform")
    if t is None:
        return AffineTransform.identity()
    if isinstance(t, AffineTransform):
        return t
    return AffineTransform.identity()


def _extract_blend_mode(kwargs: dict[str, Any]) -> str:
    return kwargs.get("blend-mode", "normal")


def _extract_opacity(kwargs: dict[str, Any]) -> float:
    return kwargs.get("opacity", 1.0)


def _pencil(x1: float, y1: float, x2: float, y2: float, **kwargs: Any) -> GraphicObject:
    """(pencil x1 y1 x2 y2 :stroke \"#333\" :stroke-width 2 :roughness 0.3 :variance 0.4)

    Hand-drawn line with positional wobble and variable-width stroke.
    Higher :roughness = more wobble; higher :variance = more width fluctuation.
    """
    params: dict[str, Any] = {
        "x1": x1, "y1": y1, "x2": x2, "y2": y2,
    }
    for k in ("roughness", "variance", "seed"):
        if k in kwargs:
            params[k] = kwargs[k]

    return GraphicObject(
        shape_type="pencil",
        params=params,
        stroke=kwargs.get("stroke", "#000000"),
        stroke_width=kwargs.get("stroke-width", 2.0),
        opacity=_extract_opacity(kwargs),
        blend_mode=_extract_blend_mode(kwargs),
        transform=_extract_transform(kwargs),
    )


def _charcoal(x1: float, y1: float, x2: float, y2: float, **kwargs: Any) -> GraphicObject:
    """(charcoal x1 y1 x2 y2 :stroke \"#222\" :stroke-width 4 :roughness 0.5 :scatter 0.3)

    Rough charcoal stroke with particle scatter.
    """
    params: dict[str, Any] = {
        "x1": x1, "y1": y1, "x2": x2, "y2": y2,
    }
    for k in ("roughness", "scatter", "seed"):
        if k in kwargs:
            params[k] = kwargs[k]

    return GraphicObject(
        shape_type="charcoal",
        params=params,
        stroke=kwargs.get("stroke", "#222222"),
        stroke_width=kwargs.get("stroke-width", 4.0),
        opacity=_extract_opacity(kwargs),
        blend_mode=_extract_blend_mode(kwargs),
        transform=_extract_transform(kwargs),
    )


def _watercolor_rect(x: float, y: float, w: float, h: float, **kwargs: Any) -> GraphicObject:
    """(watercolor-rect x y w h :fill \"#e74c3c\" :bleed 0.3 :layers 3)

    Rectangle with watercolor edge bleeding (noise-distorted edges,
    multiple translucent layers, color variation).
    """
    params: dict[str, Any] = {"x": x, "y": y, "w": w, "h": h}
    for k in ("bleed", "layers", "seed"):
        if k in kwargs:
            params[k] = kwargs[k]

    return GraphicObject(
        shape_type="watercolor-rect",
        params=params,
        fill=kwargs.get("fill", "#cc6655"),
        opacity=_extract_opacity(kwargs),
        blend_mode=_extract_blend_mode(kwargs),
        transform=_extract_transform(kwargs),
    )


def _watercolor_circle(cx: float, cy: float, r: float, **kwargs: Any) -> GraphicObject:
    """(watercolor-circle cx cy r :fill \"#e74c3c\" :bleed 0.3 :layers 3)

    Circle with watercolor edge bleeding.
    """
    params: dict[str, Any] = {"cx": cx, "cy": cy, "r": r}
    for k in ("bleed", "layers", "seed"):
        if k in kwargs:
            params[k] = kwargs[k]

    return GraphicObject(
        shape_type="watercolor-circle",
        params=params,
        fill=kwargs.get("fill", "#cc6655"),
        opacity=_extract_opacity(kwargs),
        blend_mode=_extract_blend_mode(kwargs),
        transform=_extract_transform(kwargs),
    )


def _hatch(x: float, y: float, w: float, h: float, **kwargs: Any) -> GraphicObject:
    """(hatch x y w h :stroke \"#333\" :density 0.4 :angle 45 :cross #f)

    Hatching/cross-hatching fill within a bounding box.
    :density controls line spacing (0=sparse, 1=dense).
    :cross adds perpendicular cross-hatching when true.
    """
    params: dict[str, Any] = {"x": x, "y": y, "w": w, "h": h}
    for k in ("density", "angle", "cross", "cross-angle", "seed"):
        if k in kwargs:
            params[k] = kwargs[k]

    return GraphicObject(
        shape_type="hatch",
        params=params,
        stroke=kwargs.get("stroke", "#333333"),
        stroke_width=kwargs.get("stroke-width", 1.0),
        opacity=_extract_opacity(kwargs),
        blend_mode=_extract_blend_mode(kwargs),
        transform=_extract_transform(kwargs),
    )


def _sketchify(shape: GraphicObject, **kwargs: Any) -> GraphicObject:
    """(sketchify shape :roughness 0.2 :seed 42)

    Apply sketch distortion to any existing GraphicObject.
    Renders the shape normally, then warps it with noise.
    """
    params: dict[str, Any] = {"base-shape": shape}
    for k in ("roughness", "seed"):
        if k in kwargs:
            params[k] = kwargs[k]

    return GraphicObject(
        shape_type="sketchify",
        params=params,
        opacity=_extract_opacity(kwargs),
        blend_mode=_extract_blend_mode(kwargs),
        transform=_extract_transform(kwargs),
    )


def register_sketch_builtins(env: Environment) -> None:
    """Register all hand-drawn / sketch PML builtins."""
    sketch_fns = {
        "pencil": _pencil,
        "charcoal": _charcoal,
        "watercolor-rect": _watercolor_rect,
        "watercolor-circle": _watercolor_circle,
        "hatch": _hatch,
        "sketchify": _sketchify,
    }
    for name, fn in sketch_fns.items():
        env.define(name, BuiltinProcedure(name, fn, accepts_kwargs=True))
