"""Hair component — generates hairstyle GraphicObjects."""

from __future__ import annotations

from typing import Any

from pml.graphics.objects import GraphicObject
from pml.sprites.validator import ParamSchema, validate_params
from pml.transform import AffineTransform

from .view_utils import VIEW_NAMES, sym_str

_HAIR_SCHEMA = (
    ParamSchema()
    .enum(
        "style",
        ["short", "medium", "long", "ponytail", "twintails", "spiky", "bob", "braid", "bun", "mohawk", "bald"],
        "medium",
    )
    .color("color", "#2c2c2c")
    .number("length", 60, min_val=10, max_val=200)
    .boolean("bangs", True)
    .boolean("highlights", False)
    .enum("view", VIEW_NAMES, "front")
)


def _bangs(color: str, head_w: float) -> list[GraphicObject]:
    """Create bangs (fringe) across the forehead."""
    parts: list[GraphicObject] = []
    bw = head_w * 0.85
    bh = 14
    # Main bangs block
    parts.append(GraphicObject(
        shape_type="rect",
        params={"x": -bw / 2, "y": -head_w / 2 - 4, "w": bw, "h": bh},
        fill=color,
        stroke="#1a1a1a",
        stroke_width=1.5,
    ))
    # Strand lines
    strand_spacing = bw / 6
    for i in range(5):
        sx = -bw / 2 + strand_spacing * (i + 1)
        parts.append(GraphicObject(
            shape_type="line",
            params={"x1": sx, "y1": -head_w / 2 - 4, "x2": sx + 3, "y2": -head_w / 2 - 4 + bh},
            stroke="#1a1a1a",
            stroke_width=0.8,
        ))
    return parts


def _create_short(color: str, head_w: float, length: float, bangs: bool, highlights: bool) -> list[GraphicObject]:
    parts: list[GraphicObject] = []
    # Cap shape on top of head
    cap_h = 16
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": 0, "cy": -head_w / 2 + 4, "rx": head_w / 2 + 4, "ry": cap_h},
        fill=color,
        stroke="#1a1a1a",
        stroke_width=1.5,
    ))
    # Side tufts
    for dx in (-head_w / 2 - 2, head_w / 2 + 2):
        parts.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": dx, "cy": -head_w / 4, "rx": 6, "ry": 12},
            fill=color,
            stroke="#1a1a1a",
            stroke_width=1.0,
        ))
    if bangs:
        parts.extend(_bangs(color, head_w))
    return parts


def _create_medium(color: str, head_w: float, length: float, bangs: bool, highlights: bool) -> list[GraphicObject]:
    parts: list[GraphicObject] = []
    # Top cap
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": 0, "cy": -head_w / 2 + 4, "rx": head_w / 2 + 5, "ry": 18},
        fill=color,
        stroke="#1a1a1a",
        stroke_width=1.5,
    ))
    # Side hair flowing down
    side_h = min(length * 0.6, 40)
    for dx in (-head_w / 2 - 4, head_w / 2 + 4):
        parts.append(GraphicObject(
            shape_type="rect",
            params={"x": dx - 8, "y": -head_w / 4, "w": 16, "h": side_h},
            fill=color,
            stroke="#1a1a1a",
            stroke_width=1.0,
        ))
    if bangs:
        parts.extend(_bangs(color, head_w))
    return parts


def _create_long(color: str, head_w: float, length: float, bangs: bool, highlights: bool) -> list[GraphicObject]:
    parts: list[GraphicObject] = []
    # Top cap
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": 0, "cy": -head_w / 2 + 4, "rx": head_w / 2 + 6, "ry": 20},
        fill=color,
        stroke="#1a1a1a",
        stroke_width=1.5,
    ))
    # Long side hair
    side_h = min(length * 0.8, 80)
    for dx in (-head_w / 2 - 5, head_w / 2 + 5):
        parts.append(GraphicObject(
            shape_type="rect",
            params={"x": dx - 10, "y": -head_w / 4, "w": 20, "h": side_h},
            fill=color,
            stroke="#1a1a1a",
            stroke_width=1.0,
        ))
    # Back hair
    parts.append(GraphicObject(
        shape_type="rect",
        params={"x": -head_w / 2 - 4, "y": -head_w / 2 + 10, "w": head_w + 8, "h": length * 0.7},
        fill=color,
        stroke="#1a1a1a",
        stroke_width=1.0,
    ))
    if bangs:
        parts.extend(_bangs(color, head_w))
    return parts


def _create_ponytail(color: str, head_w: float, length: float, bangs: bool, highlights: bool) -> list[GraphicObject]:
    parts = _create_short(color, head_w, length, bangs, highlights)
    # Ponytail behind head
    tail_w = 16
    tail_h = min(length * 0.7, 60)
    parts.append(GraphicObject(
        shape_type="rect",
        params={"x": -tail_w / 2, "y": -head_w / 2 - 8, "w": tail_w, "h": tail_h},
        fill=color,
        stroke="#1a1a1a",
        stroke_width=1.5,
    ))
    # Tie
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": 0, "cy": -head_w / 2 - 8, "rx": tail_w / 2 + 2, "ry": 4},
        fill="#c0392b",
        stroke="#1a1a1a",
        stroke_width=1.0,
    ))
    return parts


def _create_twintails(color: str, head_w: float, length: float, bangs: bool, highlights: bool) -> list[GraphicObject]:
    parts = _create_short(color, head_w, length, bangs, highlights)
    tail_h = min(length * 0.6, 50)
    for dx in (-head_w / 2 - 8, head_w / 2 + 8):
        parts.append(GraphicObject(
            shape_type="rect",
            params={"x": dx - 7, "y": -head_w / 4, "w": 14, "h": tail_h},
            fill=color,
            stroke="#1a1a1a",
            stroke_width=1.5,
        ))
        # Tie
        parts.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": dx, "cy": -head_w / 4, "rx": 9, "ry": 4},
            fill="#e74c3c",
            stroke="#1a1a1a",
            stroke_width=1.0,
        ))
    return parts


def _create_spiky(color: str, head_w: float, length: float, bangs: bool, highlights: bool) -> list[GraphicObject]:
    parts: list[GraphicObject] = []
    hw = head_w / 2 + 6
    # Spikes radiating outward
    spike_count = 7
    for i in range(spike_count):
        angle = -3.14 + (3.14 * i / (spike_count - 1))
        import math
        bx = hw * math.cos(angle) * 0.9
        by = hw * math.sin(angle) * 0.9 - 4
        tx = hw * math.cos(angle) * 1.5
        ty = hw * math.sin(angle) * 1.5 - 4
        # Each spike is a triangle
        perp_x = -math.sin(angle) * 6
        perp_y = math.cos(angle) * 6
        points = [
            (bx + perp_x, by + perp_y),
            (tx, ty),
            (bx - perp_x, by - perp_y),
        ]
        parts.append(GraphicObject(
            shape_type="polygon",
            params={"points": points},
            fill=color,
            stroke="#1a1a1a",
            stroke_width=1.5,
        ))
    if bangs:
        parts.extend(_bangs(color, head_w))
    return parts


def _create_bob(color: str, head_w: float, length: float, bangs: bool, highlights: bool) -> list[GraphicObject]:
    parts: list[GraphicObject] = []
    # Rounded cap + sides that curve inward
    cap_h = 20
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": 0, "cy": -head_w / 2 + 4, "rx": head_w / 2 + 6, "ry": cap_h},
        fill=color,
        stroke="#1a1a1a",
        stroke_width=1.5,
    ))
    # Bob sides (wider, ending at jawline)
    bob_h = min(length * 0.4, 28)
    for dx in (-head_w / 2 - 6, head_w / 2 + 6):
        parts.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": dx, "cy": 0, "rx": 10, "ry": bob_h},
            fill=color,
            stroke="#1a1a1a",
            stroke_width=1.0,
        ))
    if bangs:
        parts.extend(_bangs(color, head_w))
    return parts


def _create_bald(color: str, head_w: float, length: float, bangs: bool, highlights: bool) -> list[GraphicObject]:
    # Bald — just a subtle shine highlight
    return [GraphicObject(
        shape_type="ellipse",
        params={"cx": -6, "cy": -head_w / 2 + 8, "rx": 8, "ry": 5},
        fill="#FFFFFF40",
    )]


_HAIR_MAKERS = {
    "short": _create_short,
    "medium": _create_medium,
    "long": _create_long,
    "ponytail": _create_ponytail,
    "twintails": _create_twintails,
    "spiky": _create_spiky,
    "bob": _create_bob,
    "braid": _create_long,  # fallback to long
    "bun": _create_short,  # fallback to short
    "mohawk": _create_spiky,  # fallback to spiky variant
    "bald": _create_bald,
}


def create_hair(**kwargs: Any) -> GraphicObject:
    """Create a hairstyle graphic object.

    Args:
        :style — 'short | 'medium | 'long | 'ponytail | 'twintails | 'spiky | 'bob | ...
        :color — hair color
        :length — hair length in pixels (clamped 10-200)
        :bangs — whether to include bangs
        :highlights — whether to add highlight strands
        :view — 'front | 'side | 'back | 'three-quarter
    """
    p = validate_params(_HAIR_SCHEMA, {sym_str(k): v for k, v in kwargs.items()})

    style = p["style"]
    color = p["color"]
    length = p["length"]
    bangs = p["bangs"]
    highlights = p["highlights"]
    view = p["view"]

    head_w = 56

    if view == "back":
        # No bangs from behind, same head width
        bangs = False
    elif view == "side":
        head_w = int(head_w * 0.5)
    elif view == "three-quarter":
        head_w = int(head_w * 0.75)

    maker = _HAIR_MAKERS.get(style, _create_medium)
    parts = maker(color, head_w, length, bangs, highlights)

    # Side view: shift hair to the right
    if view == "side":
        for i, part in enumerate(parts):
            parts[i] = part.with_transform(
                AffineTransform.translate(12, 0)
            )

    return GraphicObject(
        shape_type="group",
        children=tuple(parts),
        metadata={"component": "hair", "style": style, "color": color, "view": view},
    )
