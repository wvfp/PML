"""PML canvas and sprite-canvas setup."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from pml.environment import Environment
from pml.types import BuiltinProcedure


@dataclass
class Canvas:
    """A drawing surface that collects GraphicObjects for rendering."""

    width: int
    height: int
    bg_color: str = "#FFFFFF"
    objects: list[Any] = field(default_factory=list)
    anchor: str = "top-left"
    padding: int = 0
    is_sprite: bool = False
    auto_fit: bool = False
    """If True, canvas will auto-size to content bounding box at render time."""
    _last_position: tuple[int, int] = (0, 0)
    """Track the right/bottom edge of the last placed element for relative positioning."""
    _tagged: dict[str, tuple[int, int, int, int]] = field(default_factory=dict)
    """Tag → (x, y, w, h) mapping for named element references."""
    _post_process_chain: list[dict[str, Any]] = field(default_factory=list)
    """Post-process shader chain applied at render time, in order."""
    clip_rect: tuple[int, int, int, int] | None = None
    """Optional (x, y, w, h) clipping rectangle applied at render time."""

    def add(self, obj: Any) -> None:
        """Add a graphic object to the canvas."""
        self.objects.append(obj)

    def compute_bounds(self) -> tuple[int, int, int, int]:
        """Compute bounding box across all objects.

        Returns (min_x, min_y, width, height) or (0, 0, 0, 0) for empty.
        Reads position from both AffineTransform (e, f) and params (x, y).
        """
        if not self.objects:
            return (0, 0, 0, 0)

        from pml.transform import AffineTransform

        min_x = 10**9
        min_y = 10**9
        max_x = -10**9
        max_y = -10**9

        from pml.graphics.layout import _intrinsic_size
        from pml.graphics.objects import GraphicObject
        from pml.graphics.layout import is_layoutbox

        for obj in self.objects:
            if is_layoutbox(obj):
                obj = obj.graphic
            if not isinstance(obj, GraphicObject):
                continue

            w, h = _intrinsic_size(obj)

            # Position from transform (used by row/column/stack layout)
            tx = float(obj.transform.e) if isinstance(obj.transform, AffineTransform) else 0.0
            ty = float(obj.transform.f) if isinstance(obj.transform, AffineTransform) else 0.0

            # Position from params (used by raw (rect x y w h) calls)
            px = float(obj.params.get("x", 0))
            py = float(obj.params.get("y", 0))

            ox = tx + px
            oy = ty + py
            min_x = int(min(min_x, ox))
            min_y = int(min(min_y, oy))
            max_x = int(max(max_x, ox + w))
            max_y = int(max(max_y, oy + h))

        if min_x > max_x or min_y > max_y:
            return (0, 0, 0, 0)

        bw = max_x - min_x
        bh = max_y - min_y
        return (int(min_x), int(min_y), int(bw + 2 * self.padding), int(bh + 2 * self.padding))

    def fit_to_content(self) -> None:
        """Resize canvas to tightly wrap all content."""
        if not self.auto_fit:
            return
        mx, my, bw, bh = self.compute_bounds()
        if bw > 0 and bh > 0:
            self.width = bw
            self.height = bh

            # Shift content so min_x, min_y land at padding
            if mx < 0 or my < 0:
                dx = self.padding - mx
                dy = self.padding - my
                from pml.transform import AffineTransform
                from pml.graphics.objects import GraphicObject
                moved = []
                for obj in self.objects:
                    if isinstance(obj, GraphicObject):
                        moved.append(obj.with_transform(
                            AffineTransform.translate(dx, dy).compose(obj.transform)
                        ))
                    else:
                        moved.append(obj)
                self.objects = moved


# Global canvas reference — set by (canvas ...) and used by (render ...)
_current_canvas: list[Canvas | None] = [None]


def _canvas(w: float = 0, h: float = 0, **kwargs: Any) -> Canvas:
    bg = kwargs.get("bg", "#FFFFFF")
    c = Canvas(width=int(w), height=int(h), bg_color=bg)
    if w == 0 or h == 0:
        c.auto_fit = True
    _current_canvas[0] = c
    return c


def _sprite_canvas(w: float = 0, h: float = 0, **kwargs: Any) -> Canvas:
    from pml.types import Symbol

    bg = kwargs.get("bg", "transparent")
    anchor = kwargs.get("anchor", Symbol("center-bottom"))
    padding = kwargs.get("padding", 0)

    anchor_str = anchor.name if isinstance(anchor, Symbol) else str(anchor)

    c = Canvas(
        width=int(w),
        height=int(h),
        bg_color=bg,
        anchor=anchor_str,
        padding=int(padding),
        is_sprite=True,
    )
    if w == 0 or h == 0:
        c.auto_fit = True
    _current_canvas[0] = c
    return c


def get_current_canvas() -> Canvas | None:
    return _current_canvas[0]


def register_canvas(env: Environment) -> None:
    env.define("canvas", BuiltinProcedure("canvas", _canvas, accepts_kwargs=True))
    env.define(
        "sprite-canvas",
        BuiltinProcedure("sprite-canvas", _sprite_canvas, accepts_kwargs=True),
    )

    # Override 'add' in the global env so PML code can add objects to canvas
    def _add(obj: Any) -> None:
        c = _current_canvas[0]
        if c is None:
            c = Canvas(width=256, height=256, bg_color="transparent")
            _current_canvas[0] = c

        # Resolve relative positioning (right-of, below, center) from LayoutBox
        from pml.graphics.layout import (
            LayoutBox,
            is_layoutbox,
            resolve_relative_positioning,
            update_tagged,
        )
        from pml.graphics.objects import GraphicObject

        if is_layoutbox(obj):
            lb: LayoutBox = obj
            resolved = resolve_relative_positioning(lb, c)
            c.add(resolved)
            update_tagged(c, lb, resolved)
        elif isinstance(obj, GraphicObject):
            c.add(obj)
        else:
            c.add(obj)

    env.define("add", BuiltinProcedure("add", _add))

    def _clip_canvas(x: float = 0, y: float = 0, w: float = 0, h: float = 0) -> None:
        """(clip-canvas x y w h) — restrict rendering to (x,y,w,h) region."""
        c = _current_canvas[0]
        if c is None:
            c = Canvas(width=256, height=256, bg_color="transparent")
            _current_canvas[0] = c
        c.clip_rect = (int(x), int(y), int(w), int(h))

    env.define(
        "clip-canvas",
        BuiltinProcedure("clip-canvas", _clip_canvas),
    )
