"""Head component — generates a head GraphicObject tree."""

from __future__ import annotations

from typing import Any

from pml.graphics.objects import GraphicObject
from pml.sprites.validator import ParamSchema, validate_params
from pml.types import Symbol

_HEAD_SCHEMA = (
    ParamSchema()
    .enum("shape", ["oval", "round", "square", "heart", "angular"], "oval")
    .color("skin", "#fce4c8")
    .enum("ears", ["normal", "pointed", "animal", "none"], "normal")
)


def _sym_str(v: Any) -> str:
    if isinstance(v, Symbol):
        return v.name
    return str(v) if v is not None else ""


def create_head(**kwargs: Any) -> GraphicObject:
    """Create a head graphic object.

    Args:
        :shape — 'oval | 'round | 'square | 'heart | 'angular
        :skin — skin color
        :ears — 'normal | 'pointed | 'animal | 'none
    """
    p = validate_params(_HEAD_SCHEMA, {_sym_str(k): v for k, v in kwargs.items()})

    shape = p["shape"]
    skin = p["skin"]
    ears = p["ears"]

    # Head dimensions (centered at origin)
    head_w = 56
    head_h = 64

    # Main head shape
    if shape in ("oval", "heart"):
        head = GraphicObject(
            shape_type="ellipse",
            params={"cx": 0, "cy": 0, "rx": head_w / 2, "ry": head_h / 2},
            fill=skin,
            stroke="#1a1a1a",
            stroke_width=2.0,
        )
    elif shape == "round":
        head = GraphicObject(
            shape_type="ellipse",
            params={"cx": 0, "cy": 0, "rx": head_w / 2, "ry": head_w / 2},
            fill=skin,
            stroke="#1a1a1a",
            stroke_width=2.0,
        )
    elif shape in ("square", "angular"):
        head = GraphicObject(
            shape_type="rect",
            params={"x": -head_w / 2, "y": -head_h / 2, "w": head_w, "h": head_h},
            fill=skin,
            stroke="#1a1a1a",
            stroke_width=2.0,
        )
    else:
        head = GraphicObject(
            shape_type="ellipse",
            params={"cx": 0, "cy": 0, "rx": head_w / 2, "ry": head_h / 2},
            fill=skin,
            stroke="#1a1a1a",
            stroke_width=2.0,
        )

    children = [head]

    # Ears
    if ears == "normal":
        ear_r = 6
        for dx in (-head_w / 2 - ear_r, head_w / 2 + ear_r):
            children.append(
                GraphicObject(
                    shape_type="ellipse",
                    params={"cx": dx, "cy": -4, "rx": ear_r, "ry": ear_r * 1.2},
                    fill=skin,
                    stroke="#1a1a1a",
                    stroke_width=1.5,
                )
            )
    elif ears == "pointed":
        for dx, flip in ((-head_w / 2 - 2, -1), (head_w / 2 + 2, 1)):
            points = [
                (dx, -4),
                (dx + 8 * flip, -30),
                (dx + 3 * flip, -2),
            ]
            children.append(
                GraphicObject(
                    shape_type="polygon",
                    params={"points": points},
                    fill=skin,
                    stroke="#1a1a1a",
                    stroke_width=1.5,
                )
            )
    elif ears == "animal":
        for dx, flip in ((-head_w / 2 + 4, -1), (head_w / 2 - 4, 1)):
            points = [
                (dx - 6 * flip, -head_h / 2 + 4),
                (dx + 4 * flip, -head_h / 2 - 18),
                (dx + 12 * flip, -head_h / 2 + 6),
            ]
            children.append(
                GraphicObject(
                    shape_type="polygon",
                    params={"points": points},
                    fill=skin,
                    stroke="#1a1a1a",
                    stroke_width=1.5,
                )
            )
    # ears == "none" → no ear shapes

    return GraphicObject(
        shape_type="group",
        children=tuple(children),
        metadata={
            "component": "head",
            "shape": shape,
            "skin": skin,
            "head_width": head_w,
            "head_height": head_h,
        },
    )
