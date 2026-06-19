"""Pillow-based render backend for PML."""

from __future__ import annotations

import math
import re
from typing import Any

import numpy as np
from PIL import Image, ImageDraw, ImageFont

from pml.backend import RenderBackend
from pml.graphics.objects import GraphicObject
from pml.transform import AffineTransform
from pml.backend.sketchy import (
    draw_pencil,
    draw_charcoal,
    draw_watercolor_rect,
    draw_watercolor_circle,
    draw_hatch,
    draw_noise_fill,
    sketchify_graphic,
)


# ======================================================================
# Color parsing
# ======================================================================

_NAMED_COLORS: dict[str, str] = {
    "transparent": "#00000000",
    "black": "#000000",
    "white": "#FFFFFF",
    "red": "#FF0000",
    "green": "#00FF00",
    "blue": "#0000FF",
    "yellow": "#FFFF00",
    "cyan": "#00FFFF",
    "magenta": "#FF00FF",
    "orange": "#FF8C00",
    "purple": "#800080",
    "pink": "#FFC0CB",
    "gray": "#808080",
    "grey": "#808080",
    "none": "#00000000",
}


def parse_color(color: str | None) -> tuple[int, int, int, int] | None:
    """Parse a color string to RGBA tuple (0-255). Returns None for no color."""
    if color is None:
        return None

    c = color.strip().lower()

    # Named color
    if c in _NAMED_COLORS:
        c = _NAMED_COLORS[c]

    # #RGB
    if re.match(r"^#[0-9a-f]{3}$", c):
        r = int(c[1] * 2, 16)
        g = int(c[2] * 2, 16)
        b = int(c[3] * 2, 16)
        return (r, g, b, 255)

    # #RRGGBB
    if re.match(r"^#[0-9a-f]{6}$", c):
        r = int(c[1:3], 16)
        g = int(c[3:5], 16)
        b = int(c[5:7], 16)
        return (r, g, b, 255)

    # #RRGGBBAA
    if re.match(r"^#[0-9a-f]{8}$", c):
        r = int(c[1:3], 16)
        g = int(c[3:5], 16)
        b = int(c[5:7], 16)
        a = int(c[7:9], 16)
        return (r, g, b, a)

    # rgb(r,g,b)
    m = re.match(r"^rgb\((\d+),\s*(\d+),\s*(\d+)\)$", c)
    if m:
        return (int(m.group(1)), int(m.group(2)), int(m.group(3)), 255)

    # rgba(r,g,b,a)
    m = re.match(r"^rgba\((\d+),\s*(\d+),\s*(\d+),\s*([\d.]+)\)$", c)
    if m:
        return (
            int(m.group(1)),
            int(m.group(2)),
            int(m.group(3)),
            int(float(m.group(4)) * 255),
        )

    # Fallback: try Pillow named color
    try:
        from PIL import ImageColor
        rgb = ImageColor.getrgb(color)
        if len(rgb) == 4:
            return rgb
        return (*rgb, 255)
    except Exception:
        return None


# ======================================================================
# SVG path parsing (extended: supports all path commands)
# ======================================================================

# ------ Bezier subdivision helpers (De Casteljau) ------

def _cubic_bezier_segment(
    p0: tuple[float, float],
    p1: tuple[float, float],
    p2: tuple[float, float],
    p3: tuple[float, float],
    steps: int = 20,
) -> list[tuple[float, float]]:
    """Subdivide a cubic bezier into *steps* line segments.

    Returns *steps* + 1 points (including start and end).
    """
    pts = []
    for t in range(steps + 1):
        s = t / steps
        u = 1.0 - s
        x = u * u * u * p0[0] + 3.0 * u * u * s * p1[0] + 3.0 * u * s * s * p2[0] + s * s * s * p3[0]
        y = u * u * u * p0[1] + 3.0 * u * u * s * p1[1] + 3.0 * u * s * s * p2[1] + s * s * s * p3[1]
        pts.append((x, y))
    return pts


def _quad_bezier_segment(
    p0: tuple[float, float],
    p1: tuple[float, float],
    p2: tuple[float, float],
    steps: int = 16,
) -> list[tuple[float, float]]:
    """Subdivide a quadratic bezier into *steps* line segments."""
    pts = []
    for t in range(steps + 1):
        s = t / steps
        u = 1.0 - s
        x = u * u * p0[0] + 2.0 * u * s * p1[0] + s * s * p2[0]
        y = u * u * p0[1] + 2.0 * u * s * p1[1] + s * s * p2[1]
        pts.append((x, y))
    return pts


# ------ Path tokeniser ------

def _tokenise_svg_path(d: str) -> list[tuple[str, list[float]]]:
    """Tokenise an SVG path string into ``[(command, [args...])]``.

    Handles implicit commands (consecutive numbers after a command) and
    implicit M→L transition (a second M-like coordinate pair after an M).
    Commands are normalised to uppercase; relative variants are recorded
    via the command string (e.g. ``"C"`` vs ``"c"``).
    """
    raw_tokens = re.findall(r"[A-Za-z]|-?[\d]+\.?[\d]*(?:[eE][+-]?\d+)?", d.strip())

    # The SVG spec: consecutive coordinate pairs after a command re-use
    # the same command.  We re-parse by scanning for command letters.
    cmds: list[tuple[str, list[float]]] = []
    i = 0
    while i < len(raw_tokens):
        tok = raw_tokens[i]
        # Is this a command letter?
        if re.match(r"^[A-Za-z]$", tok):
            cmd = tok
            i += 1
            args: list[float] = []
            # Consume following numeric tokens until next letter
            while i < len(raw_tokens) and not re.match(r"^[A-Za-z]$", raw_tokens[i]):
                try:
                    args.append(float(raw_tokens[i]))
                    i += 1
                except ValueError:
                    i += 1  # skip non-numeric
            cmds.append((cmd, args))
        else:
            # Stray numeric — skip
            i += 1

    return cmds


# ------ Coordinate helpers for relative vs absolute ------

def _abs_coord(args: list[float], offset: int, ref: float) -> float:
    """Return absolute coordinate from *args[offset]*.

    *ref* is the current reference point; if the *command* is relative
    (i.e. the original letter was lowercase) the caller should have
    already passed a negative offset, making this a no-op for relative.
    """
    return args[offset]


def _rel_coord(args: list[float], offset: int, ref: float) -> float:
    return args[offset] + ref


def _to_abs(cmd: str, args: list[float], i: int, ref: float) -> float:
    """Coordinate from *args[i]* — absolute for upper, relative for lower."""
    if cmd.isupper():
        return args[i]
    return args[i] + ref


# ------ Main parser ------

def _parse_svg_path(d: str) -> list[list[tuple[float, float]]]:
    """Parse SVG path data into a list of polylines.

    Supports: M/m (moveto), L/l (lineto), H/h (horizontal lineto),
    V/v (vertical lineto), C/c (cubic bezier), Q/q (quadratic bezier),
    S/s (smooth cubic bezier), T/t (smooth quadratic bezier),
    Z/z (closepath).

    Returns a list of polylines, each a list of ``(x, y)`` points.
    Bezier curves are subdivided into line segments via De Casteljau.
    """
    commands = _tokenise_svg_path(d)
    polylines: list[list[tuple[float, float]]] = []
    current: list[tuple[float, float]] = []
    cx, cy = 0.0, 0.0          # current point
    sx, sy = 0.0, 0.0          # subpath start point
    prev_cx, prev_cy = 0.0, 0.0  # previous control point (for S/s, T/t)

    def _finish_open() -> None:
        """Push current list as a non-closed polyline."""
        nonlocal current, polylines
        if current:
            polylines.append(current)
            current = []

    def _coordinate(args: list[float], j: int, cmd: str, ref: float) -> float:
        if cmd.isupper():
            return args[j]
        return args[j] + ref

    i = 0
    while i < len(commands):
        cmd, args = commands[i]

        if cmd in ("M", "m"):
            _finish_open()
            j = 0
            while j + 1 <= len(args):
                cx = _coordinate(args, j, cmd, cx)
                cy = _coordinate(args, j + 1, cmd, cy)
                if j == 0:
                    sx, sy = cx, cy
                    current = [(cx, cy)]
                else:
                    # Subsequent pairs after M are implicit L
                    current.append((cx, cy))
                j += 2
            i += 1

        elif cmd in ("L", "l"):
            j = 0
            while j + 1 <= len(args):
                cx = _coordinate(args, j, cmd, cx)
                cy = _coordinate(args, j + 1, cmd, cy)
                current.append((cx, cy))
                j += 2
            i += 1

        elif cmd in ("H", "h"):
            for a in args:
                cx = _coordinate([a], 0, cmd, cx)
                current.append((cx, cy))
            i += 1

        elif cmd in ("V", "v"):
            for a in args:
                cy = _coordinate([a], 0, cmd, cy)
                current.append((cx, cy))
            i += 1

        elif cmd in ("C", "c"):
            j = 0
            while j + 5 <= len(args):
                x1 = _coordinate(args, j, cmd, cx)
                y1 = _coordinate(args, j + 1, cmd, cy)
                x2 = _coordinate(args, j + 2, cmd, cx)
                y2 = _coordinate(args, j + 3, cmd, cy)
                x3 = _coordinate(args, j + 4, cmd, cx)
                y3 = _coordinate(args, j + 5, cmd, cy)
                pts = _cubic_bezier_segment(
                    (cx, cy), (x1, y1), (x2, y2), (x3, y3)
                )
                current.extend(pts[1:])  # skip first point (already in current)
                prev_cx, prev_cy = x2, y2
                cx, cy = x3, y3
                j += 6
            i += 1

        elif cmd in ("S", "s"):
            j = 0
            while j + 3 <= len(args):
                # Reflect previous control point
                ref_x = cx + (cx - prev_cx)
                ref_y = cy + (cy - prev_cy)
                x2 = _coordinate(args, j, cmd, cx)
                y2 = _coordinate(args, j + 1, cmd, cy)
                x3 = _coordinate(args, j + 2, cmd, cx)
                y3 = _coordinate(args, j + 3, cmd, cy)
                pts = _cubic_bezier_segment(
                    (cx, cy), (ref_x, ref_y), (x2, y2), (x3, y3)
                )
                current.extend(pts[1:])
                prev_cx, prev_cy = x2, y2
                cx, cy = x3, y3
                j += 4
            i += 1

        elif cmd in ("Q", "q"):
            j = 0
            while j + 3 <= len(args):
                x1 = _coordinate(args, j, cmd, cx)
                y1 = _coordinate(args, j + 1, cmd, cy)
                x2 = _coordinate(args, j + 2, cmd, cx)
                y2 = _coordinate(args, j + 3, cmd, cy)
                pts = _quad_bezier_segment(
                    (cx, cy), (x1, y1), (x2, y2)
                )
                current.extend(pts[1:])
                prev_cx, prev_cy = x1, y1
                cx, cy = x2, y2
                j += 4
            i += 1

        elif cmd in ("T", "t"):
            j = 0
            while j + 1 <= len(args):
                # Reflect previous control point (quadratic)
                ref_x = cx + (cx - prev_cx)
                ref_y = cy + (cy - prev_cy)
                x2 = _coordinate(args, j, cmd, cx)
                y2 = _coordinate(args, j + 1, cmd, cy)
                pts = _quad_bezier_segment(
                    (cx, cy), (ref_x, ref_y), (x2, y2)
                )
                current.extend(pts[1:])
                prev_cx, prev_cy = ref_x, ref_y
                cx, cy = x2, y2
                j += 2
            i += 1

        elif cmd in ("Z", "z"):
            if current and current[0] != current[-1]:
                current.append(current[0])
                cx, cy = sx, sy
            # If current has content, close it
            if current:
                polylines.append(current)
                current = []
            i += 1

        else:
            # Unknown command — skip
            i += 1

    if current:
        polylines.append(current)
    return polylines


# ======================================================================
# Gradient fills — parsing & rendering
# ======================================================================

# Regex for gradient syntax:
#   linear-gradient(angle deg, color1, color2, ...)
#   radial-gradient(color1, color2, ...)
_GRADIENT_RE = re.compile(
    r"(linear-gradient|radial-gradient)\s*"
    r"\(([^)]*)\)",
    re.IGNORECASE,
)
_ANGLE_WITH_UNIT = re.compile(r"^\s*([+-]?\d+(?:\.\d+)?)\s*(deg|turn|grad|rad)\s*$", re.IGNORECASE)
_HEX_COLOR = re.compile(r"#[0-9a-fA-F]{3,8}")


def _is_gradient_fill(fill: str | None) -> bool:
    """Return True if *fill* is a gradient string."""
    if not fill or not isinstance(fill, str):
        return False
    return bool(_GRADIENT_RE.match(fill.strip()))


def _parse_gradient_spec(fill: str) -> dict | None:
    """Parse a gradient fill string into a structured dict.

    Supported syntaxes:

    ``"linear-gradient(45deg, #ff0000, #0000ff, #00ff00)"``
    ``"radial-gradient(#ff0000, #0000ff)"``

    Returns a dict with keys:
      - ``type``: ``"linear"`` | ``"radial"``
      - ``angle``: float degrees (linear only, default 0)
      - ``stops``: list of ``(position_0_1, color_hex_str)``

    Returns ``None`` if parsing fails.
    """
    m = _GRADIENT_RE.match(fill.strip())
    if not m:
        return None

    grad_type = m.group(1).lower().replace("-gradient", "")
    raw_args = m.group(2).strip()

    # Split on commas — but not inside parentheses
    parts = _split_gradient_args(raw_args)

    if not parts:
        return None

    result: dict = {"type": grad_type, "angle": 0.0, "stops": []}

    idx = 0

    # Linear gradient may start with an angle
    if grad_type == "linear":
        angle_match = _ANGLE_WITH_UNIT.match(parts[0])
        if angle_match:
            val = float(angle_match.group(1))
            unit = angle_match.group(2).lower()
            if unit == "deg":
                result["angle"] = val % 360
            elif unit == "turn":
                result["angle"] = (val * 360) % 360
            elif unit == "grad":
                result["angle"] = (val * 0.9) % 360
            elif unit == "rad":
                result["angle"] = (math.degrees(val)) % 360
            idx = 1

    # Remaining parts are color stops
    for i in range(idx, len(parts)):
        part = parts[i].strip()
        if not part:
            continue

        # Stop can be "color position" or just "color"
        tokens = part.rsplit(None, 1)
        if len(tokens) == 2 and _is_valid_color_str(tokens[0]) and _is_position(tokens[1]):
            color = tokens[0]
            pos = _parse_position(tokens[1])
        else:
            color = part
            pos = None  # auto-distribute later

        result["stops"].append((pos, color))

    # Auto-fill null positions
    _distribute_stops(result["stops"])

    return result if result["stops"] else None


def _split_gradient_args(raw: str) -> list[str]:
    """Split gradient arguments on commas, respecting nested parens."""
    parts: list[str] = []
    depth = 0
    current: list[str] = []
    for ch in raw:
        if ch == "(":
            depth += 1
            current.append(ch)
        elif ch == ")":
            depth -= 1
            current.append(ch)
        elif ch == "," and depth == 0:
            parts.append("".join(current).strip())
            current = []
        else:
            current.append(ch)
    rest = "".join(current).strip()
    if rest:
        parts.append(rest)
    return parts


def _is_valid_color_str(s: str) -> bool:
    """Return True if *s* looks like a CSS color string."""
    s = s.strip().lower()
    if s in _NAMED_COLORS:
        return True
    if _HEX_COLOR.match(s):
        return True
    if s.startswith("rgb") or s.startswith("hsl"):
        return True
    return False


def _is_position(s: str) -> bool:
    """Return True if *s* looks like a colour-stop position."""
    s = s.strip()
    if s.endswith("%"):
        try:
            float(s[:-1])
            return True
        except ValueError:
            return False
    try:
        float(s)
        return True
    except ValueError:
        return False


def _parse_position(s: str) -> float:
    """Parse a colour-stop position string to 0.0–1.0 float."""
    s = s.strip()
    if s.endswith("%"):
        return max(0.0, min(1.0, float(s[:-1]) / 100.0))
    return max(0.0, min(1.0, float(s)))


def _distribute_stops(stops: list[tuple[float | None, str]]) -> None:
    """Auto-fill ``None`` positions evenly between known stops."""
    known_idx = [i for i, (pos, _) in enumerate(stops) if pos is not None]

    if not known_idx:
        # All None — distribute evenly
        n = len(stops)
        for i in range(n):
            stops[i] = (i / (n - 1) if n > 1 else 0.5, stops[i][1])
        return

    # Fill before first known
    if known_idx[0] > 0:
        p0 = stops[known_idx[0]][0] if known_idx[0] < len(stops) else 1.0
        for i in range(known_idx[0]):
            stops[i] = (p0 * i / known_idx[0], stops[i][1])

    # Fill between known
    for j in range(len(known_idx) - 1):
        i1 = known_idx[j]
        i2 = known_idx[j + 1]
        p1 = stops[i1][0]
        p2 = stops[i2][0]
        gap = i2 - i1
        for k in range(1, gap):
            t = k / gap
            stops[i1 + k] = (p1 + (p2 - p1) * t if p1 is not None and p2 is not None else t, stops[i1 + k][1])

    # Fill after last known
    if known_idx[-1] < len(stops) - 1:
        p_last = stops[known_idx[-1]][0] if known_idx[-1] < len(stops) else 0.0
        last = known_idx[-1]
        gap = len(stops) - 1 - last
        for k in range(1, gap + 1):
            t = k / gap
            stops[last + k] = (p_last + (1.0 - p_last) * t if p_last is not None else 1.0, stops[last + k][1])


def _make_linear_gradient_image(
    w: int, h: int,
    angle_deg: float,
    stops: list[tuple[float, str]],
) -> Image.Image:
    """Create a linear gradient image of size ``(w, h)``.

    Returns an RGBA Pillow image.
    """
    if w < 1 or h < 1:
        return Image.new("RGBA", (max(1, w), max(1, h)), (0, 0, 0, 0))

    # Gradient direction vector
    rad = math.radians(angle_deg)
    dx = math.cos(rad)
    dy = math.sin(rad)

    # Project (0,0) → (w,h) onto the gradient axis to find the range
    corners = [(0, 0), (w, 0), (0, h), (w, h)]
    proj = [x * dx + y * dy for x, y in corners]
    p_min, p_max = min(proj), max(proj)
    p_range = p_max - p_min
    if p_range < 1e-9:
        # Degenerate angle — fill with last colour
        return Image.new("RGBA", (w, h), parse_color(stops[-1][1]) or (0, 0, 0, 255))

    # Build colour LUT along the gradient axis
    sz = max(w, h) * 2  # oversample for smoothness
    lut = np.zeros((sz + 1, 4), dtype=np.uint8)
    for i in range(sz + 1):
        t = i / sz  # 0..1
        # Find which stop pair t falls between
        r, g, b, a = _interpolate_stops(t, stops)
        lut[i] = (r, g, b, a)

    # Map each pixel position to LUT index
    y_coords, x_coords = np.mgrid[0:h, 0:w]
    p_vals = x_coords * dx + y_coords * dy
    t_vals = (p_vals - p_min) / p_range
    t_clipped = np.clip(t_vals, 0.0, 1.0)
    indices = (t_clipped * sz).astype(np.uint32)
    indices = np.clip(indices, 0, sz)

    rgba = lut[indices]  # (h, w, 4)
    return Image.fromarray(rgba, "RGBA")


def _make_radial_gradient_image(
    w: int, h: int,
    stops: list[tuple[float, str]],
) -> Image.Image:
    """Create a radial gradient image of size ``(w, h)``."""
    if w < 1 or h < 1:
        return Image.new("RGBA", (max(1, w), max(1, h)), (0, 0, 0, 0))

    cx, cy = w / 2.0, h / 2.0
    max_r = math.hypot(cx, cy)
    if max_r < 1e-9:
        return Image.new("RGBA", (w, h), parse_color(stops[-1][1]) or (0, 0, 0, 255))

    sz = max(w, h)
    lut = np.zeros((sz + 1, 4), dtype=np.uint8)
    for i in range(sz + 1):
        t = i / sz
        r, g, b, a = _interpolate_stops(t, stops)
        lut[i] = (r, g, b, a)

    y_coords, x_coords = np.mgrid[0:h, 0:w]
    dist = np.sqrt((x_coords - cx) ** 2 + (y_coords - cy) ** 2)
    t_vals = dist / max_r
    t_clipped = np.clip(t_vals, 0.0, 1.0)
    indices = (t_clipped * sz).astype(np.uint32)
    indices = np.clip(indices, 0, sz)

    rgba = lut[indices]
    return Image.fromarray(rgba, "RGBA")


def _interpolate_stops(t: float, stops: list[tuple[float, str]]) -> tuple[int, int, int, int]:
    """Interpolate RGBA at position *t* (0–1) through colour stops."""
    if not stops:
        return (0, 0, 0, 0)
    if len(stops) == 1:
        return parse_color(stops[0][1]) or (0, 0, 0, 255)

    # Find the two stops that bracket t
    for i in range(len(stops) - 1):
        p1, c1 = stops[i]
        p2, c2 = stops[i + 1]
        if p1 is None or p2 is None:
            continue
        if p1 <= t <= p2:
            if abs(p2 - p1) < 1e-9:
                frac = 0.0
            else:
                frac = (t - p1) / (p2 - p1)
            rgba1 = parse_color(c1) or (0, 0, 0, 255)
            rgba2 = parse_color(c2) or (0, 0, 0, 255)
            return (
                int(rgba1[0] + (rgba2[0] - rgba1[0]) * frac),
                int(rgba1[1] + (rgba2[1] - rgba1[1]) * frac),
                int(rgba1[2] + (rgba2[2] - rgba1[2]) * frac),
                int(rgba1[3] + (rgba2[3] - rgba1[3]) * frac),
            )

    # Clamp to first or last
    if t < (stops[0][0] or 0.0):
        return parse_color(stops[0][1]) or (0, 0, 0, 255)
    return parse_color(stops[-1][1]) or (0, 0, 0, 255)


def _apply_gradient_fill(
    img: Image.Image,
    obj: GraphicObject,
    bbox: list[float],
    draw_shape: callable,
) -> None:
    """Apply a gradient fill to a shape on *img*.

    Args:
        img: Target RGBA image.
        obj: The GraphicObject (reads fill, transform).
        bbox: [left, top, right, bottom] in canvas coordinates.
        draw_shape: A callable ``(draw, gradient_img, bbox_int)`` that
            draws the shape using the gradient image as a fill pattern.
    """
    fill_str = obj.fill
    if not fill_str or not _is_gradient_fill(fill_str):
        return

    grad = _parse_gradient_spec(fill_str)
    if grad is None:
        return

    # Bounding box in integer pixel coords
    l, t, r, b = bbox
    gw = max(1, int(math.ceil(r - l)))
    gh = max(1, int(math.ceil(b - t)))

    # Render the gradient image
    if grad["type"] == "linear":
        grad_img = _make_linear_gradient_image(gw, gh, grad["angle"], grad["stops"])
    else:
        grad_img = _make_radial_gradient_image(gw, gh, grad["stops"])

    # Draw the shape using the gradient image by compositing through a mask
    # 1. Create a temporary surface at bbox size
    shape_layer = Image.new("RGBA", (gw, gh), (0, 0, 0, 0))
    draw = ImageDraw.Draw(shape_layer, "RGBA")
    draw_shape(draw, (0, 0, gw, gh))
    # shape_layer now has the shape drawn in white (mask) at the correct position

    # 2. Composite gradient through the shape mask
    #    Where shape_layer is opaque, keep gradient; where transparent, keep original
    mask = shape_layer.split()[3]  # alpha channel = shape mask
    if mask.getbbox() is None:
        return  # completely transparent — nothing to draw

    # 3. Paste gradient onto the bbox region
    region = img.crop((int(l), int(t), int(l) + gw, int(t) + gh))
    gradient_paint = Image.new("RGBA", (gw, gh), (0, 0, 0, 0))
    gradient_paint.paste(grad_img, (0, 0), mask)

    # Composite: gradient over original
    composite = Image.alpha_composite(region, gradient_paint)
    img.paste(composite, (int(l), int(t)))


# ======================================================================
# Blend-mode compositing
# ======================================================================

# Registered blend-mode functions.
# Each takes two float32 RGBA arrays (src, dst) in [0, 1] range
# and returns the composited RGBA array (also float32 in [0, 1]).
_BLEND_MODES: dict[str, callable] = {}


def _blend_normal(src: np.ndarray, dst: np.ndarray) -> np.ndarray:
    """Standard alpha compositing — handled by Pillow natively."""
    return src


def _blend_multiply(src: np.ndarray, dst: np.ndarray) -> np.ndarray:
    """Multiply: result = src * dst (per-channel)."""
    out = src.copy()
    out[:, :, :3] = src[:, :, :3] * dst[:, :, :3]
    return out


def _blend_screen(src: np.ndarray, dst: np.ndarray) -> np.ndarray:
    """Screen: result = 1 - (1 - src) * (1 - dst)."""
    out = src.copy()
    out[:, :, :3] = 1.0 - (1.0 - src[:, :, :3]) * (1.0 - dst[:, :, :3])
    return out


def _blend_overlay(src: np.ndarray, dst: np.ndarray) -> np.ndarray:
    """Overlay: 2*src*dst if dst<0.5 else 1-2*(1-src)*(1-dst)."""
    out = src.copy()
    s, d = src[:, :, :3], dst[:, :, :3]
    mask = d < 0.5
    result = np.where(mask, 2.0 * s * d, 1.0 - 2.0 * (1.0 - s) * (1.0 - d))
    out[:, :, :3] = np.clip(result, 0.0, 1.0)
    return out


def _blend_darken(src: np.ndarray, dst: np.ndarray) -> np.ndarray:
    """Darken: min(src, dst)."""
    out = src.copy()
    out[:, :, :3] = np.minimum(src[:, :, :3], dst[:, :, :3])
    return out


def _blend_lighten(src: np.ndarray, dst: np.ndarray) -> np.ndarray:
    """Lighten: max(src, dst)."""
    out = src.copy()
    out[:, :, :3] = np.maximum(src[:, :, :3], dst[:, :, :3])
    return out


def _blend_color_dodge(src: np.ndarray, dst: np.ndarray) -> np.ndarray:
    """Color dodge: dst / (1 - src), clamped."""
    out = src.copy()
    divisor = 1.0 - src[:, :, :3]
    divisor = np.clip(divisor, 1e-7, None)
    out[:, :, :3] = np.clip(dst[:, :, :3] / divisor, 0.0, 1.0)
    return out


def _blend_color_burn(src: np.ndarray, dst: np.ndarray) -> np.ndarray:
    """Color burn: 1 - (1 - dst) / src, clamped."""
    out = src.copy()
    s = np.clip(src[:, :, :3], 1e-7, None)
    out[:, :, :3] = np.clip(1.0 - (1.0 - dst[:, :, :3]) / s, 0.0, 1.0)
    return out


def _blend_soft_light(src: np.ndarray, dst: np.ndarray) -> np.ndarray:
    """Soft light (Pegtop formula)."""
    out = src.copy()
    s, d = src[:, :, :3], dst[:, :, :3]
    mask = d < 0.5
    result = np.where(
        mask,
        2.0 * s * d + s * s * (1.0 - 2.0 * d),
        2.0 * s * (1.0 - d) + np.sqrt(s) * (2.0 * d - 1.0),
    )
    out[:, :, :3] = np.clip(result, 0.0, 1.0)
    return out


def _blend_hard_light(src: np.ndarray, dst: np.ndarray) -> np.ndarray:
    """Hard light: 2*src*dst if src<0.5 else 1-2*(1-src)*(1-dst)."""
    out = src.copy()
    s, d = src[:, :, :3], dst[:, :, :3]
    mask = s < 0.5
    result = np.where(mask, 2.0 * s * d, 1.0 - 2.0 * (1.0 - s) * (1.0 - d))
    out[:, :, :3] = np.clip(result, 0.0, 1.0)
    return out


def _blend_difference(src: np.ndarray, dst: np.ndarray) -> np.ndarray:
    """Difference: abs(dst - src)."""
    out = src.copy()
    out[:, :, :3] = np.abs(dst[:, :, :3] - src[:, :, :3])
    return out


def _blend_exclusion(src: np.ndarray, dst: np.ndarray) -> np.ndarray:
    """Exclusion: src + dst - 2 * src * dst."""
    out = src.copy()
    out[:, :, :3] = src[:, :, :3] + dst[:, :, :3] - 2.0 * src[:, :, :3] * dst[:, :, :3]
    return out


def _register_blend_modes() -> None:
    _BLEND_MODES.clear()
    _BLEND_MODES["normal"] = _blend_normal
    _BLEND_MODES["multiply"] = _blend_multiply
    _BLEND_MODES["screen"] = _blend_screen
    _BLEND_MODES["overlay"] = _blend_overlay
    _BLEND_MODES["darken"] = _blend_darken
    _BLEND_MODES["lighten"] = _blend_lighten
    _BLEND_MODES["color-dodge"] = _blend_color_dodge
    _BLEND_MODES["color-burn"] = _blend_color_burn
    _BLEND_MODES["soft-light"] = _blend_soft_light
    _BLEND_MODES["hard-light"] = _blend_hard_light
    _BLEND_MODES["difference"] = _blend_difference
    _BLEND_MODES["exclusion"] = _blend_exclusion


_register_blend_modes()


def composite_with_blend(
    dst_img: Image.Image,
    src_img: Image.Image,
    blend_mode: str,
    pos: tuple[int, int] = (0, 0),
) -> None:
    """Composite *src_img* onto *dst_img* using *blend_mode* at position *pos*.

    Both images must be RGBA mode.  The compositing is performed per-pixel
    using numpy.  The blend formula is applied to RGB channels; the alpha
    channel follows the source alpha (Porter-Duff "source-over").
    """
    if blend_mode == "normal":
        # Fast path: use Pillow's native alpha compositing
        dst_img.paste(src_img, pos, src_img)
        return

    sx, sy = pos
    sw, sh = src_img.size
    dw, dh = dst_img.size

    # Region on the destination that will be replaced
    x1 = max(0, sx)
    y1 = max(0, sy)
    x2 = min(dw, sx + sw)
    y2 = min(dh, sy + sh)
    if x2 <= x1 or y2 <= y1:
        return

    src_region = src_img.crop((x1 - sx, y1 - sy, x2 - sx, y2 - sy))
    dst_region = dst_img.crop((x1, y1, x2, y2))

    src_np = np.array(src_region, dtype=np.float32) / 255.0
    dst_np = np.array(dst_region, dtype=np.float32) / 255.0

    blend_fn = _BLEND_MODES.get(blend_mode, _blend_normal)
    result_np = blend_fn(src_np, dst_np)

    # Alpha handling: standard source-over
    sa = src_np[:, :, 3:4]
    da = dst_np[:, :, 3:4]
    result_np[:, :, 3] = sa[:, :, 0] + da[:, :, 0] * (1.0 - sa[:, :, 0])
    result_np[:, :, 3] = np.clip(result_np[:, :, 3], 0.0, 1.0)

    result_img = Image.fromarray((np.clip(result_np, 0.0, 1.0) * 255 + 0.5).astype(np.uint8), "RGBA")
    dst_img.paste(result_img, (x1, y1), result_img)


# ======================================================================
# Pillow backend
# ======================================================================


class PillowBackend(RenderBackend):
    """Render backend using Pillow (PIL)."""

    def create_surface(self, width: int, height: int, bg_color: str) -> Image.Image:
        bg = parse_color(bg_color) or (255, 255, 255, 255)
        if bg[3] == 0:
            # Transparent background
            img = Image.new("RGBA", (width, height), (0, 0, 0, 0))
        else:
            img = Image.new("RGBA", (width, height), bg)
        return img

    def draw(self, surface: Image.Image, obj: GraphicObject) -> None:
        """Recursively draw a GraphicObject onto the surface."""
        # Group: render children recursively with group opacity
        if obj.shape_type == "group":
            if obj.opacity < 1.0:
                # Render group to temp, then composite with opacity
                temp = Image.new("RGBA", surface.size, (0, 0, 0, 0))
                for child in obj.children:
                    composed_child = child.with_transform(
                        obj.transform.compose(child.transform)
                    )
                    self.draw(temp, composed_child)
                # Apply group opacity
                r2, g2, b2, a2 = temp.split()
                a2 = a2.point(lambda x: int(x * obj.opacity))
                temp = Image.merge("RGBA", (r2, g2, b2, a2))
                surface.paste(temp, (0, 0), temp)
            else:
                for child in obj.children:
                    composed_child = child.with_transform(
                        obj.transform.compose(child.transform)
                    )
                    self.draw(surface, composed_child)
            return

        dispatch = {
            "circle": self._draw_circle,
            "rect": self._draw_rect,
            "ellipse": self._draw_ellipse,
            "line": self._draw_line,
            "polygon": self._draw_polygon,
            "path": self._draw_path,
            "text": self._draw_text,
            "image": self._draw_image,
            # Hand-drawn / sketch shapes
            "pencil": self._draw_pencil,
            "charcoal": self._draw_charcoal,
            "watercolor-rect": self._draw_watercolor_rect,
            "watercolor-circle": self._draw_watercolor_circle,
            "hatch": self._draw_hatch,
            "sketchify": self._draw_sketchify,
            "noise-fill": self._draw_noise_fill,
        }

        handler = dispatch.get(obj.shape_type)
        if handler is None:
            return

        # Opacity and blend mode: render to temp surface, then composite
        alpha = obj.opacity
        bm = obj.blend_mode or "normal"
        needs_temp = (bm != "normal") or (alpha < 1.0)

        if needs_temp:
            temp = Image.new("RGBA", surface.size, (0, 0, 0, 0))
            handler(temp, obj)
            if bm != "normal":
                composite_with_blend(surface, temp, bm)
            if alpha < 1.0:
                # Scale temp's alpha by opacity
                r2, g2, b2, a2 = temp.split()
                a2 = a2.point(lambda x: int(x * alpha))
                temp = Image.merge("RGBA", (r2, g2, b2, a2))
                surface.paste(temp, (0, 0), temp)
        else:
            handler(surface, obj)

    def save_image(self, surface: Image.Image, path: str, format: str) -> None:
        fmt = format.upper()
        if fmt == "PNG":
            surface.save(path, "PNG")
        elif fmt in ("JPG", "JPEG"):
            # Convert RGBA to RGB for JPEG
            if surface.mode == "RGBA":
                bg = Image.new("RGB", surface.size, (255, 255, 255))
                bg.paste(surface, mask=surface.split()[3])
                bg.save(path, "JPEG")
            else:
                surface.save(path, "JPEG")
        elif fmt == "WEBP":
            surface.save(path, "WEBP", lossless=True)
        else:
            surface.save(path)

    def save_animation(
        self, frames: list[Image.Image], path: str, format: str, fps: int
    ) -> None:
        """Save frames as an animated GIF."""
        if not frames:
            return

        fmt = format.upper()
        if fmt == "GIF":
            duration = max(1, 1000 // fps)  # milliseconds per frame

            # Convert frames to palette mode for GIF
            gif_frames: list[Image.Image] = []
            for frame in frames:
                if frame.mode == "RGBA":
                    # Composite onto white background (GIF has no alpha)
                    bg = Image.new("RGB", frame.size, (255, 255, 255))
                    bg.paste(frame, mask=frame.split()[3])
                    p_frame = bg.convert("P", palette=Image.Palette.ADAPTIVE, colors=256)
                elif frame.mode != "RGB":
                    p_frame = frame.convert("RGB").convert("P", palette=Image.Palette.ADAPTIVE, colors=256)
                else:
                    p_frame = frame.convert("P", palette=Image.Palette.ADAPTIVE, colors=256)
                gif_frames.append(p_frame)

            gif_frames[0].save(
                path,
                save_all=True,
                append_images=gif_frames[1:],
                duration=duration,
                loop=0,
                disposal=2,
            )
        else:
            raise ValueError(f"Unsupported animation format: {format}")

    # ------------------------------------------------------------------
    # Individual shape renderers
    # ------------------------------------------------------------------

    def _draw_circle(self, img: Image.Image, obj: GraphicObject) -> None:
        t = obj.transform
        cx, cy = t.apply(obj.params["x"], obj.params["y"])
        r = obj.params["r"] * math.sqrt(abs(t.a * t.d - t.b * t.c))
        bbox = [cx - r, cy - r, cx + r, cy + r]

        draw = ImageDraw.Draw(img, "RGBA")
        stroke = parse_color(obj.stroke)
        width = obj.stroke_width

        if obj.fill and _is_gradient_fill(obj.fill):
            _apply_gradient_fill(
                img, obj, bbox,
                lambda d, b: d.ellipse(b, fill=(255, 255, 255, 255)),
            )
        else:
            fill = parse_color(obj.fill)
            if fill:
                draw.ellipse(bbox, fill=fill)
        if stroke:
            draw.ellipse(bbox, outline=stroke, width=max(1, int(width)))

    def _draw_rect(self, img: Image.Image, obj: GraphicObject) -> None:
        t = obj.transform
        x, y = obj.params["x"], obj.params["y"]
        w, h = obj.params["w"], obj.params["h"]

        # Transform the four corners
        corners = [
            t.apply(x, y),
            t.apply(x + w, y),
            t.apply(x + w, y + h),
            t.apply(x, y + h),
        ]

        draw = ImageDraw.Draw(img, "RGBA")
        stroke = parse_color(obj.stroke)
        width = obj.stroke_width

        # Check if axis-aligned (no rotation/shear)
        if t.is_identity() or (abs(t.b) < 1e-9 and abs(t.c) < 1e-9):
            bbox = [
                min(corners[0][0], corners[2][0]),
                min(corners[0][1], corners[2][1]),
                max(corners[0][0], corners[2][0]),
                max(corners[0][1], corners[2][1]),
            ]
            if obj.fill and _is_gradient_fill(obj.fill):
                _apply_gradient_fill(
                    img, obj, bbox,
                    lambda d, b: d.rectangle(b, fill=(255, 255, 255, 255)),
                )
            else:
                fill = parse_color(obj.fill)
                if fill:
                    draw.rectangle(bbox, fill=fill)
            if stroke:
                draw.rectangle(bbox, outline=stroke, width=max(1, int(width)))
        else:
            # Rotated rect — draw as polygon
            if obj.fill and _is_gradient_fill(obj.fill):
                poly_bbox = [
                    min(c[0] for c in corners),
                    min(c[1] for c in corners),
                    max(c[0] for c in corners),
                    max(c[1] for c in corners),
                ]
                lo, to = poly_bbox[0], poly_bbox[1]
                offset_corners = [(px - lo, py - to) for px, py in corners]
                _apply_gradient_fill(
                    img, obj, poly_bbox,
                    lambda d, b: d.polygon(offset_corners, fill=(255, 255, 255, 255)),
                )
            else:
                fill = parse_color(obj.fill)
                if fill:
                    draw.polygon(corners, fill=fill)
            if stroke:
                draw.polygon(corners, outline=stroke)

    def _draw_ellipse(self, img: Image.Image, obj: GraphicObject) -> None:
        t = obj.transform
        cx, cy = t.apply(obj.params["cx"], obj.params["cy"])
        scale = math.sqrt(abs(t.a * t.d - t.b * t.c))
        rx = max(0, obj.params["rx"] * scale)
        ry = max(0, obj.params["ry"] * scale)
        bbox = [cx - rx, cy - ry, cx + rx, cy + ry]

        draw = ImageDraw.Draw(img, "RGBA")
        stroke = parse_color(obj.stroke)
        width = obj.stroke_width

        if obj.fill and _is_gradient_fill(obj.fill):
            _apply_gradient_fill(
                img, obj, bbox,
                lambda d, b: d.ellipse(b, fill=(255, 255, 255, 255)),
            )
        else:
            fill = parse_color(obj.fill)
            if fill:
                draw.ellipse(bbox, fill=fill)
        if stroke:
            draw.ellipse(bbox, outline=stroke, width=max(1, int(width)))

    def _draw_line(self, img: Image.Image, obj: GraphicObject) -> None:
        t = obj.transform
        p1 = t.apply(obj.params["x1"], obj.params["y1"])
        p2 = t.apply(obj.params["x2"], obj.params["y2"])

        draw = ImageDraw.Draw(img, "RGBA")
        stroke = parse_color(obj.stroke) or (0, 0, 0, 255)
        width = obj.stroke_width

        draw.line([p1, p2], fill=stroke, width=max(1, int(width)))

    def _draw_polygon(self, img: Image.Image, obj: GraphicObject) -> None:
        t = obj.transform
        points = t.apply_points(obj.params["points"])

        draw = ImageDraw.Draw(img, "RGBA")
        stroke = parse_color(obj.stroke)
        width = obj.stroke_width

        if obj.fill and _is_gradient_fill(obj.fill):
            xs = [p[0] for p in points]
            ys = [p[1] for p in points]
            bbox = [min(xs), min(ys), max(xs), max(ys)]
            lo, to = bbox[0], bbox[1]
            offset_pts = [(px - lo, py - to) for px, py in points]
            _apply_gradient_fill(
                img, obj, bbox,
                lambda d, b: d.polygon(offset_pts, fill=(255, 255, 255, 255)),
            )
        else:
            fill = parse_color(obj.fill)
            if fill:
                draw.polygon(points, fill=fill)
        if stroke:
            draw.polygon(points, outline=stroke)

    def _draw_path(self, img: Image.Image, obj: GraphicObject) -> None:
        t = obj.transform
        d = obj.params["d"]
        polylines = _parse_svg_path(d)

        draw = ImageDraw.Draw(img, "RGBA")
        stroke = parse_color(obj.stroke)
        width = obj.stroke_width
        fill = parse_color(obj.fill)

        if obj.fill and _is_gradient_fill(obj.fill):
            # Compute bounding box over all polylines
            all_pts = [pt for poly in polylines for pt in poly]
            if all_pts:
                xs = [p[0] for p in t.apply_points(all_pts)]
                ys = [p[1] for p in t.apply_points(all_pts)]
                bbox = [min(xs), min(ys), max(xs), max(ys)]
                lo, to = bbox[0], bbox[1]
                offset_polylines = [
                    [(px - lo, py - to) for px, py in t.apply_points(poly)]
                    for poly in polylines
                ]
                _apply_gradient_fill(
                    img, obj, bbox,
                    lambda d, b: [
                        d.polygon(offset_poly, fill=(255, 255, 255, 255))
                        for offset_poly in offset_polylines
                    ],
                )
        else:
            for polyline in polylines:
                pts = t.apply_points(polyline)
                if fill:
                    draw.polygon(pts, fill=fill)
                if stroke:
                    draw.line(pts, fill=stroke, width=max(1, int(width)))
        if stroke and _is_gradient_fill(obj.fill):
            # Still need to draw stroke separately for gradient fills
            for polyline in polylines:
                pts = t.apply_points(polyline)
                draw.line(pts, fill=stroke, width=max(1, int(width)))

    def _draw_text(self, img: Image.Image, obj: GraphicObject) -> None:
        t = obj.transform
        x, y = t.apply(obj.params["x"], obj.params["y"])

        draw = ImageDraw.Draw(img, "RGBA")
        fill = parse_color(obj.fill) or (0, 0, 0, 255)

        font_size = int(obj.params.get("font_size", 12))
        try:
            font = ImageFont.truetype("arial.ttf", font_size)
        except (OSError, IOError):
            try:
                font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", font_size)
            except (OSError, IOError):
                font = ImageFont.load_default()

        draw.text((x, y), obj.params["text"], fill=fill, font=font)

    def _draw_image(self, img: Image.Image, obj: GraphicObject) -> None:
        t = obj.transform
        x, y = t.apply(obj.params["x"], obj.params["y"])

        try:
            src = Image.open(obj.params["src"])
            w = obj.params.get("w")
            h = obj.params.get("h")
            if w and h:
                src = src.resize((int(w), int(h)), Image.LANCZOS)
            if src.mode != "RGBA":
                src = src.convert("RGBA")
            img.paste(src, (int(x), int(y)), src)
        except (OSError, IOError):
            pass  # Silently skip missing images

    # ------------------------------------------------------------------
    # Hand-drawn / sketch shape renderers
    # ------------------------------------------------------------------

    def _draw_pencil(self, img: Image.Image, obj: GraphicObject) -> None:
        draw_pencil(img, obj)

    def _draw_charcoal(self, img: Image.Image, obj: GraphicObject) -> None:
        draw_charcoal(img, obj)

    def _draw_watercolor_rect(self, img: Image.Image, obj: GraphicObject) -> None:
        draw_watercolor_rect(img, obj)

    def _draw_watercolor_circle(self, img: Image.Image, obj: GraphicObject) -> None:
        draw_watercolor_circle(img, obj)

    def _draw_hatch(self, img: Image.Image, obj: GraphicObject) -> None:
        draw_hatch(img, obj)

    def _draw_sketchify(self, img: Image.Image, obj: GraphicObject) -> None:
        sketchify_graphic(img, obj)

    def _draw_noise_fill(self, img: Image.Image, obj: GraphicObject) -> None:
        draw_noise_fill(img, obj)
