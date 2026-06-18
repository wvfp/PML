"""Outfit component — generates clothing GraphicObjects."""

from __future__ import annotations

from typing import Any

from pml.graphics.objects import GraphicObject
from pml.sprites.validator import ParamSchema, validate_params

from .view_utils import VIEW_NAMES, sym_str, view_width

_OUTFIT_SCHEMA = (
    ParamSchema()
    .enum("top", ["t-shirt", "jacket", "hoodie", "dress", "armor", "robe", "suit", "tank", "sailor"], "t-shirt")
    .enum("bottom", ["pants", "skirt", "shorts", "long-skirt", "armor"], "pants")
    .enum("shoes", ["boots", "sneakers", "sandals", "heels", "none"], "boots")
    .color("color-top", "#3498db")
    .color("color-bottom", "#2c3e50")
    .enum("detail", ["plain", "striped", "pattern", "badge"], "plain")
    .enum("view", ["front", "side", "back", "three-quarter"], "front")
)

_SIZE_MULTS = {"small": 0.7, "medium": 1.0, "large": 1.2}


def _draw_top(top: str, color: str, detail: str, torso_w: float, torso_h: float) -> list[GraphicObject]:
    """Draw upper garment."""
    parts: list[GraphicObject] = []

    if top == "armor":
        # Metal chest plate
        parts.append(GraphicObject(
            shape_type="rect",
            params={"x": -torso_w / 2 + 2, "y": 2, "w": torso_w - 4, "h": torso_h * 0.6},
            fill="#7f8c8d", stroke="#2c3e50", stroke_width=2.0,
        ))
        # Shoulder plates
        for dx in (-torso_w / 2 - 4, torso_w / 2 - 8):
            parts.append(GraphicObject(
                shape_type="ellipse",
                params={"cx": dx + 6, "cy": torso_h * 0.08, "rx": 12, "ry": 8},
                fill="#95a5a6", stroke="#2c3e50", stroke_width=1.5,
            ))
    elif top == "robe":
        parts.append(GraphicObject(
            shape_type="rect",
            params={"x": -torso_w / 2 - 4, "y": -4, "w": torso_w + 8, "h": torso_h + 8},
            fill=color, stroke="#1a1a1a", stroke_width=2.0,
        ))
        # Belt
        parts.append(GraphicObject(
            shape_type="rect",
            params={"x": -torso_w / 2, "y": torso_h * 0.5, "w": torso_w, "h": 6},
            fill="#8B4513", stroke="#1a1a1a", stroke_width=1.0,
        ))
    elif top == "hoodie":
        parts.append(GraphicObject(
            shape_type="rect",
            params={"x": -torso_w / 2 - 2, "y": 0, "w": torso_w + 4, "h": torso_h + 4},
            fill=color, stroke="#1a1a1a", stroke_width=2.0,
        ))
        # Hood (behind head)
        parts.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": 0, "cy": -8, "rx": torso_w * 0.4, "ry": 14},
            fill=color, stroke="#1a1a1a", stroke_width=1.5,
        ))
        # Front pocket
        parts.append(GraphicObject(
            shape_type="rect",
            params={"x": -torso_w * 0.3, "y": torso_h * 0.55, "w": torso_w * 0.6, "h": torso_h * 0.2},
            fill=None, stroke="#1a1a1a", stroke_width=1.0,
        ))
    elif top == "dress":
        parts.append(GraphicObject(
            shape_type="rect",
            params={"x": -torso_w / 2 - 2, "y": 0, "w": torso_w + 4, "h": torso_h * 1.2},
            fill=color, stroke="#1a1a1a", stroke_width=2.0,
        ))
    elif top == "suit":
        parts.append(GraphicObject(
            shape_type="rect",
            params={"x": -torso_w / 2, "y": 0, "w": torso_w, "h": torso_h},
            fill=color, stroke="#1a1a1a", stroke_width=2.0,
        ))
        # Lapels (V-shape lines)
        parts.append(GraphicObject(
            shape_type="line",
            params={"x1": 0, "y1": 0, "x2": -torso_w * 0.3, "y2": torso_h * 0.35},
            stroke="#1a1a1a", stroke_width=1.5,
        ))
        parts.append(GraphicObject(
            shape_type="line",
            params={"x1": 0, "y1": 0, "x2": torso_w * 0.3, "y2": torso_h * 0.35},
            stroke="#1a1a1a", stroke_width=1.5,
        ))
        # Tie
        parts.append(GraphicObject(
            shape_type="polygon",
            params={"points": [(0, 2), (-4, torso_h * 0.4), (0, torso_h * 0.5), (4, torso_h * 0.4)]},
            fill="#c0392b", stroke="#1a1a1a", stroke_width=1.0,
        ))
    elif top == "sailor":
        parts.append(GraphicObject(
            shape_type="rect",
            params={"x": -torso_w / 2, "y": 0, "w": torso_w, "h": torso_h},
            fill="#FFFFFF", stroke="#1a1a1a", stroke_width=2.0,
        ))
        # Sailor collar
        parts.append(GraphicObject(
            shape_type="polygon",
            params={"points": [
                (-torso_w * 0.4, 0), (0, torso_h * 0.3), (torso_w * 0.4, 0),
            ]},
            fill="#2c3e50", stroke="#1a1a1a", stroke_width=1.5,
        ))
    elif top == "tank":
        parts.append(GraphicObject(
            shape_type="rect",
            params={"x": -torso_w / 2 + 6, "y": 0, "w": torso_w - 12, "h": torso_h},
            fill=color, stroke="#1a1a1a", stroke_width=2.0,
        ))
    else:
        # t-shirt / jacket — basic rect
        parts.append(GraphicObject(
            shape_type="rect",
            params={"x": -torso_w / 2, "y": 0, "w": torso_w, "h": torso_h},
            fill=color, stroke="#1a1a1a", stroke_width=2.0,
        ))
        if top == "jacket":
            # Zipper line
            parts.append(GraphicObject(
                shape_type="line",
                params={"x1": 0, "y1": 0, "x2": 0, "y2": torso_h},
                stroke="#1a1a1a", stroke_width=1.5,
            ))

    # Detail overlay
    if detail == "striped":
        for i in range(3):
            y = torso_h * 0.2 + i * torso_h * 0.25
            parts.append(GraphicObject(
                shape_type="line",
                params={"x1": -torso_w / 2 + 4, "y1": y, "x2": torso_w / 2 - 4, "y2": y},
                stroke="#FFFFFF80", stroke_width=2.0,
            ))
    elif detail == "badge":
        parts.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": torso_w * 0.25, "cy": torso_h * 0.2, "rx": 6, "ry": 6},
            fill="#e74c3c", stroke="#1a1a1a", stroke_width=1.0,
        ))

    return parts


def _draw_bottom(bottom: str, color: str, torso_w: float, torso_h: float) -> list[GraphicObject]:
    """Draw lower garment."""
    parts: list[GraphicObject] = []
    leg_h = torso_h * 0.7
    leg_w = torso_w * 0.38
    leg_y = torso_h

    if bottom == "skirt":
        parts.append(GraphicObject(
            shape_type="polygon",
            params={"points": [
                (-torso_w / 2, leg_y), (torso_w / 2, leg_y),
                (torso_w / 2 + 8, leg_y + leg_h * 0.6), (-torso_w / 2 - 8, leg_y + leg_h * 0.6),
            ]},
            fill=color, stroke="#1a1a1a", stroke_width=2.0,
        ))
    elif bottom == "long-skirt":
        parts.append(GraphicObject(
            shape_type="polygon",
            params={"points": [
                (-torso_w / 2, leg_y), (torso_w / 2, leg_y),
                (torso_w / 2 + 12, leg_y + leg_h), (-torso_w / 2 - 12, leg_y + leg_h),
            ]},
            fill=color, stroke="#1a1a1a", stroke_width=2.0,
        ))
    elif bottom == "shorts":
        for dx in (-leg_w / 2 - 2, leg_w / 2 + 2):
            parts.append(GraphicObject(
                shape_type="rect",
                params={"x": dx - leg_w / 2, "y": leg_y, "w": leg_w, "h": leg_h * 0.4},
                fill=color, stroke="#1a1a1a", stroke_width=2.0,
            ))
    elif bottom == "armor":
        for dx in (-leg_w / 2 - 2, leg_w / 2 + 2):
            parts.append(GraphicObject(
                shape_type="rect",
                params={"x": dx - leg_w / 2, "y": leg_y, "w": leg_w, "h": leg_h},
                fill="#7f8c8d", stroke="#2c3e50", stroke_width=2.0,
            ))
    else:
        # pants — two leg rects
        for dx in (-leg_w / 2 - 2, leg_w / 2 + 2):
            parts.append(GraphicObject(
                shape_type="rect",
                params={"x": dx - leg_w / 2, "y": leg_y, "w": leg_w, "h": leg_h},
                fill=color, stroke="#1a1a1a", stroke_width=2.0,
            ))

    return parts


def _draw_shoes(shoes: str, torso_w: float, torso_h: float) -> list[GraphicObject]:
    """Draw footwear."""
    if shoes == "none":
        return []

    parts: list[GraphicObject] = []
    leg_h = torso_h * 0.7
    leg_y = torso_h + leg_h
    shoe_w = torso_w * 0.22

    shoe_color = "#2c2c2c"
    shoe_h = 10

    if shoes == "boots":
        shoe_h = 18
        shoe_color = "#5D4037"
    elif shoes == "sneakers":
        shoe_color = "#FFFFFF"
    elif shoes == "sandals":
        shoe_color = "#8B4513"
        shoe_h = 6
    elif shoes == "heels":
        shoe_color = "#c0392b"
        shoe_h = 14

    for dx in (-torso_w * 0.2, torso_w * 0.2):
        parts.append(GraphicObject(
            shape_type="rect",
            params={"x": dx - shoe_w / 2, "y": leg_y - 2, "w": shoe_w, "h": shoe_h},
            fill=shoe_color, stroke="#1a1a1a", stroke_width=1.5,
        ))

    return parts


def create_outfit(**kwargs: Any) -> GraphicObject:
    """Create clothing overlay for a character body.

    Args:
        :top — upper garment type
        :bottom — lower garment type
        :shoes — footwear type
        :color-top — upper garment color
        :color-bottom — lower garment color
        :detail — decorative detail
        :view — 'front | 'side | 'back | 'three-quarter
    """
    p = validate_params(_OUTFIT_SCHEMA, {sym_str(k): v for k, v in kwargs.items()})

    torso_w = 50
    torso_h = 64
    view = p["view"]

    vw = int(view_width(torso_w, view, front=1.0, side=0.5, back=1.0, three_quarter=0.7))

    if view == "back":
        top_parts = _draw_top(p["top"], p["color-top"], "plain", vw, torso_h)
        bottom_parts = _draw_bottom(p["bottom"], p["color-bottom"], vw, torso_h)
        shoe_parts = _draw_shoes(p["shoes"], vw, torso_h)
        top_parts.append(GraphicObject(
            shape_type="line",
            params={"x1": 0, "y1": torso_h * 0.15, "x2": 0, "y2": torso_h * 0.7},
            stroke="#1a1a1a40",
            stroke_width=1.0,
        ))

    elif view == "side":
        top_parts = _draw_top(p["top"], p["color-top"], p["detail"], vw, torso_h)
        leg_h = torso_h * 0.7
        leg_w = vw * 0.35
        leg_y = torso_h
        bottom_parts = [
            GraphicObject(
                shape_type="rect",
                params={"x": -leg_w / 2, "y": leg_y, "w": leg_w, "h": leg_h},
                fill=p["color-bottom"], stroke="#1a1a1a", stroke_width=2.0,
            )
        ]
        shoe_parts = _draw_shoes(p["shoes"], vw, torso_h)

    else:
        top_parts = _draw_top(p["top"], p["color-top"], p["detail"], vw, torso_h)
        bottom_parts = _draw_bottom(p["bottom"], p["color-bottom"], vw, torso_h)
        shoe_parts = _draw_shoes(p["shoes"], vw, torso_h)

    all_children = top_parts + bottom_parts + shoe_parts

    return GraphicObject(
        shape_type="group",
        children=tuple(all_children),
        metadata={
            "component": "outfit",
            "top": p["top"],
            "bottom": p["bottom"],
            "shoes": p["shoes"],
            "view": view,
        },
    )
