"""PML graphics primitive functions — circle, rect, line, etc."""

from __future__ import annotations

from typing import Any

from pml.environment import Environment
from pml.errors import PMLTypeError
from pml.graphics.objects import GraphicObject
from pml.transform import AffineTransform
from pml.types import BuiltinProcedure


def _extract_transform(kwargs: dict[str, Any]) -> AffineTransform:
    """Extract :transform keyword, default to identity."""
    t = kwargs.get("transform")
    if t is None:
        return AffineTransform.identity()
    if isinstance(t, AffineTransform):
        return t
    raise PMLTypeError(f":transform expects AffineTransform, got {type(t).__name__}")


def _circle(x: float, y: float, r: float, **kwargs: Any) -> GraphicObject:
    return GraphicObject(
        shape_type="circle",
        params={"x": x, "y": y, "r": r},
        fill=kwargs.get("fill"),
        stroke=kwargs.get("stroke"),
        stroke_width=kwargs.get("stroke-width", 1.0),
        transform=_extract_transform(kwargs),
    )


def _rect(x: float, y: float, w: float, h: float, **kwargs: Any) -> GraphicObject:
    return GraphicObject(
        shape_type="rect",
        params={"x": x, "y": y, "w": w, "h": h},
        fill=kwargs.get("fill"),
        stroke=kwargs.get("stroke"),
        stroke_width=kwargs.get("stroke-width", 1.0),
        transform=_extract_transform(kwargs),
    )


def _ellipse(cx: float, cy: float, rx: float, ry: float, **kwargs: Any) -> GraphicObject:
    return GraphicObject(
        shape_type="ellipse",
        params={"cx": cx, "cy": cy, "rx": rx, "ry": ry},
        fill=kwargs.get("fill"),
        stroke=kwargs.get("stroke"),
        stroke_width=kwargs.get("stroke-width", 1.0),
        transform=_extract_transform(kwargs),
    )


def _line(x1: float, y1: float, x2: float, y2: float, **kwargs: Any) -> GraphicObject:
    return GraphicObject(
        shape_type="line",
        params={"x1": x1, "y1": y1, "x2": x2, "y2": y2},
        fill=None,
        stroke=kwargs.get("stroke", "#000000"),
        stroke_width=kwargs.get("stroke-width", 1.0),
        transform=_extract_transform(kwargs),
    )


def _polygon(points: list, **kwargs: Any) -> GraphicObject:
    """points: list of (x y) pairs → list of [x, y] lists."""
    if not isinstance(points, list):
        raise PMLTypeError("polygon: points must be a list of (x y) pairs")
    pts = []
    for p in points:
        if not isinstance(p, list) or len(p) != 2:
            raise PMLTypeError(f"polygon: expected (x y), got {p!r}")
        pts.append((p[0], p[1]))
    return GraphicObject(
        shape_type="polygon",
        params={"points": pts},
        fill=kwargs.get("fill"),
        stroke=kwargs.get("stroke"),
        stroke_width=kwargs.get("stroke-width", 1.0),
        transform=_extract_transform(kwargs),
    )


def _path(d_string: str, **kwargs: Any) -> GraphicObject:
    """SVG path data string."""
    return GraphicObject(
        shape_type="path",
        params={"d": d_string},
        fill=kwargs.get("fill"),
        stroke=kwargs.get("stroke"),
        stroke_width=kwargs.get("stroke-width", 1.0),
        transform=_extract_transform(kwargs),
    )


def _text(s: str, x: float, y: float, **kwargs: Any) -> GraphicObject:
    return GraphicObject(
        shape_type="text",
        params={
            "text": s,
            "x": x,
            "y": y,
            "font_family": kwargs.get("font-family", "default"),
            "font_size": kwargs.get("font-size", 12),
        },
        fill=kwargs.get("fill", "#000000"),
        stroke=kwargs.get("stroke"),
        stroke_width=kwargs.get("stroke-width", 1.0),
        transform=_extract_transform(kwargs),
    )


def _image(src: str, x: float, y: float, *args: Any, **kwargs: Any) -> GraphicObject:
    w = args[0] if len(args) > 0 else None
    h = args[1] if len(args) > 1 else None
    return GraphicObject(
        shape_type="image",
        params={"src": src, "x": x, "y": y, "w": w, "h": h},
        transform=_extract_transform(kwargs),
    )


def _group(*children: Any, **kwargs: Any) -> GraphicObject:
    """Group multiple graphic objects."""
    objs = []
    for child in children:
        if isinstance(child, GraphicObject):
            objs.append(child)
        else:
            raise PMLTypeError(f"group: expected GraphicObject, got {type(child).__name__}")
    return GraphicObject(
        shape_type="group",
        children=tuple(objs),
        transform=_extract_transform(kwargs),
    )


def register_primitives(env: Environment) -> None:
    """Register all graphic primitive functions."""
    # These accept keyword args
    kw_fns = {
        "circle": _circle,
        "rect": _rect,
        "ellipse": _ellipse,
        "line": _line,
        "polygon": _polygon,
        "path": _path,
        "text": _text,
        "image": _image,
        "group": _group,
    }
    for name, fn in kw_fns.items():
        env.define(name, BuiltinProcedure(name, fn, accepts_kwargs=True))

    # graphic-object? predicate
    env.define(
        "graphic-object?",
        BuiltinProcedure("graphic-object?", lambda x: isinstance(x, GraphicObject)),
    )
