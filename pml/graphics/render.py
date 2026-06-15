"""PML render function — outputs graphics to files."""

from __future__ import annotations

import json
import os
from typing import Any

from pml.environment import Environment
from pml.errors import PMLTypeError
from pml.graphics.canvas import Canvas, get_current_canvas
from pml.graphics.objects import GraphicObject
from pml.types import BuiltinProcedure, Symbol


# Global output directory override (set by CLI --output-dir)
_output_dir: str | None = None


def set_output_dir(path: str | None) -> None:
    """Set the global output directory for rendered files."""
    global _output_dir
    _output_dir = os.path.abspath(path) if path else None
    if _output_dir:
        os.makedirs(_output_dir, exist_ok=True)


def _resolve_output_path(filename: str) -> str:
    """Prepend global output dir if set, otherwise return filename as-is."""
    if _output_dir:
        return os.path.join(_output_dir, os.path.basename(filename))
    return filename


def _render(filename: str, **kwargs: Any) -> str:
    """Render the current canvas contents to a file."""
    from pml.backend.pillow import PillowBackend

    filename = _resolve_output_path(filename)

    canvas = get_current_canvas()
    if canvas is None:
        canvas = Canvas(width=256, height=256, bg_color="transparent")

    # Determine output format
    fmt = kwargs.get("format")
    if fmt is None:
        ext = os.path.splitext(filename)[1].lower().lstrip(".")
        fmt = ext if ext else "png"
    elif isinstance(fmt, Symbol):
        fmt = fmt.name
    fmt = str(fmt).upper()

    backend = PillowBackend()

    # Animation path: GIF format with registered animations
    if fmt == "GIF":
        from pml.animation.timeline import Timeline

        timeline = Timeline.get_current()
        if timeline.animations:
            fps = int(kwargs.get("fps", 24))
            frames = timeline.render_frames(
                canvas=canvas,
                fps=fps,
                backend=backend,
                width=canvas.width,
                height=canvas.height,
                is_sprite=canvas.is_sprite,
                bg_color=canvas.bg_color,
            )
            backend.save_animation(frames, filename, fmt, fps)
            return filename

    # Static render path
    surface = backend.create_surface(canvas.width, canvas.height, canvas.bg_color)

    # For sprite canvases, auto-center the content
    if canvas.is_sprite:
        from pml.transform import AffineTransform
        center_t = AffineTransform.translate(
            canvas.width / 2,
            canvas.height * 0.45,
        )
        for obj in canvas.objects:
            if isinstance(obj, GraphicObject):
                centered = obj.with_transform(center_t.compose(obj.transform))
                backend.draw(surface, centered)
    else:
        # Draw all objects
        for obj in canvas.objects:
            if isinstance(obj, GraphicObject):
                backend.draw(surface, obj)

    # Save
    backend.save_image(surface, filename, fmt)

    # Generate metadata for sprites
    if canvas.is_sprite:
        meta = {
            "file": filename,
            "width": canvas.width,
            "height": canvas.height,
            "anchor": canvas.anchor,
            "padding": canvas.padding,
            "format": fmt,
            "pml_version": "0.1.0",
        }
        meta_path = os.path.splitext(filename)[0] + ".meta.json"
        with open(meta_path, "w", encoding="utf-8") as f:
            json.dump(meta, f, indent=2)

    return filename


def _render_set(name: str, **kwargs: Any) -> list[str]:
    """Render at multiple scales."""
    from pml.backend.pillow import PillowBackend

    content = kwargs.get("content")
    scales = kwargs.get("scales", [1, 2, 4])
    base_size = kwargs.get("base-size", (64, 64))

    if content is None:
        return []

    backend = PillowBackend()
    results = []

    for s in scales:
        w = int(base_size[0]) * int(s)
        h = int(base_size[1]) * int(s)
        surface = backend.create_surface(w, h, "transparent")

        if isinstance(content, GraphicObject):
            # Scale the graphic
            from pml.transform import AffineTransform
            scaled = content.with_transform(
                AffineTransform.scale(s, s).compose(content.transform)
            )
            backend.draw(surface, scaled)

        suffix = f"@{s}x"
        out_name = f"{name}{suffix}.png"
        backend.save_image(surface, out_name, "PNG")
        results.append(out_name)

    return results


def _render_spritesheet(*args: Any, **kwargs: Any) -> str:
    """(render-spritesheet \"path.png\" sprite... :cols 4 :cell-width 64 :cell-height 64 :padding 2)

    Render multiple sprites into a grid-based spritesheet image.
    Returns the output filename.

    Each positional arg after the path is treated as a sprite to place in the grid.
    """
    from pml.backend.pillow import PillowBackend

    if not args:
        raise PMLTypeError("render-spritesheet expects at least a filename")

    filename = _resolve_output_path(str(args[0]))
    sprites = list(args[1:])

    cols = int(kwargs.get("cols", 4))
    cell_w = int(kwargs.get("cell-width", 64))
    cell_h = int(kwargs.get("cell-height", 64))
    padding = int(kwargs.get("padding", 2))
    bg = str(kwargs.get("bg", "transparent"))

    if not sprites:
        return filename

    rows = max(1, (len(sprites) + cols - 1) // cols)
    total_w = cols * (cell_w + padding) + padding
    total_h = rows * (cell_h + padding) + padding

    backend = PillowBackend()
    surface = backend.create_surface(total_w, total_h, bg)

    # Frame metadata: describes each cell
    frames: list[dict[str, Any]] = []

    for i, sprite in enumerate(sprites):
        if not isinstance(sprite, GraphicObject):
            continue

        col = i % cols
        row = i // cols
        x = padding + col * (cell_w + padding)
        y = padding + row * (cell_h + padding)

        # Create a cell-sized surface and render the sprite onto it
        cell_surface = backend.create_surface(cell_w, cell_h, "transparent")
        backend.draw(cell_surface, sprite)

        # Paste cell onto the spritesheet
        surface.paste(cell_surface, (x, y), cell_surface)

        frames.append({
            "index": i,
            "x": x,
            "y": y,
            "width": cell_w,
            "height": cell_h,
            "col": col,
            "row": row,
        })

    # Save the spritesheet
    backend.save_image(surface, filename, "PNG")

    # Write metadata JSON
    meta: dict[str, Any] = {
        "file": filename,
        "format": "PNG",
        "total_width": total_w,
        "total_height": total_h,
        "cols": cols,
        "rows": rows,
        "cell_width": cell_w,
        "cell_height": cell_h,
        "padding": padding,
        "frames": frames,
        "pml_version": "0.1.0",
    }
    meta_path = os.path.splitext(filename)[0] + ".spritesheet.json"
    with open(meta_path, "w", encoding="utf-8") as f:
        json.dump(meta, f, indent=2)

    return filename


def register_render(env: Environment) -> None:
    env.define("render", BuiltinProcedure("render", _render, accepts_kwargs=True))
    env.define("render-set", BuiltinProcedure("render-set", _render_set, accepts_kwargs=True))
    env.define("render-spritesheet", BuiltinProcedure("render-spritesheet", _render_spritesheet, accepts_kwargs=True))
