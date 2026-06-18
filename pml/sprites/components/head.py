"""Head component — generates a head GraphicObject tree."""

from __future__ import annotations

from typing import Any

from pml.graphics.objects import GraphicObject
from pml.sprites.validator import ParamSchema, validate_params

from .view_utils import VIEW_NAMES, sym_str, view_width

_HEAD_SCHEMA = (
    ParamSchema()
    .enum("shape", ["oval", "round", "square", "heart", "angular"], "oval")
    .color("skin", "#fce4c8")
    .enum("ears", ["normal", "pointed", "animal", "none"], "normal")
    .enum("view", VIEW_NAMES, "front")
)


def _make_ear(dx: float, dy: float, rx: float, ry: float, skin: str) -> GraphicObject:
    return GraphicObject(
        shape_type="ellipse",
        params={"cx": dx, "cy": dy, "rx": rx, "ry": ry},
        fill=skin,
        stroke="#1a1a1a",
        stroke_width=1.5,
    )


def _make_pointed_ear(dx: float, flip: float, skin: str) -> GraphicObject:
    pts = [
        (dx, -4),
        (dx + 8 * flip, -30),
        (dx + 3 * flip, -2),
    ]
    return GraphicObject(
        shape_type="polygon",
        params={"points": pts},
        fill=skin,
        stroke="#1a1a1a",
        stroke_width=1.5,
    )


def _make_animal_ear(dx: float, flip: float, top_y: float, skin: str) -> GraphicObject:
    pts = [
        (dx - 6 * flip, top_y),
        (dx + 4 * flip, top_y - 22),
        (dx + 12 * flip, top_y + 2),
    ]
    return GraphicObject(
        shape_type="polygon",
        params={"points": pts},
        fill=skin,
        stroke="#1a1a1a",
        stroke_width=1.5,
    )


def create_head(**kwargs: Any) -> GraphicObject:
    """Create a head graphic object.

    Args:
        :shape — 'oval | 'round | 'square | 'heart | 'angular
        :skin — skin color
        :ears — 'normal | 'pointed | 'animal | 'none
        :view — 'front | 'side | 'back | 'three-quarter
    """
    p = validate_params(_HEAD_SCHEMA, {sym_str(k): v for k, v in kwargs.items()})

    shape = p["shape"]
    skin = p["skin"]
    ears = p["ears"]
    view = p["view"]

    # Head dimensions (centered at origin)
    head_w = 56
    head_h = 64

    vw = view_width(head_w, view, front=1.0, side=0.55, back=1.0, three_quarter=0.75)

    def _head_shape(w: float, h: float, s: str) -> GraphicObject:
        if s in ("oval", "heart"):
            return GraphicObject(
                shape_type="ellipse",
                params={"cx": 0, "cy": 0, "rx": w / 2, "ry": h / 2},
                fill=skin,
                stroke="#1a1a1a",
                stroke_width=2.0,
            )
        elif s == "round":
            return GraphicObject(
                shape_type="ellipse",
                params={"cx": 0, "cy": 0, "rx": w / 2, "ry": w / 2},
                fill=skin,
                stroke="#1a1a1a",
                stroke_width=2.0,
            )
        else:
            return GraphicObject(
                shape_type="rect",
                params={"x": -w / 2, "y": -h / 2, "w": w, "h": h},
                fill=skin,
                stroke="#1a1a1a",
                stroke_width=2.0,
            )

    head = _head_shape(vw, head_h, shape)
    children = [head]

    # Ears — view-dependent
    if view == "front":
        if ears == "normal":
            ear_r = 6
            for dx in (-vw / 2 - ear_r, vw / 2 + ear_r):
                children.append(_make_ear(dx, -4, ear_r, ear_r * 1.2, skin))
        elif ears == "pointed":
            for dx, flip in ((-vw / 2 - 2, -1), (vw / 2 + 2, 1)):
                children.append(_make_pointed_ear(dx, flip, skin))
        elif ears == "animal":
            for dx, flip in ((-vw / 2 + 4, -1), (vw / 2 - 4, 1)):
                children.append(_make_animal_ear(dx, flip, -head_h / 2 + 4, skin))
    elif view == "side":
        # Only one ear visible on the right (forward-facing) side
        if ears in ("normal", "pointed", "animal"):
            dx = vw / 2 + 4
            if ears == "normal":
                children.append(_make_ear(dx, -4, 6, 7, skin))
            elif ears == "pointed":
                children.append(_make_pointed_ear(dx, 1, skin))
            elif ears == "animal":
                children.append(_make_animal_ear(dx, 1, -head_h / 2 + 4, skin))
    elif view == "three-quarter":
        # Both ears visible but offset
        if ears == "normal":
            for dx in (-vw / 2 - 5, vw / 2 + 5):
                children.append(_make_ear(dx, -4, 5, 6, skin))
        elif ears == "pointed":
            for dx, flip in ((-vw / 2 - 2, -1), (vw / 2 + 2, 1)):
                children.append(_make_pointed_ear(dx, flip, skin))
        elif ears == "animal":
            for dx, flip in ((-vw / 2 + 4, -1), (vw / 2 - 4, 1)):
                children.append(_make_animal_ear(dx, flip, -head_h / 2 + 4, skin))
    # view == "back": no ears, or subtle bumps

    return GraphicObject(
        shape_type="group",
        children=tuple(children),
        metadata={
            "component": "head",
            "shape": shape,
            "skin": skin,
            "view": view,
            "head_width": vw,
            "head_height": head_h,
        },
    )
