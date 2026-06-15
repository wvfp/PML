"""Anime eyes component — generates eye GraphicObjects in various styles."""

from __future__ import annotations

from typing import Any

from pml.graphics.objects import GraphicObject
from pml.sprites.validator import ParamSchema, validate_params
from pml.types import Symbol

_EYES_SCHEMA = (
    ParamSchema()
    .enum("style", ["shoujo", "shounen", "round", "sharp", "sleepy", "closed"], "shoujo")
    .color("color", "#4a90d9")
    .number("size", 1.0, min_val=0.3, max_val=3.0)
    .number("spacing", 1.0, min_val=0.5, max_val=2.0)
    .boolean("highlight", True)
    .enum("lashes", ["none", "short", "long"], "none")
)


def _sym_str(v: Any) -> str:
    if isinstance(v, Symbol):
        return v.name
    return str(v) if v is not None else ""


def _make_eye_shoujo(cx: float, cy: float, size: float, color: str, highlight: bool) -> list[GraphicObject]:
    """Shoujo (large, expressive) eyes."""
    parts: list[GraphicObject] = []
    ew = 14 * size
    eh = 18 * size

    # White
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": cx, "cy": cy, "rx": ew, "ry": eh},
        fill="#FFFFFF",
        stroke="#1a1a1a",
        stroke_width=2.0,
    ))

    # Iris
    iris_r = ew * 0.7
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": cx, "cy": cy + 2, "rx": iris_r, "ry": iris_r * 1.1},
        fill=color,
        stroke="#1a1a1a",
        stroke_width=1.0,
    ))

    # Pupil
    pupil_r = iris_r * 0.5
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": cx, "cy": cy + 2, "rx": pupil_r, "ry": pupil_r},
        fill="#0a0a0a",
    ))

    # Highlight
    if highlight:
        hl_r = ew * 0.3
        parts.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": cx - ew * 0.2, "cy": cy - eh * 0.2, "rx": hl_r, "ry": hl_r},
            fill="#FFFFFF",
        ))
        # Small secondary highlight
        parts.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": cx + ew * 0.25, "cy": cy + eh * 0.15, "rx": hl_r * 0.5, "ry": hl_r * 0.5},
            fill="#FFFFFF",
        ))

    return parts


def _make_eye_shounen(cx: float, cy: float, size: float, color: str, highlight: bool) -> list[GraphicObject]:
    """Shounen (sharper, more angular) eyes."""
    parts: list[GraphicObject] = []
    ew = 12 * size
    eh = 10 * size

    # White (more rectangular)
    parts.append(GraphicObject(
        shape_type="rect",
        params={"x": cx - ew, "y": cy - eh / 2, "w": ew * 2, "h": eh},
        fill="#FFFFFF",
        stroke="#1a1a1a",
        stroke_width=2.0,
    ))

    # Iris
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": cx, "cy": cy, "rx": ew * 0.5, "ry": eh * 0.45},
        fill=color,
        stroke="#1a1a1a",
        stroke_width=1.0,
    ))

    # Pupil
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": cx, "cy": cy, "rx": ew * 0.25, "ry": eh * 0.3},
        fill="#0a0a0a",
    ))

    if highlight:
        parts.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": cx - ew * 0.2, "cy": cy - eh * 0.15, "rx": ew * 0.15, "ry": ew * 0.15},
            fill="#FFFFFF",
        ))

    # Eyebrow (angular line above)
    parts.append(GraphicObject(
        shape_type="line",
        params={"x1": cx - ew, "y1": cy - eh - 4, "x2": cx + ew * 0.8, "y2": cy - eh - 2},
        stroke="#1a1a1a",
        stroke_width=3.0,
    ))

    return parts


def _make_eye_round(cx: float, cy: float, size: float, color: str, highlight: bool) -> list[GraphicObject]:
    """Round (cute, chibi-style) eyes."""
    parts: list[GraphicObject] = []
    r = 10 * size

    # Big round eye
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": cx, "cy": cy, "rx": r, "ry": r},
        fill="#1a1a1a",
    ))

    # Color ring
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": cx, "cy": cy + 1, "rx": r * 0.7, "ry": r * 0.7},
        fill=color,
    ))

    # Pupil
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": cx, "cy": cy + 1, "rx": r * 0.35, "ry": r * 0.35},
        fill="#0a0a0a",
    ))

    if highlight:
        parts.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": cx - r * 0.3, "cy": cy - r * 0.3, "rx": r * 0.35, "ry": r * 0.35},
            fill="#FFFFFF",
        ))

    return parts


def _make_eye_sharp(cx: float, cy: float, size: float, color: str, highlight: bool) -> list[GraphicObject]:
    """Sharp (narrow, intense) eyes."""
    parts: list[GraphicObject] = []
    ew = 14 * size
    eh = 5 * size

    # Narrow white
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": cx, "cy": cy, "rx": ew, "ry": eh},
        fill="#FFFFFF",
        stroke="#1a1a1a",
        stroke_width=2.5,
    ))

    # Iris
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": cx, "cy": cy, "rx": ew * 0.4, "ry": eh * 0.85},
        fill=color,
    ))

    # Pupil
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": cx, "cy": cy, "rx": ew * 0.2, "ry": eh * 0.6},
        fill="#0a0a0a",
    ))

    if highlight:
        parts.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": cx - ew * 0.15, "cy": cy - eh * 0.3, "rx": ew * 0.1, "ry": ew * 0.1},
            fill="#FFFFFF",
        ))

    return parts


def _make_eye_sleepy(cx: float, cy: float, size: float, color: str, highlight: bool) -> list[GraphicObject]:
    """Sleepy (half-closed) eyes."""
    parts: list[GraphicObject] = []
    ew = 12 * size
    eh = 4 * size

    # Half-visible eye
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": cx, "cy": cy + eh, "rx": ew, "ry": eh * 2},
        fill="#FFFFFF",
        stroke="#1a1a1a",
        stroke_width=1.5,
    ))

    # Upper eyelid line
    parts.append(GraphicObject(
        shape_type="line",
        params={"x1": cx - ew, "y1": cy, "x2": cx + ew, "y2": cy},
        stroke="#1a1a1a",
        stroke_width=2.5,
    ))

    # Small iris peek
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": cx, "cy": cy + eh * 0.5, "rx": ew * 0.35, "ry": eh * 0.7},
        fill=color,
    ))

    return parts


def _make_eye_closed(cx: float, cy: float, size: float, color: str, highlight: bool) -> list[GraphicObject]:
    """Closed eyes (arc line)."""
    parts: list[GraphicObject] = []
    ew = 12 * size

    # Simple arc approximated as a line with thickness
    parts.append(GraphicObject(
        shape_type="line",
        params={"x1": cx - ew, "y1": cy, "x2": cx + ew, "y2": cy},
        stroke="#1a1a1a",
        stroke_width=3.0 * size,
    ))

    # Lashes (optional)
    for dx in (-ew * 0.7, 0, ew * 0.7):
        parts.append(GraphicObject(
            shape_type="line",
            params={"x1": cx + dx, "y1": cy, "x2": cx + dx, "y2": cy - 4 * size},
            stroke="#1a1a1a",
            stroke_width=1.5,
        ))

    return parts


_EYE_MAKERS = {
    "shoujo": _make_eye_shoujo,
    "shounen": _make_eye_shounen,
    "round": _make_eye_round,
    "sharp": _make_eye_sharp,
    "sleepy": _make_eye_sleepy,
    "closed": _make_eye_closed,
}


def create_eyes(**kwargs: Any) -> GraphicObject:
    """Create anime-style eyes.

    Args:
        :style — 'shoujo | 'shounen | 'round | 'sharp | 'sleepy | 'closed
        :color — iris color
        :size — scale factor (default 1.0)
        :spacing — eye spacing factor (default 1.0)
        :highlight — whether to draw eye highlights
        :lashes — 'none | 'short | 'long
    """
    p = validate_params(_EYES_SCHEMA, {_sym_str(k): v for k, v in kwargs.items()})

    style = p["style"]
    color = p["color"]
    size = p["size"]
    spacing = p["spacing"]
    highlight = p["highlight"]

    base_spacing = 24 * spacing
    left_cx = -base_spacing / 2
    right_cx = base_spacing / 2

    maker = _EYE_MAKERS.get(style, _make_eye_shoujo)
    left_parts = maker(left_cx, 0, size, color, highlight)
    right_parts = maker(right_cx, 0, size, color, highlight)

    all_children = left_parts + right_parts

    # Add lashes if requested
    if p["lashes"] in ("short", "long"):
        lash_len = 6 if p["lashes"] == "short" else 10
        for cx in (left_cx, right_cx):
            for dx in (-10 * size, 0, 10 * size):
                all_children.append(GraphicObject(
                    shape_type="line",
                    params={
                        "x1": cx + dx,
                        "y1": -12 * size,
                        "x2": cx + dx + 2,
                        "y2": -12 * size - lash_len * size,
                    },
                    stroke="#1a1a1a",
                    stroke_width=1.5,
                ))

    return GraphicObject(
        shape_type="group",
        children=tuple(all_children),
        metadata={"component": "eyes", "style": style, "color": color},
    )
