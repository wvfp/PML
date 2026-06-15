"""Scene elements — tile, decoration, background."""

from __future__ import annotations

import math
import random
from typing import Any

from pml.graphics.objects import GraphicObject
from pml.sprites.validator import ParamSchema, validate_params
from pml.types import Symbol

# ======================================================================
# Schemas
# ======================================================================

_TILE_SCHEMA = (
    ParamSchema()
    .enum("type", ["grass", "stone", "wood", "sand", "water", "snow", "brick"], "grass")
    .number("size", 32, min_val=8, max_val=128)
    .number("variant", 0, min_val=0, max_val=3)
    .enum("edge", ["none", "top", "bottom", "left", "right"], "none")
)

_DECORATION_SCHEMA = (
    ParamSchema()
    .enum("type", [
        "tree", "bush", "rock", "flower", "mushroom",
        "crate", "barrel", "torch", "sign", "fence", "lamp",
    ], "tree")
    .enum("size", ["small", "medium", "large"], "medium")
    .enum("season", ["spring", "summer", "autumn", "winter"], "summer")
    .number("variant", 0, min_val=0, max_val=3)
)

_BACKGROUND_SCHEMA = (
    ParamSchema()
    .enum("type", ["sky", "forest", "dungeon", "town", "ocean", "mountain"], "sky")
    .enum("time", ["day", "dusk", "night", "dawn"], "day")
    .enum("weather", ["clear", "cloudy", "rain", "snow", "fog"], "clear")
    .number("width", 256, min_val=64, max_val=1024)
    .number("height", 256, min_val=64, max_val=1024)
    .number("parallax", 1.0, min_val=0.1, max_val=5.0)
)

# ======================================================================
# Tile colors
# ======================================================================

_TILE_COLORS = {
    "grass":  {"base": "#4a7c2e", "detail": "#5d9636", "edge": "#3a6424"},
    "stone":  {"base": "#7f8c8d", "detail": "#95a5a6", "edge": "#616a6b"},
    "wood":   {"base": "#8B6914", "detail": "#a37e2c", "edge": "#6b4f10"},
    "sand":   {"base": "#d4b96a", "detail": "#e0ca82", "edge": "#b89e52"},
    "water":  {"base": "#2980b9", "detail": "#3498db", "edge": "#1f6391"},
    "snow":   {"base": "#ecf0f1", "detail": "#ffffff", "edge": "#bdc3c7"},
    "brick":  {"base": "#a0522d", "detail": "#c0603a", "edge": "#7a3e22"},
}

_SIZE_SCALE = {"small": 0.6, "medium": 1.0, "large": 1.5}

_SEASON_COLORS = {
    "spring": {"leaf": "#6abf69", "flower": "#f06292", "trunk": "#6d4c2e"},
    "summer": {"leaf": "#2e7d32", "flower": "#ffeb3b", "trunk": "#5d4037"},
    "autumn": {"leaf": "#e65100", "flower": "#ff8f00", "trunk": "#4e342e"},
    "winter": {"leaf": "#b0bec5", "flower": "#e0e0e0", "trunk": "#6d4c41"},
}


def _sym_str(v: Any) -> str:
    if isinstance(v, Symbol):
        return v.name
    return str(v) if v is not None else ""


# ======================================================================
# Tile
# ======================================================================


def create_tile(**kwargs: Any) -> GraphicObject:
    """Create a terrain tile."""
    p = validate_params(_TILE_SCHEMA, {_sym_str(k): v for k, v in kwargs.items()})
    tile_type = p["type"]
    sz = int(p["size"])
    variant = int(p["variant"])
    edge = p["edge"]
    colors = _TILE_COLORS.get(tile_type, _TILE_COLORS["grass"])

    children: list[GraphicObject] = []
    half = sz / 2

    # Base fill
    children.append(GraphicObject(
        shape_type="rect",
        params={"x": -half, "y": -half, "w": sz, "h": sz},
        fill=colors["base"],
        stroke=None,
    ))

    # Detail pattern based on type and variant
    detail_parts = _tile_detail(tile_type, sz, variant, colors)
    children.extend(detail_parts)

    # Edge highlight/shadow
    if edge != "none":
        children.extend(_tile_edge(edge, sz, colors))

    return GraphicObject(
        shape_type="group",
        children=tuple(children),
        metadata={
            "component": "tile",
            "tile_type": tile_type,
            "size": sz,
            "variant": variant,
            "edge": edge,
        },
    )


def _tile_detail(tile_type: str, sz: int, variant: int, colors: dict) -> list[GraphicObject]:
    """Generate tile surface detail."""
    half = sz / 2
    parts: list[GraphicObject] = []
    # Use variant as seed for reproducible randomness
    rng = random.Random(variant * 31 + hash(tile_type) % 100)

    if tile_type == "grass":
        for _ in range(6):
            x = rng.uniform(-half * 0.8, half * 0.8)
            y = rng.uniform(-half * 0.8, half * 0.8)
            h = rng.uniform(3, 6)
            parts.append(GraphicObject(
                shape_type="line",
                params={"x1": x, "y1": y, "x2": x + rng.uniform(-1, 1), "y2": y - h},
                stroke=colors["detail"], stroke_width=1.0,
            ))

    elif tile_type == "stone":
        for _ in range(3):
            x = rng.uniform(-half * 0.6, half * 0.6)
            y = rng.uniform(-half * 0.6, half * 0.6)
            r = rng.uniform(2, 4)
            parts.append(GraphicObject(
                shape_type="ellipse",
                params={"cx": x, "cy": y, "rx": r, "ry": r * 0.7},
                fill=colors["detail"], stroke=None,
            ))

    elif tile_type == "wood":
        for i in range(3):
            y = -half + (i + 1) * (sz / 4)
            parts.append(GraphicObject(
                shape_type="line",
                params={"x1": -half, "y1": y, "x2": half, "y2": y},
                stroke=colors["detail"], stroke_width=1.0,
            ))

    elif tile_type == "sand":
        for _ in range(8):
            x = rng.uniform(-half * 0.8, half * 0.8)
            y = rng.uniform(-half * 0.8, half * 0.8)
            parts.append(GraphicObject(
                shape_type="circle",
                params={"cx": x, "cy": y, "r": 1},
                fill=colors["detail"], stroke=None,
            ))

    elif tile_type == "water":
        for i in range(2):
            y = -half * 0.4 + i * (sz * 0.4)
            parts.append(GraphicObject(
                shape_type="line",
                params={"x1": -half * 0.6, "y1": y, "x2": half * 0.6, "y2": y + 2},
                stroke=colors["detail"], stroke_width=1.5,
            ))

    elif tile_type == "snow":
        for _ in range(4):
            x = rng.uniform(-half * 0.7, half * 0.7)
            y = rng.uniform(-half * 0.7, half * 0.7)
            parts.append(GraphicObject(
                shape_type="circle",
                params={"cx": x, "cy": y, "r": rng.uniform(1, 2)},
                fill=colors["detail"], stroke=None,
            ))

    elif tile_type == "brick":
        brick_h = sz / 4
        brick_w = sz / 2
        for row in range(4):
            y = -half + row * brick_h
            offset = brick_w / 2 if row % 2 else 0
            for col in range(-1, 3):
                x = -half + col * brick_w + offset
                parts.append(GraphicObject(
                    shape_type="rect",
                    params={"x": x + 0.5, "y": y + 0.5, "w": brick_w - 1, "h": brick_h - 1},
                    fill=None, stroke=colors["edge"], stroke_width=0.8,
                ))

    return parts


def _tile_edge(edge: str, sz: int, colors: dict) -> list[GraphicObject]:
    """Generate edge highlight/shadow."""
    half = sz / 2
    if edge == "top":
        return [GraphicObject(
            shape_type="line",
            params={"x1": -half, "y1": -half, "x2": half, "y2": -half},
            stroke=colors["edge"], stroke_width=2.0,
        )]
    if edge == "bottom":
        return [GraphicObject(
            shape_type="line",
            params={"x1": -half, "y1": half, "x2": half, "y2": half},
            stroke=colors["edge"], stroke_width=2.0,
        )]
    if edge == "left":
        return [GraphicObject(
            shape_type="line",
            params={"x1": -half, "y1": -half, "x2": -half, "y2": half},
            stroke=colors["edge"], stroke_width=2.0,
        )]
    if edge == "right":
        return [GraphicObject(
            shape_type="line",
            params={"x1": half, "y1": -half, "x2": half, "y2": half},
            stroke=colors["edge"], stroke_width=2.0,
        )]
    return []


# ======================================================================
# Decoration
# ======================================================================


def create_decoration(**kwargs: Any) -> GraphicObject:
    """Create a scene decoration object."""
    p = validate_params(_DECORATION_SCHEMA, {_sym_str(k): v for k, v in kwargs.items()})
    dec_type = p["type"]
    scale = _SIZE_SCALE.get(p["size"], 1.0)
    season = p["season"]
    variant = int(p["variant"])
    season_c = _SEASON_COLORS.get(season, _SEASON_COLORS["summer"])

    makers = {
        "tree": _make_tree,
        "bush": _make_bush,
        "rock": _make_rock,
        "flower": _make_flower,
        "mushroom": _make_mushroom,
        "crate": _make_crate,
        "barrel": _make_barrel,
        "torch": _make_torch,
        "sign": _make_sign,
        "fence": _make_fence,
        "lamp": _make_lamp,
    }

    maker = makers.get(dec_type, _make_tree)
    children = maker(scale, season_c, variant)

    return GraphicObject(
        shape_type="group",
        children=tuple(children),
        metadata={
            "component": "decoration",
            "deco_type": dec_type,
            "size": p["size"],
            "season": season,
            "variant": variant,
        },
    )


def _make_tree(scale: float, sc: dict, variant: int) -> list[GraphicObject]:
    """Tree — trunk + canopy layers."""
    parts: list[GraphicObject] = []
    trunk_w = 8 * scale
    trunk_h = 30 * scale
    canopy_r = 22 * scale

    # Trunk
    parts.append(GraphicObject(
        shape_type="rect",
        params={"x": -trunk_w / 2, "y": 0, "w": trunk_w, "h": trunk_h},
        fill=sc["trunk"], stroke="#3e2723", stroke_width=1.0,
    ))

    # Canopy — 2-3 overlapping circles
    rng = random.Random(variant * 17)
    layers = 2 if variant % 2 == 0 else 3
    for i in range(layers):
        ox = rng.uniform(-5, 5) * scale
        oy = -trunk_h * 0.3 - i * canopy_r * 0.5
        r = canopy_r * (1.0 - i * 0.15)
        parts.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": ox, "cy": oy, "rx": r, "ry": r * 0.85},
            fill=sc["leaf"], stroke="#1b5e20", stroke_width=1.0,
        ))

    return parts


def _make_bush(scale: float, sc: dict, variant: int) -> list[GraphicObject]:
    """Bush — low cluster of foliage."""
    parts: list[GraphicObject] = []
    r = 16 * scale
    rng = random.Random(variant * 13)

    for i in range(3):
        ox = (i - 1) * r * 0.6 + rng.uniform(-2, 2)
        parts.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": ox, "cy": -r * 0.3, "rx": r * 0.7, "ry": r * 0.5},
            fill=sc["leaf"], stroke="#2e7d32", stroke_width=1.0,
        ))

    return parts


def _make_rock(scale: float, sc: dict, variant: int) -> list[GraphicObject]:
    """Rock — irregular polygon."""
    parts: list[GraphicObject] = []
    r = 14 * scale
    rng = random.Random(variant * 23 + 7)

    points = []
    n_sides = 6
    for i in range(n_sides):
        angle = (2 * math.pi * i / n_sides) - math.pi / 2
        dist = r * rng.uniform(0.7, 1.1)
        points.append([math.cos(angle) * dist, math.sin(angle) * dist * 0.7])

    parts.append(GraphicObject(
        shape_type="polygon",
        params={"points": points},
        fill="#78909c", stroke="#455a64", stroke_width=1.5,
    ))

    # Highlight
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": -r * 0.2, "cy": -r * 0.2, "rx": r * 0.25, "ry": r * 0.15},
        fill="#90a4ae", stroke=None,
    ))

    return parts


def _make_flower(scale: float, sc: dict, variant: int) -> list[GraphicObject]:
    """Flower — stem + petals + center."""
    parts: list[GraphicObject] = []
    stem_h = 20 * scale
    petal_r = 4 * scale

    # Stem
    parts.append(GraphicObject(
        shape_type="line",
        params={"x1": 0, "y1": 0, "x2": 0, "y2": -stem_h},
        stroke="#4caf50", stroke_width=1.5,
    ))

    # Petals
    n_petals = 5
    cy = -stem_h
    for i in range(n_petals):
        angle = (2 * math.pi * i / n_petals) - math.pi / 2
        px = math.cos(angle) * petal_r * 1.5
        py = cy + math.sin(angle) * petal_r * 1.5
        parts.append(GraphicObject(
            shape_type="ellipse",
            params={"cx": px, "cy": py, "rx": petal_r, "ry": petal_r * 0.7},
            fill=sc["flower"], stroke=None,
        ))

    # Center
    parts.append(GraphicObject(
        shape_type="circle",
        params={"cx": 0, "cy": cy, "r": petal_r * 0.6},
        fill="#ffeb3b", stroke=None,
    ))

    return parts


def _make_mushroom(scale: float, sc: dict, variant: int) -> list[GraphicObject]:
    """Mushroom — stem + cap."""
    parts: list[GraphicObject] = []
    stem_w = 6 * scale
    stem_h = 12 * scale
    cap_r = 12 * scale

    # Stem
    parts.append(GraphicObject(
        shape_type="rect",
        params={"x": -stem_w / 2, "y": -stem_h, "w": stem_w, "h": stem_h},
        fill="#f5f5dc", stroke="#bfae8e", stroke_width=1.0,
    ))

    # Cap
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": 0, "cy": -stem_h - cap_r * 0.3, "rx": cap_r, "ry": cap_r * 0.6},
        fill="#e53935", stroke="#b71c1c", stroke_width=1.0,
    ))

    # Spots
    rng = random.Random(variant * 11)
    for _ in range(3):
        sx = rng.uniform(-cap_r * 0.5, cap_r * 0.5)
        sy = -stem_h - cap_r * 0.3 + rng.uniform(-cap_r * 0.2, cap_r * 0.1)
        parts.append(GraphicObject(
            shape_type="circle",
            params={"cx": sx, "cy": sy, "r": 2 * scale},
            fill="#ffffff", stroke=None,
        ))

    return parts


def _make_crate(scale: float, sc: dict, variant: int) -> list[GraphicObject]:
    """Crate — wooden box with planks."""
    parts: list[GraphicObject] = []
    sz = 28 * scale

    # Main body
    parts.append(GraphicObject(
        shape_type="rect",
        params={"x": -sz / 2, "y": -sz, "w": sz, "h": sz},
        fill="#a1887f", stroke="#5d4037", stroke_width=1.5,
    ))

    # Cross planks
    parts.append(GraphicObject(
        shape_type="line",
        params={"x1": -sz / 2, "y1": -sz / 2, "x2": sz / 2, "y2": -sz / 2},
        stroke="#6d4c41", stroke_width=1.5,
    ))
    parts.append(GraphicObject(
        shape_type="line",
        params={"x1": 0, "y1": -sz, "x2": 0, "y2": 0},
        stroke="#6d4c41", stroke_width=1.5,
    ))

    return parts


def _make_barrel(scale: float, sc: dict, variant: int) -> list[GraphicObject]:
    """Barrel — cylinder with bands."""
    parts: list[GraphicObject] = []
    w = 22 * scale
    h = 30 * scale

    # Body
    parts.append(GraphicObject(
        shape_type="rect",
        params={"x": -w / 2, "y": -h, "w": w, "h": h},
        fill="#8d6e63", stroke="#4e342e", stroke_width=1.5,
    ))

    # Bands
    for frac in [0.2, 0.5, 0.8]:
        by = -h + h * frac
        parts.append(GraphicObject(
            shape_type="line",
            params={"x1": -w / 2, "y1": by, "x2": w / 2, "y2": by},
            stroke="#5d4037", stroke_width=2.0,
        ))

    return parts


def _make_torch(scale: float, sc: dict, variant: int) -> list[GraphicObject]:
    """Torch — stick + flame."""
    parts: list[GraphicObject] = []
    stick_h = 28 * scale

    # Stick
    parts.append(GraphicObject(
        shape_type="line",
        params={"x1": 0, "y1": 0, "x2": 0, "y2": -stick_h},
        stroke="#6d4c41", stroke_width=3.0 * scale,
    ))

    # Flame — two overlapping ellipses
    flame_y = -stick_h - 6 * scale
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": 0, "cy": flame_y, "rx": 5 * scale, "ry": 8 * scale},
        fill="#ff9800", stroke=None,
    ))
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": 0, "cy": flame_y - 2 * scale, "rx": 3 * scale, "ry": 5 * scale},
        fill="#ffeb3b", stroke=None,
    ))

    return parts


def _make_sign(scale: float, sc: dict, variant: int) -> list[GraphicObject]:
    """Sign — post + board."""
    parts: list[GraphicObject] = []
    post_h = 36 * scale
    board_w = 30 * scale
    board_h = 18 * scale

    # Post
    parts.append(GraphicObject(
        shape_type="line",
        params={"x1": 0, "y1": 0, "x2": 0, "y2": -post_h},
        stroke="#6d4c41", stroke_width=3.0 * scale,
    ))

    # Board
    board_y = -post_h + board_h / 2
    parts.append(GraphicObject(
        shape_type="rect",
        params={"x": -board_w / 2, "y": board_y - board_h / 2, "w": board_w, "h": board_h},
        fill="#d7ccc8", stroke="#5d4037", stroke_width=1.5,
    ))

    return parts


def _make_fence(scale: float, sc: dict, variant: int) -> list[GraphicObject]:
    """Fence — posts + rails."""
    parts: list[GraphicObject] = []
    post_h = 24 * scale
    rail_w = 32 * scale
    post_w = 4 * scale

    # Two posts
    for x in [-rail_w / 2, rail_w / 2]:
        parts.append(GraphicObject(
            shape_type="rect",
            params={"x": x - post_w / 2, "y": -post_h, "w": post_w, "h": post_h},
            fill="#8d6e63", stroke="#4e342e", stroke_width=1.0,
        ))

    # Rails
    for frac in [0.35, 0.7]:
        ry = -post_h * frac
        parts.append(GraphicObject(
            shape_type="line",
            params={"x1": -rail_w / 2, "y1": ry, "x2": rail_w / 2, "y2": ry},
            stroke="#6d4c41", stroke_width=2.0 * scale,
        ))

    return parts


def _make_lamp(scale: float, sc: dict, variant: int) -> list[GraphicObject]:
    """Lamp — post + lantern."""
    parts: list[GraphicObject] = []
    post_h = 40 * scale
    lantern_r = 6 * scale

    # Post
    parts.append(GraphicObject(
        shape_type="line",
        params={"x1": 0, "y1": 0, "x2": 0, "y2": -post_h},
        stroke="#37474f", stroke_width=3.0 * scale,
    ))

    # Lantern housing
    ly = -post_h - lantern_r
    parts.append(GraphicObject(
        shape_type="rect",
        params={"x": -lantern_r, "y": ly - lantern_r, "w": lantern_r * 2, "h": lantern_r * 2},
        fill="#455a64", stroke="#263238", stroke_width=1.0,
    ))

    # Glow
    parts.append(GraphicObject(
        shape_type="ellipse",
        params={"cx": 0, "cy": ly, "rx": lantern_r * 0.7, "ry": lantern_r * 0.7},
        fill="#fff9c4", stroke=None,
    ))

    return parts


# ======================================================================
# Background
# ======================================================================

_TIME_COLORS = {
    "day":   {"sky_top": "#87ceeb", "sky_bot": "#e0f7fa", "ambient": "#ffffff"},
    "dusk":  {"sky_top": "#e65100", "sky_bot": "#ff8f00", "ambient": "#ffcc80"},
    "night": {"sky_top": "#0d1b2a", "sky_bot": "#1b2838", "ambient": "#37474f"},
    "dawn":  {"sky_top": "#4a148c", "sky_bot": "#ff6f00", "ambient": "#ce93d8"},
}


def create_background(**kwargs: Any) -> GraphicObject:
    """Create a scene background."""
    p = validate_params(_BACKGROUND_SCHEMA, {_sym_str(k): v for k, v in kwargs.items()})
    bg_type = p["type"]
    time = p["time"]
    weather = p["weather"]
    w = int(p["width"])
    h = int(p["height"])
    parallax = p["parallax"]

    tc = _TIME_COLORS.get(time, _TIME_COLORS["day"])
    children: list[GraphicObject] = []
    half_w = w / 2
    half_h = h / 2

    # Sky gradient (two rects)
    children.append(GraphicObject(
        shape_type="rect",
        params={"x": -half_w, "y": -half_h, "w": w, "h": h / 2},
        fill=tc["sky_top"], stroke=None,
    ))
    children.append(GraphicObject(
        shape_type="rect",
        params={"x": -half_w, "y": 0, "w": w, "h": h / 2},
        fill=tc["sky_bot"], stroke=None,
    ))

    # Terrain layer based on type
    terrain = _bg_terrain(bg_type, w, h, tc)
    children.extend(terrain)

    # Weather overlay
    weather_parts = _bg_weather(weather, w, h)
    children.extend(weather_parts)

    return GraphicObject(
        shape_type="group",
        children=tuple(children),
        metadata={
            "component": "background",
            "bg_type": bg_type,
            "time": time,
            "weather": weather,
            "width": w,
            "height": h,
            "parallax": parallax,
        },
    )


def _bg_terrain(bg_type: str, w: int, h: int, tc: dict) -> list[GraphicObject]:
    """Generate terrain layer for background type."""
    parts: list[GraphicObject] = []
    half_w = w / 2
    ground_y = h * 0.15  # ground starts below center

    terrain_colors = {
        "sky":      None,  # sky only, no terrain
        "forest":   {"ground": "#2e7d32", "detail": "#1b5e20"},
        "dungeon":  {"ground": "#424242", "detail": "#212121"},
        "town":     {"ground": "#8d6e63", "detail": "#5d4037"},
        "ocean":    {"ground": "#0277bd", "detail": "#01579b"},
        "mountain": {"ground": "#607d8b", "detail": "#455a64"},
    }

    tc2 = terrain_colors.get(bg_type)
    if tc2 is None:
        return parts

    # Ground rect
    parts.append(GraphicObject(
        shape_type="rect",
        params={"x": -half_w, "y": ground_y, "w": w, "h": h / 2 - ground_y + h * 0.05},
        fill=tc2["ground"], stroke=None,
    ))

    # Type-specific features
    if bg_type == "forest":
        rng = random.Random(42)
        for i in range(5):
            tx = -half_w + (i + 0.5) * (w / 5) + rng.uniform(-10, 10)
            tree_h = rng.uniform(30, 50)
            parts.append(GraphicObject(
                shape_type="line",
                params={"x1": tx, "y1": ground_y, "x2": tx, "y2": ground_y - tree_h},
                stroke="#5d4037", stroke_width=4,
            ))
            parts.append(GraphicObject(
                shape_type="ellipse",
                params={"cx": tx, "cy": ground_y - tree_h - 10, "rx": 15, "ry": 18},
                fill=tc2["detail"], stroke=None,
            ))

    elif bg_type == "mountain":
        for i in range(3):
            mx = -half_w * 0.6 + i * half_w * 0.6
            mh = 40 + i * 15
            points = [[mx - 30, ground_y], [mx, ground_y - mh], [mx + 30, ground_y]]
            parts.append(GraphicObject(
                shape_type="polygon",
                params={"points": points},
                fill=tc2["detail"], stroke="#37474f", stroke_width=1.0,
            ))

    elif bg_type == "ocean":
        rng = random.Random(99)
        for i in range(4):
            wy = ground_y + 5 + i * 8
            parts.append(GraphicObject(
                shape_type="line",
                params={"x1": -half_w * 0.8, "y1": wy, "x2": half_w * 0.8, "y2": wy + rng.uniform(-2, 2)},
                stroke="#0288d1", stroke_width=1.5,
            ))

    elif bg_type == "town":
        rng = random.Random(77)
        for i in range(4):
            bx = -half_w + (i + 0.5) * (w / 4) + rng.uniform(-8, 8)
            bw = rng.uniform(18, 28)
            bh = rng.uniform(25, 45)
            parts.append(GraphicObject(
                shape_type="rect",
                params={"x": bx - bw / 2, "y": ground_y - bh, "w": bw, "h": bh},
                fill="#a1887f", stroke="#5d4037", stroke_width=1.0,
            ))

    elif bg_type == "dungeon":
        # Stone pillars
        for i in range(3):
            px = -half_w * 0.6 + i * half_w * 0.6
            parts.append(GraphicObject(
                shape_type="rect",
                params={"x": px - 6, "y": ground_y - 50, "w": 12, "h": 50},
                fill="#616161", stroke="#424242", stroke_width=1.0,
            ))

    return parts


def _bg_weather(weather: str, w: int, h: int) -> list[GraphicObject]:
    """Generate weather overlay."""
    parts: list[GraphicObject] = []
    half_w = w / 2
    half_h = h / 2
    rng = random.Random(weather.__hash__() % 100)

    if weather == "cloudy":
        for _ in range(4):
            cx = rng.uniform(-half_w * 0.7, half_w * 0.7)
            cy = rng.uniform(-half_h * 0.7, -half_h * 0.3)
            parts.append(GraphicObject(
                shape_type="ellipse",
                params={"cx": cx, "cy": cy, "rx": 25, "ry": 12},
                fill="#b0bec580", stroke=None,
            ))

    elif weather == "rain":
        for _ in range(20):
            rx = rng.uniform(-half_w * 0.9, half_w * 0.9)
            ry = rng.uniform(-half_h, half_h)
            parts.append(GraphicObject(
                shape_type="line",
                params={"x1": rx, "y1": ry, "x2": rx - 1, "y2": ry + 8},
                stroke="#90a4ae80", stroke_width=1.0,
            ))

    elif weather == "snow":
        for _ in range(15):
            sx = rng.uniform(-half_w * 0.9, half_w * 0.9)
            sy = rng.uniform(-half_h, half_h)
            parts.append(GraphicObject(
                shape_type="circle",
                params={"cx": sx, "cy": sy, "r": 2},
                fill="#ffffffcc", stroke=None,
            ))

    elif weather == "fog":
        parts.append(GraphicObject(
            shape_type="rect",
            params={"x": -half_w, "y": -half_h * 0.3, "w": w, "h": h * 0.6},
            fill="#cfd8dc60", stroke=None,
        ))

    return parts
