"""UI widget components — button, panel, health-bar, icon."""

from __future__ import annotations

from typing import Any

from pml.graphics.objects import GraphicObject
from pml.sprites.validator import ParamSchema, validate_params
from pml.types import Symbol

# ======================================================================
# Schemas
# ======================================================================

_BUTTON_SCHEMA = (
    ParamSchema()
    .any_type("label", "Button")
    .number("width", 120, min_val=40, max_val=600)
    .number("height", 40, min_val=20, max_val=200)
    .enum("style", ["rounded", "sharp", "pixel", "ornate"], "rounded")
    .enum("state", ["normal", "hover", "pressed", "disabled"], "normal")
    .color("color", "#3498db")
    .color("text-color", "#FFFFFF")
)

_PANEL_SCHEMA = (
    ParamSchema()
    .number("width", 200, min_val=60, max_val=800)
    .number("height", 120, min_val=40, max_val=600)
    .enum("style", ["simple", "ornate", "pixel", "glass"], "simple")
    .any_type("title", "")
    .color("color", "#2c3e50")
    .number("border-width", 2, min_val=0, max_val=10)
)

_HEALTH_BAR_SCHEMA = (
    ParamSchema()
    .number("value", 0.75, min_val=0.0, max_val=1.0)
    .number("width", 150, min_val=40, max_val=400)
    .number("height", 16, min_val=6, max_val=60)
    .color("color", "#e74c3c")
    .color("bg-color", "#2c3e50")
    .enum("style", ["flat", "segmented", "gradient"], "flat")
)

_ICON_SCHEMA = (
    ParamSchema()
    .enum("type", ["heart", "star", "coin", "gem", "shield", "skull"], "heart")
    .number("size", 24, min_val=8, max_val=128)
    .color("color", "#e74c3c")
    .enum("style", ["flat", "pixel", "detailed"], "flat")
)


def _sym_str(v: Any) -> str:
    if isinstance(v, Symbol):
        return v.name
    return str(v) if v is not None else ""


# ======================================================================
# Button
# ======================================================================

_STATE_COLORS = {
    "normal": 0,
    "hover": 20,
    "pressed": -30,
    "disabled": -60,
}


def _shift_color(hex_color: str, amount: int) -> str:
    """Lighten or darken a hex color by amount."""
    try:
        h = hex_color.lstrip("#")
        if len(h) == 3:
            h = h[0] * 2 + h[1] * 2 + h[2] * 2
        r = max(0, min(255, int(h[0:2], 16) + amount))
        g = max(0, min(255, int(h[2:4], 16) + amount))
        b = max(0, min(255, int(h[4:6], 16) + amount))
        return f"#{r:02x}{g:02x}{b:02x}"
    except (ValueError, IndexError):
        return hex_color


def create_button(**kwargs: Any) -> GraphicObject:
    p = validate_params(_BUTTON_SCHEMA, {_sym_str(k): v for k, v in kwargs.items()})
    w, h = p["width"], p["height"]
    color = _shift_color(p["color"], _STATE_COLORS.get(p["state"], 0))
    children: list[GraphicObject] = []

    style = p["style"]
    if style == "rounded":
        # Rounded rect approximated as rect + corner ellipses
        r = min(h / 3, 10)
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -w / 2 + r, "y": -h / 2, "w": w - 2 * r, "h": h},
            fill=color, stroke="#1a1a1a", stroke_width=2.0,
        ))
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -w / 2, "y": -h / 2 + r, "w": w, "h": h - 2 * r},
            fill=color,
        ))
        for cx, cy in [(-w / 2 + r, -h / 2 + r), (w / 2 - r, -h / 2 + r),
                        (-w / 2 + r, h / 2 - r), (w / 2 - r, h / 2 - r)]:
            children.append(GraphicObject(
                shape_type="ellipse",
                params={"cx": cx, "cy": cy, "rx": r, "ry": r},
                fill=color,
            ))
    elif style == "pixel":
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -w / 2, "y": -h / 2, "w": w, "h": h},
            fill=color, stroke="#000000", stroke_width=3.0,
        ))
        # Pixel border highlight
        children.append(GraphicObject(
            shape_type="line",
            params={"x1": -w / 2 + 2, "y1": -h / 2 + 2, "x2": w / 2 - 2, "y2": -h / 2 + 2},
            stroke="#FFFFFF60", stroke_width=2.0,
        ))
    elif style == "ornate":
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -w / 2, "y": -h / 2, "w": w, "h": h},
            fill=color, stroke="#d4ac0f", stroke_width=3.0,
        ))
        # Corner decorations
        for cx, cy in [(-w / 2, -h / 2), (w / 2, -h / 2), (-w / 2, h / 2), (w / 2, h / 2)]:
            children.append(GraphicObject(
                shape_type="ellipse",
                params={"cx": cx, "cy": cy, "rx": 4, "ry": 4},
                fill="#d4ac0f",
            ))
    else:
        # sharp
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -w / 2, "y": -h / 2, "w": w, "h": h},
            fill=color, stroke="#1a1a1a", stroke_width=2.0,
        ))

    # Label text
    if p["label"]:
        children.append(GraphicObject(
            shape_type="text",
            params={"text": str(p["label"]), "x": -len(str(p["label"])) * 3.5, "y": 4,
                    "font_size": int(h * 0.4), "font_family": "default"},
            fill=p["text-color"],
        ))

    return GraphicObject(
        shape_type="group",
        children=tuple(children),
        metadata={"component": "button", "state": p["state"]},
    )


# ======================================================================
# Panel
# ======================================================================


def create_panel(**kwargs: Any) -> GraphicObject:
    p = validate_params(_PANEL_SCHEMA, {_sym_str(k): v for k, v in kwargs.items()})
    w, h = p["width"], p["height"]
    bw = p["border-width"]
    children: list[GraphicObject] = []

    style = p["style"]
    bg = p["color"]
    border_c = "#d4ac0f" if style == "ornate" else "#1a1a1a"

    if style == "glass":
        bg = bg + "80"  # semi-transparent

    # Main panel body
    children.append(GraphicObject(
        shape_type="rect",
        params={"x": -w / 2, "y": -h / 2, "w": w, "h": h},
        fill=bg, stroke=border_c, stroke_width=bw,
    ))

    # Title bar
    if p["title"]:
        title_h = 20
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -w / 2, "y": -h / 2, "w": w, "h": title_h},
            fill=_shift_color(bg, 20), stroke=border_c, stroke_width=1.0,
        ))
        children.append(GraphicObject(
            shape_type="text",
            params={"text": str(p["title"]), "x": -w / 2 + 8, "y": -h / 2 + 14,
                    "font_size": 12, "font_family": "default"},
            fill="#FFFFFF",
        ))

    if style == "ornate":
        # Corner flourishes
        corner_r = 6
        for cx, cy in [(-w / 2, -h / 2), (w / 2, -h / 2), (-w / 2, h / 2), (w / 2, h / 2)]:
            children.append(GraphicObject(
                shape_type="ellipse",
                params={"cx": cx, "cy": cy, "rx": corner_r, "ry": corner_r},
                fill="#d4ac0f", stroke="#1a1a1a", stroke_width=1.0,
            ))

    return GraphicObject(
        shape_type="group",
        children=tuple(children),
        metadata={"component": "panel", "style": style},
    )


# ======================================================================
# Health Bar
# ======================================================================


def create_health_bar(**kwargs: Any) -> GraphicObject:
    p = validate_params(_HEALTH_BAR_SCHEMA, {_sym_str(k): v for k, v in kwargs.items()})
    w, h = p["width"], p["height"]
    value = p["value"]
    children: list[GraphicObject] = []

    # Background
    children.append(GraphicObject(
        shape_type="rect",
        params={"x": -w / 2, "y": -h / 2, "w": w, "h": h},
        fill=p["bg-color"], stroke="#1a1a1a", stroke_width=1.5,
    ))

    # Fill
    fill_w = max(0, (w - 4) * value)
    bar_style = p["style"]

    if bar_style == "segmented":
        seg_count = 10
        seg_w = (w - 4) / seg_count
        filled_segs = int(seg_count * value)
        for i in range(filled_segs):
            children.append(GraphicObject(
                shape_type="rect",
                params={"x": -w / 2 + 2 + i * seg_w, "y": -h / 2 + 2, "w": seg_w - 1, "h": h - 4},
                fill=p["color"],
            ))
    elif bar_style == "gradient":
        # Approximate gradient with two rects
        mid_w = fill_w / 2
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -w / 2 + 2, "y": -h / 2 + 2, "w": mid_w, "h": h - 4},
            fill=_shift_color(p["color"], 30),
        ))
        if fill_w > mid_w:
            children.append(GraphicObject(
                shape_type="rect",
                params={"x": -w / 2 + 2 + mid_w, "y": -h / 2 + 2, "w": fill_w - mid_w, "h": h - 4},
                fill=p["color"],
            ))
    else:
        # flat
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -w / 2 + 2, "y": -h / 2 + 2, "w": fill_w, "h": h - 4},
            fill=p["color"],
        ))

    return GraphicObject(
        shape_type="group",
        children=tuple(children),
        metadata={"component": "health-bar", "value": value},
    )


# ======================================================================
# Icon
# ======================================================================


def create_icon(**kwargs: Any) -> GraphicObject:
    p = validate_params(_ICON_SCHEMA, {_sym_str(k): v for k, v in kwargs.items()})
    s = p["size"] / 24  # normalize to base 24px
    color = p["color"]
    children: list[GraphicObject] = []

    itype = p["type"]

    if itype == "heart":
        # Two circles + triangle
        r = 7 * s
        children.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": -5 * s, "cy": -3 * s, "rx": r, "ry": r},
            fill=color, stroke="#1a1a1a", stroke_width=1.5,
        ))
        children.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": 5 * s, "cy": -3 * s, "rx": r, "ry": r},
            fill=color, stroke="#1a1a1a", stroke_width=1.5,
        ))
        children.append(GraphicObject(
            shape_type="polygon",
            params={"points": [(-12 * s, 0), (12 * s, 0), (0, 14 * s)]},
            fill=color, stroke="#1a1a1a", stroke_width=1.5,
        ))

    elif itype == "star":
        # 5-pointed star
        import math
        points = []
        for i in range(10):
            angle = math.pi / 2 + i * math.pi / 5
            r = 12 * s if i % 2 == 0 else 5 * s
            points.append((r * math.cos(angle), -r * math.sin(angle)))
        children.append(GraphicObject(
            shape_type="polygon",
            params={"points": points},
            fill=color, stroke="#1a1a1a", stroke_width=1.5,
        ))

    elif itype == "coin":
        children.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": 0, "cy": 0, "rx": 10 * s, "ry": 10 * s},
            fill=color, stroke="#1a1a1a", stroke_width=2.0,
        ))
        children.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": 0, "cy": 0, "rx": 7 * s, "ry": 7 * s},
            fill=None, stroke=_shift_color(color, -40), stroke_width=1.0,
        ))

    elif itype == "gem":
        children.append(GraphicObject(
            shape_type="polygon",
            params={"points": [
                (0, -12 * s), (10 * s, -4 * s), (8 * s, 8 * s),
                (-8 * s, 8 * s), (-10 * s, -4 * s),
            ]},
            fill=color, stroke="#1a1a1a", stroke_width=1.5,
        ))
        # Highlight
        children.append(GraphicObject(
            shape_type="line",
            params={"x1": -4 * s, "y1": -6 * s, "x2": 2 * s, "y2": -2 * s},
            stroke="#FFFFFF80", stroke_width=2.0,
        ))

    elif itype == "shield":
        children.append(GraphicObject(
            shape_type="polygon",
            params={"points": [
                (0, -12 * s), (12 * s, -6 * s), (10 * s, 8 * s),
                (0, 14 * s), (-10 * s, 8 * s), (-12 * s, -6 * s),
            ]},
            fill=color, stroke="#1a1a1a", stroke_width=2.0,
        ))
        # Center line
        children.append(GraphicObject(
            shape_type="line",
            params={"x1": 0, "y1": -10 * s, "x2": 0, "y2": 12 * s},
            stroke="#1a1a1a", stroke_width=1.5,
        ))

    elif itype == "skull":
        # Head
        children.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": 0, "cy": -4 * s, "rx": 10 * s, "ry": 10 * s},
            fill=color, stroke="#1a1a1a", stroke_width=2.0,
        ))
        # Jaw
        children.append(GraphicObject(
            shape_type="rect",
            params={"x": -6 * s, "y": 4 * s, "w": 12 * s, "h": 6 * s},
            fill=color, stroke="#1a1a1a", stroke_width=1.5,
        ))
        # Eyes
        for dx in (-4 * s, 4 * s):
            children.append(GraphicObject(
                shape_type="ellipse",
                params={"cx": dx, "cy": -5 * s, "rx": 3 * s, "ry": 3 * s},
                fill="#1a1a1a",
            ))

    return GraphicObject(
        shape_type="group",
        children=tuple(children),
        metadata={"component": "icon", "type": itype},
    )
