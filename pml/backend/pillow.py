"""Pillow-based render backend for PML."""

from __future__ import annotations

import math
import re
from typing import Any

from PIL import Image, ImageDraw, ImageFont

from pml.backend import RenderBackend
from pml.graphics.objects import GraphicObject
from pml.transform import AffineTransform


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
# SVG path parsing (simplified)
# ======================================================================

def _parse_svg_path(d: str) -> list[list[tuple[float, float]]]:
    """Parse SVG path data into a list of polylines.

    Supports: M (moveto), L (lineto), H (horizontal lineto),
    V (vertical lineto), Z (closepath).
    All commands are uppercase (absolute) only for simplicity.
    """
    tokens = re.findall(r"[A-Za-z]|-?[\d.]+", d)
    polylines: list[list[tuple[float, float]]] = []
    current: list[tuple[float, float]] = []
    cx, cy = 0.0, 0.0
    i = 0

    while i < len(tokens):
        cmd = tokens[i]
        if cmd == "M":
            if current:
                polylines.append(current)
            cx, cy = float(tokens[i + 1]), float(tokens[i + 2])
            current = [(cx, cy)]
            i += 3
        elif cmd == "L":
            cx, cy = float(tokens[i + 1]), float(tokens[i + 2])
            current.append((cx, cy))
            i += 3
        elif cmd == "H":
            cx = float(tokens[i + 1])
            current.append((cx, cy))
            i += 2
        elif cmd == "V":
            cy = float(tokens[i + 1])
            current.append((cx, cy))
            i += 2
        elif cmd == "Z" or cmd == "z":
            if current:
                current.append(current[0])
                polylines.append(current)
                current = []
            i += 1
        elif cmd.isalpha():
            # Unknown command — skip
            i += 1
        else:
            # Numeric without command — treat as L continuation
            if current:
                cx, cy = float(tokens[i]), float(tokens[i + 1])
                current.append((cx, cy))
                i += 2
            else:
                i += 1

    if current:
        polylines.append(current)
    return polylines


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
        if obj.shape_type == "group":
            for child in obj.children:
                # Compose group transform with child transform
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
        }

        handler = dispatch.get(obj.shape_type)
        if handler:
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
        fill = parse_color(obj.fill)
        stroke = parse_color(obj.stroke)
        width = obj.stroke_width

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
        fill = parse_color(obj.fill)
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
            if fill:
                draw.rectangle(bbox, fill=fill)
            if stroke:
                draw.rectangle(bbox, outline=stroke, width=max(1, int(width)))
        else:
            # Rotated rect — draw as polygon
            if fill:
                draw.polygon(corners, fill=fill)
            if stroke:
                draw.polygon(corners, outline=stroke)

    def _draw_ellipse(self, img: Image.Image, obj: GraphicObject) -> None:
        t = obj.transform
        cx, cy = t.apply(obj.params["cx"], obj.params["cy"])
        # Approximate scaled radii
        scale = math.sqrt(abs(t.a * t.d - t.b * t.c))
        rx = obj.params["rx"] * scale
        ry = obj.params["ry"] * scale
        bbox = [cx - rx, cy - ry, cx + rx, cy + ry]

        draw = ImageDraw.Draw(img, "RGBA")
        fill = parse_color(obj.fill)
        stroke = parse_color(obj.stroke)
        width = obj.stroke_width

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
        fill = parse_color(obj.fill)
        stroke = parse_color(obj.stroke)

        if fill:
            draw.polygon(points, fill=fill)
        if stroke:
            draw.polygon(points, outline=stroke)

    def _draw_path(self, img: Image.Image, obj: GraphicObject) -> None:
        t = obj.transform
        d = obj.params["d"]
        polylines = _parse_svg_path(d)

        draw = ImageDraw.Draw(img, "RGBA")
        fill = parse_color(obj.fill)
        stroke = parse_color(obj.stroke)
        width = obj.stroke_width

        for polyline in polylines:
            pts = t.apply_points(polyline)
            if fill:
                draw.polygon(pts, fill=fill)
            if stroke:
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
