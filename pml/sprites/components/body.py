"""Body component — generates a torso GraphicObject tree."""

from __future__ import annotations

from typing import Any

from pml.graphics.objects import GraphicObject
from pml.sprites.validator import ParamSchema, validate_params
from pml.types import Symbol

_BODY_SCHEMA = (
    ParamSchema()
    .number("height", 160, min_val=80, max_val=300)
    .enum("build", ["slim", "average", "muscular", "chubby"], "average")
    .color("skin", "#fce4c8")
    .enum("proportions", ["realistic", "anime", "chibi"], "anime")
)

# Build width multipliers
_BUILD_WIDTHS = {"slim": 0.8, "average": 1.0, "muscular": 1.15, "chubby": 1.25}

# Proportion ratios (torso height as fraction of total height)
_PROP_RATIOS = {"realistic": 0.45, "anime": 0.40, "chibi": 0.30}


def _sym_str(v: Any) -> str:
    if isinstance(v, Symbol):
        return v.name
    return str(v) if v is not None else ""


def create_body(**kwargs: Any) -> GraphicObject:
    """Create a body (torso) graphic object.

    Args:
        :height — total character height in pixels (default 160)
        :build — 'slim | 'average | 'muscular | 'chubby
        :skin — skin color
        :proportions — 'realistic | 'anime | 'chibi
    """
    p = validate_params(_BODY_SCHEMA, {_sym_str(k): v for k, v in kwargs.items()})

    height = p["height"]
    build = p["build"]
    skin = p["skin"]
    proportions = p["proportions"]

    torso_h = height * _PROP_RATIOS.get(proportions, 0.40)
    torso_w = 50 * _BUILD_WIDTHS.get(build, 1.0)

    # Torso center at origin
    torso = GraphicObject(
        shape_type="rect",
        params={"x": -torso_w / 2, "y": 0, "w": torso_w, "h": torso_h},
        fill=skin,
        stroke="#1a1a1a",
        stroke_width=2.0,
        metadata={"component": "body", "build": build, "proportions": proportions},
    )

    # Neck
    neck_w = torso_w * 0.35
    neck_h = height * 0.06
    neck = GraphicObject(
        shape_type="rect",
        params={"x": -neck_w / 2, "y": -neck_h, "w": neck_w, "h": neck_h},
        fill=skin,
        stroke="#1a1a1a",
        stroke_width=1.5,
    )

    # Shoulders (ellipses at top of torso)
    shoulder_r = torso_w * 0.18
    left_shoulder = GraphicObject(
        shape_type="ellipse",
        params={"cx": -torso_w / 2, "cy": torso_h * 0.08, "rx": shoulder_r, "ry": shoulder_r * 0.8},
        fill=skin,
        stroke="#1a1a1a",
        stroke_width=1.5,
    )
    right_shoulder = GraphicObject(
        shape_type="ellipse",
        params={"cx": torso_w / 2, "cy": torso_h * 0.08, "rx": shoulder_r, "ry": shoulder_r * 0.8},
        fill=skin,
        stroke="#1a1a1a",
        stroke_width=1.5,
    )

    return GraphicObject(
        shape_type="group",
        children=(torso, neck, left_shoulder, right_shoulder),
        metadata={
            "component": "body",
            "height": height,
            "torso_height": torso_h,
            "torso_width": torso_w,
            "build": build,
            "skin": skin,
        },
    )
