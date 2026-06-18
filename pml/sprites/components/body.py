"""Body component — generates a torso GraphicObject tree."""

from __future__ import annotations

from typing import Any

from pml.graphics.objects import GraphicObject
from pml.sprites.validator import ParamSchema, validate_params

from .view_utils import VIEW_NAMES, sym_str, view_width

_BODY_SCHEMA = (
    ParamSchema()
    .number("height", 160, min_val=80, max_val=300)
    .enum("build", ["slim", "average", "muscular", "chubby"], "average")
    .color("skin", "#fce4c8")
    .enum("proportions", ["realistic", "anime", "chibi"], "anime")
    .enum("view", VIEW_NAMES, "front")
)

# Build width multipliers
_BUILD_WIDTHS = {"slim": 0.8, "average": 1.0, "muscular": 1.15, "chubby": 1.25}

# Proportion ratios (torso height as fraction of total height)
_PROP_RATIOS = {"realistic": 0.45, "anime": 0.40, "chibi": 0.30}


def create_body(**kwargs: Any) -> GraphicObject:
    """Create a body (torso) graphic object.

    Args:
        :height — total character height in pixels (default 160)
        :build — 'slim | 'average | 'muscular | 'chubby
        :skin — skin color
        :proportions — 'realistic | 'anime | 'chibi
        :view — 'front | 'side | 'back | 'three-quarter
    """
    p = validate_params(_BODY_SCHEMA, {sym_str(k): v for k, v in kwargs.items()})

    height = p["height"]
    build = p["build"]
    skin = p["skin"]
    proportions = p["proportions"]
    view = p["view"]

    torso_h = height * _PROP_RATIOS.get(proportions, 0.40)
    torso_w = 50 * _BUILD_WIDTHS.get(build, 1.0)

    vw = view_width(torso_w, view, front=1.0, side=0.5, back=1.0, three_quarter=0.7)
    neck_w = max(vw * 0.2, 8.0)

    children: list[GraphicObject] = []

    if view == "side":
        # Side profile — narrower torso, single shoulder
        torso = GraphicObject(
            shape_type="rect",
            params={"x": -vw / 2, "y": 0, "w": vw, "h": torso_h},
            fill=skin,
            stroke="#1a1a1a",
            stroke_width=2.0,
            metadata={"component": "body", "build": build, "proportions": proportions},
        )
        children.append(torso)

        # Neck
        neck = GraphicObject(
            shape_type="rect",
            params={"x": -4, "y": -torso_h * 0.1, "w": 8, "h": torso_h * 0.1},
            fill=skin,
            stroke="#1a1a1a",
            stroke_width=1.5,
        )
        children.append(neck)

        # Single shoulder on forward side
        shoulder = GraphicObject(
            shape_type="ellipse",
            params={"cx": vw / 2, "cy": torso_h * 0.08, "rx": vw * 0.35, "ry": vw * 0.25},
            fill=skin,
            stroke="#1a1a1a",
            stroke_width=1.5,
        )
        children.append(shoulder)

    elif view == "back":
        # Same as front but slightly wider shoulders
        torso = GraphicObject(
            shape_type="rect",
            params={"x": -vw / 2, "y": 0, "w": vw, "h": torso_h},
            fill=skin,
            stroke="#1a1a1a",
            stroke_width=2.0,
            metadata={"component": "body", "build": build, "proportions": proportions},
        )
        children.append(torso)

        neck = GraphicObject(
            shape_type="rect",
            params={"x": -neck_w / 2, "y": -torso_h * 0.1, "w": neck_w, "h": torso_h * 0.1},
            fill=skin,
            stroke="#1a1a1a",
            stroke_width=1.5,
        )
        children.append(neck)

        # Back shoulders — both visible
        shoulder_r = vw * 0.18
        for dx in (-vw / 2, vw / 2):
            children.append(GraphicObject(
                shape_type="ellipse",
                params={"cx": dx, "cy": torso_h * 0.08, "rx": shoulder_r, "ry": shoulder_r * 0.8},
                fill=skin,
                stroke="#1a1a1a",
                stroke_width=1.5,
            ))

    elif view == "three-quarter":
        # Asymmetric — wider on forward side
        torso = GraphicObject(
            shape_type="rect",
            params={"x": -vw / 2, "y": 0, "w": vw, "h": torso_h},
            fill=skin,
            stroke="#1a1a1a",
            stroke_width=2.0,
            metadata={"component": "body", "build": build, "proportions": proportions},
        )
        children.append(torso)

        neck = GraphicObject(
            shape_type="rect",
            params={"x": -neck_w / 2, "y": -torso_h * 0.1, "w": neck_w, "h": torso_h * 0.1},
            fill=skin,
            stroke="#1a1a1a",
            stroke_width=1.5,
        )
        children.append(neck)

        # Both shoulders — forward one larger
        for dx, rx_mul in ((-vw / 2, 0.7), (vw / 2, 1.0)):
            children.append(GraphicObject(
                shape_type="ellipse",
                params={"cx": dx, "cy": torso_h * 0.08, "rx": vw * 0.2 * rx_mul, "ry": vw * 0.2 * 0.8},
                fill=skin,
                stroke="#1a1a1a",
                stroke_width=1.5,
            ))

    else:
        # Front view — original
        torso = GraphicObject(
            shape_type="rect",
            params={"x": -vw / 2, "y": 0, "w": vw, "h": torso_h},
            fill=skin,
            stroke="#1a1a1a",
            stroke_width=2.0,
            metadata={"component": "body", "build": build, "proportions": proportions},
        )
        children.append(torso)

        neck = GraphicObject(
            shape_type="rect",
            params={"x": -neck_w / 2, "y": -torso_h * 0.1, "w": neck_w, "h": torso_h * 0.1},
            fill=skin,
            stroke="#1a1a1a",
            stroke_width=1.5,
        )
        children.append(neck)

        shoulder_r = vw * 0.18
        for dx in (-vw / 2, vw / 2):
            children.append(GraphicObject(
                shape_type="ellipse",
                params={"cx": dx, "cy": torso_h * 0.08, "rx": shoulder_r, "ry": shoulder_r * 0.8},
                fill=skin,
                stroke="#1a1a1a",
                stroke_width=1.5,
            ))

    return GraphicObject(
        shape_type="group",
        children=tuple(children),
        metadata={
            "component": "body",
            "height": height,
            "torso_height": torso_h,
            "torso_width": vw,
            "build": build,
            "skin": skin,
            "view": view,
        },
    )
