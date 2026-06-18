"""Item components — weapon, potion, chest, generic-item."""

from __future__ import annotations

import math
from typing import Any

from pml.graphics.objects import GraphicObject
from pml.sprites.validator import ParamSchema, validate_params

from .view_utils import sym_str

# ======================================================================
# Schemas
# ======================================================================

_WEAPON_SCHEMA = (
    ParamSchema()
    .enum("type", ["sword", "axe", "bow", "staff", "dagger", "spear", "gun"], "sword")
    .enum("size", ["small", "medium", "large", "legendary"], "medium")
    .enum("material", ["iron", "steel", "gold", "crystal", "wood", "bone"], "steel")
    .enum("element", ["none", "fire", "ice", "lightning", "holy", "dark"], "none")
    .enum("ornament", ["plain", "gem", "rune", "engraved"], "plain")
)

_POTION_SCHEMA = (
    ParamSchema()
    .enum("type", ["health", "mana", "buff", "poison", "bomb"], "health")
    .enum("container", ["bottle", "flask", "vial", "jar"], "bottle")
    .color("color", "#e74c3c")
    .enum("size", ["small", "medium", "large"], "medium")
)

_CHEST_SCHEMA = (
    ParamSchema()
    .enum("type", ["wooden", "iron", "gold", "crystal"], "wooden")
    .enum("state", ["closed", "open", "locked"], "closed")
    .enum("size", ["small", "medium", "large"], "medium")
)

_GENERIC_ITEM_SCHEMA = (
    ParamSchema()
    .any_type("name", "item")
    .enum("base-shape", ["circle", "rect", "diamond", "custom"], "circle")
    .color("color", "#95a5a6")
    .any_type("detail", None)
    .boolean("outline", True)
)

# ======================================================================
# Material colors
# ======================================================================

_MATERIAL_COLORS = {
    "iron": "#7f8c8d",
    "steel": "#bdc3c7",
    "gold": "#f1c40f",
    "crystal": "#85c1e9",
    "wood": "#8B4513",
    "bone": "#f5f5dc",
}

_ELEMENT_COLORS = {
    "fire": "#e74c3c",
    "ice": "#3498db",
    "lightning": "#f1c40f",
    "holy": "#f9e79f",
    "dark": "#6c3483",
}

_SIZE_SCALE = {"small": 0.7, "medium": 1.0, "large": 1.4, "legendary": 1.8}


def _element_glow(element: str, cx: float, cy: float, r: float) -> list[GraphicObject]:
    """Add element glow effect."""
    if element == "none" or element not in _ELEMENT_COLORS:
        return []
    color = _ELEMENT_COLORS[element]
    return [GraphicObject(
        shape_type="ellipse",
        params={"cx": cx, "cy": cy, "rx": r, "ry": r},
        fill=color + "40",
        stroke=color,
        stroke_width=1.0,
    )]


def _ornament_deco(ornament: str, x: float, y: float) -> list[GraphicObject]:
    """Add ornament decoration."""
    if ornament == "plain":
        return []
    if ornament == "gem":
        return [GraphicObject(
            shape_type="ellipse",
            params={"cx": x, "cy": y, "rx": 4, "ry": 4},
            fill="#e74c3c", stroke="#1a1a1a", stroke_width=1.0,
        )]
    if ornament == "rune":
        return [GraphicObject(
            shape_type="line",
            params={"x1": x - 3, "y1": y - 3, "x2": x + 3, "y2": y + 3},
            stroke="#85c1e9", stroke_width=1.5,
        )]
    if ornament == "engraved":
        return [GraphicObject(
            shape_type="line",
            params={"x1": x - 4, "y1": y, "x2": x + 4, "y2": y},
            stroke="#1a1a1a", stroke_width=0.8,
        )]
    return []


# ======================================================================
# Weapon
# ======================================================================


def create_weapon(**kwargs: Any) -> GraphicObject:
    p = validate_params(_WEAPON_SCHEMA, {sym_str(k): v for k, v in kwargs.items()})
    s = _SIZE_SCALE.get(p["size"], 1.0)
    mat_color = _MATERIAL_COLORS.get(p["material"], "#bdc3c7")
    children: list[GraphicObject] = []

    wtype = p["type"]

    if wtype == "sword":
        blade_h = 60 * s
        # Blade
        children.append(GraphicObject(
            shape_type="polygon",
            params={"points": [(0, 0), (-5 * s, -blade_h), (5 * s, -blade_h)]},
            fill=mat_color, stroke="#1a1a1a", stroke_width=1.5,
        ))
        # Guard
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -12 * s, "y": 0, "w": 24 * s, "h": 4 * s},
            fill="#8B4513", stroke="#1a1a1a", stroke_width=1.0,
        ))
        # Handle
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -3 * s, "y": 4 * s, "w": 6 * s, "h": 16 * s},
            fill="#5D4037", stroke="#1a1a1a", stroke_width=1.0,
        ))
        children.extend(_ornament_deco(p["ornament"], 0, -blade_h * 0.5))

    elif wtype == "axe":
        handle_h = 50 * s
        # Handle
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -2 * s, "y": 0, "w": 4 * s, "h": handle_h},
            fill="#8B4513", stroke="#1a1a1a", stroke_width=1.0,
        ))
        # Axe head
        children.append(GraphicObject(
            shape_type="polygon",
            params={"points": [
                (2 * s, 4 * s), (20 * s, -8 * s), (20 * s, 12 * s),
            ]},
            fill=mat_color, stroke="#1a1a1a", stroke_width=1.5,
        ))

    elif wtype == "bow":
        bow_h = 50 * s
        # Bow arc (two lines forming arc shape)
        children.append(GraphicObject(
            shape_type="line",
            params={"x1": 0, "y1": -bow_h / 2, "x2": 8 * s, "y2": 0},
            stroke=mat_color, stroke_width=3 * s,
        ))
        children.append(GraphicObject(
            shape_type="line",
            params={"x1": 8 * s, "y1": 0, "x2": 0, "y2": bow_h / 2},
            stroke=mat_color, stroke_width=3 * s,
        ))
        # String
        children.append(GraphicObject(
            shape_type="line",
            params={"x1": 0, "y1": -bow_h / 2, "x2": 0, "y2": bow_h / 2},
            stroke="#f5f5dc", stroke_width=1.0,
        ))

    elif wtype == "staff":
        staff_h = 80 * s
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -2 * s, "y": -staff_h, "w": 4 * s, "h": staff_h},
            fill=mat_color if p["material"] == "wood" else "#5D4037",
            stroke="#1a1a1a", stroke_width=1.5,
        ))
        # Orb on top
        children.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": 0, "cy": -staff_h - 8 * s, "rx": 8 * s, "ry": 8 * s},
            fill=_ELEMENT_COLORS.get(p["element"], "#85c1e9"),
            stroke="#1a1a1a", stroke_width=1.5,
        ))
        children.extend(_element_glow(p["element"], 0, -staff_h - 8 * s, 12 * s))

    elif wtype == "dagger":
        blade_h = 30 * s
        children.append(GraphicObject(
            shape_type="polygon",
            params={"points": [(0, 0), (-4 * s, -blade_h), (4 * s, -blade_h)]},
            fill=mat_color, stroke="#1a1a1a", stroke_width=1.5,
        ))
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -8 * s, "y": 0, "w": 16 * s, "h": 3 * s},
            fill="#8B4513", stroke="#1a1a1a", stroke_width=1.0,
        ))
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -2 * s, "y": 3 * s, "w": 4 * s, "h": 10 * s},
            fill="#5D4037", stroke="#1a1a1a", stroke_width=1.0,
        ))

    elif wtype == "spear":
        shaft_h = 80 * s
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -2 * s, "y": -shaft_h, "w": 4 * s, "h": shaft_h},
            fill="#8B4513", stroke="#1a1a1a", stroke_width=1.0,
        ))
        # Spear tip
        children.append(GraphicObject(
            shape_type="polygon",
            params={"points": [(0, -shaft_h - 16 * s), (-5 * s, -shaft_h), (5 * s, -shaft_h)]},
            fill=mat_color, stroke="#1a1a1a", stroke_width=1.5,
        ))

    elif wtype == "gun":
        # Barrel
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": 0, "y": -4 * s, "w": 30 * s, "h": 8 * s},
            fill=mat_color, stroke="#1a1a1a", stroke_width=1.5,
        ))
        # Grip
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": 2 * s, "y": 4 * s, "w": 8 * s, "h": 16 * s},
            fill="#5D4037", stroke="#1a1a1a", stroke_width=1.0,
        ))

    children.extend(_element_glow(p["element"], 0, -20 * s, 20 * s))

    return GraphicObject(
        shape_type="group",
        children=tuple(children),
        metadata={"component": "weapon", "type": wtype, "size": p["size"]},
    )


# ======================================================================
# Potion
# ======================================================================

_POTION_COLORS = {
    "health": "#e74c3c",
    "mana": "#3498db",
    "buff": "#f1c40f",
    "poison": "#27ae60",
    "bomb": "#e67e22",
}


def create_potion(**kwargs: Any) -> GraphicObject:
    p = validate_params(_POTION_SCHEMA, {sym_str(k): v for k, v in kwargs.items()})
    s = _SIZE_SCALE.get(p["size"], 1.0)
    liquid_color = _POTION_COLORS.get(p["type"], p["color"])
    children: list[GraphicObject] = []

    container = p["container"]

    if container == "bottle":
        # Body
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -10 * s, "y": -20 * s, "w": 20 * s, "h": 24 * s},
            fill="#d5e8f0", stroke="#1a1a1a", stroke_width=1.5,
        ))
        # Liquid inside
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -8 * s, "y": -14 * s, "w": 16 * s, "h": 16 * s},
            fill=liquid_color,
        ))
        # Neck
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -4 * s, "y": -28 * s, "w": 8 * s, "h": 10 * s},
            fill="#d5e8f0", stroke="#1a1a1a", stroke_width=1.0,
        ))
        # Cork
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -5 * s, "y": -32 * s, "w": 10 * s, "h": 5 * s},
            fill="#8B4513", stroke="#1a1a1a", stroke_width=1.0,
        ))
    elif container == "flask":
        # Round body
        children.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": 0, "cy": -8 * s, "rx": 14 * s, "ry": 14 * s},
            fill="#d5e8f0", stroke="#1a1a1a", stroke_width=1.5,
        ))
        children.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": 0, "cy": -6 * s, "rx": 11 * s, "ry": 10 * s},
            fill=liquid_color,
        ))
        # Neck
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -3 * s, "y": -26 * s, "w": 6 * s, "h": 10 * s},
            fill="#d5e8f0", stroke="#1a1a1a", stroke_width=1.0,
        ))
    elif container == "vial":
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -6 * s, "y": -24 * s, "w": 12 * s, "h": 28 * s},
            fill="#d5e8f0", stroke="#1a1a1a", stroke_width=1.5,
        ))
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -4 * s, "y": -16 * s, "w": 8 * s, "h": 18 * s},
            fill=liquid_color,
        ))
    else:  # jar
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -12 * s, "y": -18 * s, "w": 24 * s, "h": 22 * s},
            fill="#d5e8f0", stroke="#1a1a1a", stroke_width=1.5,
        ))
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -10 * s, "y": -14 * s, "w": 20 * s, "h": 16 * s},
            fill=liquid_color,
        ))
        # Lid
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -14 * s, "y": -22 * s, "w": 28 * s, "h": 5 * s},
            fill="#7f8c8d", stroke="#1a1a1a", stroke_width=1.0,
        ))

    return GraphicObject(
        shape_type="group",
        children=tuple(children),
        metadata={"component": "potion", "type": p["type"], "container": container},
    )


# ======================================================================
# Chest
# ======================================================================

_CHEST_COLORS = {
    "wooden": ("#8B4513", "#5D4037"),
    "iron": ("#7f8c8d", "#5d6d7e"),
    "gold": ("#f1c40f", "#d4ac0f"),
    "crystal": ("#85c1e9", "#5dade2"),
}


def create_chest(**kwargs: Any) -> GraphicObject:
    p = validate_params(_CHEST_SCHEMA, {sym_str(k): v for k, v in kwargs.items()})
    s = _SIZE_SCALE.get(p["size"], 1.0)
    main_c, dark_c = _CHEST_COLORS.get(p["type"], ("#8B4513", "#5D4037"))
    children: list[GraphicObject] = []

    w = 40 * s
    h = 28 * s

    # Body
    children.append(GraphicObject(
        shape_type="rect",
        params={"x": -w / 2, "y": -h / 2, "w": w, "h": h},
        fill=main_c, stroke="#1a1a1a", stroke_width=2.0,
    ))

    if p["state"] == "open":
        # Open lid (tilted back)
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -w / 2, "y": -h / 2 - h * 0.5, "w": w, "h": h * 0.4},
            fill=dark_c, stroke="#1a1a1a", stroke_width=1.5,
        ))
        # Gold glow inside
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -w / 2 + 4, "y": -h / 2 + 2, "w": w - 8, "h": h * 0.4},
            fill="#f9e79f",
        ))
    else:
        # Closed lid
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -w / 2, "y": -h / 2 - h * 0.3, "w": w, "h": h * 0.35},
            fill=dark_c, stroke="#1a1a1a", stroke_width=1.5,
        ))
        # Lock
        if p["state"] == "locked":
            children.append(GraphicObject(
                shape_type="ellipse",
                params={"cx": 0, "cy": -h * 0.15, "rx": 4 * s, "ry": 4 * s},
                fill="#f1c40f", stroke="#1a1a1a", stroke_width=1.0,
            ))

    # Metal bands
    children.append(GraphicObject(
        shape_type="line",
        params={"x1": -w / 2, "y1": 0, "x2": w / 2, "y2": 0},
        stroke="#1a1a1a", stroke_width=1.5,
    ))

    return GraphicObject(
        shape_type="group",
        children=tuple(children),
        metadata={"component": "chest", "type": p["type"], "state": p["state"]},
    )


# ======================================================================
# Generic Item
# ======================================================================


def create_generic_item(**kwargs: Any) -> GraphicObject:
    p = validate_params(_GENERIC_ITEM_SCHEMA, {sym_str(k): v for k, v in kwargs.items()})
    children: list[GraphicObject] = []
    color = p["color"]
    outline = "#1a1a1a" if p["outline"] else None
    ow = 2.0 if p["outline"] else 0

    shape = p["base-shape"]
    if shape == "circle":
        children.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": 0, "cy": 0, "rx": 16, "ry": 16},
            fill=color, stroke=outline, stroke_width=ow,
        ))
    elif shape == "rect":
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -16, "y": -16, "w": 32, "h": 32},
            fill=color, stroke=outline, stroke_width=ow,
        ))
    elif shape == "diamond":
        children.append(GraphicObject(
            shape_type="polygon",
            params={"points": [(0, -18), (18, 0), (0, 18), (-18, 0)]},
            fill=color, stroke=outline, stroke_width=ow,
        ))

    # Overlay detail if provided
    detail = p.get("detail")
    if isinstance(detail, GraphicObject):
        children.append(detail)

    return GraphicObject(
        shape_type="group",
        children=tuple(children),
        metadata={"component": "generic-item", "name": p["name"]},
    )
