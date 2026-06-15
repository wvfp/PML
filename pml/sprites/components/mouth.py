"""Mouth component — generates mouth GraphicObjects in various styles."""

from __future__ import annotations

from typing import Any

from pml.graphics.objects import GraphicObject
from pml.sprites.validator import ParamSchema, validate_params
from pml.types import Symbol

_MOUTH_SCHEMA = (
    ParamSchema()
    .enum("style", ["neutral", "smile", "frown", "open", "cat", "fang"], "neutral")
    .number("size", 1.0, min_val=0.3, max_val=3.0)
)


def _sym_str(v: Any) -> str:
    if isinstance(v, Symbol):
        return v.name
    return str(v) if v is not None else ""


def create_mouth(**kwargs: Any) -> GraphicObject:
    """Create a mouth graphic object.

    Args:
        :style — 'neutral | 'smile | 'frown | 'open | 'cat | 'fang
        :size — scale factor (default 1.0)
    """
    p = validate_params(_MOUTH_SCHEMA, {_sym_str(k): v for k, v in kwargs.items()})

    style = p["style"]
    s = p["size"]
    w = 12 * s  # mouth width
    children: list[GraphicObject] = []

    if style == "neutral":
        # Simple horizontal line
        children.append(GraphicObject(
            shape_type="line",
            params={"x1": -w / 2, "y1": 0, "x2": w / 2, "y2": 0},
            stroke="#c0392b",
            stroke_width=2.0 * s,
        ))

    elif style == "smile":
        # Arc approximated as a shallow V shape (two lines)
        children.append(GraphicObject(
            shape_type="line",
            params={"x1": -w / 2, "y1": -2 * s, "x2": 0, "y2": 4 * s},
            stroke="#c0392b",
            stroke_width=2.0 * s,
        ))
        children.append(GraphicObject(
            shape_type="line",
            params={"x1": 0, "y1": 4 * s, "x2": w / 2, "y2": -2 * s},
            stroke="#c0392b",
            stroke_width=2.0 * s,
        ))

    elif style == "frown":
        # Inverted arc
        children.append(GraphicObject(
            shape_type="line",
            params={"x1": -w / 2, "y1": 4 * s, "x2": 0, "y2": -2 * s},
            stroke="#c0392b",
            stroke_width=2.0 * s,
        ))
        children.append(GraphicObject(
            shape_type="line",
            params={"x1": 0, "y1": -2 * s, "x2": w / 2, "y2": 4 * s},
            stroke="#c0392b",
            stroke_width=2.0 * s,
        ))

    elif style == "open":
        # Open mouth (ellipse)
        children.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": 0, "cy": 2 * s, "rx": w / 2, "ry": 6 * s},
            fill="#3d1010",
            stroke="#c0392b",
            stroke_width=1.5,
        ))
        # Teeth highlight
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -w / 3, "y": -1 * s, "w": w * 2 / 3, "h": 3 * s},
            fill="#FFFFFF",
        ))

    elif style == "cat":
        # Cat mouth (ω shape — three small arcs)
        cat_w = 8 * s
        for dx in (-cat_w, 0, cat_w):
            children.append(GraphicObject(
                shape_type="ellipse",
                params={"cx": dx, "cy": 2 * s, "rx": 4 * s, "ry": 3 * s},
                fill=None,
                stroke="#c0392b",
                stroke_width=1.5,
            ))

    elif style == "fang":
        # Smirk with a fang
        children.append(GraphicObject(
            shape_type="line",
            params={"x1": -w / 2, "y1": -2 * s, "x2": w / 2, "y2": -2 * s},
            stroke="#c0392b",
            stroke_width=2.0 * s,
        ))
        # Fang triangle
        fang_x = w / 3
        children.append(GraphicObject(
            shape_type="polygon",
            params={"points": [
                (fang_x - 3 * s, -2 * s),
                (fang_x + 3 * s, -2 * s),
                (fang_x, 6 * s),
            ]},
            fill="#FFFFFF",
            stroke="#c0392b",
            stroke_width=1.0,
        ))

    return GraphicObject(
        shape_type="group",
        children=tuple(children),
        metadata={"component": "mouth", "style": style},
    )
